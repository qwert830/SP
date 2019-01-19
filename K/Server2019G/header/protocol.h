
#define MAX_BUFF_SIZE   4096
#define MAX_PACKET_SIZE  255

#define MAX_USER 2000

#define MY_SERVER_PORT  4000

#define LOBBYNUMBER -1
#define MAX_ROOMNUMBER 200
#define MAX_ROOMLIMIT 10

enum room_status { JOINABLE = 1, FULL = 2, EMPTY = 3, PLAY = 4 };

#define CS_REFRESH 98
#define CS_JOIN 99
#define CS_AUTOJOIN 100
#define CS_QUIT 101

#define SC_REFRESH 97;
#define SC_PUT_PLAYER  98
#define SC_JOIN_PLAYER 99
#define SC_QUIT_PLAYER 100

#pragma pack (push, 1)
// 클라이언트 -> 서버------------------------
struct cs_refresh_packet {
	unsigned char size;
	unsigned char type;
};

struct cs_join_packet {
	unsigned char size;
	unsigned char type;
	unsigned int roomnumber;
};

struct cs_autojoin_packet {
	unsigned char size;
	unsigned char type;
};

struct cs_quit_packet {
	unsigned char size;
	unsigned char type;
};

struct cs_attack_packet {
	unsigned char size;
	unsigned char type;
	float up;
	float right;
	float look;
};

struct cs_angle_packet {
	unsigned char size;
	unsigned char type;
	float up;
	float right;
	float look;
};

struct cs_status_packet {
	unsigned char size;
	unsigned char type;
	unsigned char status;
};
// 서버 -> 클라이언트------------------------

struct sc_roomstatus_packet {
	unsigned char size;
	unsigned char type;
	unsigned char roomstatus[200];
};

struct sc_player_join_packet {
	unsigned char size;
	unsigned char type;
	unsigned int id;
};

struct sc_player_quit_packet {
	unsigned char size;
	unsigned char type;
	unsigned int id;
};

struct sc_status_packet {
	unsigned char size;
	unsigned char type;
	unsigned char status;
};

struct sc_timer_packet {
	unsigned char size;
	unsigned char type;
	unsigned int timer;
};

struct sc_angle_packet {
	unsigned char size;
	unsigned char type;
	float up;
	float right;
	float look;
};

struct sc_attack_packet {
	unsigned char size;
	unsigned char type;
	float up;
	float right;
	float look;
};
struct sc_hit_packet {
	unsigned char size;
	unsigned char type;
	unsigned int hp;
};

#pragma pack (pop)