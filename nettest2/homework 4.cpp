//--------------------------------------------------------------------------------------
// File: lecture 8.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "groundwork.h"


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

typedef std::map<char, PlayerInfo>::iterator pIterator;

void OpenConsole()
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pVertexBuffer_sky = NULL;
//states for turning off and on the depth buffer
ID3D11DepthStencilState			*ds_on, *ds_off;
ID3D11BlendState*					g_BlendState;

ID3D11Buffer*                       g_pCBuffer = NULL;

ID3D11ShaderResourceView*           g_pTextureRV = NULL;


ID3D11SamplerState*                 g_pSamplerLinear = NULL;
XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor( 0.7f, 0.7f, 0.7f, 1.0f );

camera								cam;
level								level1;
vector<billboard*>					smokeray;
map<char, billboard*>					players;
billboard							fire;
XMFLOAT3							rocket_position;
#define ROCKETRADIUS				10

PlayerInfo g_Player = { 0, 0, 0.0, 0.0, 0.0 };
GameState g_gameState = { 0 };
std::map<char, PlayerInfo> g_Players;

SOCKET g_listenSocket = INVALID_SOCKET;
SOCKET g_clientSocket = INVALID_SOCKET;
char g_netBuffer[sizeof(PlayerInfo)];
char g_Server[20] = "localhost";
//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();

//--------------------------------------------------------------------------------------
// Player handler
//--------------------------------------------------------------------------------------
DWORD WINAPI handlePlayers(LPVOID lpParam)
{
	PlayerInfo tPlayer;

	// Just Connected
	receiveMessage(g_clientSocket, g_netBuffer, sizeof(PlayerInfo));
	memcpy(&g_Player, g_netBuffer, sizeof(PlayerInfo));

	while (true)
	{
		//Send my location
		memcpy(g_netBuffer, &g_Player, sizeof(PlayerInfo));
		sendMsg(g_clientSocket, g_netBuffer, sizeof(PlayerInfo));
		//fprintf(stdout, "Position Sent\n");

		unsigned char numPlayers = 0;
		unsigned char truth = 1;
		
		receiveMessage(g_clientSocket, (char*)&numPlayers, sizeof(char));

		if (numPlayers > 0)
		{
			PlayerInfo* newPlayers;
			char* newPlayerBuf = (char*)malloc(sizeof(PlayerInfo) * numPlayers);
			sendMsg(g_clientSocket, (char*)&truth, sizeof(char));
			receiveMessage(g_clientSocket, newPlayerBuf, sizeof(PlayerInfo) * numPlayers);
			newPlayers = (PlayerInfo*)newPlayerBuf;

			for (int i = 0; i < numPlayers; i++)
			{
				PlayerInfo* c = &newPlayers[i];
				g_Players[c->id] = *c;
				if(c->id != g_Player.id)
					fprintf(stdout, "Player %d: %.2f %.2f %.2f\n", c->id, c->x, c->y, c->z);
			}

			free(newPlayerBuf);
		}
		else
		{
			fprintf(stderr, "No clients\n");
		}

		//sendMsg(g_clientSocket, (char*)&g_gameState.whoLit, sizeof(char));
		//receiveMessage(g_clientSocket, (char*)&g_gameState.whoLit, sizeof(char));
	}
}
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }
	srand(time(0));
    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 7", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if( FAILED( hr ) )
		return hr;

	
    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

	  

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    // Compile the vertex shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile( L"shader.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
    if( FAILED( hr ) )
    {    
        pVSBlob->Release();
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    // Compile the pixel shader
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"shader.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;
	//create skybox vertex buffer
	SimpleVertex vertices_skybox[] =
		{
		//top
		/*{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)  },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)  },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)  },*/
				
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(2.0f / 4, 1.0f/3) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f / 4, 1.0f/3) },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(2.0f / 4, 0.0f / 3) },				
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(2.0f / 4, 0.0f / 3) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f / 4, 1.0f/3) },
				{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f / 4, 0.0f / 3) },

		//bottom
		/*{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)  },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},*/
				
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f / 4, 2.0f / 3) },
				{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(2.0f / 4, 2.0f / 3) },
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(2.0f / 4, 3.0f / 3) },				
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f / 4, 2.0f / 3) },
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(2.0f / 4, 3.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f / 4, 3.0f / 3) },

		//left
		/*{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)  },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)  },*/
				
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(2.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(2.0f / 4, -2.0f / 3) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(2.0f / 4, -2.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f / 4, -2.0f / 3) },

		//right
		/*{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)  },*/
				
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(4.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(3.0f/4, -1.0f / 3) },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(3.0f/4, -2.0f / 3) },				
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(4.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(3.0f/4, -2.0f / 3) },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(4.0f / 4, -2.0f / 3) },

		//back
		/*{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)  },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)  },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },*/
				
				{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f / 4, -2.0f / 3) },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f / 4, -2.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f / 4, -2.0f / 3) },

		//front
		/*{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)  },*/
				
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(2.0f / 4, -2.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(3.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(2.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(3.0f / 4, -1.0f / 3) },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(2.0f / 4, -2.0f / 3) },
				{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(3.0f / 4, -2.0f / 3) },
		};
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices_skybox;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_sky);
	if (FAILED(hr))
		return hr;
    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        { XMFLOAT3( -1.0f, -1.0f, 0.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f ), XMFLOAT2( 0.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 0.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( -1.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 0.0f ) },

		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },		
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },		
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }

    };


    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 12;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

 
    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBuffer);
    if( FAILED( hr ) )
        return hr;
    

    // Load the Texture
    hr = D3DX11CreateShaderResourceViewFromFile( g_pd3dDevice, L"playa1.gif", NULL, NULL, &g_pTextureRV, NULL );
    if( FAILED( hr ) )
        return hr;

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if( FAILED( hr ) )
        return hr;

    // Initialize the world matrices
    g_World = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -6.0f, 0.0f );//camera position
    XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );//look at
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );// normal vector on at vector (always up)
    g_View = XMMatrixLookAtLH( Eye, At, Up );

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);

	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose( g_View );
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
	constantbuffer.info = XMFLOAT4(1, 1, 1, 1);
    g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0 );

	//blendstate:
	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);


	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);
	

	//create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);

	level1.init("level.bmp");
	level1.init_texture(g_pd3dDevice, L"wall1.jpg");
	level1.init_texture(g_pd3dDevice, L"wall2.jpg");
	level1.init_texture(g_pd3dDevice, L"floor.jpg");
	level1.init_texture(g_pd3dDevice, L"ceiling.jpg");
	
	rocket_position = XMFLOAT3(0, 0, ROCKETRADIUS);

	OpenConsole();

	FILE* hostFile = NULL;
	hostFile = fopen("host.txt", "r");
	if (hostFile != NULL)
	{
		fscanf(hostFile, "%s", g_Server);
	}

	g_clientSocket = clientConnect(g_Server);
	CreateThread(NULL, 0, handlePlayers, NULL, NULL, NULL);


    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pSamplerLinear ) g_pSamplerLinear->Release();
    if( g_pTextureRV ) g_pTextureRV->Release();
    if(g_pCBuffer) g_pCBuffer->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
	{

	}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
	{


	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
	{


	}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////
void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
	{
	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
		{
		}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
		{
		}
	
	}


BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
	{

	return TRUE;
	}
void OnTimer(HWND hwnd, UINT id)
	{

	}
//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{
	switch (vk)
		{
			case 65:cam.a = 0;//a
				break;
			case 68: cam.d = 0;//d
				break;
			case 32: //space
				break;
			case 87: cam.w = 0; //w
				break;
			case 83:cam.s = 0; //s
			default:break;

		}

	}

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{

	switch (vk)
		{
			default:break;
			case 65:cam.a = 1;//a
				break;
			case 68: cam.d = 1;//d
				break;
			case 32: //space
			break;
			case 87: cam.w = 1; //w
				break;
			case 83:cam.s = 1; //s
				break;
			case 27: PostQuitMessage(0);//escape
				break;
		}
	}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
#include <windowsx.h>
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
	HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLBD);
	HANDLE_MSG(hWnd, WM_LBUTTONUP, OnLBU);
	HANDLE_MSG(hWnd, WM_MOUSEMOVE, OnMM);
	HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
	HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
	HANDLE_MSG(hWnd, WM_KEYDOWN, OnKeyDown);
	HANDLE_MSG(hWnd, WM_KEYUP, OnKeyUp);
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}



//--------------------------------------------------------------------------------------
// sprites
//--------------------------------------------------------------------------------------
class sprites
	{
	public:
		XMFLOAT3 position;
		XMFLOAT3 impulse;
		float rotation_x;
		float rotation_y;
		float rotation_z;
		sprites()
			{
			impulse = position = XMFLOAT3(0, 0, 0);
			rotation_x = rotation_y = rotation_z;
			}
		XMMATRIX animation() 
			{
			//update position:
			position.x = position.x + impulse.x; //newtons law
			position.y = position.y + impulse.y; //newtons law
			position.z = position.z + impulse.z; //newtons law

			XMMATRIX M;
			//make matrix M:
			XMMATRIX R,Rx,Ry,Rz,T;
			T = XMMatrixTranslation(position.x, position.y, position.z);
			Rx = XMMatrixRotationX(rotation_x);
			Ry = XMMatrixRotationX(rotation_y);
			Rz = XMMatrixRotationX(rotation_z);
			R = Rx*Ry*Rz;
			M = R*T;
			return M;
			}
	};
sprites mario;

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------

void animate_rocket(float elapsed_microseconds)
	{
	
	//going to a new position
	static float rocket_angle = 0.0;
	rocket_angle +=  elapsed_microseconds / 1000000.0;

	rocket_position.x = sin(rocket_angle) * ROCKETRADIUS;
	rocket_position.z = cos(rocket_angle) * ROCKETRADIUS;
	rocket_position.y = 0;
	

	//update all billboards (growing and transparency)
	for (int ii = 0; ii < smokeray.size(); ii++)
		{
		smokeray[ii]->transparency -= 0.0002;
		smokeray[ii]->scale+= 0.0003;
		if (smokeray[ii]->transparency < 0) // means its dead
			{
			smokeray.erase(smokeray.begin() + ii);
			ii--;
			}
		}

	//apply a new billboard
	static float time = 0;
	time += elapsed_microseconds;
	if (time < 120000)//every 10 milliseconds
		return;
	time = 0;
	billboard *new_bill = new billboard;
	new_bill->position = rocket_position;
	new_bill->scale = 1. + (float)(rand() % 100) / 300.;
	smokeray.push_back(new_bill);
	
	}

void animate_fire(XMFLOAT3 pos)
{
	fire.position = pos;
	fire.scale = 1;
	fire.transparency = 0.5;
}

float vecDist(XMFLOAT3 p, XMFLOAT3 q)
{
	return sqrtf(pow(p.x - q.x, 2) + pow(p.y - q.y, 2) + pow(p.z - q.z, 2));
}

void animate_players()
{
	for (pIterator i = g_Players.begin(); i != g_Players.end(); i++)
	{
		PlayerInfo* p = &i->second;
		if (p->id == g_Player.id)
			continue;
		if (players.count(p->id))
		{
			players[p->id]->position = XMFLOAT3(p->x, p->y, p->z);
		}
		else
		{
			players[p->id] = new billboard();
			players[p->id]->position = XMFLOAT3(p->x, p->y, p->z);
			players[p->id]->scale = 1;
		}
	}
}

void render_billboard(billboard* bill, XMMATRIX& view, XMMATRIX& worldmatrix, ConstantBuffer& constantbuffer, ID3D11DeviceContext* &pImmediateContext)
{
	worldmatrix = bill->get_matrix(view);

	constantbuffer.World = XMMatrixTranspose(worldmatrix);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.info = XMFLOAT4(bill->transparency, 1, 1, 1);
	pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	pImmediateContext->Draw(12, 0);
}
//*******************************************************
void Render()
{
static StopWatchMicro_ stopwatch;
long elapsed = stopwatch.elapse_micro();
stopwatch.start();//restart



UINT stride = sizeof(SimpleVertex);
UINT offset = 0;
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

  
    // Clear the back buffer
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    // Clear the depth buffer to 1.0 (max depth)
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	cam.animation(elapsed, g_Player);
	XMMATRIX view = cam.get_matrix(&g_View);

	XMMATRIX Iview = view;
	Iview._41 = Iview._42 = Iview._43 = 0.0;
	XMVECTOR det;
	Iview = XMMatrixInverse(&det, Iview);

	//ENTER MATRIX CALCULATION HERE
	XMMATRIX worldmatrix;
	worldmatrix = XMMatrixIdentity();
	//XMMATRIX S, T, R, R2;
	//worldmatrix = .... probably define some other matrices here!
	
	
	animate_players();






    // Update skybox constant buffer
    ConstantBuffer constantbuffer;
	constantbuffer.World	=	XMMatrixTranspose(worldmatrix);
	constantbuffer.View		=	XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
    g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0 );
    // Render skybox
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);	
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBuffer);   
    g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pTextureRV );
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );

	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
    //g_pImmediateContext->Draw( 12, 0 );

	//draw all billboards:

	/*for (int ii = 0; ii < smokeray.size(); ii++)
		{
		// Update skybox constant buffer
		ConstantBuffer constantbuffer;
		worldmatrix = smokeray[ii]->get_matrix(view);

		constantbuffer.World = XMMatrixTranspose(worldmatrix);
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->Draw(12, 0);


		}*/
	for (auto i = players.begin(); i != players.end(); i++)
	{
		ConstantBuffer constantbuffer;
		render_billboard(i->second, view, worldmatrix, constantbuffer, g_pImmediateContext);
	}


	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);



    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present( 0, 0 );
}


