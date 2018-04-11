#include "Atmosphere.h"
#include "AtmosphereConstants.h"
#include "AtmosphericScattering.hlsl.h"

Texture2D<float2> gTexture2Da : register(t0);

struct PSOutput
{
	float4 outColor0	: SV_Target0;
	float4 outColor1	: SV_Target1;
};

PSOutput Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	PSOutput o;
	o.outColor0 = float4(0.0, 0.0, 0.0, 1.0);
	o.outColor1 = float4(0.0, 0.0, 0.0, 1.0);

	const float3 uvw = float3(inUV.x, inUV.y, GetWQ().x); 
	float height, cos_view_zenith, cos_sun_zenith;
	LUTCoordToWorldCoord(uvw, height, cos_view_zenith, cos_sun_zenith);
	float3 start_pos	= float3(0, height, 0);
	float3 view_dir		= ComputeViewDir(cos_view_zenith);
	float3 light_dir	= ComputeLightDir(view_dir, cos_sun_zenith);

	// Intersect view ray with the top of the atmosphere and the earth
	float4 distances = RayDoubleSphereIntersect(start_pos - EARTH_CENTER, view_dir, float2(EARTH_RADIUS, ATM_TOP_RADIUS));
	float2 ray_distance_to_earth = distances.xy;
	float2 ray_distance_to_atm_top = distances.zw;

	if (ray_distance_to_atm_top.y <= 0)
		return o; // Error: Ray misses the earth

	bool is_ground = false;
	float ray_distance = ray_distance_to_atm_top.y;
	if (ray_distance_to_earth.x > 0)
	{
		ray_distance = min(ray_distance, ray_distance_to_earth.x); // Ray hits the earth
		is_ground = true;
	}

	float3 end_pos = start_pos + view_dir * ray_distance;

	PrecomputedSctrParams	params;
	params.mRayleighSctrCoeff	= GetParam(0).xyz;
	params.mRayleighScaleHeight	= GetParam(0).w;
	params.mMieSctrCoeff		= GetParam(1).xyz;
	params.mMieScaleHeight		= GetParam(1).w;
	params.mMieAbsorption		= GetParam(2).x;

	// Integrate single-scattering
	float3 transmittance;
	float3 inscatterR;
	float3 inscatterM;
	SingleScattering(
		start_pos,
		end_pos,
		light_dir,
		params,
		gTexture2Da,	// optical depth
		SamplerLinearClamp,
		inscatterR,
		inscatterM,
		transmittance,
		INSCATTER_INTEGRAL_STEPS
		);

	if(is_ground)
	{
		float3 ground_albedo = float3(0.45, 0.45, 0.45);
		float height = (length(end_pos - EARTH_CENTER) - EARTH_RADIUS);
		float cos_light_zenith = dot(normalize(end_pos - EARTH_CENTER), light_dir);
		float2 optdepth_to_top = gTexture2Da.SampleLevel(SamplerLinearClamp, float2(height / ATM_TOP_HEIGHT, cos_light_zenith*0.5 + 0.5), 0).xy;
		float3 trans_to_top		= exp(-params.mRayleighSctrCoeff * optdepth_to_top.x - params.mMieSctrCoeff * optdepth_to_top.y);
		inscatterR += ground_albedo * saturate(cos_light_zenith) * transmittance * trans_to_top;
		inscatterM += ground_albedo * saturate(cos_light_zenith) * transmittance * trans_to_top;
	}

	o.outColor0.xyz = inscatterR;
	o.outColor1.xyz = inscatterM;

	return o;
}
