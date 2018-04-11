#ifndef ATMOSPHERE_CONSTANTS_H
#define ATMOSPHERE_CONSTANTS_H

#ifdef __cplusplus
#include <src/lib/math/Math.hpp>
using float2 = math::Vector2<float>;
using float3 = math::Vector3<float>;
using float4 = math::Vector4<float>;
#endif	// __cplusplus

#define	EARTH_RADIUS 6360.f // [km]
#define	ATM_TOP_HEIGHT 260.f // [km]
#define	ATM_TOP_RADIUS	(EARTH_RADIUS + ATM_TOP_HEIGHT)
#define	INSCATTER_INTEGRAL_STEPS 512
#define	EARTH_CENTER	float3(0,-EARTH_RADIUS,0)
#define	LUT_HEIGHT_MARGIN 0.004 // [km]

#if 0
// Low quality
#define	TEX4D_U	4
#define	TEX4D_V	64
#define	TEX4D_W	8
#else
// High quality
#define	TEX4D_U	12	// height
#define	TEX4D_V	96	// view zenith
#define	TEX4D_W	24	// sun zenith
#endif

struct PrecomputedSctrParams
{
	float3	mRayleighSctrCoeff;			// [nm^{-1}]
	float	mRayleighScaleHeight;		// [km]

	float3	mMieSctrCoeff;				// [nm^{-1}]
	float	mMieScaleHeight;			// [km]

	float	mMieAbsorption;				// [-]
	float3	mPadding;

#ifdef __cplusplus
	PrecomputedSctrParams()
		: mRayleighSctrCoeff(5.78f, 13.6f, 33.1f)
		, mRayleighScaleHeight(7.997f)
		, mMieSctrCoeff(20.0f, 20.0f, 20.0f)
		, mMieScaleHeight(3.0f)
		, mMieAbsorption(1.11f)
	{}
#endif	// __cplusplus
};
#ifdef __cplusplus
static_assert(sizeof(PrecomputedSctrParams) % 16 == 0, "sizeof(""PrecomputedSctrParams"") is not multiple of 16");
#else
cbuffer cbPrecomputedSctrParams : register(b1)
{
	PrecomputedSctrParams gCBPrecomputedSctrParams;
}
#endif


struct RuntimeSctrParams
{
	float	mMieAsymmetry;				// [-]
	float3	mPadding;

#ifdef __cplusplus
	RuntimeSctrParams()
		: mMieAsymmetry(0.76f)
	{}
#endif	// __cplusplus
};
#ifdef __cplusplus
static_assert(sizeof(RuntimeSctrParams) % 16 == 0, "sizeof(""RuntimeSctrParams"") is not multiple of 16");
#else
cbuffer cbRuntimeSctrParams : register(b2)
{
	RuntimeSctrParams gCBRuntimeSctrParams;
}
#endif

#endif // ATMOSPHERE_CONSTANTS_H
