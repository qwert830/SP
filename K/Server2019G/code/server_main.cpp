#define WIN32_LEAN_AND_MEAN  
#define INITGUID

#include <WinSock2.h>

#pragma comment (lib, "ws2_32.lib")

#include <thread>
#include <vector>
#include <array>
#include <unordered_set>
#include <iostream>
#include "../header/protocol.h"
#include <chrono>
#include <mutex>
#include <string>

#define UNICODE
#include <sqlext.h>  
#include <locale.h>


#define MAX(a,b)	((a)>(b))?(a):(b)
#define	MIN(a,b)	((a)<(b))?(a):(b)

using namespace std;


enum kind_of_work { RECV = 1 };


HANDLE ghiocp;

struct EXOVER {
	WSAOVERLAPPED m_over;
	char m_iobuf[MAX_BUFF_SIZE];
	WSABUF m_wsabuf;
	char work;
};

class Client {
public:
	SOCKET m_Socket;
	bool m_IsConnected;
	wstring m_ID;
	int Score;
	int m_RoomNumber;
	EXOVER m_rxover;
	int m_packet_size;
	int m_prev_packet_size;
	char m_packet[MAX_PACKET_SIZE];

	Client() {
		m_IsConnected = false;
		m_RoomNumber = LOBBYNUMBER;
		m_prev_packet_size = 0;
		ZeroMemory(&m_rxover.m_over, sizeof(WSAOVERLAPPED));
		m_rxover.m_wsabuf.buf = m_rxover.m_iobuf;
		m_rxover.m_wsabuf.len = sizeof(m_rxover.m_wsabuf.buf);
		m_rxover.work = RECV;
	}
};

class Room {
public:
	unordered_set<int> m_JoinIdList;
	unsigned char m_CurrentNum;
	mutex m_mJoinIdList;
	unsigned char m_RoomStatus;
	Room() {
		m_JoinIdList.clear();
		m_CurrentNum = 0;
		m_RoomStatus = EMPTY;
	}

	bool join(const int id) {
		if (m_CurrentNum < MAX_ROOMLIMIT) {
			m_JoinIdList.insert(id);
			m_CurrentNum++;
			if (m_CurrentNum == MAX_ROOMLIMIT)
				m_RoomStatus = FULL;
			else
				m_RoomStatus = JOINABLE;
			return true;
		}
		
		return false;
	}

	void quit(const int id) {
		m_JoinIdList.erase(id);
		m_CurrentNum--;
		if(m_CurrentNum == 0) 
			m_RoomStatus = EMPTY;
		else
			m_RoomStatus = JOINABLE;
	}
};


array <Client, MAX_USER> g_clients;
array <Room, MAX_ROOMNUMBER> g_rooms;




void error_display(const char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"에러" << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
	while (true);
}

void ErrorDisplay(const char * location)
{
	error_display(location, WSAGetLastError());
}

void Initialize()
{
	setlocale(LC_ALL, "korean");
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	ghiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
}

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];

	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

bool DBCall_Login(int key, const wstring& id, const wstring& password) {
	bool res = false;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstate = 0;
	SQLRETURN retcode;

	SQLWCHAR szID[MAX_IDSIZE] = { 0, };
	
	SQLINTEGER szScore = 0;
	SQLLEN cbID = 0, cbScore = 0;

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GS2019G", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstate);
					wstring command = L"EXEC USERLOGIN ";
					command += id + L"," + password;
					retcode = SQLExecDirect(hstate, (SQLWCHAR *)command.c_str(), SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						// Bind columns
						SQLBindCol(hstate, 1, SQL_C_WCHAR, szID, sizeof(szID), &cbID);
						SQLBindCol(hstate, 2, SQL_INTEGER, &szScore, sizeof(int), &cbScore);
						// Fetch and print each row of data. On an error, display a message and exit.  
						while (1) {
							retcode = SQLFetch(hstate);
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								cout << "요기까진 왔는데?" << endl;
								
								if (id == szID) {
									g_clients[key].m_ID = szID;
									g_clients[key].Score = szScore;
									res = true;
								}
							}
							else
								break;
						}
					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						SQLCancel(hstate);
						SQLFreeHandle(SQL_HANDLE_STMT, hstate);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return res;
}

int DBCall_GetData(const wstring& id) {
	int res = -1;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstate = 0;
	SQLRETURN retcode;

	SQLWCHAR szID[MAX_IDSIZE] = { 0, };
	SQLWCHAR szPassword[MAX_IDSIZE] = { 0, };
	SQLINTEGER szScore = 0;
	SQLLEN cbID = 0, cbPassword = 0, cbScore = 0;

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GS2019G", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstate);
					wstring command = L"EXEC GETDATA ";
					command += id;
					retcode = SQLExecDirect(hstate, (SQLWCHAR *)command.c_str(), SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						// Bind columns
						SQLBindCol(hstate, 1, SQL_C_CHAR, &szID, MAX_IDSIZE, &cbID);
						SQLBindCol(hstate, 2, SQL_INTEGER, &szPassword, 1, &cbPassword);
						SQLBindCol(hstate, 3, SQL_INTEGER, &szScore, 1, &cbScore);
						// Fetch and print each row of data. On an error, display a message and exit.  
						while (1) {
							retcode = SQLFetch(hstate);
							if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
								cout << "SQL에러!" << endl;
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								if (id == szID) {
									res = szScore;
								}
							}
							else
								break;
						}
					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						SQLCancel(hstate);
						SQLFreeHandle(SQL_HANDLE_STMT, hstate);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return res;
}

bool DBCall_Regist(int key, const wstring& id, const wstring& password) {
	bool res = false;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstate = 0;
	SQLRETURN retcode;
	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GS2019G", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				// Allocate statement handle  

				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstate);
					wstring command = L"EXEC USERREGIST ";
					command += id + L"," + password;
					retcode = SQLExecDirect(hstate, (SQLWCHAR *)command.c_str(), SQL_NTS);
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						res = true;
						SQLCancel(hstate);
						SQLFreeHandle(SQL_HANDLE_STMT, hstate);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return res;
}

void DBCall_SetData(int key, int score) {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstate = 0;
	SQLRETURN retcode;
	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GS2019G", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				// Allocate statement handle  

				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstate);
					wstring command = L"EXEC SETDATA ";
					wchar_t itoares[10];
					_itow(score, itoares, 10);
					command += g_clients[key].m_ID + L"," + itoares;
					retcode = SQLExecDirect(hstate, (SQLWCHAR *)command.c_str(), SQL_NTS);
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstate);
						SQLFreeHandle(SQL_HANDLE_STMT, hstate);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

void StartRecv(int id)
{
	unsigned long r_flag = 0;
	ZeroMemory(&g_clients[id].m_rxover.m_over, sizeof(WSAOVERLAPPED));
	int ret = WSARecv(g_clients[id].m_Socket, &g_clients[id].m_rxover.m_wsabuf, 1, NULL, &r_flag, &g_clients[id].m_rxover.m_over, NULL);
	if (0 != ret)
	{
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			error_display(" Recv Error ", err_no);
	}
}

inline void SendPacket(int id, void * ptr)
{
 	char *packet = reinterpret_cast<char *>(ptr);
	unsigned char sizepacket = packet[0];
	EXOVER *s_over = new EXOVER;
	s_over->work = -1;
	memcpy(s_over->m_iobuf, packet, sizepacket);
	s_over->m_wsabuf.buf = s_over->m_iobuf;
	s_over->m_wsabuf.len = sizepacket;
	ZeroMemory(&s_over->m_over, sizeof(WSAOVERLAPPED));
	int ret = WSASend(g_clients[id].m_Socket, &s_over->m_wsabuf, 1, NULL, 0, &s_over->m_over, NULL);
	if (0 != ret)
	{
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) error_display("Send error\n", err_no);
	}
}

inline void ProcessPacket(int id, char *packet)
{
	switch (packet[1])
	{
	case CS_REGIST:
	{
		sc_id_packet idp;
		idp.size = sizeof(sc_id_packet);
		cs_userinfo_packet* packet_regist = reinterpret_cast<cs_userinfo_packet*>(packet);
		if (DBCall_Regist(id, packet_regist->id, packet_regist->password)) {
			cout << id << " 번 유저 회원가입 성공" << endl;
			idp.type = SC_SUCCESS;
		}
		else
			idp.type = SC_FAIL;
		SendPacket(id, &idp);
	}
		break;
	case CS_LOGIN:
	{
		sc_id_packet idp;
		idp.size = sizeof(sc_id_packet);
		cs_userinfo_packet * packet_login = reinterpret_cast<cs_userinfo_packet*>(packet);
		if (DBCall_Login(id, packet_login->id, packet_login->password)) {
			cout << id << " 번 유저 로그인 성공" << endl;
			idp.type = SC_SUCCESS;
		}
		else
			idp.type = SC_FAIL;
		SendPacket(id, &idp);
	}
		break;
	case CS_REFRESH:
		sc_roomstatus_packet packet_rs;
		packet_rs.size = sizeof(sc_roomstatus_packet);
		packet_rs.type = SC_REFRESH;
		for (int i = 0; i < 200; ++i) {
			packet_rs.roomstatus[i] = g_rooms[i].m_RoomStatus;
		}
		SendPacket(id, &packet_rs);
		break;
	case CS_JOIN:
		if (g_clients[id].m_RoomNumber == LOBBYNUMBER)
		{
			cs_join_packet* packet_join = reinterpret_cast<cs_join_packet*>(packet);
			if (0 <= packet_join->roomnumber && packet_join->roomnumber < MAX_ROOMNUMBER) {
				g_rooms[packet_join->roomnumber].m_mJoinIdList.lock();
				if (g_rooms[packet_join->roomnumber].join(id)) {
					g_clients[id].m_RoomNumber = packet_join->roomnumber;
					sc_player_join_packet p;
					p.id = id;
					p.type = SC_JOIN_PLAYER;
					p.size = sizeof(p);
					////방 인원 전원에게 해당 아이디가 조인했음을 알림
					//for (int d : g_rooms[packet_join->roomnumber].m_JoinIdList) {
					//	SendPacket(d, &p);
					//}
					////해당 아이디에게 방에 접속중인 인원의 정보를 보냄
					//for (int d : g_rooms[packet_join->roomnumber].m_JoinIdList) {
					//	p.id = d;
					//	p.type = SC_PUT_PLAYER;
					//	p.size = sizeof(p);
					//	SendPacket(id, &p);
					//}
				}
				g_rooms[packet_join->roomnumber].m_mJoinIdList.unlock();
			}
		}
		break;
	case CS_AUTOJOIN:
		if(g_clients[id].m_RoomNumber == LOBBYNUMBER)
		{
			for (int i = 0; i < MAX_ROOMNUMBER; ++i) {
				g_rooms[i].m_mJoinIdList.lock();
				if (g_rooms[i].m_RoomStatus == EMPTY || g_rooms[i].m_RoomStatus == JOINABLE) {
					if (g_rooms[i].join(id)) {
						g_rooms[i].m_mJoinIdList.unlock();
						g_clients[id].m_RoomNumber = i;
						sc_player_join_packet p;
						p.id = id;
						p.type = SC_JOIN_PLAYER;
						p.size = sizeof(p);
						////방 인원 전원에게 해당 아이디가 조인했음을 알림
						//for (int d : g_rooms[i].m_JoinIdList) {
						//	SendPacket(d, &p);
						//}
						////해당 아이디에게 방에 접속중인 인원의 정보를 보냄
						//for (int d : g_rooms[i].m_JoinIdList) {
						//	if (id == d) continue;
						//	p.id = d;
						//	p.type = SC_PUT_PLAYER;
						//	p.size = sizeof(p);
						//	SendPacket(id, &p);
						//}
						break;
					}
					else {
						g_rooms[i].m_mJoinIdList.unlock();
						cout << "방입장 오류" << endl;
						break;
					}
				}
				else
					g_rooms[i].m_mJoinIdList.unlock();
			}
		}
		break;
	case CS_QUIT:
		if (g_clients[id].m_RoomNumber == LOBBYNUMBER) break;
		g_rooms[g_clients[id].m_RoomNumber].m_mJoinIdList.lock();
		g_rooms[g_clients[id].m_RoomNumber].quit(id);
		sc_player_quit_packet p;
		p.id = id;
		p.type = SC_QUIT_PLAYER;
		p.size = sizeof(p);
		//남은 방 인원 전원에게 해당 아이디가 퇴장했음을 알림
		//for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
		//	SendPacket(d, &p);
		//}
		g_rooms[g_clients[id].m_RoomNumber].m_mJoinIdList.unlock();
		g_clients[id].m_RoomNumber = -1;
		break;
	default:
		cout << "Unkown Packet Type from Client [" << id << "]\n";
		return;
	}
}

void DisconnectPlayer(int id)
{
	sc_player_quit_packet p;
	p.id = id;
	p.type = SC_QUIT_PLAYER;
	p.size = sizeof(p);
	if (g_clients[id].m_RoomNumber != -1) {
		for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
			SendPacket(d, &p);
		}
	}
	closesocket(g_clients[id].m_Socket);
	g_clients[id].m_IsConnected = false;
}

void worker_thread()
{
	while (true)
	{
		unsigned long io_size;
		unsigned long long iocp_key;
		WSAOVERLAPPED *over;
		BOOL ret = GetQueuedCompletionStatus(ghiocp, &io_size, &iocp_key, &over, INFINITE);
		int key = static_cast<int>(iocp_key);
		//cout << "Network I/O with Client [" << key << "]\n";
		if (FALSE == ret)
		{
			cout << "Error in GQCS" << endl;
			DisconnectPlayer(key);
			continue;
		}
		//else if (0 == io_size)
		//{
		//	cout << "Error in GQCS?" << endl;
		//	DisconnectPlayer(key);
		//	continue;
		//}

		EXOVER *p_over = reinterpret_cast<EXOVER*>(over);
		if (RECV == p_over->work)
		{
			//cout << "Packet From Client [" << key << "]\n";
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
			//cout << "A packet was sent to Client[" << key << "]\n";
			delete p_over;
		}
	}
}

void Accept_Threads()
{
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN bind_addr;
	ZeroMemory(&bind_addr, sizeof(SOCKADDR_IN));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(MY_SERVER_PORT);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	::bind(s, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
	listen(s, 1000);

	while (true)
	{
		SOCKADDR_IN c_addr;
		ZeroMemory(&c_addr, sizeof(SOCKADDR_IN));
		c_addr.sin_family = AF_INET;
		c_addr.sin_port = htons(MY_SERVER_PORT);
		c_addr.sin_addr.s_addr = INADDR_ANY;
		int addr_size = sizeof(sockaddr);

		SOCKET cs = WSAAccept(s, reinterpret_cast<sockaddr *>(&c_addr), &addr_size, NULL, NULL);
		if (INVALID_SOCKET == cs)
		{
			ErrorDisplay("In Accept Thread:WSAAccept()");
			continue;
		}

		//cout << "New Client Connected!\n";
		int id = -1;
		for (int i = 0; i < MAX_USER; ++i) {
			if (false == g_clients[i].m_IsConnected)
			{
				id = i;
				break;
			}
		}

		if (-1 == id) {
			cout << "MAX_USER_Exceeded\n";
			continue;
		}
		if(id % 100 == 0 || id == MAX_USER -1)
			cout << "ID of new Client is [" << id << "]\n";

		g_clients[id].m_Socket = cs;
		g_clients[id].m_packet_size = 0;
		g_clients[id].m_prev_packet_size = 0;
		g_clients[id].m_RoomNumber = LOBBYNUMBER;
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(cs), ghiocp, id, 0);
		g_clients[id].m_IsConnected = true;
		StartRecv(id);
		
	}
}

int main()
{
	vector <thread> w_threads;
	Initialize();
	for (int i = 0; i < 2; ++i)
		w_threads.push_back(thread{ worker_thread });

	thread a_thread{ Accept_Threads };

	for (auto &t : w_threads)
		t.join();

	a_thread.join();

}
