//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" )
//#include <iostream>

#pragma comment(lib, "ws2_32")

#include "../../Server2019G/header/protocol.h"
#include "resource.h"
#include <winsock2.h>
#include <string>

using namespace std;

#define	WM_SOCKET	WM_USER + 1
#define SERVERPORT 9000
#define BUF_SIZE    512


#define SERVERPORT 9000
#define BUF_SIZE    512
#define	WM_SOCKET	WM_USER + 1

SOCKET g_mysocket;
WSABUF	send_wsabuf;
char 	send_buffer[BUF_SIZE];
WSABUF	recv_wsabuf;
char	recv_buffer[BUF_SIZE];
char	packet_buffer[BUF_SIZE];
DWORD		in_packet_size = 0;
int		saved_packet_size = 0;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

HWND hList, hDlgC;
HWND hJoinB, hAutoJoinB, hRefB, hReadyB, hQuitB, hUnreadyB;

void ProcessPacket(char *ptr)
{

	switch (ptr[1])
	{
	case SC_REFRESH:
	{
		EnableWindow(hRefB, TRUE);
		sc_roomstatus_packet* rsp = reinterpret_cast<sc_roomstatus_packet*>(ptr);
		wstring msg;
		wchar_t itoares[10];
		for (int i = 0; i < MAX_ROOMNUMBER; ++i) {
			_itow(i, itoares, 10);
			msg += itoares;
			switch (rsp->roomstatus[i]) {
			case RS_EMPTY:
				msg += L" 번방 - 비어있음";
				break;
			case RS_FULL:
				msg += L" 번방 - 만원";
				break;
			case RS_JOINABLE:
				msg += L" 번방 - 입장가능";
				break;
			}
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg.c_str());
			msg.clear();
		}
		break;
	}
	case SC_JOIN_PLAYER:
	{
		sc_player_join_packet* pjp = reinterpret_cast<sc_player_join_packet*>(ptr);
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)pjp->id);

		break;
	}
	case SC_QUIT_PLAYER:
	{
		sc_player_quit_packet* qp = reinterpret_cast<sc_player_quit_packet*>(ptr);

		int i = SendMessage(hList, LB_FINDSTRINGEXACT, 0, (LPARAM)qp->id);
		SendMessage(hList, LB_DELETESTRING, i, 0);
		break;
	}
	case US_READY:
	{
		sc_usercondition_packet* ucp = reinterpret_cast<sc_usercondition_packet*>(ptr);
		int i = SendMessage(hList, LB_FINDSTRINGEXACT, 0, (LPARAM)ucp->id);
		if (i != LB_ERR) {
			wstring ids = ucp->id;
			ids += L" - READY";
			SendMessage(hList, LB_DELETESTRING, i, 0);
			SendMessage(hList, LB_INSERTSTRING, i, (LPARAM)ids.c_str());
		}
		else {
			EnableWindow(hUnreadyB, TRUE);
			EnableWindow(hReadyB, FALSE);
		}
		break;
	}
	case US_WAIT:
	{
		sc_usercondition_packet* ucp = reinterpret_cast<sc_usercondition_packet*>(ptr);
		wstring ids = ucp->id;
		ids += L" - READY";
		int i = SendMessage(hList, LB_FINDSTRINGEXACT, 0, (LPARAM)ids.c_str());
		if (i != LB_ERR) {
			SendMessage(hList, LB_DELETESTRING, i, 0);
			SendMessage(hList, LB_INSERTSTRING, i, (LPARAM)ucp->id);
		}
		else {
			EnableWindow(hUnreadyB, FALSE);
			EnableWindow(hReadyB, TRUE);
		}
		break;
	}
	}
}


void ReadPacket(SOCKET sock)
{
	DWORD iobyte, ioflag = 0;

	int ret = WSARecv(sock, &recv_wsabuf, 1, &iobyte, &ioflag, NULL, NULL);
	if (ret) {
		int err_code = WSAGetLastError();
	}

	BYTE *ptr = reinterpret_cast<BYTE *>(recv_buffer);

	while (0 != iobyte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (iobyte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			iobyte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, iobyte);
			saved_packet_size += iobyte;
			iobyte = 0;
		}
	}
}

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);


	closesocket(g_mysocket);
	WSACleanup();
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WSADATA wsadata;
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		hDlgC = hDlg;
		hList = GetDlgItem(hDlg, DISPLAYL);
		hJoinB = GetDlgItem(hDlg, SJOIN);
		hAutoJoinB = GetDlgItem(hDlg, ATJOIN);
		hRefB = GetDlgItem(hDlg, IDOK);
		hReadyB = GetDlgItem(hDlg, READYB);
		hUnreadyB = GetDlgItem(hDlg, UNREADYB);
		hQuitB = GetDlgItem(hDlg, QUITB);

		EnableWindow(hReadyB, FALSE);
		EnableWindow(hQuitB, FALSE);
		EnableWindow(hUnreadyB, FALSE);
		WSAStartup(MAKEWORD(2, 2), &wsadata);

		g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

		SOCKADDR_IN ServerAddr;
		ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(MY_SERVER_PORT);
		ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

		WSAAsyncSelect(g_mysocket, hDlg, WM_SOCKET, FD_CLOSE | FD_READ);

		send_wsabuf.buf = send_buffer;
		send_wsabuf.len = BUF_SIZE;
		recv_wsabuf.buf = recv_buffer;
		recv_wsabuf.len = BUF_SIZE;

		DWORD iobyte;
		cs_refresh_packet* refp = reinterpret_cast<cs_refresh_packet*>(send_buffer);
		send_wsabuf.len = sizeof(cs_refresh_packet);
		refp->size = sizeof(cs_refresh_packet);
		refp->type = CS_REFRESH;
		WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		{
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			DWORD iobyte;
			cs_refresh_packet* refp = reinterpret_cast<cs_refresh_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_refresh_packet);
			refp->size = sizeof(cs_refresh_packet);
			refp->type = CS_REFRESH;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			return TRUE;
		}
		case ATJOIN:
		{
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			DWORD iobyte;
			cs_autojoin_packet* atp = reinterpret_cast<cs_autojoin_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_autojoin_packet);
			atp->size = sizeof(cs_autojoin_packet);
			atp->type = CS_AUTOJOIN;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			EnableWindow(hRefB, FALSE);
			EnableWindow(hReadyB, TRUE);
			EnableWindow(hQuitB, TRUE);
			EnableWindow(hAutoJoinB, FALSE);
			EnableWindow(hJoinB, FALSE);
			return TRUE;
		}
		case SJOIN:
		{
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			DWORD iobyte;
			BOOL res;
			cs_join_packet* jp = reinterpret_cast<cs_join_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_join_packet);
			jp->size = sizeof(cs_join_packet);
			jp->type = CS_JOIN;
			jp->roomnumber = GetDlgItemInt(hDlg, RNINPUT, &res, FALSE);
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			EnableWindow(hRefB, FALSE);
			EnableWindow(hReadyB, TRUE);
			EnableWindow(hQuitB, TRUE);
			EnableWindow(hAutoJoinB, FALSE);
			EnableWindow(hJoinB, FALSE);
			//방에 입장하지 못했을 시 다시 활성화시켜야 할 것.
			return TRUE;
		}
		case READYB:
		{
			DWORD iobyte;
			cs_ready_packet* rdp = reinterpret_cast<cs_ready_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_ready_packet);
			rdp->size = sizeof(cs_ready_packet);
			rdp->type = CS_READY;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			return TRUE;
		}
		case UNREADYB:
		{
			DWORD iobyte;
			cs_ready_packet* rdp = reinterpret_cast<cs_ready_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_ready_packet);
			rdp->size = sizeof(cs_ready_packet);
			rdp->type = CS_UNREADY;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			return TRUE;
		}
		case QUITB:
		{
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			DWORD iobyte;
			cs_quit_packet* qp = reinterpret_cast<cs_quit_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_quit_packet);
			qp->size = sizeof(cs_quit_packet);
			qp->type = CS_QUIT;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);

			cs_refresh_packet* refp = reinterpret_cast<cs_refresh_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_refresh_packet);
			refp->size = sizeof(cs_refresh_packet);
			refp->type = CS_REFRESH;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);

			EnableWindow(hReadyB, FALSE);
			EnableWindow(hQuitB, FALSE);
			EnableWindow(hAutoJoinB, TRUE);
			EnableWindow(hJoinB, TRUE);
			return TRUE;
		}
		}
		return FALSE;
	case WM_SOCKET:
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			break;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			ReadPacket((SOCKET)wParam);
			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			break;
		}
		break;
	case WM_CLOSE:
		closesocket(g_mysocket);
		WSACleanup();
		EndDialog(hDlg, 0);
		break;
	}

	return FALSE;
}