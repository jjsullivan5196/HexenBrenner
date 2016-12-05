#ifndef SOCKETS_GAME_H
#define SOCKETS_GAME_H
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

// Serverside
SOCKET getListenSocket(); // Setup listener socket
SOCKET getClientSocket(SOCKET ListenSocket); // Get client connection
int cleanupSockets(SOCKET ClientSocket); // Cleanup client connection

// Clientside
SOCKET clientConnect(char* server); // Connect to server
int dropClient(SOCKET ConnectSocket); // Cleanup connection

int sendMsg(SOCKET ClientSocket, char* buffer, int buflen); //Send a message
int receiveMessage(SOCKET ClientSocket, char* buffer, int buflen); //Receive a message

typedef struct {
	char id;
	float x, y, z;
	char alive;
} PlayerInfo;

typedef struct {
	PlayerInfo client;
	SOCKET cSocket;
} clientInfo;

void printPlayer(PlayerInfo *p);

#endif