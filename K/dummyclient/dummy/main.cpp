#define WIN32_LEAN_AND_MEAN  

#include <WinSock2.h>
#pragma comment (lib, "ws2_32.lib")

#include <iostream>
#include "../../Server2019G/header/protocol.h"
#include <thread>
#include <vector>
#include <array>
#include <atomic>

#define SERVERPORT 9000
#define BUF_SIZE    512

using namespace std;
using namespace chrono;

enum kind_of_work { RECV = 1 };

HANDLE g_hiocp;

atomic_int num_connections;

struct EXOVER {
	WSAOVERLAPPED m_over;
	char m_iobuf[MAX_BUFF_SIZE];
	WSABUF m_wsabuf;
	char work;
};


class Client {
public:
	SOCKET m_s;
	int m_id;
	int m_roomnumber;
	bool m_connect;
	EXOVER m_rxover;
	int m_packet_size;
	int m_prev_packet_size;
	char m_packet[MAX_PACKET_SIZE];

	Client() {
		m_roomnumber = -1;
		m_prev_packet_size = 0;
		m_connect = false;
		ZeroMemory(&m_rxover.m_over, sizeof(WSAOVERLAPPED));
		m_rxover.m_wsabuf.buf = m_rxover.m_iobuf;
		m_rxover.m_wsabuf.len = sizeof(m_rxover.m_wsabuf.buf);
		m_rxover.work = RECV;
	}
};

array <Client, MAX_USER> g_clients;
high_resolution_clock::time_point last_connect_time;



void ProcessPacket(int id, char *packet)
{
	sc_player_join_packet* sc_joinPacket;
	sc_player_quit_packet* sc_quitPacket;
	switch (packet[1])
	{
	case SC_JOIN_PLAYER:
		sc_joinPacket = reinterpret_cast<sc_player_join_packet*>(packet);

		break;
	case SC_QUIT_PLAYER:
		sc_quitPacket = reinterpret_cast<sc_player_quit_packet*>(packet);

		break;
	}
}

void StartRecv(int id)
{
	unsigned long r_flag = 0;
	ZeroMemory(&g_clients[id].m_rxover.m_over, sizeof(WSAOVERLAPPED));
	int ret = WSARecv(g_clients[id].m_s, &g_clients[id].m_rxover.m_wsabuf, 1, NULL, &r_flag, &g_clients[id].m_rxover.m_over, NULL);
}
void SendPacket(int id, void * ptr)
{
	char *packet = reinterpret_cast<char *>(ptr);
	EXOVER *s_over = new EXOVER;
	s_over->work = -1;
	memcpy(s_over->m_iobuf, packet, packet[0]);
	s_over->m_wsabuf.buf = s_over->m_iobuf;
	s_over->m_wsabuf.len = s_over->m_iobuf[0];
	ZeroMemory(&s_over->m_over, sizeof(WSAOVERLAPPED));
	int ret = WSASend(g_clients[id].m_s, &s_over->m_wsabuf, 1, NULL, 0, &s_over->m_over, NULL);
}

void Worker_Thread()
{
	while (true)
	{
		unsigned long io_size;
		unsigned long long iocp_key;
		WSAOVERLAPPED *over;
		BOOL ret = GetQueuedCompletionStatus(g_hiocp, &io_size, &iocp_key, &over, INFINITE);
		int key = static_cast<int>(iocp_key);

		if (FALSE == ret)
		{
			cout << "Error in GQCS" << endl;
			continue;
		}
		if (0 == io_size)
		{
			continue;
		}

		EXOVER *p_over = reinterpret_cast<EXOVER*>(over);
		if (RECV == p_over->work)
		{
			int work_size = io_size;
			char *wptr = p_over->m_iobuf;
			while (0 < work_size)
			{
				int p_size;
				if (0 != g_clients[key].m_packet_size)
					p_size = g_clients[key].m_packet_size;
				else
				{
					p_size = wptr[0];
					g_clients[key].m_packet_size = p_size;
				}

				int need_size = p_size - g_clients[key].m_prev_packet_size;
				if (need_size <= work_size)
				{
					memcpy(g_clients[key].m_packet + g_clients[key].m_prev_packet_size, wptr, need_size);
					ProcessPacket(key, g_clients[key].m_packet);
					g_clients[key].m_prev_packet_size = 0;
					g_clients[key].m_packet_size = 0;
					work_size -= need_size;
					wptr += need_size;
				}
				else
				{
					memcpy(g_clients[key].m_packet + g_clients[key].m_prev_packet_size, wptr, work_size);
					g_clients[key].m_prev_packet_size += work_size;
					work_size -= work_size;
					wptr += work_size;
				}
			}
			StartRecv(key);
		}
		else
		{

			delete p_over;
		}
	}
}
void Adjust_Number_Of_Client()
{

	if (num_connections >= MAX_USER) return;
	if (high_resolution_clock::now() < last_connect_time + 100ms) return;

	g_clients[num_connections].m_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(MY_SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int Result = WSAConnect(g_clients[num_connections].m_s, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);
	if (Result == -1) {
		return;
	}
	g_clients[num_connections].m_packet_size = 0;
	g_clients[num_connections].m_prev_packet_size = 0;
	ZeroMemory(&g_clients[num_connections].m_rxover, sizeof(g_clients[num_connections].m_rxover));
	g_clients[num_connections].m_rxover.work = RECV;
	g_clients[num_connections].m_rxover.m_wsabuf.buf =
		reinterpret_cast<CHAR *>(g_clients[num_connections].m_rxover.m_iobuf);
	g_clients[num_connections].m_rxover.m_wsabuf.len = sizeof(g_clients[num_connections].m_rxover.m_iobuf);

	DWORD recv_flag = 0;
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_clients[num_connections].m_s), g_hiocp, num_connections, 0);
	WSARecv(g_clients[num_connections].m_s, &g_clients[num_connections].m_rxover.m_wsabuf, 1,
		NULL, &recv_flag, &g_clients[num_connections].m_rxover.m_over, NULL);
	g_clients[num_connections].m_connect = true;
	num_connections++;

}

void Test_Threads()
{
	//접속부
	while(1){
		Adjust_Number_Of_Client();
		if (num_connections == MAX_USER) {
			break;
		}
	}
	//방입장부
	for (int i = 0; i < MAX_USER / 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			cs_join_packet jpacket;
			jpacket.type = CS_JOIN;
			jpacket.size = sizeof(cs_join_packet);
			jpacket.roomnumber = i;
			SendPacket(i * 10 + j, &jpacket);
		}
	}
	cout << "입장완료" << endl;
	//방퇴장부
	for (int i = 0; i < MAX_USER; ++i) {
		cs_quit_packet qpacket;
		qpacket.type = CS_QUIT;
		qpacket.size = sizeof(cs_quit_packet);
		SendPacket(i, &qpacket);
	}
	cout << "퇴장완료" << endl;
}


int main()
{
	vector <thread> w_threads;

	num_connections = 0;
	last_connect_time = high_resolution_clock::now();
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	g_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	for (int i = 0; i < 2; ++i)
		w_threads.push_back(thread{ Worker_Thread });

	thread a_thread{ Test_Threads };

	for (auto &t : w_threads)
		t.join();

	a_thread.join();
}