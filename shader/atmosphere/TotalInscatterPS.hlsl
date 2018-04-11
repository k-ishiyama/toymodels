#include "Atmosphere.h"
#include "AtmosphereConstants.h"
#include "AtmosphericScattering.hlsl.h"
#include "../Random.hlsl"

Texture3D<float4> gTexture3Da : register(t4);
Texture3D<float4> gTexture3Db : register(t5);
Texture3D<float4> gTexture3Dc : register(t6);
Texture3D<float4> gTexture3Dd : register(t7);

struct PSOutput
{
	float4 outColor0	: SV_Target0;
	float4 outColor1	: SV_Target1;
};

PSOutput Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	PSOutput o;
	const uint slice = GetDepthSlice();
	float4 color00 = gTexture3Da.Load(uint4(inPosition.xy, slice, 0));
	float4 color01 = gTexture3Db.Load(uint4(inPosition.xy, slice, 0));
	float4 color10 = gTexture3Dc.Load(uint4(inPosition.xy, slice, 0));
	float4 color11 = gTexture3Dd.Load(uint4(inPosition.xy, slice, 0));
	o.outColor0 = color00 + color10;
	o.outColor1 = color01 + color11;
	// o.outColor0 = color00;
	// o.outColor1 = color01;
	// o.outColor0 = color10;
	// o.outColor1 = color11;
	return o;
}
