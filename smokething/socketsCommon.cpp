#include "socketsCommon.h"

void OpenConsole()
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

void* rpcCall(const SOCKET socket, const BYTE header, const void* data, const UINT32 size, UINT32* retSize)
{
	MsgHead newMsg;
	MsgHead retMsg;
	BYTE ack;
	newMsg.head = header;
	newMsg.size = size;

	void* returnData;

	//Send request
	send(socket, (char*)&newMsg, sizeof(MsgHead), DEFAULT_FLAGS);

	if (header == HEAD_TERMINATE)
	{
		//we're done!
		return NULL;
	}

	//Receive ack
	recv(socket, (char*)&ack, sizeof(BYTE), DEFAULT_FLAGS);

	//Send data
	send(socket, (char*)data, size, DEFAULT_FLAGS);

	//Receive return info
	recv(socket, (char*)&retMsg, sizeof(MsgHead), DEFAULT_FLAGS);

	//Allocate space for return
	returnData = (void*)malloc(retMsg.size);
	if(retSize != NULL)
		*retSize = retMsg.size;

	//Send ack
	ack = 1;
	send(socket, (char*)&ack, sizeof(BYTE), DEFAULT_FLAGS);

	//Receive return data
	recv(socket, (char*)returnData, retMsg.size, DEFAULT_FLAGS);

	//Return RPC result
	return returnData;
}

void updateGameState(GameState* gState, const SOCKET socket)
{
	GameState* t_gameState = (GameState*)rpcCall(socket, HEAD_GETSTATE, NODATA, NBYTES, NULL);
	memcpy(gState, t_gameState, sizeof(GameState));
	free(t_gameState);
}

void sendPlayer(const PlayerInfo* player, const SOCKET socket)
{
	void* ack = rpcCall(socket, HEAD_PUSHPLAYER, player, sizeof(PlayerInfo), NULL);
	free(ack);
}

void connectPlayer(PlayerInfo* player, const SOCKET socket)
{
	PlayerInfo* t_player = (PlayerInfo*)rpcCall(socket, HEAD_CONNECT, NODATA, NBYTES, NULL);
	memcpy(player, t_player, sizeof(PlayerInfo));
	free(t_player);
}

void updatePlayers(PlayerMap& players, BYTE mId, const SOCKET socket) {
	UINT32 retSize = 0;
	PlayerInfo* update = (PlayerInfo*)rpcCall(socket, HEAD_GETPLAYERS, NODATA, NBYTES, &retSize);
	for (int i = 0; i < (retSize / sizeof(PlayerInfo)); i++)
	{
		PlayerInfo* n = &update[i];
		if (n->id != mId)
		{
			PlayerInfo* set = &players[n->id];
			memcpy(set, n, sizeof(PlayerInfo));
		}
	}
	free(update);
}

void disconnectClient(const SOCKET socket, const BYTE mId)
{
	rpcCall(socket, HEAD_TERMINATE, NODATA, mId, NULL);
}