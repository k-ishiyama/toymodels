#include "Atmosphere.h"
#include "AtmosphereConstants.h"
#include "AtmosphericScattering.hlsl.h"
#include "../Random.hlsl"

Texture3D<float4> gTexture3Da : register(t4);
Texture3D<float4> gTexture3Db : register(t5);

struct PSOutput
{
	float4 outColor0	: SV_Target0;
	float4 outColor1	: SV_Target1;
};

PSOutput Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	PSOutput o;
	const uint slice = GetDepthSlice();
	o.outColor0 = gTexture3Da.Load(uint4(inPosition.xy, slice, 0));
	o.outColor1 = gTexture3Db.Load(uint4(inPosition.xy, slice, 0));
	return o;
}
