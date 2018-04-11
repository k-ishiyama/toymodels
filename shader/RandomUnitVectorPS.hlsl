// Ref: https://www.shadertoy.com/view/4djSRW
#define HASHSCALE3 float3(443.897, 441.423, 437.195)
float2 hash21(float p)
{
	float3 p3 = frac(float3(p,p,p) * HASHSCALE3);
	p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.xx+p3.yz)*p3.zy);
}

//===============================================

float4 Entry(float4 Pos : SV_POSITION, float2 uv : TexCoord) : SV_Target
{
	float4 outColor = float4(0.0, 0.0, 0.0, 1.0);
	float2 random = hash21(uv.x);

	float z = random.x * 2.0 - 1.0;
	float t = random.y * 3.14159265;
	float r = sqrt(1.0 - z * z);
	float x = r * cos(t);
	float y = r * sin(t);
	
	outColor.xyz = float3(x, y, z);
    return outColor;
}
