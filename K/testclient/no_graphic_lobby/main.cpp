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
// 소켓 함수 오류 출력 후 종료
void err_quit(wchar_t *msg)
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

// 소켓 함수 오류 출력
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

wstring m2w(const char* pStr) {
	int len = -1;
	wstring wstrOut;
	int nchar = MultiByteToWideChar(CP_ACP, 0, pStr, len, NULL, 0);
	wstrOut.resize(nchar);
	MultiByteToWideChar(CP_ACP, 0, pStr, len, const_cast<wchar_t*>(wstrOut.c_str()), nchar);
	return wstrOut;
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
		LOGIN = 1, REGIST,
		REFRESH = 0, AUTOJOIN, JOINROOM, QUITROOM,
	};

	int command, retval;
	sc_roomstatus_packet* rsp = reinterpret_cast<sc_roomstatus_packet*>(rbuf);
	sc_id_packet* idp = reinterpret_cast<sc_id_packet*>(rbuf);

	cs_userinfo_packet* packet_uif = reinterpret_cast<cs_userinfo_packet*>(sbuf);
	cs_refresh_packet* packet_ref = reinterpret_cast<cs_refresh_packet*>(sbuf);
	cs_join_packet* packet_join = reinterpret_cast<cs_join_packet*>(sbuf);
	cs_autojoin_packet* packet_autojoin = reinterpret_cast<cs_autojoin_packet*>(sbuf);
	cs_quit_packet* packet_quit = reinterpret_cast<cs_quit_packet*>(sbuf);
	cs_ready_packet* packet_ready = reinterpret_cast<cs_ready_packet*>(sbuf);
	cs_gameresult_packet* packet_gameresult = reinterpret_cast<cs_gameresult_packet*>(sbuf);

	bool logined = true;
	while (logined) {
		ZeroMemory(sbuf, MAX_PACKET_SIZE);
		ZeroMemory(rbuf, MAX_PACKET_SIZE);
		cout << "[1:LOGIN]\n[2:REGIST]" << endl;
		cin >> command;

		char tmp[10];
		switch (command) {
		case REGIST:
			cout << "ID(영어, 숫자 10자이내) :";
			cin >> tmp;
			memcpy(packet_uif->id, m2w(tmp).c_str(), 10);
			cout << "PASSWORD(영어, 숫자 10자이내) :";
			cin >> tmp;
			memcpy(packet_uif->password, m2w(tmp).c_str(), 10);
			packet_uif->size = sizeof(cs_userinfo_packet);
			packet_uif->type = CS_REGIST;
			send(sock, sbuf, sizeof(cs_userinfo_packet), 0);
			recvn(sock, rbuf, sizeof(sc_id_packet), 0);
			if (idp->type != SC_SUCCESS) {
				cout << "회원가입 실패" << endl;
			}
			break;
		case LOGIN:
			cout << "ID :";
			cin >> tmp;
			memcpy(packet_uif->id, m2w(tmp).c_str(), 10);
			cout << "PASSWORD :";
			cin >> tmp;
			memcpy(packet_uif->password, m2w(tmp).c_str(), 10);
			packet_uif->size = sizeof(cs_userinfo_packet);
			packet_uif->type = CS_LOGIN;
			send(sock, sbuf, sizeof(cs_userinfo_packet), 0);
			recvn(sock, rbuf, sizeof(sc_id_packet), 0);
			if (idp->type == SC_SUCCESS) {
				logined = false;
				system("cls");
			}
			else
				cout << "로그인 실패" << endl;
			break;
		}
	}
	
	int status = US_LOBBY;
	while (1) {
		ZeroMemory(sbuf, MAX_PACKET_SIZE);
		ZeroMemory(rbuf, MAX_PACKET_SIZE);
		//스위치문으로 현재 상태에 따른 출력 및 커맨드를 따로 지정해 줄 것.
		switch (status) {
		case US_LOBBY:
		{
			cout << "[0: REFRESH]\n[1: AUTOJOIN]\n[2: JOINROOM]\n[3: QUITROOM]\n";
			cout << "input command : ";
			cin >> command;
			switch (command) {
			case REFRESH:
				packet_ref->size = sizeof(cs_refresh_packet);
				packet_ref->type = CS_REFRESH;
				retval = send(sock, sbuf, sizeof(cs_refresh_packet), 0);
				retval = recvn(sock, rbuf, sizeof(sc_roomstatus_packet), 0);
				for (int i = 0; i < MAX_ROOMNUMBER; ++i) {
					switch (rsp->roomstatus[i]) {
					case RS_EMPTY:
						//cout << i << " 번방 - 빈방" << endl;
						break;
					case RS_FULL:
						cout << i << " 번방 - 만원" << endl;
						break;
					case RS_JOINABLE:
						cout << i << " 번방 - 입장가능" << endl;
						break;
					}
				}
				break;
			case AUTOJOIN:
				packet_autojoin->size = sizeof(cs_autojoin_packet);
				packet_autojoin->type = CS_AUTOJOIN;
				retval = send(sock, sbuf, sizeof(cs_autojoin_packet), 0);
				break;
			case JOINROOM:
				packet_join->size = sizeof(cs_join_packet);
				packet_join->type = CS_JOIN;
				cout << "들어가고 싶은 방 번호(0~199): ";
				cin >> packet_join->roomnumber;
				retval = send(sock, sbuf, sizeof(cs_join_packet), 0);

				break;
			case QUITROOM:
				packet_quit->size = sizeof(cs_quit_packet);
				packet_quit->type = CS_QUIT;
				retval = send(sock, sbuf, sizeof(cs_quit_packet), 0);
				break;
			}
		}
			break;
		case US_WAIT:
			cout << "[0: READY]\n[1: QUITROOM]\n";
			cout << "input command : ";
			cin >> command;
			if (command == QUITROOM) {
				packet_quit->size = sizeof(cs_quit_packet);
				packet_quit->type = CS_QUIT;
				retval = send(sock, sbuf, sizeof(cs_quit_packet), 0);
				break;
			}
			else {
				packet_ready->size = sizeof(cs_ready_packet);
				packet_ready->type = CS_READY;
				retval = send(sock, sbuf, sizeof(cs_ready_packet), 0);
				break;
			}
		case US_READY:
			cout << "[0: UNREADY]\n[1: QUITROOM]\n";
			cout << "input command : ";
			cin >> command;
			if (command == QUITROOM) {
				packet_quit->size = sizeof(cs_quit_packet);
				packet_quit->type = CS_QUIT;
				retval = send(sock, sbuf, sizeof(cs_quit_packet), 0);
				break;
			}
			else {
				packet_ready->size = sizeof(cs_ready_packet);
				packet_ready->type = CS_UNREADY;
				retval = send(sock, sbuf, sizeof(cs_ready_packet), 0);
				break;
			}
		case US_PLAY:
			cout << "플레이 상태 돌입, 게임 결과 패킷 전송" << endl;
			packet_gameresult->size = sizeof(cs_gameresult_packet);
			packet_gameresult->type = CS_GAMERESULT;
			packet_gameresult->score = 1;
			cout << "결과 패킷 전송 완료" << endl;
			status = US_LOBBY;
			break;
		}


		

	}


}