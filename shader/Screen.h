#ifndef SCREEN_H
#define SCREEN_H

#define	M_PI	3.14159265

//--------------------------------------------------

struct ConstantBufferParam
{
	float4		mFloat4[16];
};

cbuffer cbConstantBufferParam : register(b0)
{
	ConstantBufferParam gCBParam;
}

float4	GetParam(const int index) { return gCBParam.mFloat4[index]; }

// following functions will be deprecated
float	GetElapsedTime() { return GetParam(0).x; }
float2	GetScreenResolution() { return GetParam(0).yz; }

//--------------------------------------------------

struct Tex3DShaderParams
{
	float2		mWQ;
	uint		mDepthSlice;
	float		mPadding;
};

cbuffer cbConstantBufferParam : register(b1)
{
	Tex3DShaderParams	gTex3dParam;
}

float2	GetWQ() { return gTex3dParam.mWQ; }
float	GetCurrentDepth() { return GetWQ().x; }
uint	GetDepthSlice() { return gTex3dParam.mDepthSlice; }

//--------------------------------------------------

SamplerState SamplerLinearClamp : register(s0)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

SamplerState SamplerLinearWrap : register(s1)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

//--------------------------------------------------

#endif // SCREEN_H
