
#define MAX_BUFF_SIZE   4097
#define MAX_PACKET_SIZE  257

#define MAX_USER 2000

#define MY_SERVER_PORT  4000

#define MAX_STR_SIZE  100

#define MAX_NICK_SIZE 12

#define MAX_ROOMNUMBER 200
#define MAX_ROOMLIMIT 10


#define CS_REGIST 100

#define SC_PUT_PLAYER    99
#define SC_REMOVE_PLAYER 100

#pragma pack (push, 1)
// 클라이언트 -> 서버------------------------

struct cs_packet_regist {
	unsigned char size;
	unsigned char type;
	unsigned int roomnumber;
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

struct sc_status_packet {
	unsigned char size;
	unsigned char type;
};

struct sc_timer_packet {
	unsigned char size;
	unsigned char type;
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