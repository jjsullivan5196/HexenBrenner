//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture2D txDepth : register(t1);
Texture2D tx : register(t2);
SamplerState samLinear : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
matrix World;
matrix View;
matrix Projection;
float3 info;
};

//--------------------------------------------------------------------------------------
;

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Tex = input.Tex;
    
    return output;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VSlevel(VS_INPUT input)
{
	VS_INPUT output = (PS_INPUT)0;
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;

	return output;
}
float4 PSlevel(PS_INPUT input) : SV_Target
{
	float4 color = tx.Sample(samLinear, input.Tex.xy);
	float depth = saturate(input.Pos.z / input.Pos.w);

	//depth = pow(depth, 0.97);
	//color = (depth*0.9 + 0.02);
	return color * depth * 2.5;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    float4 color = txDiffuse.Sample( samLinear, input.Tex );
	float depth = saturate(input.Pos.z / input.Pos.w);
	//depth = pow(depth,0.97);
	//color = depth;// (depth*0.9 + 0.02);

	if (info.x >= 1)
	{
		color.r *= info.y;
		color = color * depth * 12;
	}
	else
	{
		color = color * depth * 2.5;
	}
	

	return color;
}
