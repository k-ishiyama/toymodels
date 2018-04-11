#include "HelloWorld.h"

float4 Entry(float4 Pos : SV_POSITION, float2 uv : TexCoord) : SV_Target
{
	float	time = GetElapsedTime();
	float4 outColor = float4(0.0, 0.0, 0.0, 1.0);
	outColor.xyz = float3(uv.x, uv.y, 0.5 + 0.5 * sin(time));
    return outColor;
}
