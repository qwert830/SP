#define MAX_BUFF_SIZE   4096
#define MAX_PACKET_SIZE  255

#define FRAME_PER_SEC (1.0f / 60.0f)

#define MAX_USER 2000

#define MY_SERVER_PORT  4000

#define MAX_IDSIZE 10

#define LOBBYNUMBER -1
#define MAX_ROOMNUMBER 200
#define MAX_ROOMLIMIT 10

enum room_status { RS_JOINABLE = 1, RS_FULL = 2, RS_EMPTY = 3, RS_PLAY = 4 };

enum user_status { US_WAIT, US_READY, US_PLAY, US_LOBBY};

enum team_kind{
	RED_TEAM = 1,
	BLUE_TEAM,
	RED_READER,
	BLUE_READER
};

enum move_direction {
	STOP_DR = 100,
	UP_DR,
	DOWN_DR,
	LEFT_DR,
	RIGHT_DR,
	ULEFT_DR,
	URIGHT_DR,
	DLEFT_DR,
	DRIGHT_DR,
};

//무브디렉션이랑 겹치면 안됨. 패킷종류가 50개 넘으면 무브디렉션 시작점이든 패킷종류 시작점이든 둘중하날 바꿔야함.

enum CS_PacketKind { 
	CS_REFRESH = 50,
	CS_JOIN,
	CS_AUTOJOIN,
	CS_QUIT,
	CS_REGIST,
	CS_LOGIN,
	CS_READY,
	CS_UNREADY,
	CS_GAMERESULT,
	CS_MOVE,
	CS_ANGLE,
	CS_ATTACK,
	CS_JUMP
};

enum SC_PacketKind {
	SC_REFRESH = 50,
	SC_JOIN_PLAYER,
	SC_QUIT_PLAYER,
	SC_LOGINFAIL,
	SC_LOGINSUCCESS,
	SC_REGISTFAIL,
	SC_REGISTSUCCESS,
	SC_GO,
	SC_READY,
	SC_UNREADY,
	SC_POSITION,
	SC_ANGLE,
	SC_GAMEOVER_REDWIN,
	SC_GAMEOVER_BLUEWIN,
	SC_HIT,
	SC_DEAD,
	SC_ATTACK,
	SC_TIMER,
	SC_JUMP
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
	wchar_t attacker[10];
	wchar_t hitted[10];
	float cx, cy, cz;
	float px, py, pz;
};

struct cs_angle_packet {
	unsigned char size;
	unsigned char type;
	float lookx, looky, lookz;
	float rx, ry, rz;
};

struct cs_movestatus_packet {
	unsigned char size;
	unsigned char type;
	float x, y, z;
};

struct cs_jump_packet {
	unsigned char size;
	unsigned char type;
	float x, y, z;
	float power;
};
// 서버 -> 클라이언트------------------------

//id와 패스워드를 똑바로 쳤는지 확인하는 패킷
struct sc_id_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
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
	unsigned char readystatus;
	wchar_t id[10];
};

struct sc_player_quit_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
};

struct sc_movestatus_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
	float x, y, z;
};

struct sc_timer_packet {
	unsigned char size;
	unsigned char type;
	int timer;
};

struct sc_angle_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
	float lookx, looky, lookz;
	float rx, ry, rz;
};

struct sc_attack_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
	float cx, cy, cz;
	float px, py, pz;
};

struct sc_hit_packet {
	unsigned char size;
	unsigned char type;
	unsigned int hp;
};

struct sc_gameover_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
};

struct sc_teaminfo_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
	float x, y, z;
	float r;
};

struct sc_jump_packet {
	unsigned char size;
	unsigned char type;
	wchar_t id[10];
	float x, y, z;
	float power;
};

#pragma pack (pop)