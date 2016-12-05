#include "socketsCommon.h"

int receiveMessage(SOCKET ClientSocket, char* buffer, int buflen)
{
	return recv(ClientSocket, buffer, buflen, 0);
}

int sendMsg(SOCKET ClientSocket, char* buffer, int buflen)
{
	int iSendResult = send(ClientSocket, buffer, buflen, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
}