#define MAX_BUFF_SIZE   4096
#define MAX_PACKET_SIZE  255

#define MAX_USER 2000

#define MY_SERVER_PORT  4000

#define MAX_IDSIZE 10

#define LOBBYNUMBER -1
#define MAX_ROOMNUMBER 200
#define MAX_ROOMLIMIT 10

enum room_status { RS_JOINABLE = 1, RS_FULL = 2, RS_EMPTY = 3, RS_PLAY = 4 };

enum user_status { US_WAIT, US_READY, US_PLAY, };

enum CS_PacketKind { 
	CS_REFRESH = 50,
	CS_JOIN,
	CS_AUTOJOIN,
	CS_QUIT,
	CS_REGIST,
	CS_LOGIN,
	CS_READY,
};

enum SC_PacketKind {
	SC_REFRESH = 50,
	SC_PUT_PLAYER,
	SC_JOIN_PLAYER,
	SC_QUIT_PLAYER,
	SC_FAIL,
	SC_SUCCESS,
	SC_GO
};
#pragma pack (push, 1)
// 클라이언트 -> 서버------------------------

struct cs_userinfo_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
	wchar_t password[10];
};

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

struct cs_ready_packet {
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

struct sc_id_packet {
	unsigned char size;
	unsigned char type;
};

struct sc_roomstatus_packet {
	unsigned char size;
	unsigned char type;
	unsigned char roomstatus[200];
};

struct sc_usercondition_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
};

struct sc_player_join_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
};

struct sc_player_quit_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
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