#ifndef ATMOSPHERIC_SCATTERING_H
#define ATMOSPHERIC_SCATTERING_H

//------------------------------------------------------
//	Precomputed atmospheric scattering
//
//	 References
//		[Bruneton08]	Bruneton, E., Neyret, F. "Precomputed atmospheric scattering", Proc. of EGSR'08, Vol 27, no 4, pp. 1079--1086 (2008)
//		[Elek09]		Elek, O. "Rendering Parametrizable Planetary Atmosphere with Multiple Scattering in Real-Time", CESCG 2009. (2009)
//		[Yusov13]		Yusov, E. "Outdoor Light Scattering Sample Update", https://software.intel.com/en-us/blogs/2013/09/19/otdoor-light-scattering-sample-update, (2013)
//		[Hillaire16]	Hillaire, S. "Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite", SIGGRAPH 2016. (2016)
//------------------------------------------------------

//------------------------------------------------------
//	Intersection tests
//------------------------------------------------------
float4 RayDoubleSphereIntersect(
	in float3 inRayOrigin,
	in float3 inRayDir,
	in float2 inSphereRadii	//	x = 1st sphere, y = 2nd sphere
	)
{
	float a = dot(inRayDir, inRayDir);
	float b = 2.0 * dot(inRayOrigin, inRayDir);
	float2 c = dot(inRayOrigin, inRayOrigin) - inSphereRadii * inSphereRadii;
	float2 d = b*b - 4.0 * a*c;
	// d < 0 .. Ray misses the sphere
	// d = 0 .. Ray intersects the sphere in one point
	// d > 0 .. Ray intersects the sphere in two points
	float2 real_root_mask = (d.xy >= 0.0);
	d = sqrt(max(d, 0.0));
	float4 distance = float4(-b - d.x, -b + d.x, -b - d.y, -b + d.y) / (2.0 * a);
	distance = lerp(float4(-1.0, -1.0, -1.0, -1.0), distance, real_root_mask.xxyy);
	// distance.x = distance to the intersection point of the 1st sphere (near side)
	// distance.y = distance to the intersection point of the 1st sphere (far side)
	// distance.z = distance to the intersection point of the 2nd sphere (near side)
	// distance.w = distance to the intersection point of the 2nd sphere (far side)
	return distance;
}

//------------------------------------------------------
//	 [Bruneton09]
//------------------------------------------------------
float3 UnpackMieInscatter(float4 inRayleighMie, float3 inRayleighScatteringCoeff)
{
	return inRayleighMie.xyz * inRayleighMie.w / max(inRayleighMie.x, 1e-9) * (inRayleighScatteringCoeff.x / inRayleighScatteringCoeff);
}

float4 PackInscatter(float3 inRayleigh, float3 inMie)
{
	return float4(inRayleigh.xyz, inMie.x);
}

//------------------------------------------------------
//	 3d LookUpTable parametrization [Yusov13]
//------------------------------------------------------
#define LUT_RESOLUTION	float3(TEX4D_U, TEX4D_V, TEX4D_W)

float3 ComputeViewDir(in float cosViewZenith)
{
	return float3(sqrt(saturate(1.0 - cosViewZenith * cosViewZenith)), cosViewZenith, 0.0);
}

float3 ComputeLightDir(in float3 inViewDir, in float cosLightZenith)
{
	float3 light_dir;
	light_dir.x = (inViewDir.x > 0) ? (1. - cosLightZenith * inViewDir.y) / inViewDir.x : 0;
	light_dir.y = cosLightZenith;
	light_dir.z = sqrt(saturate(1 - dot(light_dir.xy, light_dir.xy)));
	//light_dir = normalize(light_dir); // Do not normalize light_dir [Yusov13]
	return light_dir;
}

float GetCosHorizonAngle(float height)
{
	// Due to numeric precision issues, height might sometimes be slightly negative [Yusov13]
	height = max(height, 0);
	return -sqrt(height * (2 * EARTH_RADIUS + height)) / (EARTH_RADIUS + height);
}

float ZenithAngle2TexCoord(float inCosZenith, float inHeight, in float inTextureResolution)
{
	float prev_tex_coord = 1;
	float tex_coord;
	float cos_horizon = GetCosHorizonAngle(inHeight);
	// When performing look-ups into the scattering texture, it is very important that all the look-ups are consistent
	// wrt to the horizon. This means that if the first look-up is above (below) horizon, then the second look-up
	// should also be above (below) horizon. 
	// We use previous texture coordinate, if it is provided, to find out if previous look-up was above or below
	// horizon. If texture coordinate is negative, then this is the first look-up
	float offset;
	if (inCosZenith > cos_horizon)
	{
		// Scale to [0,1]
		tex_coord = saturate((inCosZenith - cos_horizon) / (1.0 - cos_horizon));
		tex_coord = sqrt(sqrt(tex_coord));

		offset = 0.5;
		// Now remap texture coordinate to the upper half of the texture.
		// To avoid filtering across discontinuity at 0.5, we must map
		// the texture coordinate to [0.5 + 0.5/inTextureResolution, 1 - 0.5/inTextureResolution]
		//
		//      0.5   1.5               D/2+0.5        D-0.5  texture coordinate x dimension
		//       |     |                   |            |
		//    |  X  |  X  | .... |  X  ||  X  | .... |  X  |  
		//       0     1          D/2-1   D/2          D-1    texel index
		//
		tex_coord = offset + 0.5 / inTextureResolution + tex_coord * (inTextureResolution / 2.0 - 1.0) / inTextureResolution;
	}
	else
	{
		tex_coord = saturate((cos_horizon - inCosZenith) / (cos_horizon - (-1)));
		tex_coord = sqrt(sqrt(tex_coord));

		offset = 0.0;
		// Now remap texture coordinate to the lower half of the texture.
		// To avoid filtering across discontinuity at 0.5, we must map
		// the texture coordinate to [0.5, 0.5 - 0.5/inTextureResolution]
		//
		//      0.5   1.5        D/2-0.5             texture coordinate x dimension
		//       |     |            |       
		//    |  X  |  X  | .... |  X  ||  X  | .... 
		//       0     1          D/2-1   D/2        texel index
		//
		tex_coord = offset + 0.5 / inTextureResolution + tex_coord * (inTextureResolution / 2.0 - 1.0) / inTextureResolution;
	}

	return tex_coord;
}

float TexCoord2ZenithAngle(float inTexcoord, float inHeight, in float inTextureResolution)
{
	float cos_zenith;
	float cos_horizon = GetCosHorizonAngle(inHeight);
	[flatten] if (inTexcoord > 0.5)
	{
		// Remap to [0,1] from the upper half of the texture [0.5 + 0.5/inTextureResolution, 1 - 0.5/inTextureResolution]
		inTexcoord = saturate((inTexcoord - (0.5 + 0.5 / inTextureResolution)) * inTextureResolution / (inTextureResolution / 2.0 - 1.0));
		inTexcoord *= inTexcoord;
		inTexcoord *= inTexcoord;
		// Assure that the ray does NOT hit Earth
		cos_zenith = max((cos_horizon + inTexcoord * (1.0 - cos_horizon)), cos_horizon + 1e-4);
	}
	else
	{
		// Remap to [0,1] from the lower half of the texture [0.5, 0.5 - 0.5/inTextureResolution]
		inTexcoord = saturate((inTexcoord - 0.5 / inTextureResolution) * inTextureResolution / (inTextureResolution / 2.0 - 1.0));
		inTexcoord *= inTexcoord;
		inTexcoord *= inTexcoord;
		// Assure that the ray DOES hit Earth
		cos_zenith = min((cos_horizon - inTexcoord * (1.0 + cos_horizon)), cos_horizon - 1e-4);
	}
	return cos_zenith;
}

float3 WorldCoordToLUTCoord(
	float inHeight,
	float inCosViewZenith,
	float inCosLightZenith
)
{
	float3 uvw;
	float height = clamp(inHeight, LUT_HEIGHT_MARGIN, ATM_TOP_HEIGHT - LUT_HEIGHT_MARGIN);
	uvw.z = saturate((height - LUT_HEIGHT_MARGIN) / (ATM_TOP_HEIGHT - 2.0 * LUT_HEIGHT_MARGIN));
	uvw.z = sqrt(uvw.z);
	uvw.y = ZenithAngle2TexCoord(inCosViewZenith, height, LUT_RESOLUTION.y);
	uvw.x = (atan(max(inCosLightZenith, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5; // Bruneton's formula [Bruneton09]
	uvw.xz = ((uvw * (LUT_RESOLUTION - 1.0) + 0.5) / LUT_RESOLUTION).xz;
	return uvw;
}

void LUTCoordToWorldCoord(
	in float3 inUVW,
	out float outHeight,
	out float outCosViewZenith,
	out float outCosLightZenith
)
{
	// Rescale to exactly 0,1 range
	inUVW.xz = saturate((inUVW* LUT_RESOLUTION - 0.5) / (LUT_RESOLUTION - 1.0)).xz;
	inUVW.z = inUVW.z * inUVW.z;
	outHeight = inUVW.z * (ATM_TOP_HEIGHT - 2.0 * LUT_HEIGHT_MARGIN) + LUT_HEIGHT_MARGIN;
	outCosViewZenith = TexCoord2ZenithAngle(inUVW.y, outHeight, LUT_RESOLUTION.y);
	outCosLightZenith = tan((2.0 * inUVW.x - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); // Bruneton's formula [Bruneton09]
}

float4 SamplePrecomputedTexture(
	float3 inStartPos,
	float3 inViewDir,
	float3 inLightDir,
	in Texture3D<float4> inTexture3d,
	in SamplerState		inSampler)
{
	float3 dir = inStartPos - EARTH_CENTER;
	float dist = length(dir);
	dir /= dist;
	float height = dist - EARTH_RADIUS;
	float cos_view_zenith = dot(dir, inViewDir);
	float cos_light_zenith = dot(dir, inLightDir);

	float3 uvw = WorldCoordToLUTCoord(height, cos_view_zenith, cos_light_zenith);
	return inTexture3d.SampleLevel(inSampler, uvw, 0);
}

void LookUpPrecomputedScatteringPacked(
	float3 inStartPos,
	float3 inViewDir,
	float3 inLightDir,
	float3 inBetaR,
	in Texture3D<float4> inPackedInscatterTexture,
	in SamplerState	inSampler,
	out float3 outInscatterR,
	out float3 outInscatterM
)
{
	float4 packed_inscatter = SamplePrecomputedTexture(inStartPos, inViewDir, inLightDir, inPackedInscatterTexture, inSampler);
	outInscatterR = packed_inscatter.xyz;
	outInscatterM = UnpackMieInscatter(packed_inscatter, inBetaR);
}

void LookUpPrecomputedScatteringSeparated(
	float3 inStartPos,
	float3 inViewDir,
	float3 inLightDir,
	in Texture3D<float4> inInscatterTextureR,
	in Texture3D<float4> inInscatterTextureM,
	in SamplerState		inSampler,
	out float3 outInscatterR,
	out float3 outInscatterM
)
{
	outInscatterR = SamplePrecomputedTexture(inStartPos, inViewDir, inLightDir, inInscatterTextureR, inSampler).xyz;
	outInscatterM = SamplePrecomputedTexture(inStartPos, inViewDir, inLightDir, inInscatterTextureM, inSampler).xyz;
}

//------------------------------------------------------
//	 3d LookUpTable parametrization for sunlight and skylight
//------------------------------------------------------
void SunlightLUTCoordToWorldCoord(float2 inUV, out float outHeight, out float outSunZenith)
{
	// Rescale to exactly 0,1 range
	inUV.xy		= saturate((inUV * LUT_RESOLUTION.xz - 0.5) / (LUT_RESOLUTION.xz - 1.0));
	outHeight		= inUV.x * inUV.x * (ATM_TOP_HEIGHT - 2.0 * LUT_HEIGHT_MARGIN) + LUT_HEIGHT_MARGIN;
	outSunZenith	= tan((2.0 * inUV.y - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); // Bruneton's formula [Bruneton09]
}

float2 WorldCoordToSunlightLUTCoord(float inHeight, float inSunZenith)
{
	float2 outUV = float2(0, 0);
	float height = clamp(inHeight, LUT_HEIGHT_MARGIN, ATM_TOP_HEIGHT - LUT_HEIGHT_MARGIN);
	outUV.x		= saturate((height - LUT_HEIGHT_MARGIN) / (ATM_TOP_HEIGHT - 2.0 * LUT_HEIGHT_MARGIN));
	outUV.x		= sqrt(outUV.x);
	outUV.y		= (atan(max(inSunZenith, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5; // Bruneton's formula [Bruneton09]
	outUV		= ((outUV * (LUT_RESOLUTION.xz - 1.0) + 0.5) / LUT_RESOLUTION.xz);
	return outUV;
}

float3 SampleLightTexture(
	float3 inStartPos,
	float3 inLightDir,
	in Texture2D<float4> inTexture2d,
	in SamplerState		inSampler)
{
	float3 dir = inStartPos - EARTH_CENTER;
	float dist = length(dir);
	dir /= dist;
	float height = dist - EARTH_RADIUS;
	float cos_light_zenith = dot(dir, inLightDir);
	float2 uv = WorldCoordToSunlightLUTCoord(height, cos_light_zenith);
	float3 light = inTexture2d.SampleLevel(inSampler, uv, 0).xyz;
	return light;
}

//------------------------------------------------------
//	Phase functions
//------------------------------------------------------
float RayleighPhase(float mu)
{
	return 3.0 / 4.0 * 1.0 / (4.0 * M_PI)*(1.0 + mu*mu);
}

float HenyeyGreensteinPhaseFunc(float mu, float g)
{
	return (1.0 - g*g) / ((4.0 * M_PI) * pow(1.0 + g*g - 2.0*g*mu, 1.5));
}

float CornetteShanksPhaseFunc(float mu, float g)
{	
	return (3.0 / 2.0) / (4.0 * M_PI) * (1.0 - g*g) / (2.0 + g*g) * (1.0 + mu*mu)*pow(abs(1.0 + g*g - 2.0*g*mu), -1.5);
}

float MiePhase(float mu, float g)
{
	return CornetteShanksPhaseFunc(mu, g);
}

//------------------------------------------------------
//	Naive optical depth
//		- We call the optical depth considering only height-falloff as "naive optical depth"
//------------------------------------------------------
float2 CalulateNaiveOpticalDepth(
	in float3 inStartPos,
	in float3 inEndPos,
	in float2 inScaleHeights,
	const int inNumSteps)
{
	const float3	dr			= (inEndPos - inStartPos) / inNumSteps;
	const float		length_dr	= length(dr);
	const float		start_height= abs(length(inStartPos - EARTH_CENTER) - EARTH_RADIUS);

	float2 optical_depth	= 0.0;
	for (int i = 0; i <= inNumSteps; ++i)
	{
		float3	curr_pos	= inStartPos + dr * float(i);
		float	height		= abs(length(curr_pos - EARTH_CENTER) - EARTH_RADIUS);
		float2	integrand	= exp(-height / inScaleHeights);
		optical_depth += integrand * length_dr;
	}
	return optical_depth;
}

float2 CalculateNaiveOpticalDepthAlongRay(
	in float3 inWorldPos,
	in float3 inRayDir,
	in float2 inScaleHeights,
	const int inNumSteps)
{
	// Ray - {Earth, The top of atmosphere} intersection test
	float4 distances = RayDoubleSphereIntersect(inWorldPos - EARTH_CENTER, inRayDir, float2(EARTH_RADIUS, ATM_TOP_RADIUS));

	float2 ray_distance_to_earth = distances.xy;
	if (ray_distance_to_earth.x > 0)
		return float2(1e20, 1e20); // huge optical depth

	float2	ray_distance_to_atm_top	= distances.zw;
	float	ray_length				= ray_distance_to_atm_top.y; // far side
	float3	intersection_pos		= inWorldPos + inRayDir * ray_length;
	return CalulateNaiveOpticalDepth(inWorldPos, intersection_pos, inScaleHeights, inNumSteps);
}

//------------------------------------------------------
//	 Single scattering
//------------------------------------------------------
void SingleScattering(
	in float3 inStartPos,
	in float3 inEndPos,
	in float3 inLightDir,
	in PrecomputedSctrParams	inParams,
	in Texture2D<float2>		inOpticalDepthTexture,
	in SamplerState				inOpticalDepthSampler,
	out float3 outInscatterR,
	out float3 outInscatterM,
	out float3 outTransmittance,
	const int inNumSteps)
{
	outTransmittance	= float3(0.0, 0.0, 0.0);
	outInscatterR		= float3(0.0, 0.0, 0.0);
	outInscatterM		= float3(0.0, 0.0, 0.0);

	float3 dr = (inEndPos - inStartPos) / float(inNumSteps);
	float length_dr = length(dr);

	float mie_g = 0.0;
	float3 betaR = inParams.mRayleighSctrCoeff.rgb;
	float3 betaM = inParams.mMieSctrCoeff.rgb  * inParams.mMieAbsorption * (1.0 - mie_g);
	float2 scale_height = float2(inParams.mRayleighScaleHeight, inParams.mMieScaleHeight);

	float2 optdepth_from_cam	= float2(0.0, 0.0);
	// Integrand: exp(-h(x)/H) * t(x->Pc) * t(x->s)
	for (int i = 0; i <= inNumSteps; i++)
	{
		float3 sample_pos = inStartPos + dr * float(i);
		float3 integrandR, integrandM;

		// optdepth_from_cam_integrand = ( exp(-h(x)/HR), exp(-h(x)/HM) )
		// optdepth_to_top =  \int^Pc_x exp(-h(x')/H) dx'
		float height = (length(sample_pos - EARTH_CENTER) - EARTH_RADIUS);
		float2 optdepth_from_cam_integrand = exp(-height / scale_height);

		// optdepth_to_top is precalculated in the texture
		float	cos_light_zenith = dot(normalize(sample_pos - EARTH_CENTER), inLightDir);
		float2	optdepth_to_top = inOpticalDepthTexture.SampleLevel(inOpticalDepthSampler, float2(height / ATM_TOP_HEIGHT, cos_light_zenith*0.5 + 0.5), 0).xy;

		float3 trans_from_cam	= exp(-betaR * optdepth_from_cam.x - betaM * optdepth_from_cam.y);
		float3 trans_to_top		= exp(-betaR * optdepth_to_top.x - betaM * optdepth_to_top.y);

		// integrand: exp(-h(x)/H) * t(x->Pc) * t(x->s)
		integrandR = optdepth_from_cam_integrand.x * trans_from_cam * trans_to_top;
		integrandM = optdepth_from_cam_integrand.y * trans_from_cam * trans_to_top;

		outInscatterR += integrandR * length_dr;
		outInscatterM += integrandM * length_dr;
		optdepth_from_cam += optdepth_from_cam_integrand * length_dr;
	}

	float3 optical_depthR = inParams.mRayleighSctrCoeff.rgb * optdepth_from_cam.x;
	float3 optical_depthM = inParams.mMieSctrCoeff.rgb * inParams.mMieAbsorption * (1.0 - mie_g) * optdepth_from_cam.y;
	outTransmittance = exp(-(optical_depthR + optical_depthM));
	outInscatterR *= inParams.mRayleighSctrCoeff.rgb;
	outInscatterM *= inParams.mMieSctrCoeff.rgb;
}

//------------------------------------------------------
//	 Multiple scattering
//------------------------------------------------------
void MultipleScattering(
	in float3 inStartPos,
	in float3 inEndPos,
	in float3 inLightDir,
	in PrecomputedSctrParams	inParams,
	in Texture3D<float4>		inInscatterTextureR,
	in Texture3D<float4>		inInscatterTextureM,
	in SamplerState				inSamplerLinearClamp,
	out float3 outInscatterR,
	out float3 outInscatterM,
	out float3 outTransmittance,
	const int inNumSteps)
{
	outTransmittance	= float3(0.0, 0.0, 0.0);
	outInscatterR		= float3(0.0, 0.0, 0.0);
	outInscatterM		= float3(0.0, 0.0, 0.0);

	float3 dr = (inEndPos - inStartPos) / float(inNumSteps);
	float3 view_dir = normalize(inEndPos - inStartPos);
	float length_dr = length(dr);

	float mie_g = 0.0;
	float mu = dot(view_dir, inLightDir);
	float3 betaR = inParams.mRayleighSctrCoeff.rgb;
	float3 betaM = inParams.mMieSctrCoeff.rgb  * inParams.mMieAbsorption * (1.0 - mie_g);
	float2 scale_height = float2(inParams.mRayleighScaleHeight, inParams.mMieScaleHeight);

	float2 optdepth_from_cam	= float2(0.0, 0.0);
	// Integrand: exp(-h(x)/H) * t(x->Pc) * t(x->s)
	for (int i = 0; i <= inNumSteps; i++)
	{
		float3 sample_pos = inStartPos + dr * float(i);

		// optdepth_from_cam_integrand = ( exp(-h(x)/HR), exp(-h(x)/HM) )
		float height = (length(sample_pos - EARTH_CENTER) - EARTH_RADIUS);
		float2 optdepth_from_cam_integrand = exp(-height / scale_height);
		float3 trans_from_cam	= exp(-betaR * optdepth_from_cam.x - betaM * optdepth_from_cam.y);

		float3 inscatter_r;
		float3 inscatter_m;
		LookUpPrecomputedScatteringSeparated(
				sample_pos, view_dir, inLightDir,
				inInscatterTextureR, inInscatterTextureM, inSamplerLinearClamp,
				inscatter_r, inscatter_m);
		outInscatterR += inscatter_r * optdepth_from_cam_integrand.x * trans_from_cam * length_dr;
		outInscatterM += inscatter_m * optdepth_from_cam_integrand.y * trans_from_cam * length_dr;
		optdepth_from_cam += optdepth_from_cam_integrand * length_dr;
	}

	float3 optical_depthR = inParams.mRayleighSctrCoeff.rgb * optdepth_from_cam.x;
	float3 optical_depthM = inParams.mMieSctrCoeff.rgb * inParams.mMieAbsorption * (1.0 - mie_g) * optdepth_from_cam.y;
	outTransmittance = exp(-(optical_depthR + optical_depthM));
	outInscatterR *= inParams.mRayleighSctrCoeff.xyz;
	outInscatterM *= inParams.mMieSctrCoeff.xyz;
}

#endif // ATMOSPHERIC_SCATTERING_H
