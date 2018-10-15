#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" )
#include <iostream>


#pragma comment(lib, "ws2_32")
#include "resource.h"
#include "../../2018Server/2018Server/protocol.h"
#include <winsock2.h>
#include <stdio.h>
using namespace std;

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
int		g_myid;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

class PLAYER {
private:
public:
	WCHAR message[256];
	DWORD message_time;
};

PLAYER player;
PLAYER other[MAX_USER];

inline void ProcessPacket(char *ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_PUT_PLAYER:
	{

		break;
	}
	case SC_REMOVE_PLAYER:
	{

		break;
	}
	case SC_CHAT:
	{

		break;

	}
	}
}

inline void ReadPacket(SOCKET sock)
{
	DWORD iobyte, ioflag = 0;

	int ret = WSARecv(sock, &recv_wsabuf, 1, &iobyte, &ioflag, NULL, NULL);
	if (ret) {
		int err_code = WSAGetLastError();
		printf("Recv Error [%d]\n", err_code);
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
SOCKET sock;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);


	closesocket(sock);
	WSACleanup();
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hList, hButton;
	static WSADATA wsadata;

	switch (uMsg) {
	case WM_INITDIALOG:
	{
		hList = GetDlgItem(hDlg, IDC_EDIT1);
		hButton = GetDlgItem(hDlg, IDOK);

		WSAStartup(MAKEWORD(2, 2), &wsadata);

		g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

		SOCKADDR_IN ServerAddr;
		char ipaddr[17];
		cout << "input server IP addr :";
		cin >> ipaddr;
		ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(MY_SERVER_PORT);
		ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);

		WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

		WSAAsyncSelect(g_mysocket, hDlg, WM_SOCKET, FD_CLOSE | FD_READ);

		send_wsabuf.buf = send_buffer;
		send_wsabuf.len = BUF_SIZE;
		recv_wsabuf.buf = recv_buffer;
		recv_wsabuf.len = BUF_SIZE;

		cout << "input room number :";

		cs_packet_regist *registpacket = reinterpret_cast<cs_packet_regist *>(send_buffer);

		registpacket->size = sizeof(cs_packet_regist);
		send_wsabuf.len = sizeof(cs_packet_regist);
		registpacket->type = CS_REGIST;
		cin >> registpacket->roomnumber;
		DWORD iobyte;
		WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);



		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			wchar_t chatbuff[MAX_STR_SIZE];
			GetDlgItemText(hDlg, IDC_EDIT1, chatbuff, MAX_STR_SIZE);
			SetDlgItemText(hDlg, IDC_EDIT1, L"");
			//여기서 채팅내용 전송
			cs_packet_chat *chatpacket = reinterpret_cast<cs_packet_chat *>(send_buffer);

			chatpacket->size = sizeof(cs_packet_chat);
			send_wsabuf.len = sizeof(cs_packet_chat);
			chatpacket->type = CS_CHAT;
			memcpy_s(chatpacket->message, sizeof(chatpacket->message), chatbuff, sizeof(chatbuff));
			DWORD iobyte;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);

			return TRUE;
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
		break;
	}
	
	return FALSE;
}