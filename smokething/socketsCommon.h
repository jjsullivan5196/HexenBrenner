#ifndef SOCKETS_GAME_H
#define SOCKETS_GAME_H
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_FLAGS 0
#define NODATA "NULL"
#define NBYTES 5

const BYTE HEAD_TERMINATE = 0;
const BYTE HEAD_CONNECT = 1;
const BYTE HEAD_GETSTATE = 2;
const BYTE HEAD_GETPLAYERS = 3;
const BYTE HEAD_PUSHPLAYER = 4;

const BYTE MODE_LOBBY = 1;
const BYTE MODE_RUN = 2;
const BYTE MODE_OVER = 3;

// Serverside
SOCKET getListenSocket(); // Setup listener socket
SOCKET getClientSocket(SOCKET ListenSocket); // Get client connection
int cleanupSockets(SOCKET ClientSocket); // Cleanup client connection

// Clientside
SOCKET clientConnect(char* server); // Connect to server
int dropClient(SOCKET ConnectSocket); // Cleanup connection

typedef struct {
	BYTE id;
	BYTE alive;
	BYTE connected;
	float x, y, z;
} PlayerInfo;

typedef struct {
	BYTE numPlayers;
	BYTE playersAlive;
	BYTE mode;
	BYTE fire_id;
	BYTE timer;
} GameState;

typedef struct {
	BYTE head;
	UINT32 size;
} MsgHead;

typedef std::map<BYTE, PlayerInfo> PlayerMap;

//RPC stuff
void* rpcCall(const SOCKET socket, const BYTE header, const void* data, const UINT32 size, UINT32* retSize);
void updateGameState(GameState* gState, const SOCKET socket);
void sendPlayer(const PlayerInfo* player, const SOCKET socket);
void connectPlayer(PlayerInfo* player, const SOCKET socket);
void updatePlayers(PlayerMap& players, BYTE mId, const SOCKET socket);
void disconnectClient(const SOCKET socket, const BYTE mId);

void printPlayer(PlayerInfo *p);

void OpenConsole();
DWORD WINAPI handlePlayers(LPVOID lpParam);

#endif