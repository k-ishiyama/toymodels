#include "HelloWorld.h"
#include "../Util.hlsl"

float4 Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	const float		time		= GetElapsedTime();
	const float2	resolution	= GetScreenResolution();

	float4	outColor = float4(0.0, 0.0, 0.0, 1.0);
	outColor.xyz = float3(inUV.x, inUV.y, 0.5 + 0.5 * sin(time));
	outColor.xy = inPosition.xy / resolution;

	// outColor.xyz = 0;
	DebugDrawTexture2D(outColor, gTexture2Da, SamplerLinearClamp, inUV, float2(0.0, 0.0 ), float2(0.25, 0.25));
	DebugDrawTexture3D(outColor, gTexture3Da, SamplerLinearClamp, inUV, float2(0.0, 0.25), float2(1.0 , 0.05));
	DebugDrawTexture3D(outColor, gTexture3Db, SamplerLinearClamp, inUV, float2(0.0, 0.30), float2(1.0 , 0.05));


    return outColor;
}
