#include "socketsCommon.h"

void printPlayer(PlayerInfo* p)
{
	fprintf(stdout, "Player: %d\nLocation: (%.2f, %.2f, %.2f)\n", p->id, p->x, p->y, p->z);
}