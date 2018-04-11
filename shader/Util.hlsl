#ifndef UTIL_HLSL
#define UTIL_HLSL

//-------------------------------------------------------------------------
float3 UVToDirection(float2 inUV)
{
    float2 st = (inUV * 2.0 - float2(1.0, 1.0)) * float2(3.14159265, 0.5 * 3.14159265); 
    return float3(cos(st.y) * cos(st.x), sin(st.y), cos(st.y) * sin(st.x));
}

float2 DirectionToUV(float3 inDir)
{
	return float2(atan2(inDir.z, inDir.x) / (2.0 * 3.14159265) + 0.5, 1.0 - acos(inDir.y) / 3.14159265);
}

//---------------------------------------------------------------------------------

float3 UVToOmniDirection(
		float2 inClipPos	// ([-1, 1], [-1, 1])
)
{
	float3	outViewDir = float3(0.0, 0.0, 1.0);
	float	z2 = inClipPos.x * inClipPos.x + inClipPos.y * inClipPos.y;
   	if (z2 <= 1.0)
   	{
		float t = acos(1.0 - z2);
		float s = atan2(inClipPos.y, inClipPos.x);
		outViewDir = -float3( sin(t) * cos(s), cos(t), sin(t) * sin(s)); 
   	}
	return outViewDir;
}

float3 UVToViewDirection(
		float2		inClipPos,
		float3x3	inCameraMatrix
		)
{
	float3 dir = normalize(float3(inClipPos, 2.0));
	return mul(inCameraMatrix, dir);
}

#if 0
float3 UVToViewDirection(
		float2		inClipPos,	//	[-1,1]
		float3x3	inSO3,
		float		inFoV		//	[rad]
		)
{
	float3 dir = normalize(float3(inClipPos, tan(inFoV)));
	return mul(inSO3, dir);
}
#endif

//--------------------------------------------------------------------------------

void DebugDrawTexture2D(inout float4 ioColor, Texture2D<float4> inTexture, SamplerState inSampler, float2 inUV, float2 inPos, float2 inSize)
{
	float right		= inPos.x + inSize.x;
	float bottom	= inPos.y + inSize.y;
	if (inUV.x >= inPos.x && inUV.x < right && inUV.y >= inPos.y && inUV.y < bottom)
	{
		float2 tex_uv = (inUV - inPos) / inSize;
		ioColor = inTexture.SampleLevel(inSampler, tex_uv, 0);
	}
}

void DebugDrawTexture3D(inout float4 ioColor, Texture3D<float4> inTexture, SamplerState inSampler, float2 inUV, float2 inPos, float2 inSize)
{
	float right		= inPos.x + inSize.x;
	float bottom	= inPos.y + inSize.y;
	if (inUV.x >= inPos.x && inUV.x < right && inUV.y >= inPos.y && inUV.y < bottom)
	{
		float3 size;
		inTexture.GetDimensions(size.x, size.y, size.z);

		float2 tex_uv	= (inUV - inPos) / inSize;
		float3 tex_uvw = float3(frac(tex_uv.x * size.z), tex_uv.y, floor(tex_uv.x * size.z) / size.z);
		ioColor = inTexture.SampleLevel(inSampler, tex_uvw, 0);
	}
}

#endif // UTIL_HLSL
