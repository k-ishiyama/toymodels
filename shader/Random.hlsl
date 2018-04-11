#ifndef RANDOM_HLSL
#define RANDOM_HLSL

// https://www.shadertoy.com/view/4dlcR4
#define W	uint2(0x3504f335u, 0x8fc1ecd5u)
#define M	741103597u
uint hash(uint2 x)
{
	x *= W;		// x' = Fx(x), y' = Fy(y)
	x.x ^= x.y;	// combine
	x.x *= M;	// MLCG constant
	return x.x;
}

float Hash31(float3 n)
{
	return frac(sin(dot(n, float3(12.9898, 4.1414, 2.23620679))) * 43758.5453);
}

// https://www.shadertoy.com/view/4djSRW
#define HASHSCALE3 float3(.1031, .1030, .0973)
float2 Hash22(float2 p)
{
	float3 p3 = frac(p.xyx * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return frac((p3.xx+p3.yz)*p3.zy);
}

float2 Hash21(float p)
{
	float3 p3 = frac(float3(p, p, p) * HASHSCALE3);
	p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.xx+p3.yz)*p3.zy);

}

float3 RandomUnitVector(float2 p)
{
	float2 random = Hash22(p);
	float z = random.x * 2.0 - 1.0;
	float t = random.y * 3.14159265;
	float r = sqrt(1.0 - z * z);
	float x = r * cos(t);
	float y = r * sin(t);
	return float3(x, y, z);
}

#endif // RANDOM_HLSL
