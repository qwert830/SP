#pragma once
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" )
#pragma comment(lib, "ws2_32")
#include <winsock2.h>

#include "../../../K/Server2019G/header/protocol.h"

#include <iostream>
using namespace std;

#define	WM_SOCKET	WM_USER + 1
#define BUF_SIZE    512
#define MY_SERVER_PORT  4000

class NetworkModule
{
	SOCKET	m_mysocket;
	WSABUF	send_wsabuf;
	char 	send_buffer[BUF_SIZE];
	WSABUF	recv_wsabuf;
	char	recv_buffer[BUF_SIZE];
	char	packet_buffer[BUF_SIZE];
	DWORD	in_packet_size = 0;
	int		saved_packet_size = 0;
public:
	NetworkModule();
	~NetworkModule();

	virtual void ProcessPacket(char *ptr) = 0;

	void init_Network(HWND& hWnd);
	void ReadPacket(SOCKET sock);

};

