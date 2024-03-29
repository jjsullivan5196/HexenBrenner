#include "socketsCommon.h"
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "resource.h"
using namespace std;

XMFLOAT3 mul(XMFLOAT3 v, XMMATRIX &M);
struct SimpleVertex
	{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	};
struct SimpleVertexLVL
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct ConstantBuffer
	{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMFLOAT3 info;
	};


class billboard
{
public:
	billboard()
	{
		position = XMFLOAT3(0, 0, 0);
		scale = 1;
		transparency = 1;
	}
	XMFLOAT3 position; //obvious
	float scale;		//in case it can grow
	float transparency; //for later use
	XMMATRIX get_matrix(XMMATRIX &ViewMatrix)
	{

		XMMATRIX view, R, T, S;
		view = ViewMatrix;
		//eliminate camera translation:
		view._41 = view._42 = view._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, view);//inverse rotation
		T = XMMatrixTranslation(position.x, position.y, position.z);
		S = XMMatrixScaling(scale, scale, scale);
		return S*R*T;
	}

	XMMATRIX get_matrix_y(XMMATRIX &ViewMatrix) //enemy-type
	{

	}
};


//*****************************************
class bitmap
	{

	public:
		BYTE *image;
		int array_size;
		BITMAPFILEHEADER bmfh;
		BITMAPINFOHEADER bmih;
		bitmap()
			{
			image = NULL;
			}
		~bitmap()
			{
			if(image)
				delete[] image;
			array_size = 0;
			}
		bool read_image(char *filename)
			{
			ifstream bmpfile(filename, ios::in | ios::binary);
			if (!bmpfile.is_open()) return FALSE;	// Error opening file
			bmpfile.read((char*)&bmfh, sizeof(BITMAPFILEHEADER));
			bmpfile.read((char*)&bmih, sizeof(BITMAPINFOHEADER));
			bmpfile.seekg(bmfh.bfOffBits, ios::beg);
			//make the array
			if (image)delete[] image;
			int size = bmih.biWidth*bmih.biHeight * 3;
			image = new BYTE[size];//3 because red, green and blue, each one byte
			bmpfile.read((char*)image,size);
			array_size = size;
			bmpfile.close();
			check_save();
			return TRUE;
			}
		BYTE get_pixel(int x, int y,int color_offset) //color_offset = 0,1 or 2 for red, green and blue
			{
			int array_position = x*3 + y* bmih.biWidth*3+ color_offset;
			if (array_position >= array_size) return 0;
			if (array_position < 0) return 0;
			return image[array_position];
			}
		void check_save()
			{
			ofstream nbmpfile("newpic.bmp", ios::out | ios::binary);
			if (!nbmpfile.is_open()) return;
			nbmpfile.write((char*)&bmfh, sizeof(BITMAPFILEHEADER));
			nbmpfile.write((char*)&bmih, sizeof(BITMAPINFOHEADER));
			//offset:
			int rest = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
			if (rest > 0)
				{
				BYTE *r = new BYTE[rest];
				memset(r, 0, rest);
				nbmpfile.write((char*)&r, rest);
				}
			nbmpfile.write((char*)image, array_size);
			nbmpfile.close();

			}
	};
////////////////////////////////////////////////////////////////////////////////
//lets assume a wall is 10/10 big!
#define FULLWALL 2
#define HALFWALL 1
class wall
	{
	public:
		XMFLOAT3 position;
			int texture_no;
			int rotation; //0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
			wall()
				{
				texture_no = 0;
				rotation = 0;
				position = XMFLOAT3(0,0,0);
				}
			XMMATRIX get_matrix()
				{
				XMMATRIX R, T, T_offset;				
				R = XMMatrixIdentity();
				T_offset = XMMatrixTranslation(0, 0, -HALFWALL);
				T = XMMatrixTranslation(position.x, position.y, position.z);
				switch (rotation)//0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
					{
						default:
						case 0:	R = XMMatrixRotationY(XM_PI);		T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 1: R = XMMatrixRotationY(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 2:										T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 3: R = XMMatrixRotationY(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 4: R = XMMatrixRotationX(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
						case 5: R = XMMatrixRotationX(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
					}
				return T_offset * R * T;
				}
	};
//********************************************************************************************
#define MAXTEXTURE 30
#define TEXPARTS 3
XMFLOAT2 get_level_tex_coords(int pic, XMFLOAT2 coords);
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
class level
	{
	private:
		bitmap leveldata;
		vector<wall*> walls;						//all wall positions
		ID3D11Buffer*                       VertexBuffer = NULL;
		int texture_count;
		ID3D11VertexShader*                 vertexshader;
		ID3D11PixelShader*                  pixelshader;
		ID3D11InputLayout*                  VertexLayout = NULL;
		ID3D11ShaderResourceView* texture;
		void process_level()
			{
			//we have to get the level to the middle:
			int x_offset = (leveldata.bmih.biWidth/2)*FULLWALL;

			//lets go over each pixel without the borders!, only the inner ones
			for (int yy = 1; yy < (leveldata.bmih.biHeight - 1);yy++)
				for (int xx = 1; xx < (leveldata.bmih.biWidth - 1); xx++)
					{
					//wall information is the interface between pixels:
					//blue to something not blue: wall. texture number = 255 - blue
					//green only: floor. texture number = 255 - green
					//red only: ceiling. texture number = 255 - red
					//green and red: floor and ceiling ............
					BYTE red, green, blue;

					blue = leveldata.get_pixel(xx, yy, 0);
					green = leveldata.get_pixel(xx, yy, 1);
					red = leveldata.get_pixel(xx, yy, 2);
					
					if (blue > 0)//wall possible
						{
						int texno = 255 - blue;
						BYTE left_red = leveldata.get_pixel(xx - 1, yy, 2);
						BYTE left_green = leveldata.get_pixel(xx - 1, yy, 1);
						BYTE right_red = leveldata.get_pixel(xx + 1, yy, 2);
						BYTE right_green = leveldata.get_pixel(xx + 1, yy, 1);
						BYTE top_red = leveldata.get_pixel(xx, yy+1, 2);
						BYTE top_green = leveldata.get_pixel(xx, yy+1, 1);
						BYTE bottom_red = leveldata.get_pixel(xx, yy-1, 2);
						BYTE bottom_green = leveldata.get_pixel(xx, yy-1, 1);

						if (left_red>0 || left_green > 0)//to the left
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 3, texno);
						if (right_red>0 || right_green > 0)//to the right
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 1, texno);
						if (top_red>0 || top_green > 0)//to the top
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 2, texno);
						if (bottom_red>0 || bottom_green > 0)//to the bottom
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 0, texno);
						}
					if (red > 0)//ceiling
						{
						int texno = 255 - red;
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0,yy*FULLWALL), 5, texno);
						}
					if (green > 0)//floor
						{
						int texno = 255 - green;
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0,yy*FULLWALL), 4, texno);
						}
					}
			}
		void init_wall(XMFLOAT3 pos, int rotation, int texture_no)
			{
			wall *w = new wall;
			walls.push_back(w);
			w->position = pos;
			w->rotation = rotation;
			w->texture_no = texture_no;
			}
		SimpleVertexLVL *pvertices;
	public:
		level()
			{
				
				pvertices = NULL;
				texture_count = 0;
				
					texture = NULL;
			}
		void init(char *level_bitmap)
			{
			if(!leveldata.read_image(level_bitmap))return;
			process_level();
			}
		bool init_texture(ID3D11Device* pd3dDevice,LPCWSTR filename)
			{
			// Load the Texture
			ID3D11ShaderResourceView *tex;
			HRESULT hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, filename, NULL, NULL, &tex, NULL);
			if (FAILED(hr))
				return FALSE;
			texture = tex;
			return TRUE;
			}
		
		XMMATRIX get_wall_matrix(int no)
			{
			if (no < 0 || no >= walls.size()) return XMMatrixIdentity();
			return walls[no]->get_matrix();
			}
		int get_wall_count()
			{
			return walls.size();
			}
		void render_level(ID3D11DeviceContext* ImmediateContext,XMMATRIX *view, XMMATRIX *projection, ID3D11Buffer* dx_cbuffer)
			{
			//set up everything for the waqlls/floors/ceilings:
			UINT stride = sizeof(SimpleVertexLVL);
			UINT offset = 0;			
			ID3D11InputLayout*           old_VertexLayout = NULL;
			ImmediateContext->IAGetInputLayout(&old_VertexLayout);
			ImmediateContext->IASetInputLayout(VertexLayout);
			ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
			ConstantBuffer constantbuffer;			
			constantbuffer.View = XMMatrixTranspose(*view);
			constantbuffer.Projection = XMMatrixTranspose(*projection);			
			XMMATRIX wall_matrix,S;
		
			//S = XMMatrixScaling(FULLWALL, FULLWALL, FULLWALL);
			
				constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
				ImmediateContext->VSSetShader(vertexshader, NULL, 0);
				ImmediateContext->PSSetShader(pixelshader, NULL, 0);
				ImmediateContext->UpdateSubresource(dx_cbuffer, 0, NULL, &constantbuffer, 0, 0);
				ImmediateContext->VSSetConstantBuffers(0, 1, &dx_cbuffer);
				ImmediateContext->PSSetConstantBuffers(0, 1, &dx_cbuffer);
				ImmediateContext->PSSetShaderResources(2, 1, &texture);
				ImmediateContext->Draw(walls.size()*12,0);

				ImmediateContext->IASetInputLayout(old_VertexLayout);
				
			}
		void make_big_level_object(ID3D11Device* g_pd3dDevice, XMMATRIX *view, XMMATRIX *projection)
		{
			if (pvertices)return;
			// Compile the pixel shader
			ID3DBlob* pPSBlob = NULL;
			HRESULT hr = CompileShaderFromFile(L"shader.fx", "PSlevel", "ps_4_0", &pPSBlob);
			if (FAILED(hr))
			{
				MessageBox(NULL,
					L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
				return;
			}

			// Create the pixel shader
			hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelshader);
			pPSBlob->Release();
			if (FAILED(hr))
				return;
			ID3DBlob* pVSBlob = NULL;
			hr = CompileShaderFromFile(L"shader.fx", "VSlevel", "vs_4_0", &pVSBlob);
			if (FAILED(hr))
			{
				MessageBox(NULL,
					L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
				return;
			}
			// Create the vertex shader
			hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexshader);
			if (FAILED(hr))
			{
				pVSBlob->Release();
				return;
			}
			// Define the input layout
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			UINT numElements = ARRAYSIZE(layout);

			// Create the input layout
			hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
				pVSBlob->GetBufferSize(), &VertexLayout);

			SimpleVertexLVL vertices[] = //12 vertices
			{
				{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },

				{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }

			};
			pvertices = new SimpleVertexLVL[12 * walls.size()];
			SimpleVertexLVL ver[100];
			XMMATRIX S = XMMatrixScaling(2, 2, 2);

			int oo = 0;
			for (int ii = 0; ii < walls.size(); ii++)
				{
				XMMATRIX wall_matrix = walls[ii]->get_matrix();
				for (int uu = 0; uu < 12; uu++)
					{
					pvertices[ii*12 + uu].Pos = mul(vertices[uu].Pos,S* wall_matrix);
					pvertices[ii * 12 + uu].Tex = get_level_tex_coords(walls[ii]->texture_no,vertices[uu].Tex);
					}
				}
			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(SimpleVertexLVL) * walls.size()*12;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			D3D11_SUBRESOURCE_DATA InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = pvertices;
			hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &VertexBuffer);
			if (FAILED(hr))
				return;
			int z;
			z = 0;
		}
	};



	class camera
	{
	private:

	public:
		int w, s, a, d;
		float speedRatio = 100000.0;
		XMFLOAT3 position;
		XMFLOAT3 rotation;
		camera()
		{
			w = s = a = d = 0;
			rotation = XMFLOAT3(0, 0, 0);
			position = XMFLOAT3(0, 0.5, -20);
		}
		void animation(float elapsed_microseconds, PlayerInfo* p)
		{
			XMMATRIX R, T;
			R = XMMatrixRotationY(-rotation.y);

			XMFLOAT3 oldpos = position;
			XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
			XMVECTOR f = XMLoadFloat3(&forward);
			f = XMVector3TransformCoord(f, R);
			XMStoreFloat3(&forward, f);
			XMFLOAT3 side = XMFLOAT3(1, 0, 0);
			XMVECTOR si = XMLoadFloat3(&side);
			si = XMVector3TransformCoord(si, R);
			XMStoreFloat3(&side, si);

			float speed = elapsed_microseconds / speedRatio;

			if (w)
			{
				position.x -= forward.x * speed;
				position.y -= forward.y * speed;
				position.z -= forward.z * speed;
			}
			if (s)
			{
				position.x += forward.x * speed;
				position.y += forward.y * speed;
				position.z += forward.z * speed;
			}
			if (d)
			{
				position.x -= side.x * speed;
				position.y -= side.y * speed;
				position.z -= side.z * speed;
			} 
			if (a)
			{
				position.x += side.x * speed;
				position.y += side.y * speed;
				position.z += side.z * speed;
			}

			if (position.x < -49 || position.x > 64.5)
			{
				position.x = oldpos.x;
			}
			if (position.z > -19 || position.z < -133)
			{
				position.z = oldpos.z;
			}

			p->x = -position.x;
			p->y = -position.y;
			p->z = -position.z;
		}
		XMMATRIX get_matrix(XMMATRIX *view)
		{
			XMMATRIX R, T, W;
			R = XMMatrixRotationY(rotation.y);
			T = XMMatrixTranslation(position.x, position.y, position.z);
			W = T*(*view)*R;
			return W;
		}
	};

	class StopWatchMicro_
	{
	private:
		LARGE_INTEGER last, frequency;
	public:
		StopWatchMicro_()
		{
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&last);

		}
		long double elapse_micro()
		{
			LARGE_INTEGER now, dif;
			QueryPerformanceCounter(&now);
			dif.QuadPart = now.QuadPart - last.QuadPart;
			long double fdiff = (long double)dif.QuadPart;
			fdiff /= (long double)frequency.QuadPart;
			return fdiff*1000000.;
		}
		long double elapse_milli()
		{
			elapse_micro() / 1000.;
		}
		void start()
		{
			QueryPerformanceCounter(&last);
		}
	};