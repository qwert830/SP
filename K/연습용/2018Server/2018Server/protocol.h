
#define MAX_BUFF_SIZE   4000
#define MAX_PACKET_SIZE  513

#define MAX_USER 2000

#define MY_SERVER_PORT  4000

#define MAX_STR_SIZE  100

#define MAX_NICK_SIZE 12

#define MAX_ROOMNUMBER 1000

#define CS_CHAT	 1

#define CS_REGIST 100

#define SC_PUT_PLAYER    1
#define SC_REMOVE_PLAYER 2
#define SC_CHAT			 3

#pragma pack (push, 1)
// 클라이언트 -> 서버------------------------

struct cs_packet_regist {
	unsigned char size;
	unsigned char type;
	unsigned int roomnumber;
};

struct cs_packet_chat {
	unsigned char size;
	unsigned char type;
	wchar_t message;
};
//-------------------------------------------

struct sc_packet_put_player {
	unsigned char size;
	unsigned char type;
	unsigned int id;
};
struct sc_packet_remove_player {
	unsigned char size;
	unsigned char type;
	unsigned int id;
};

struct sc_packet_chat {
	unsigned char size;
	unsigned char type;
	wchar_t message;
};

#pragma pack (pop)