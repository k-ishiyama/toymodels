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
	const float3 uvw = float3(inUV.x, inUV.y, GetWQ().x);

	float height, cos_view_zenith, cos_sun_zenith;
	LUTCoordToWorldCoord(uvw, height, cos_view_zenith, cos_sun_zenith);
	float3 start_pos	= float3(0, height, 0);
	float3 view_dir		= ComputeViewDir(cos_view_zenith);
	float3 light_dir	= ComputeLightDir(view_dir, cos_sun_zenith);

	PrecomputedSctrParams	params;
	params.mRayleighSctrCoeff	= GetParam(0).xyz;
	params.mRayleighScaleHeight	= GetParam(0).w;
	params.mMieSctrCoeff		= GetParam(1).xyz;
	params.mMieScaleHeight		= GetParam(1).w;
	params.mMieAbsorption		= GetParam(2).x;

	float2 scale_height		= float2(params.mRayleighScaleHeight, params.mMieScaleHeight);
	float2 height_factor	= exp(-height / scale_height);

#define NUM_RANDOMDIR_SAMPLES	128
	float3 inscatter_sum_r = float3(0, 0, 0);
	float3 inscatter_sum_m = float3(0, 0, 0);
	for (int i = 0; i < NUM_RANDOMDIR_SAMPLES; ++i)
	{
		float3 rand_dir = normalize(RandomUnitVector(uvw.xy * Hash21(float(i) + uvw.z * 2236.20679) * 1414.24356));
		float mu = dot(view_dir, rand_dir);

		float3	inscatter_r, inscatter_m;
		LookUpPrecomputedScatteringSeparated(
			start_pos, rand_dir, light_dir,
			gTexture3Da, gTexture3Db, SamplerLinearClamp,
			inscatter_r, inscatter_m
		);

		float mie_g = 0.0; // [Elek09]
		inscatter_r *= RayleighPhase(mu);
		inscatter_m *= CornetteShanksPhaseFunc(mu, mie_g);

		inscatter_sum_r += inscatter_r;
		inscatter_sum_m += inscatter_m;
	}

	o.outColor0 = float4(inscatter_sum_r * 4.0 * 3.14159265 / float(NUM_RANDOMDIR_SAMPLES), 0.0);
	o.outColor1 = float4(inscatter_sum_m * 4.0 * 3.14159265 / float(NUM_RANDOMDIR_SAMPLES), 0.0);

	return o;
}
