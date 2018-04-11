#include "Atmosphere.h"
#include "AtmosphereConstants.h"
#include "AtmosphericScattering.hlsl.h"

float2 Entry(float4 inPosition : SV_POSITION, float2 inUV : TexCoord) : SV_Target
{
	const float2 scale_heights = float2(GetParam(0).w, GetParam(1).w);

	// u : height, v : zenith altitude
	float costheta = 2.0 * inUV.y - 1.0;
	float height = lerp(0.0, ATM_TOP_HEIGHT, inUV.x);
	height = clamp(height, LUT_HEIGHT_MARGIN, ATM_TOP_HEIGHT - LUT_HEIGHT_MARGIN);

	float sintheta = sqrt(saturate(1.0 - costheta * costheta));
	float3 sample_pos = float3(0.0, height, 0.0);
	float3 ray_dir = float3(0.0, costheta, sintheta);

	const int num_steps = 128;
	return CalculateNaiveOpticalDepthAlongRay(sample_pos, ray_dir, scale_heights, num_steps);
}
