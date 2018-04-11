#include "HelloWorld.h"

struct PSOutput
{
	float4 outColor0	: SV_Target0;
	float4 outColor1	: SV_Target1;
};

PSOutput Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	const float	time	= GetElapsedTime();
	const float	depth	= GetCurrentDepth();

	float4 outColor0 = float4(0.0, 0.0, 0.0, 1.0);
	float4 outColor1 = float4(0.0, 0.0, 0.0, 1.0);
	// outColor.xyz = float3(inUV.x, inUV.y, 0.5 + 0.5 * sin(time));
	outColor0.xyz = float3(inUV.x, inUV.y, depth);
	outColor1.xyz = float3(inUV.x, depth, inUV.y);

	PSOutput o;
	o.outColor0 = outColor0;
	o.outColor1 = outColor1;
	return o;
}
