#include "Atmosphere.h"
#include "AtmosphereConstants.h"
#include "AtmosphericScattering.hlsl.h"
#include "../Random.hlsl"

Texture2D<float4> gTexture2Da : register(t0);
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
	o.outColor0 = float4(0.0, 0.0, 0.0, 0.0);
	o.outColor1 = float4(0.0, 0.0, 0.0, 0.0);
	
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

	// Intersect view ray with the top of the atmosphere and the earth
	float4 distances = RayDoubleSphereIntersect(start_pos - EARTH_CENTER, view_dir, float2(EARTH_RADIUS, ATM_TOP_RADIUS));
	float2 ray_distance_to_earth	= distances.xy;
	float2 ray_distance_to_atm_top	= distances.zw;

	if (ray_distance_to_atm_top.y <= 0)
		return o; // Error: Ray misses the earth

	float ray_distance = ray_distance_to_atm_top.y;
	if (ray_distance_to_earth.x > 0)
		ray_distance = min(ray_distance, ray_distance_to_earth.x); // Ray hits the earth
	float3 end_pos = start_pos + view_dir * ray_distance;


	const float NUM_SAMPLES = 256.0;
	float step_length = ray_distance / NUM_SAMPLES;

	const float mie_g = 0.0;
	float mu = dot(light_dir, view_dir);

	float2 scale_height = float2(params.mRayleighScaleHeight, params.mMieScaleHeight);
	float2 height_factor = exp(-(length(start_pos - EARTH_CENTER) - EARTH_RADIUS) / scale_height);
	float3 inscatter_r = float3(0.0, 0.0, 0.0);
	float3 inscatter_m = float3(0.0, 0.0, 0.0);
	float3 transmittance;
	MultipleScattering(
			start_pos, end_pos, light_dir, params,
			gTexture3Da, gTexture3Db, SamplerLinearClamp,
			inscatter_r, inscatter_m, transmittance, INSCATTER_INTEGRAL_STEPS);

	o.outColor0.xyz = inscatter_r;
	o.outColor1.xyz = inscatter_m;

	// (We ignored the reflection from the ground)

	return o;
}
