struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

PixelInputType VS(VertexInputType input)
{
	PixelInputType output;

	output.position = input.position;
	output.tex = input.tex;
	output.normal = input.normal;

	return output;
}

Texture2D shaderTexture;
SamplerState SampleType;

float4 PS(PixelInputType input) : SV_TARGET
{
	float4 textureColor;
	float4 color;
	textureColor = shaderTexture.Sample(SampleType, input.tex);
	textureColor.xyz = float3(1, 1, 1) - textureColor.xyz;
	color = saturate(float4(input.normal,1));
	color = color * textureColor;
	return color;
}