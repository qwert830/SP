#define WIN32_LEAN_AND_MEAN  
#define INITGUID
#define UNICODE
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <thread>
#include <vector>
#include <array>
#include <unordered_set>
#include <iostream>
#include "../header/protocol.h"
#include <chrono>
#include <mutex>
#include <string>
#include <sqlext.h>  
#include <locale.h>


#define MAX(a,b)	((a)>(b))?(a):(b)
#define	MIN(a,b)	((a)<(b))?(a):(b)

using namespace std;


enum kind_of_work { RECV = 1, GAMEACTION, PERIODICACTION };

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
	int m_Score;
	float x;
	float y;
	float z;
	int m_RoomNumber;
	char m_Condition;
	unsigned char m_MoveDirection;

	EXOVER m_rxover;
	int m_packet_size;
	int m_prev_packet_size;
	char m_packet[MAX_PACKET_SIZE];
	Client() {
		m_ID = L"Temp";
		m_Score = 0;
		m_IsConnected = false;
		x = 0;
		y = 0;
		z = 0;
		m_RoomNumber = LOBBYNUMBER;
		m_Condition = US_WAIT;
		m_MoveDirection = STOP_DR;

		ZeroMemory(&m_rxover.m_over, sizeof(WSAOVERLAPPED));
		m_rxover.m_wsabuf.buf = m_rxover.m_iobuf;
		m_rxover.m_wsabuf.len = sizeof(m_rxover.m_wsabuf.buf);
		m_rxover.work = RECV;
		m_prev_packet_size = 0;
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
		m_RoomStatus = RS_EMPTY;
	}

	bool join(const int id) {
		if (m_CurrentNum < MAX_ROOMLIMIT) {
			m_JoinIdList.insert(id);
			m_CurrentNum++;
			if (m_CurrentNum == MAX_ROOMLIMIT)
				m_RoomStatus = RS_FULL;
			else
				m_RoomStatus = RS_JOINABLE;
			return true;
		}

		return false;
	}

	void quit(const int id) {
		m_JoinIdList.erase(id);
		m_CurrentNum--;
		if (m_CurrentNum == 0)
			m_RoomStatus = RS_EMPTY;
		else
			m_RoomStatus = RS_JOINABLE;
	}

	bool gosign(array <Client, MAX_USER>& g_clients) {
		if (m_CurrentNum % 2 != 0)
			return false;
		for (int d : m_JoinIdList) {
			if (g_clients[d].m_Condition != US_READY)
				return false;
		}
		m_RoomStatus = RS_PLAY;
		for (int d : m_JoinIdList) {
			g_clients[d].m_Condition = US_PLAY;
		}
		return true;
	}

};

HANDLE ghiocp;
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
								if (id == szID) {
									g_clients[key].m_ID = szID;
									g_clients[key].m_Score = szScore;
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
			idp.type = SC_REGISTSUCCESS;
		}
		else
			idp.type = SC_REGISTFAIL;
		SendPacket(id, &idp);
	}
		break;
	case CS_LOGIN:
	{
		sc_id_packet idp;
		idp.size = sizeof(sc_id_packet);
		cs_userinfo_packet* packet_login = reinterpret_cast<cs_userinfo_packet*>(packet);
		for (int i = 0; i < MAX_USER; ++i) {
			if (g_clients[i].m_ID == packet_login->id) {
				idp.type = SC_LOGINFAIL;
				SendPacket(id, &idp);
				return;
			}
		}
		if (DBCall_Login(id, packet_login->id, packet_login->password)) {
			idp.type = SC_LOGINSUCCESS;
		}
		else
			idp.type = SC_LOGINFAIL;
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
			if (0 <= packet_join->roomnumber && packet_join->roomnumber < MAX_ROOMNUMBER
				&& g_rooms[packet_join->roomnumber].m_RoomStatus != RS_FULL
				&& g_rooms[packet_join->roomnumber].m_RoomStatus != RS_PLAY) {
				g_rooms[packet_join->roomnumber].m_mJoinIdList.lock();
				if (g_rooms[packet_join->roomnumber].join(id)) {
					g_clients[id].m_RoomNumber = packet_join->roomnumber;
					sc_player_join_packet p;
					p.type = SC_JOIN_PLAYER;
					p.size = sizeof(sc_player_join_packet);
					wcscpy(p.id, g_clients[id].m_ID.c_str());
					//방 인원 전원에게 해당 아이디가 조인했음을 알림
					for (int d : g_rooms[packet_join->roomnumber].m_JoinIdList) {
						if (d == id) continue;
						SendPacket(d, &p);
					}
					//해당 아이디에게 방에 접속중인 인원의 정보를 보냄
					for (int d : g_rooms[packet_join->roomnumber].m_JoinIdList) {
						if (d == id) continue;
						wcscpy(p.id, g_clients[d].m_ID.c_str());
						SendPacket(id, &p);
					}
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
				if (g_rooms[i].m_RoomStatus == RS_EMPTY || g_rooms[i].m_RoomStatus == RS_JOINABLE) {
					if (g_rooms[i].join(id)) {
						g_rooms[i].m_mJoinIdList.unlock();
						g_clients[id].m_RoomNumber = i;
						sc_player_join_packet p;
						p.type = SC_JOIN_PLAYER;
						p.size = sizeof(sc_player_join_packet);
						wcscpy(p.id, g_clients[id].m_ID.c_str());
						//방 인원 전원에게 해당 아이디가 조인했음을 알림
						for (int d : g_rooms[i].m_JoinIdList) {
							if (d == id) continue;
							SendPacket(d, &p);
						}
						//해당 아이디에게 방에 접속중인 인원의 정보를 보냄
						for (int d : g_rooms[i].m_JoinIdList) {
							if (d == id) continue;
							wcscpy(p.id, g_clients[d].m_ID.c_str());
							SendPacket(id, &p);
						}
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
	{
		g_rooms[g_clients[id].m_RoomNumber].m_mJoinIdList.lock();
		g_rooms[g_clients[id].m_RoomNumber].quit(id);
		sc_player_quit_packet p;
		wcscpy(p.id, g_clients[id].m_ID.c_str());
		p.type = SC_QUIT_PLAYER;
		p.size = sizeof(sc_player_quit_packet);
		//남은 방 인원 전원에게 해당 아이디가 퇴장했음을 알림
		for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
			SendPacket(d, &p);
		}
		g_rooms[g_clients[id].m_RoomNumber].m_mJoinIdList.unlock();
		g_clients[id].m_RoomNumber = LOBBYNUMBER;
		break;
	}
	case CS_READY:
		g_clients[id].m_Condition = US_READY;
		if (g_rooms[g_clients[id].m_RoomNumber].gosign(g_clients)) {
			g_clients[id].m_Condition = US_PLAY;
			sc_usercondition_packet p;
			p.size = sizeof(sc_usercondition_packet);
			p.type = SC_GO;
			for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
				SendPacket(d, &p);
			}
		}
		else {
			sc_usercondition_packet p;
			p.size = sizeof(sc_usercondition_packet);
			p.type = US_READY;
			wcscpy(p.id, g_clients[id].m_ID.c_str());
			for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
				SendPacket(d, &p);
			}
		}
		break;
	case CS_UNREADY:
		g_clients[id].m_Condition = US_WAIT;
		sc_usercondition_packet p;
		p.size = sizeof(sc_usercondition_packet);
		p.type = US_WAIT;
		wcscpy(p.id, g_clients[id].m_ID.c_str());
		for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
			SendPacket(d, &p);
		}
		break;
	case CS_GAMERESULT:
	{
		cs_gameresult_packet* packet_gs = reinterpret_cast<cs_gameresult_packet*>(packet);
		DBCall_SetData(id, g_clients[id].m_Score + packet_gs->score);

		g_rooms[g_clients[id].m_RoomNumber].m_mJoinIdList.lock();
		g_rooms[g_clients[id].m_RoomNumber].quit(id);
		g_rooms[g_clients[id].m_RoomNumber].m_mJoinIdList.unlock();
		g_clients[id].m_RoomNumber = LOBBYNUMBER;
		break;
	}
	{
	case STOP_DR:
	case LEFT_DR:
	case RIGHT_DR:
	case UP_DR:
	case DOWN_DR:
	case ULEFT_DR:
	case URIGHT_DR:
	case DLEFT_DR:
	case DRIGHT_DR:
		g_clients[id].m_MoveDirection = packet[1];
		{
			sc_movestatus_packet p;
			p.size = sizeof(sc_movestatus_packet);
			p.type = packet[1];
			wcscpy(p.id, g_clients[id].m_ID.c_str());
			p.x = g_clients[id].x;
			p.y = g_clients[id].y;
			for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
				SendPacket(d, &p);
			}
		}
		break;
	}
	default:
		cout << "Unkown Packet Type from Client [" << id << "]\n";
		return;
	}
}

void DisconnectPlayer(int id)
{
	sc_player_quit_packet p;
	memcpy(p.id, g_clients[id].m_ID.c_str(), sizeof(g_clients[id].m_ID.c_str()));
	p.type = SC_QUIT_PLAYER;
	p.size = sizeof(p);
	if (g_clients[id].m_RoomNumber != -1) {
		for (int d : g_rooms[g_clients[id].m_RoomNumber].m_JoinIdList) {
			if (d == id) continue;
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
		else if (0 == io_size)
		{
			cout << "Error in GQCS?" << endl;
			DisconnectPlayer(key);
			continue;
		}

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
		else if (GAMEACTION == p_over->work)
		{
			switch (g_clients[key].m_MoveDirection) {
			case LEFT_DR:
				g_clients[key].x--;
				break;
			case RIGHT_DR:
				g_clients[key].x++;
				break;
			case UP_DR:
				g_clients[key].y--;
				break;
			case DOWN_DR:
				g_clients[key].y++;
				break;
			case ULEFT_DR:
				g_clients[key].y -= 0.7;
				g_clients[key].x -= 0.7;
				break;
			case URIGHT_DR:
				g_clients[key].y -= 0.7;
				g_clients[key].x += 0.7;
				break;
			case DLEFT_DR:
				g_clients[key].y += 0.7;
				g_clients[key].x -= 0.7;
				break;
			case DRIGHT_DR:
				g_clients[key].y += 0.7;
				g_clients[key].x += 0.7;
				break;
			}

		}
		else if (PERIODICACTION == p_over->work) {
			sc_movestatus_packet p;
			p.size = sizeof(sc_movestatus_packet);
			p.type = g_clients[key].m_MoveDirection;
			wcscpy(p.id, g_clients[key].m_ID.c_str());
			p.x = g_clients[key].x;
			p.y = g_clients[key].y;
			SendPacket(key, &p);
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

		cout << "ID of new Client is [" << id << "]\n";

		g_clients[id].m_Socket = cs;
		g_clients[id].m_packet_size = 0;
		g_clients[id].m_prev_packet_size = 0;
		g_clients[id].m_RoomNumber = LOBBYNUMBER;
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(cs), ghiocp, id, 0);
		g_clients[id].m_IsConnected = true;
		wchar_t itr[10];
		_itow(id, itr, 10);
		g_clients[id].m_ID += itr;
		StartRecv(id);	

	}
}

void Timer_Thread() {
	double delay;
	int count = 0;
	EXOVER over, periodic;
	ZeroMemory(&over.m_over, sizeof(WSAOVERLAPPED));
	over.m_wsabuf.buf = over.m_iobuf;
	over.m_wsabuf.len = sizeof(over.m_wsabuf.buf);
	over.work = GAMEACTION;

	ZeroMemory(&periodic.m_over, sizeof(WSAOVERLAPPED));
	periodic.m_wsabuf.buf = periodic.m_iobuf;
	periodic.m_wsabuf.len = sizeof(periodic.m_wsabuf.buf);
	periodic.work = PERIODICACTION;


	while (true) {
		chrono::system_clock::time_point t1 = chrono::system_clock::now();
		count = (count + 1) % 60;
		for (Room& d : g_rooms) {
			if (d.m_RoomStatus != RS_PLAY) continue;
			for (int d : d.m_JoinIdList) {
				PostQueuedCompletionStatus(ghiocp, 1, d, &over.m_over);
				if(!count)
					PostQueuedCompletionStatus(ghiocp, 1, d, &periodic.m_over);
			}
		}
		chrono::system_clock::time_point t2 = chrono::system_clock::now();
		chrono::duration<double> sptime = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
		delay = MAX(0, 1 - sptime.count());
		Sleep(delay * 16.6);
	}
}

int main()
{
	vector <thread> w_threads;
	Initialize();
	for (int i = 0; i < 2; ++i)
		w_threads.push_back(thread{ worker_thread });

	thread a_thread{ Accept_Threads };
	thread t_thread{ Timer_Thread };

	for (auto &t : w_threads)
		t.join();

	a_thread.join();
	t_thread.join();
}