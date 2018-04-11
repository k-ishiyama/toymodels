#include "../Random.hlsl"
#include "../Util.hlsl"
#include "../Screen.h"

//===============================================
float ValueNoise31(float3 x, float3 boundary)
{
	float3 b = boundary;
	float3 p = floor(x);
	float3 f = frac(x);
	f = f * f * (3.0 - 2.0 * f);

	return	lerp(
				lerp(
					lerp(
						Hash31(fmod(p + float3(0, 0, 0), b)),
						Hash31(fmod(p + float3(1, 0, 0), b)),
						f.x
					),
					lerp(
						Hash31(fmod(p + float3(0, 1, 0), b)),
						Hash31(fmod(p + float3(1, 1, 0), b)),
						f.x
					),
					f.y
				),
				lerp(
					lerp(
						Hash31(fmod(p + float3(0, 0, 1), b)),
						Hash31(fmod(p + float3(1, 0, 1), b)),
						f.x
					),
					lerp(
						Hash31(fmod(p + float3(0, 1, 1), b)),
						Hash31(fmod(p + float3(1, 1, 1), b)),
						f.x
					),
					f.y
				),
				f.z
			);
}

// x = [0,1]
float TilableFbm31(float3 x, float inFrequency, const int inNumOctaves)
{
	const float boundary = inFrequency;	// take the boundary at max of 1st octave

	float amp	= 1.0;
	float freq	= inFrequency;
	float result= 0.0;
	float norm	= amp;
	for (int i = 0; i < inNumOctaves; i++)
	{
		result += amp * ValueNoise31(x * freq, float3(boundary, boundary, boundary));
		amp *= 0.5;
		freq *= 2.0;
		norm += amp;
	}

	return result / norm;
}

float4 Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	const float		depth	= GetWQ().x;
	const float3	uvw		= float3(inUV, depth);

	float4 outColor = float4(0.0, 0.0, 0.0, 1.0);

	float h = TilableFbm31(uvw, 16, 8);
	outColor.xyz = h.xxx;

    return outColor;
}
