#pragma once
#include <map>
#include <string>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx11tex.h>
#include <xnamath.h>

/*========================================================================================================*/
//	How to use this class based on lecture projects:
//
//		1. Copy & paste "Font.h" "Font.cpp" "FontMap.png" "Font_FX.hlsl" into your project folder.
///			Note: 
///			You need to set properties for "Font_FX.hlsl" in Visual Studio, if it's in your project/solution.
///			E.X.
///				Right click "Font_FX.hlsl" in solution -> Properties -> HLSL compiler -> Entrypoint Name: VS
//		2. Include the header file in the main cpp file. 
///			#include "Font.h" 
//		3. Create an object of the font class.
///			Font font;
//		4. Initialize your Font class object after initializing dx11. 
///			font.init(g_pd3dDevice, g_pImmediateContext, font.defaultFontMapDesc);		
//		5. Draw texts in the render function.
///			font << "CST320 SPR2016";
/*========================================================================================================*/

class Font
{
private:
	struct SimpleVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Tex;
		XMFLOAT3 Norm;
	};
public:
	struct FontMapDesc
	{
		FontMapDesc() :rows(0), columns(0),
		characters(0), widths(0), filePath(0), shaderPath(0){}
		UINT rows, columns;
		TCHAR *characters;
		float *widths;
		TCHAR *filePath;
		TCHAR *shaderPath;
	}const defaultFontMapDesc;
	enum Anchor
	{
		TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER
	};
public:
	Font();
	~Font();

	///init(ID3D11Device *@input1, ID3D11DeviceContext *@input2,const FontMapDesc &@input3) -> bool @output1
	//	Initialize the font object. 
	//		@input1:
	//				A pointer to the ID3D11Device.
	//		@input2:
	//				A pointer to the ID3D11DeviceContext.
	//		@input3:
	//				The Fontmap description. Users can define their own fontmaps.
	//				Or use the defaultFontMapDesc. 
	//	E.X.
	//		Font font;
	//		font.init(g_pd3dDevice, g_pImmediateContext, font.defaultFontMapDesc);
	bool init(ID3D11Device* device, ID3D11DeviceContext *deviceContext, const FontMapDesc &fontMapDesc);

	///release()
	//	Release the memory. This method will automatically called when the object is destroyed.
	void release();

	///getPosition() -> XMFLOAT3 @output1
	//		@output1:
	//			The position of the anchor point.
	XMFLOAT3 getPosition();

	///getScaling() -> XMFLOAT3 @output1
	//		@output1:
	//			The scaling of the current font object.
	XMFLOAT3 getScaling();

	///getColor() -> XMFLOAT3 @output1
	//		@output1:
	//			The color of the current font object.
	XMFLOAT3 getColor();

	///setPosition(const XMFLOAT3 & @input1)
	//	Set position. The default vaule is (0,0,0)
	//		@input1:
	//			 The position of the anchor point.
	//		E.X.
	//			font.setPosition(XMFLOAT3(-0.5, 0.5, 0));
	void setPosition(const XMFLOAT3 &p);

	///setScaling(const XMFLOAT3 & @input1)
	//	Set scaling. The default vaule is (1,1,1)
	//		@input1:
	//			The scaling of the current font object.
	void setScaling(const XMFLOAT3 &s);

	///setColor(const XMFLOAT3 & @input1)
	//	Set color value in RGB. The default vaule is (0,0,0)
	//		@input1:
	//			The color of the current font object.
	//		E.X.
	//			font.setColor(XMFLOAT3(1, 0, 0));
	void setColor(const XMFLOAT3 &c);

	///setLeading(float @input1)
	//	Set leading. The default vaule is 60/768. (768 is the default window height)
	//		@input1:
	//			The leading of the current font. (Leading: The distance between two lines.)
	void setLeading(float l);

	///setKerning(float @input1)
	//	Set kerning. The default vaule is 0.6*(60/1024). (1024 is the default window width)
	//		@input1:
	//			The kerning of the current font. 
	//			(Kerning: The distance between two characters in the same line.)
	void setKerning(float k);

	///setDeviceContext(ID3D11DeviceContext *@input1)
	//		@input1:
	//			A pointer to the ID3D11DeviceContext.
	void setDeviceContext(ID3D11DeviceContext *d);

	///setWindowSize(UINT @input1,UINT @input2)
	//	Set the current window size. The default value is 1024x768.
	//		@input1:
	//			The width of the current window.
	//		@input2:
	//			The height of the current window.
	void setWindowSize(UINT x,UINT y);

	///setAnchorPoint(Anchor @input1)
	//	Set the anchor point by specify the anchor model. 
	//		@input1:
	//			Anchor model: TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER.
	//		E.X.
	//			font.setAnchorPoint(Font::Anchor::TOP_RIGHT);
	void setAnchorPoint(Anchor a);

	///printf(std::string @input1)
	//		@input1:
	//			The characters need to be drawed.
	void printf(std::string s);

	///operator<<(std::string @input1)
	//	Overide the << operator.
	//		@input1:
	//			The characters need to be drawed.
	void operator<<(std::string s); 
private:
	bool createVertexBuffer(ID3D11Device* device);
	bool createVertexShader(ID3D11Device * device,TCHAR* shaderPath);
	bool createPixelShader(ID3D11Device * device, TCHAR* shaderPath);
	bool createSampler(ID3D11Device * device);
	void createDepthStencilStates(ID3D11Device * device);
	bool createDefaultFontMap();
	bool updateBuffer(const XMFLOAT3 & topLeft, const XMFLOAT3 & bottomRight, const XMFLOAT4 & uv);
private:
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
private:
	ID3D11DeviceContext *m_deviceContext;
	ID3D11Buffer *m_vertexBuffer;
	XMFLOAT3 m_scaling, m_translation,m_color;
	Anchor m_anchor;
	std::map < TCHAR, XMFLOAT4> fontMap;
	std::map < TCHAR, float> widthMap;
	float m_leading, m_kerning;
	UINT windowWidth, windowHeight;
	ID3D11ShaderResourceView* m_texture;
	ID3D11VertexShader *m_vertexShader;
	ID3D11PixelShader  *m_pixelShader;
	ID3D11InputLayout* m_inputLayout;
	ID3D11SamplerState *m_sampler;
	ID3D11DepthStencilState	*m_dsOn, *m_dsOff;
};

