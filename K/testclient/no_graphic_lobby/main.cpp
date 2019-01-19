#define WIN32_LEAN_AND_MEAN  

#include <WinSock2.h>
#pragma comment (lib, "ws2_32.lib")

#include "../../Server2019G/header/protocol.h"

#include <iostream>

using namespace std;

int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}
// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int main() {

	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	SOCKET sock;
	sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	SOCKADDR_IN ServerAddr;
	char ipaddr[17];
	cout << "input server IP addr :";
	cin >> ipaddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(MY_SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);

	WSAConnect(sock, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	char sbuf[MAX_PACKET_SIZE];
	char rbuf[MAX_PACKET_SIZE];
	enum command_list {
		REFRESH = 0, AUTOJOIN, JOINROOM, QUITROOM
	};

	int command, retval;
	sc_roomstatus_packet* rsp = reinterpret_cast<sc_roomstatus_packet*>(rbuf);
	
	cs_refresh_packet* packet_ref = reinterpret_cast<cs_refresh_packet*>(sbuf);
	cs_join_packet* packet_join = reinterpret_cast<cs_join_packet*>(sbuf);
	cs_autojoin_packet* packet_autojoin = reinterpret_cast<cs_autojoin_packet*>(sbuf);
	cs_quit_packet* packet_quit = reinterpret_cast<cs_quit_packet*>(sbuf);
	while (1) {
		ZeroMemory(sbuf, MAX_PACKET_SIZE);
		ZeroMemory(rbuf, MAX_PACKET_SIZE);
		cout << "[0: REFRESH]\n[1: AUTOJOIN]\n[2: JOINROOM]\n[3: QUITROOM]\n";
		cout << "input command : ";
		cin >> command;
		switch (command) {
		case REFRESH:
			packet_ref->size = sizeof(cs_refresh_packet);
			packet_ref->type = CS_REFRESH;
			retval = send(sock, sbuf, sizeof(cs_refresh_packet), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			
			retval = recvn(sock, rbuf, sizeof(sc_roomstatus_packet), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
			}
			for (int i = 0; i < MAX_ROOMNUMBER; ++i) {
				switch (rsp->roomstatus[i]) {
				case EMPTY:
					//cout << i << " ���� - ���" << endl;
					break;
				case FULL:
					cout << i << " ���� - ����" << endl;
					break;
				case JOINABLE:
					cout << i << " ���� - ���尡��" << endl;
					break;
				}
			}
			break;
		case AUTOJOIN:
			packet_autojoin->size = sizeof(cs_autojoin_packet);
			packet_autojoin->type = CS_AUTOJOIN;
			retval = send(sock, sbuf, sizeof(cs_autojoin_packet), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			break;
		case JOINROOM:
			packet_join->size = sizeof(cs_join_packet);
			packet_join->type = CS_JOIN;
			cout << "���� ���� �� ��ȣ(0~199): ";
			cin >> packet_join->roomnumber;
			retval = send(sock, sbuf, sizeof(cs_join_packet), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			break;
		case QUITROOM:
			packet_quit->size = sizeof(cs_quit_packet);
			packet_quit->type = CS_QUIT;
			retval = send(sock, sbuf, sizeof(cs_quit_packet), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			break;
		}

		

	}


}