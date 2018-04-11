#include "Atmosphere.h"
#include "AtmosphereConstants.h"
#include "AtmosphericScattering.hlsl.h"
#include "../Util.hlsl"

// #pragma pack_matrix(row_major)	// Sets the matrix packing alignment to row major

Texture2D<float4> gTexture2Da : register(t0);
Texture2D<float4> gTexture2Db : register(t1);
Texture2D<float4> gTexture2Dc : register(t2);
Texture2D<float4> gTexture2Dd : register(t3);
Texture3D<float4> gTexture3Da : register(t4);
Texture3D<float4> gTexture3Db : register(t5);
Texture3D<float4> gTexture3Dc : register(t6);
Texture3D<float4> gTexture3Dd : register(t7);

//-------------------------------------------------------------------------

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((x*(a*x + b)) / (x*(c*x + d) + e));
}

//-------------------------------------------------------------------------
#define	GND_TOP_HEIGHT 4.5	// [km]
#define GND_TOP_RADIUS	(EARTH_RADIUS + GND_TOP_HEIGHT)
#define GND_BOTTOM_HEIGHT 0.0
#define USE_PACKED_INSCATTER	1


float3x3 CalculateCameraMatrix(float3 look_to, float3 up_dir)
{
	float3 forward	= look_to;
	float3 right	= normalize( cross(look_to	, up_dir	) );
	float3 up		= normalize( cross(right	, look_to	) );
    return float3x3(right, up, forward);
}

//--------------------------------------------------------

float4 Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	const float		time		= GetParam(0).x;
	const float2	resolution	= GetParam(0).yz;
	const float2	clip_pos	= (2.0 * float2(inUV.x, 1.0 - inUV.y) - 1.0) * resolution.xy / resolution.yy;
	const float3	extraterrestrial_sunlight = 1e5 * float3(1, 1, 1);

	float4 outColor = float4(0.0, 0.0, 0.0, 1.0);

	float		fov				= tan(M_PI/3.25);
	float3		light_dir		= GetParam(5).xyz;
	float3		camera_pos		= GetParam(4).xyz;
	float3x3	camera_matrix	= float3x3(GetParam(1).xyz, GetParam(2).xyz, GetParam(3).xyz);
	float3		view_dir		= UVToViewDirection(clip_pos * fov, camera_matrix);
	const int camera_mode = 0;
	if (camera_mode == 1)
		view_dir =  UVToOmniDirection(clip_pos * fov);

	float ray_length = 1e4;
	float3 world_pos = camera_pos + ray_length * view_dir;
	float4 distances = RayDoubleSphereIntersect(camera_pos, view_dir, float2(EARTH_RADIUS, ATM_TOP_RADIUS));
	float2 ray_distance_to_earth = distances.xy;
	float2 ray_distance_to_atm_top = distances.zw;
	bool is_ground		= (ray_distance_to_earth.x > 0);
	bool is_atmosphere	= (ray_distance_to_atm_top.y > 0);
	if(!is_atmosphere)
		return outColor;

	world_pos = camera_pos + ray_distance_to_earth.x * view_dir;

	if(ray_distance_to_atm_top.x > 0)
		camera_pos = camera_pos + ray_distance_to_atm_top.x * view_dir;

	const float3	atm_top_pos				= camera_pos + ray_distance_to_atm_top.y * view_dir;
	const bool		shadow_enabled			= GetParam(4).w;
	const float		mie_g					= GetParam(6).y;

	PrecomputedSctrParams	sctr_params;
	sctr_params.mRayleighSctrCoeff		= GetParam(8).xyz;
	sctr_params.mRayleighScaleHeight	= GetParam(8).w;
	sctr_params.mMieSctrCoeff			= GetParam(9).xyz;
	sctr_params.mMieScaleHeight			= GetParam(9).w;
	sctr_params.mMieAbsorption			= GetParam(10).x;

	ray_length = ray_distance_to_atm_top.y;
	if(ray_distance_to_earth.x > 0.0)
		ray_length = min(ray_distance_to_earth.x, ray_distance_to_atm_top.y);

	world_pos = camera_pos + ray_length * view_dir;
	float3	inscatterR, inscatterM;
#if USE_PACKED_INSCATTER
	LookUpPrecomputedScatteringPacked(
		camera_pos + EARTH_CENTER, view_dir, light_dir, sctr_params.mRayleighSctrCoeff,
		gTexture3Da, SamplerLinearClamp,
		inscatterR, inscatterM
	);
#else
	LookUpPrecomputedScatteringSeparated(
		camera_pos + EARTH_CENTER, view_dir, light_dir,
		gTexture3Db, gTexture3Dc, SamplerLinearClamp,
		inscatterR, inscatterM
	);
#endif
	float VoL = dot(view_dir, light_dir);
	inscatterR *= RayleighPhase(VoL);
	inscatterM *= MiePhase(VoL, mie_g);
	outColor.xyz = extraterrestrial_sunlight * (inscatterR + inscatterM);

	const float exposure_compensation = GetParam(6).z;
	outColor.xyz *= exp2(exposure_compensation);
	outColor.xyz = ACESFilm(outColor.xyz);
	outColor.xyz = pow(outColor.xyz, 1.0 / 2.2);

	// DebugDrawTexture3D(outColor, gTexture3Da, SamplerLinearClamp, inUV, float2(0.0 , 0.80), float2(0.25, 0.20));
	// DebugDrawTexture3D(outColor, gTexture3Db, SamplerLinearClamp, inUV, float2(0.25, 0.80), float2(0.25, 0.20));
	// DebugDrawTexture3D(outColor, gTexture3Dc, SamplerLinearClamp, inUV, float2(0.5 , 0.80), float2(0.25, 0.20));
	// DebugDrawTexture2D(outColor, gTexture2Dc, SamplerLinearClamp, inUV, float2(0.0 , 0.80), float2(0.25, 0.20));
	// DebugDrawTexture2D(outColor, gTexture2Dd, SamplerLinearClamp, inUV, float2(0.25, 0.80), float2(0.25, 0.20));

    return outColor;
}
