#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" )
#include <iostream>

#pragma comment(lib, "ws2_32")

#include "../../Server2019G/header/protocol.h"
#include "resource.h"
#include <winsock2.h>
#include <string>
#include <vector>

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

class Client {
public:
	wstring ID;
	int Score;
	float x;
	float y;
	unsigned char movedir;

	Client() : Score(0), x(0), y(0), movedir(STOP_DR) {
	}

	Client(wstring id, int scr, int xd, int yd) : ID(id), Score(scr), x(xd), y(yd), movedir(STOP_DR) {

	}

};


BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

HWND hList, hDlgC, hIDEdit, hPWEdit, hRNInput;
HWND hJoinB, hAutoJoinB, hRefB, hReadyB, hQuitB, hLoginB, hRegistB;
char game_status = US_LOBBY;
char movedirection = STOP_DR;
int timer;

vector<Client> g_client;



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
			case RS_PLAY:
				msg += L" 번방 - 게임 중";
				break;
			}
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg.c_str());
			msg.clear();
		}
	}
		break;
	case SC_JOIN_PLAYER:
	{
		sc_player_join_packet* pjp = reinterpret_cast<sc_player_join_packet*>(ptr);
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)pjp->id);
		g_client.emplace_back(pjp->id,0,0,0);
	}
		break;
	case SC_QUIT_PLAYER:
	{
		sc_player_quit_packet* qp = reinterpret_cast<sc_player_quit_packet*>(ptr);
		for (int i = 0; i < g_client.size(); ++i) {
			if (g_client[i].ID == qp->id) {
				g_client.erase(g_client.begin() + i);
			}
		}
		int i = SendMessage(hList, LB_FINDSTRINGEXACT, 0, (LPARAM)qp->id);
		if (i != LB_ERR) {
			SendMessage(hList, LB_DELETESTRING, i, 0);
		}
		else {
			wstring ids = qp->id;
			ids += L" - READY";
			SendMessage(hList, LB_FINDSTRINGEXACT, 0, (LPARAM)ids.c_str());
			SendMessage(hList, LB_DELETESTRING, i, 0);
		}
	}
		break;
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
	}
		break;
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
			EnableWindow(hReadyB, TRUE);
		}
	}
		break;
	case SC_GO:
		EnableWindow(hReadyB, FALSE);
		EnableWindow(hRNInput, FALSE);
		EnableWindow(hQuitB, FALSE);
		EnableWindow(hList, FALSE);

		game_status = US_PLAY;
		SendMessage(hList, LB_RESETCONTENT, 0, 0);
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"게임중입니다...");
		SetTimer(hDlgC, 1, 16, NULL);
		break;
	case SC_LOGINSUCCESS:
	{		
		EnableWindow(hRefB, TRUE);
		EnableWindow(hReadyB, FALSE);
		EnableWindow(hQuitB, FALSE);
		EnableWindow(hAutoJoinB, TRUE);
		EnableWindow(hJoinB, TRUE);
		EnableWindow(hIDEdit, FALSE);
		EnableWindow(hPWEdit, FALSE);
		EnableWindow(hLoginB, FALSE);
		EnableWindow(hRegistB, FALSE);
		EnableWindow(hRNInput, TRUE);
		wchar_t ID[10];
		GetDlgItemText(hDlgC, IDC_IDEDIT, ID, 10);
		g_client.emplace_back(ID, 0, 0, 0);
		SetDlgItemText(hDlgC, IDC_IDEDIT, L"로그인 성공");
		DWORD iobyte;
		cs_refresh_packet* refp = reinterpret_cast<cs_refresh_packet*>(send_buffer);
		send_wsabuf.len = sizeof(cs_refresh_packet);
		refp->size = sizeof(cs_refresh_packet);
		refp->type = CS_REFRESH;
		WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	}
		break;
	case SC_REGISTSUCCESS:
		EnableWindow(hIDEdit, TRUE);
		EnableWindow(hPWEdit, TRUE);
		EnableWindow(hLoginB, TRUE);
		EnableWindow(hRegistB,TRUE);
		SetDlgItemText(hDlgC, IDC_IDEDIT, L"회원가입 성공");
		break;
	case SC_LOGINFAIL:
	case SC_REGISTFAIL:
		EnableWindow(hIDEdit, TRUE);
		EnableWindow(hPWEdit, TRUE);
		EnableWindow(hLoginB, TRUE);
		EnableWindow(hRegistB, TRUE);
		SetDlgItemText(hDlgC, IDC_IDEDIT, L"실패");
		break;
	case STOP_DR:
	case LEFT_DR:
	case RIGHT_DR:
	case UP_DR:
	case DOWN_DR:
	case ULEFT_DR:
	case URIGHT_DR:
	case DLEFT_DR:
	case DRIGHT_DR:
	{
		sc_movestatus_packet* msp = reinterpret_cast<sc_movestatus_packet*>(ptr);
		for (Client& d : g_client) {
			if (d.ID == msp->id) {
				d.x = msp->x;
				d.y = msp->y;
				d.movedir = msp->type;
			}
		}
	}

		break;
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


char ipaddr[17];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	//cout << "서버의 IP주소 입력 : ";
	//cin >> ipaddr;
	FreeConsole();
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);


	closesocket(g_mysocket);
	WSACleanup();
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WSADATA wsadata;
	static RECT Wnd;
	static HBITMAP hBit;
	HDC hdc, memdc;
	PAINTSTRUCT ps;
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		timer = 500;
		WSAStartup(MAKEWORD(2, 2), &wsadata);

		g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

		SOCKADDR_IN ServerAddr;
		ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(MY_SERVER_PORT);
		//ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);
		ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

		WSAAsyncSelect(g_mysocket, hDlg, WM_SOCKET, FD_CLOSE | FD_READ);

		send_wsabuf.buf = send_buffer;
		send_wsabuf.len = BUF_SIZE;
		recv_wsabuf.buf = recv_buffer;
		recv_wsabuf.len = BUF_SIZE;

		hDlgC = hDlg;
		hList = GetDlgItem(hDlg, DISPLAYL);
		hJoinB = GetDlgItem(hDlg, SJOIN);
		hAutoJoinB = GetDlgItem(hDlg, ATJOIN);
		hRefB = GetDlgItem(hDlg, IDOK);
		hReadyB = GetDlgItem(hDlg, READYB);
		hQuitB = GetDlgItem(hDlg, QUITB);
		hLoginB = GetDlgItem(hDlg, IDC_LOGINB);
		hRegistB = GetDlgItem(hDlg, IDC_REGISTB);
		hIDEdit = GetDlgItem(hDlg, IDC_IDEDIT);
		hPWEdit = GetDlgItem(hDlg, IDC_PWEDIT);
		hRNInput = GetDlgItem(hDlg, RNINPUT);

		EnableWindow(hRefB, FALSE);
		EnableWindow(hReadyB, FALSE);
		EnableWindow(hQuitB, FALSE);
		EnableWindow(hAutoJoinB, FALSE);
		EnableWindow(hJoinB, FALSE);
		EnableWindow(hRNInput, FALSE);


		GetClientRect(hDlg, &Wnd);
		SetTimer(hDlgC, 1, 10, NULL);

		return TRUE;
	}
	case WM_LBUTTONDOWN:
		if (game_status == US_PLAY)
		{
			DWORD iobyte;
			cs_movestatus_packet* msp = reinterpret_cast<cs_movestatus_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_movestatus_packet);
			msp->size = sizeof(cs_movestatus_packet);
			msp->type = rand() % 8 + UP_DR;
			g_client[0].movedir = msp->type;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		}
		return TRUE;
	case WM_LBUTTONUP:
		if (game_status == US_PLAY)
		{
			DWORD iobyte;
			cs_movestatus_packet* msp = reinterpret_cast<cs_movestatus_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_movestatus_packet);
			msp->size = sizeof(cs_movestatus_packet);
			msp->type = STOP_DR;
			g_client[0].movedir = msp->type;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		}
	return TRUE;
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

			wchar_t tmp[10];
			GetDlgItemText(hDlg, READYB, tmp, 10);
			
			if (!wcscmp(tmp, L"준비")) {
				SetDlgItemText(hDlg, READYB, L"준비 취소");
				rdp->type = CS_READY;
			}
			else {
				SetDlgItemText(hDlg, READYB, L"준비");
				rdp->type = CS_UNREADY;
			}
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
			SetDlgItemText(hDlg, READYB, L"준비");
			EnableWindow(hReadyB, FALSE);
			EnableWindow(hQuitB, FALSE);
			EnableWindow(hAutoJoinB, TRUE);
			EnableWindow(hJoinB, TRUE);
			EnableWindow(hRNInput, TRUE);
			return TRUE;
		}
		case IDC_LOGINB:
		{
			wchar_t ID[10];
			GetDlgItemText(hDlg, IDC_IDEDIT, ID, 10);
			wchar_t PASS[10];
			GetDlgItemText(hDlg, IDC_PWEDIT, PASS, 10);
			DWORD iobyte;
			cs_userinfo_packet* uip = reinterpret_cast<cs_userinfo_packet*>(send_buffer);
			uip->size = sizeof(cs_userinfo_packet);
			send_wsabuf.len = sizeof(cs_userinfo_packet);
			uip->type = CS_LOGIN;
			memcpy(uip->id, ID, sizeof(uip->id));
			memcpy(uip->password, PASS, sizeof(uip->password));
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			EnableWindow(hIDEdit, FALSE);
			EnableWindow(hPWEdit, FALSE);
			EnableWindow(hLoginB, FALSE);
			EnableWindow(hRegistB, FALSE);
			return TRUE;
		}
		case IDC_REGISTB:
		{
			wchar_t ID[10];
			GetDlgItemText(hDlg, IDC_IDEDIT, ID, 10);
			wchar_t PASS[10];
			GetDlgItemText(hDlg, IDC_PWEDIT, PASS, 10);
			DWORD iobyte;
			cs_userinfo_packet* uip = reinterpret_cast<cs_userinfo_packet*>(send_buffer);
			uip->size = sizeof(cs_userinfo_packet);
			send_wsabuf.len = sizeof(cs_userinfo_packet);
			uip->type = CS_REGIST;
			memcpy(uip->id, ID, sizeof(uip->id));
			memcpy(uip->password, PASS, sizeof(uip->password));
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			EnableWindow(hIDEdit, FALSE);
			EnableWindow(hPWEdit, FALSE);
			EnableWindow(hLoginB, FALSE);
			EnableWindow(hRegistB, FALSE);
			return TRUE;
		}
		}
		return FALSE;
	case WM_TIMER:
	if(game_status == -10){
		timer--;
		wstring msg;
		wchar_t itoares[10];
		_itow(timer-1, itoares, 10);
		msg += L"게임중입니다... ";
		msg += itoares;
		msg += L"초 남음";
		SendMessage(hList, LB_RESETCONTENT, 0, 0);
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg.c_str());
		if (timer <= 0) {
			KillTimer(hDlgC, 1);
			timer = 500;
			DWORD iobyte;
			cs_gameresult_packet* grp = reinterpret_cast<cs_gameresult_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_gameresult_packet);
			grp->size = sizeof(cs_gameresult_packet);
			grp->type = CS_GAMERESULT;
			grp->score = 10;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			
			SendMessage(hList, LB_RESETCONTENT, 0, 0);

			cs_refresh_packet* refp = reinterpret_cast<cs_refresh_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_refresh_packet);
			refp->size = sizeof(cs_refresh_packet);
			refp->type = CS_REFRESH;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);

			EnableWindow(hReadyB, FALSE);
			EnableWindow(hQuitB, FALSE);
			EnableWindow(hAutoJoinB, TRUE);
			EnableWindow(hJoinB, TRUE);
		}
	}
	for (Client& d : g_client) {
		switch (d.movedir) {
		case LEFT_DR:
			d.x--;
			break;
		case RIGHT_DR:
			d.x++;
			break;
		case UP_DR:
			d.y--;
			break;
		case DOWN_DR:
			d.y++;
			break;
		case ULEFT_DR:
			d.y--;
			d.x--;
			break;
		case URIGHT_DR:
			d.y--;
			d.x++;
			break;
		case DLEFT_DR:
			d.y++;
			d.x--;
			break;
		case DRIGHT_DR:
			d.y++;
			d.x++;
			break;
		}
	}
	InvalidateRgn(hDlg, NULL, FALSE);
		return TRUE;
	case WM_PAINT:
		hdc = BeginPaint(hDlg, &ps);
		memdc = CreateCompatibleDC(hdc);
		hBit = CreateCompatibleBitmap(hdc, Wnd.right, Wnd.bottom);
		SelectObject(memdc, hBit);
		Rectangle(memdc, 0, 0, Wnd.right, Wnd.bottom);


		for (Client& d : g_client) {
			Ellipse(memdc, Wnd.right / 2 - 10 + d.x, Wnd.bottom / 2 - 10 + d.y, Wnd.right / 2 + 10 + d.x, Wnd.bottom / 2 + 10 + d.y);
		}
		if(!g_client.empty())
			Ellipse(memdc, Wnd.right / 2 - 5 + g_client[0].x, Wnd.bottom / 2 - 5 + g_client[0].y, Wnd.right / 2 + 5 + g_client[0].x, Wnd.bottom / 2 + 5 + g_client[0].y);
			
			

		BitBlt(hdc, 0, 0, Wnd.right, Wnd.bottom, memdc, 0, 0, SRCCOPY);

		DeleteDC(memdc);
		EndPaint(hDlg, &ps);
		return TRUE;
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