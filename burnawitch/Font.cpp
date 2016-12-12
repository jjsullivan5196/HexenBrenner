#include "Font.h"
#include "cassert"
#include <vector>
#include <d3dcompiler.h>

#define RELEASE_COM(x) if(x){x->Release(); x=0;}

Font::Font()
{
	m_anchor = TOP_LEFT;
	m_scaling = XMFLOAT3(1, 1, 1);
	m_color = m_translation = XMFLOAT3(0, 0, 0);
	m_leading = 60.0f / 768; m_kerning = 60.0f / 1024*0.6f;
	windowWidth = 1024;
	windowHeight = 768;
}

Font::~Font()
{
	this->release();
}

bool Font::init(ID3D11Device * device, ID3D11DeviceContext * deviceContext, const FontMapDesc & fontMapDesc)
{
	HRESULT hr = S_OK;
	m_deviceContext = deviceContext;
	if (!createVertexBuffer(device))
	{
		return false;
	}
	bool a, b;
	if (fontMapDesc.shaderPath)
	{
		a = createVertexShader(device, fontMapDesc.shaderPath);
		b = createPixelShader(device, fontMapDesc.shaderPath);
	}
	else
	{
		a = createVertexShader(device, L"Font_FX.hlsl");
		b = createPixelShader(device, L"Font_FX.hlsl");
	}
	if (!a || !b)
	{
		return false;
	}
	createDepthStencilStates(device);
	if (!fontMapDesc.characters)
	{
		hr = D3DX11CreateShaderResourceViewFromFile(device, L"FontMap.png", NULL, NULL, &m_texture, NULL);
		if (FAILED(hr))
		{
			return false;
		}
		return createDefaultFontMap();
	}
	hr = D3DX11CreateShaderResourceViewFromFile(device, fontMapDesc.filePath, NULL, NULL, &m_texture, NULL);
	if (FAILED(hr))
	{
		return false;
	}
	for (UINT r = 0; r < fontMapDesc.rows; r++)
	{
		for (UINT c = 0; c < fontMapDesc.columns; c++)
		{
			UINT index = c + r*fontMapDesc.columns;
			char value = fontMapDesc.characters[index];
			fontMap[c] = XMFLOAT4(1.0f*c / fontMapDesc.columns, 1.0f*(c + 1) / fontMapDesc.columns,
					1.0f*r / fontMapDesc.rows, 1.0f*(r + 1) / fontMapDesc.rows);
			widthMap[c] = fontMapDesc.widths[index];
		}
	}
	return true;
}

void Font::release()
{
	RELEASE_COM(m_vertexBuffer);
	RELEASE_COM(m_vertexShader);
	RELEASE_COM(m_pixelShader);
	RELEASE_COM(m_inputLayout);
	RELEASE_COM(m_sampler);
	RELEASE_COM(m_dsOn);
	RELEASE_COM(m_dsOff);
}

XMFLOAT3 Font::getPosition()
{
	return m_translation;
}

XMFLOAT3 Font::getScaling()
{
	return m_scaling;
}

XMFLOAT3 Font::getColor()
{
	return m_color;
}

void Font::setPosition(const XMFLOAT3 & p)
{
	m_translation = p;
}

void Font::setScaling(const XMFLOAT3 & s)
{
	m_scaling = s;
}

void Font::setColor(const XMFLOAT3 & c)
{
	m_color = c;
}

void Font::setLeading(float l)
{
	m_leading = l;
}

void Font::setKerning(float k)
{
	m_kerning = k;
}

void Font::setDeviceContext(ID3D11DeviceContext * d)
{
	m_deviceContext = d;
}

void Font::setWindowSize(UINT x, UINT y)
{
	windowWidth = x;
	windowHeight = y;
}

void Font::setAnchorPoint(Anchor a)
{
	m_anchor = a;
}

void Font::printf(std::string s)
{
	UINT lines = 0;
	std::vector<float> offsetX;
	float offsetY=0;
	UINT length = s.size();
	XMMATRIX M = XMMatrixScaling(m_scaling.x, m_scaling.y, m_scaling.z)*
		XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);
	float fontLength=0;
	float fontHeight = 60.0f / windowHeight;
	float fontWidth = 60.0f / windowWidth * 0.6f;

	m_deviceContext->VSSetShader(m_vertexShader, 0, 0);
	m_deviceContext->IASetInputLayout(m_inputLayout);
	m_deviceContext->PSSetShader(m_pixelShader, 0, 0);
	m_deviceContext->PSSetShaderResources(0, 1, &m_texture);
	m_deviceContext->PSSetSamplers(0, 1, &m_sampler);
	m_deviceContext->OMSetDepthStencilState(m_dsOff, 1);

	if (m_anchor != TOP_LEFT) 
	{
		float offset = 0;
		for (int i = 0; i < length; i++)
		{
			offset += m_kerning*widthMap[s[i]];
			if (s[i] == '\n' || s[i] == '\r' || i == length - 1)
			{
				offsetX.push_back(offset);
				offset = 0;
			}
		}
	}
	for (int i = 0; i < length; i++)
	{
		XMFLOAT3 TL(-1, 1, 0), BR(1, -1, 0);
		XMVECTOR vTL, vBR;
		if (s[i] == '\n' || s[i] == '\r')
		{
			fontLength = 0;
			lines++;
			continue;
		}
		switch (m_anchor)
		{
		default:
		case TOP_LEFT:
			vTL = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontLength, -m_leading*lines, 0)), M);
			vBR = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontWidth + fontLength, -m_leading*lines - fontHeight, 0)), M);
			break;
		case TOP_RIGHT:
			vTL = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontLength - offsetX[lines], -m_leading*lines, 0)), M);
			vBR = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontWidth + fontLength - offsetX[lines], -m_leading*lines - fontHeight, 0)), M);
			break;
		case BOTTOM_LEFT:
			offsetY = m_leading*offsetX.size();
			vTL = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontLength, -m_leading*lines+ offsetY, 0)), M);
			vBR = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontWidth + fontLength, -m_leading*lines - fontHeight+ offsetY, 0)), M);
			break;
		case BOTTOM_RIGHT:
			offsetY = m_leading*offsetX.size();
			vTL = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontLength - offsetX[lines], -m_leading*lines + offsetY, 0)), M);
			vBR = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontWidth + fontLength - offsetX[lines], -m_leading*lines - fontHeight + offsetY, 0)), M);
			break;
		case CENTER:
			{
				offsetY = m_leading*offsetX.size() / 2;
				float halfOffsetx = offsetX[lines] / 2;
				vTL = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontLength - halfOffsetx, -m_leading*lines + offsetY, 0)), M);
				vBR = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(fontWidth + fontLength - halfOffsetx, -m_leading*lines - fontHeight + offsetY, 0)), M);
				break; 
			}
		}
		XMStoreFloat3(&TL, vTL);
		XMStoreFloat3(&BR, vBR);
		assert(updateBuffer(TL, BR, fontMap[s[i]]));
		UINT stride, offset;
		stride = sizeof(SimpleVertex);
		offset = 0;
		m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
		m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_deviceContext->Draw(6, 0);
		fontLength += m_kerning*widthMap[s[i]];
	}
	m_deviceContext->OMSetDepthStencilState(m_dsOn, 1);
}

void Font::operator<<(std::string s)
{
	this->printf(s);
}

bool Font::createVertexBuffer(ID3D11Device * device)
{
	SimpleVertex* vertices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	HRESULT result;
	UINT length = sizeof(SimpleVertex)* 6;
	vertices = new SimpleVertex[6];
	if (!vertices)
	{
		return false;
	}
	ZeroMemory(&vertices[0], length);
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = length;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}
	delete[] vertices;
	vertices = 0;
	return true;
}

bool Font::createVertexShader(ID3D11Device * device, TCHAR* shaderPath)
{
	ID3DBlob* pVSBlob = NULL;
	HRESULT hr = CompileShaderFromFile(shaderPath, "VS", "vs_5_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"Font shader cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}
	hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_vertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return false;
	}
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = device->CreateInputLayout(layout, 3, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
		&m_inputLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return FALSE;
	return true;
}

bool Font::createPixelShader(ID3D11Device * device, TCHAR* shaderPath)
{
	HRESULT hr = S_OK;
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(shaderPath, "PS", "ps_5_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return FALSE;
	}
	// Create the pixel shader
	hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return FALSE;
	return true;
}

bool Font::createSampler(ID3D11Device * device)
{
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = device->CreateSamplerState(&sampDesc, &m_sampler);
	if (FAILED(hr))
		return false;
	return true;
}

void Font::createDepthStencilStates(ID3D11Device * device)
{
	D3D11_DEPTH_STENCIL_DESC DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	device->CreateDepthStencilState(&DS_ON, &m_dsOn);
	device->CreateDepthStencilState(&DS_OFF, &m_dsOff);
}

bool Font::createDefaultFontMap()
{
	FontMapDesc fontMapDesc;
	fontMapDesc.rows = 10;
	fontMapDesc.columns = 10;
	TCHAR characters[100];
	fontMapDesc.characters = characters;

	for (int r = 0; r < 10; r++)
	{
		characters[10 * r] = ' ' + r;
		characters[1 + 10 * r] = '*' + r;
		characters[2 + 10 * r] = '4' + r;
	}
	characters[3] = characters[4] = characters[5] = characters[6] = 0;
	for (int r = 1; r < 10; r++)
	{
		characters[3 + 10 * r] = '>' + r - 1;
		characters[4 + 10 * r] = 'G' + r - 1;
		characters[5 + 10 * r] = 'P' + r - 1;
		characters[6 + 10 * r] = 'Y' + r - 1;
	}
	characters[96] = 0;
	for (int r = 0; r < 10; r++)
	{
		characters[7 + 10 * r] = 'a' + r;
		characters[8 + 10 * r] = 'k' + r;
		characters[9 + 10 * r] = 'u' + r;
	}

	for (int r = 0; r < fontMapDesc.rows; r++)
	{
		for (int c = 0; c < fontMapDesc.columns; c++)
		{
			fontMap[fontMapDesc.characters[c + r*fontMapDesc.columns]] =
				XMFLOAT4(1.0f*c / fontMapDesc.columns, 1.0f*(c + 1) / fontMapDesc.columns,
					1.0f*r / fontMapDesc.rows, 1.0f*(r + 1) / fontMapDesc.rows);
		}
	}
	char c = ' ';//1
	widthMap[c + 0] = 21.47f / 72.0f;
	widthMap[c + 1] = 21.47f / 72.0f;
	widthMap[c + 2] = 26.11f / 72.0f;
	widthMap[c + 3] = 38.17f / 72.0f;
	widthMap[c + 4] = 38.17f / 72.0f;
	widthMap[c + 5] = 58.15f / 72.0f;
	widthMap[c + 6] = 44.82f / 72.0f;
	widthMap[c + 7] = 16.26f / 72.0f;
	widthMap[c + 8] = 24.78f / 72.0f;
	widthMap[c + 9] = 24.78f / 72.0f;
	c += 10;//2
	widthMap[c + 0] = 100.15f / 72.0f - 1;
	widthMap[c + 1] = 111.84f / 72.0f - 1;
	widthMap[c + 2] = 93.47f / 72.0f - 1;
	widthMap[c + 3] = 96.78f / 72.0f - 1;
	widthMap[c + 4] = 93.47f / 72.0f - 1;
	widthMap[c + 5] = 93.47f / 72.0f - 1;
	widthMap[c + 6] = 110.17f / 72.0f - 1;
	widthMap[c + 7] = 110.17f / 72.0f - 1;
	widthMap[c + 8] = 110.17f / 72.0f - 1;
	widthMap[c + 9] = 110.17f / 72.0f - 1;
	c += 10;//3
	widthMap[c + 0] = 110.17f / 72.0f - 1;
	widthMap[c + 1] = 110.17f / 72.0f - 1;
	widthMap[c + 2] = 110.17f / 72.0f - 1;
	widthMap[c + 3] = 110.17f / 72.0f - 1;
	widthMap[c + 4] = 110.17f / 72.0f - 1;
	widthMap[c + 5] = 110.17f / 72.0f - 1;
	widthMap[c + 6] = 165.47f / 72.0f - 2;
	widthMap[c + 7] = 165.47f / 72.0f - 2;
	widthMap[c + 8] = 183.84f / 72.0f - 2;
	widthMap[c + 9] = 183.84f / 72.0f - 2;
	c += 10;//4
	widthMap[c + 0] = 255.84f / 72.0f - 3;
	widthMap[c + 1] = 254.17f / 72.0f - 3;
	widthMap[c + 2] = 281.71f / 72.0f - 3;
	widthMap[c + 3] = 260.82f / 72.0f - 3;
	widthMap[c + 4] = 260.82f / 72.0f - 3;
	widthMap[c + 5] = 264.13f / 72.0f - 3;
	widthMap[c + 6] = 264.13f / 72.0f - 3;
	widthMap[c + 7] = 260.82f / 72.0f - 3;
	widthMap[c + 8] = 257.45f / 72.0f - 3;
	c += 9;//5
	widthMap[c + 0] = 339.47f / 72.0f - 4;
	widthMap[c + 1] = 336.13f / 72.0f - 4;
	widthMap[c + 2] = 309.47f / 72.0f - 4;
	widthMap[c + 3] = 322.80f / 72.0f - 4;
	widthMap[c + 4] = 332.82f / 72.0f - 4;
	widthMap[c + 5] = 323.94f / 72.0f - 4;
	widthMap[c + 6] = 342.78f / 72.0f - 4;
	widthMap[c + 7] = 336.13f / 72.0f - 4;
	widthMap[c + 8] = 339.47f / 72.0f - 4;
	c += 9;//6
	widthMap[c + 0] = 403.74f / 72.0f - 5;
	widthMap[c + 1] = 411.47f / 72.0f - 5;
	widthMap[c + 2] = 408.13f / 72.0f - 5;
	widthMap[c + 3] = 404.82f / 72.0f - 5;
	widthMap[c + 4] = 400.37f / 72.0f - 5;
	widthMap[c + 5] = 408.13f / 72.0f - 5;
	widthMap[c + 6] = 404.82f / 72.0f - 5;
	widthMap[c + 7] = 421.43f / 72.0f - 5;
	widthMap[c + 8] = 404.82f / 72.0f - 5;
	c += 9;//7
	widthMap[c + 0] = 475.74f / 72.0f - 6;
	widthMap[c + 1] = 473.45f / 72.0f - 6;
	widthMap[c + 2] = 453.47f / 72.0f - 6;
	widthMap[c + 3] = 453.47f / 72.0f - 6;
	widthMap[c + 4] = 453.47f / 72.0f - 6;
	widthMap[c + 5] = 464.95f / 72.0f - 6;
	widthMap[c + 6] = 471.07f / 72.0f - 6;
	widthMap[c + 7] = 456.78f / 72.0f - 6;
	c += 8;//8
	widthMap[c + 0] = 542.17f / 72.0f - 7;
	widthMap[c + 1] = 542.17f / 72.0f - 7;
	widthMap[c + 2] = 538.80f / 72.0f - 7;
	widthMap[c + 3] = 542.17f / 72.0f - 7;
	widthMap[c + 4] = 542.17f / 72.0f - 7;
	widthMap[c + 5] = 525.47f / 72.0f - 7;
	widthMap[c + 6] = 542.17f / 72.0f - 7;
	widthMap[c + 7] = 542.17f / 72.0f - 7;
	widthMap[c + 8] = 522.13f / 72.0f - 7;
	widthMap[c + 9] = 524.88f / 72.0f - 7;
	c += 10;//9
	widthMap[c + 0] = 610.80f / 72.0f - 8;
	widthMap[c + 1] = 594.13f / 72.0f - 8;
	widthMap[c + 2] = 630.78f / 72.0f - 8;
	widthMap[c + 3] = 614.17f / 72.0f - 8;
	widthMap[c + 4] = 614.17f / 72.0f - 8;
	widthMap[c + 5] = 614.17f / 72.0f - 8;
	widthMap[c + 6] = 614.17f / 72.0f - 8;
	widthMap[c + 7] = 600.78f / 72.0f - 8;
	widthMap[c + 8] = 610.80f / 72.0f - 8;
	widthMap[c + 9] = 597.47f / 72.0f - 8;
	c += 10;//10
	widthMap[c + 0] = 686.17f / 72.0f - 9;
	widthMap[c + 1] = 682.80f / 72.0f - 9;
	widthMap[c + 2] = 696.13f / 72.0f - 9;
	widthMap[c + 3] = 682.80f / 72.0f - 9;
	widthMap[c + 4] = 682.80f / 72.0f - 9;
	widthMap[c + 5] = 682.80f / 72.0f - 9;
	widthMap[c + 6] = 675.85f / 72.0f - 9;
	widthMap[c + 7] = 668.40f / 72.0f - 9;
	widthMap[c + 8] = 672.84f / 72.0f - 9;
	widthMap[c + 9] = 687.84f / 72.0f - 9;
	return true;
}

bool Font::updateBuffer(const XMFLOAT3 & topLeft,const XMFLOAT3 & bottomRight, const XMFLOAT4 & uv)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	SimpleVertex *verticesPtr, *vertices = new SimpleVertex[6];
	XMFLOAT2 u, v;
	u.x = uv.x; u.y = uv.y;
	v.x = uv.z; v.y = uv.w;
	HRESULT result;
	// First triangle.
	vertices[0].Pos = XMFLOAT3(topLeft.x, topLeft.y, topLeft.z);  // Top left.
	vertices[0].Tex = XMFLOAT2(u.x, v.x);
	vertices[1].Pos = XMFLOAT3(bottomRight.x, bottomRight.y, bottomRight.z);  // Bottom right.
	vertices[1].Tex = XMFLOAT2(u.y, v.y);
	vertices[2].Pos = XMFLOAT3(topLeft.x, bottomRight.y, topLeft.z);  // Bottom left.
	vertices[2].Tex = XMFLOAT2(u.x, v.y);

	// Second triangle.
	vertices[3].Pos = XMFLOAT3(topLeft.x, topLeft.y, topLeft.z);    // Top left.
	vertices[3].Tex = XMFLOAT2(u.x, v.x);
	vertices[4].Pos = XMFLOAT3(bottomRight.x, topLeft.y, bottomRight.z);  // Top right.
	vertices[4].Tex = XMFLOAT2(u.y, v.x);
	vertices[5].Pos = XMFLOAT3(bottomRight.x, bottomRight.y, bottomRight.z);  // Bottom right.
	vertices[5].Tex = XMFLOAT2(u.y, v.y);

	for (int i = 0; i < 6; i++)
	{
		vertices[i].Norm = m_color;
	}
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	result = m_deviceContext->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}
	verticesPtr = (SimpleVertex*)mappedResource.pData;
	memcpy(verticesPtr, (void*)vertices, (sizeof(SimpleVertex)* 6));
	m_deviceContext->Unmap(m_vertexBuffer, 0);
	delete[] vertices;
	vertices = 0;
	return true;
}

HRESULT Font::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}
