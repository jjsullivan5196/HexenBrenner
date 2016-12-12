#include "groundwork.h"

XMFLOAT3 mul(XMFLOAT3 v, XMMATRIX &M)
{
	XMVECTOR f = XMLoadFloat3(&v);
	f = XMVector3TransformCoord(f, M);
	XMStoreFloat3(&v, f);
	v.x += M._41;
	v.y += M._42;
	v.z += M._43;
	return v;
}

XMFLOAT2 get_level_tex_coords(int pic, XMFLOAT2 coords)
{
	float one = 1. / (float)TEXPARTS;
	int x = pic % TEXPARTS;
	int y = pic / (float)TEXPARTS;
	coords.x /= (float)TEXPARTS;
	coords.y /= (float)TEXPARTS;
	coords.x += one*x;
	coords.y += one*y;
	return coords;
}

