//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
#include <iostream>
#include <memory>
#include <chrono>
#include <src/lib/math/Math.hpp>
#include <src/lib/window/Window.hpp>
#include <src/lib/window/Log.hpp>
#include <src/lib/etc/EventHandler.hpp>
#include <src/lib/gpu/Render.hpp>
#include <src/lib/gpu/FullScreenTriangle.hpp>
#include <src/thirdparty/imgui/imgui/imgui.h>
#include <src/thirdparty/imgui/imgui/examples/directx11_example/imgui_impl_dx11.h>
#include <shader/atmosphere/AtmosphereConstants.h>
#include "InputEvent.h"
#include "Camera.h"

/*
	Calculate the scattering coefficients[Yusov13]
	@return scattering coefficients (R, G, B)  [10^{-6}/m]
*/
math::Float3 ComputeRayleighScatteringCoefficients()
{
	// For details, see "A practical Analytic Model for Daylight" by Preetham & Hoffman, p.23

	// Wave lengths
	// [BN08] follows [REK04] and gives the following values for Rayleigh scattering coefficients:
	// RayleighBeta(lambda = (680nm, 550nm, 440nm) ) = (5.8, 13.5, 33.1)e-6
	static const double dWaveLengths[] =
	{
		680e-9,     // red
		550e-9,     // green
		440e-9      // blue
	};

	math::Float3 out_coeff;

	// Calculate angular and total scattering coefficients for Rayleigh scattering:
	{
		double n = 1.0003;    // - Refractive index of air in the visible spectrum
		double N = 2.545e+25; // - Number of molecules per unit volume
		double Pn = 0.035;    // - Depolarization factor for air which expresses corrections 
							  //   due to anisotropy of air molecules

		double dRayleighConst = 8.0*math::PI<double>*math::PI<double>*math::PI<double> * (n*n - 1.0) * (n*n - 1.0) / (3.0 * N) * (6.0 + 3.0*Pn) / (6.0 - 7.0*Pn);
		for (int i = 0; i < 3; ++i)
		{
			double dSctrCoeff;
			{
				double Lambda2 = dWaveLengths[i] * dWaveLengths[i];
				double Lambda4 = Lambda2 * Lambda2;
				dSctrCoeff = dRayleighConst / Lambda4;
				// Total Rayleigh scattering coefficient is the integral of angular scattering coefficient in all directions
				out_coeff[i] = static_cast<float>(dSctrCoeff);
			}
		}
		// Air molecules do not absorb light, so extinction coefficient is only caused by out-scattering
	}
	return out_coeff;
}

//--------------------------------------------------------------------------------------------
// Utility class for full screen triangle
//--------------------------------------------------------------------------------------------
template<typename TUpdater, typename TTexture, size_t TNumTextures>
struct Screen
{
	static_assert(TNumTextures <= TUpdater::sMaxNumResourceTextures, "Too many resource textures");

	Screen() {}

	Screen(Device& inDevice, const TextureDesc& inDesc)
	{
		ScreenCreationContext<TTexture> creation_context(inDevice);
		for(size_t i = 0; i < TNumTextures; ++i)
		{
			mTextures[i] = std::make_unique<TTexture>(inDevice, inDesc);
			creation_context.mResourceTextures[i] = mTextures[i].get();
		}
		mScreen = std::make_unique<TUpdater>(creation_context);
	}

	Screen(Device& inDevice, std::array<const TTexture*, TNumTextures> inTextures)
	{
		ScreenCreationContext<TTexture> creation_context(inDevice);
		for (size_t i = 0; i < TNumTextures; ++i)
			creation_context.mResourceTextures[i] = inTextures[i];
		mScreen = std::make_unique<TUpdater>(creation_context);
	}

	void Update()
	{
		mScreen->Update(mUpdateContext);
	}

	ScreenUpdateContext& GetUpdateContext() { return mUpdateContext; }

	const TTexture& GetTexture(size_t index) const { return *mTextures.at(index); }

protected:
	std::unique_ptr<TUpdater>							mScreen = nullptr;
	std::array<std::unique_ptr<TTexture>, TNumTextures>	mTextures = { nullptr };
	ScreenUpdateContext									mUpdateContext;
};
template<size_t TNumTextures> using ScreenBuffer2D = Screen<ScreenBufferUpdater2D, Texture2D, TNumTextures>;
template<size_t TNumTextures> using ScreenBuffer3D = Screen<ScreenBufferUpdater3D, Texture3D, TNumTextures>;
using PrimaryScreen = Screen<ScreenUpdater, Texture2D, 1>;

//--------------------------------------------------------------------------------------------
// Precomputed Atmospheric Scattering
//--------------------------------------------------------------------------------------------
struct PrecomputedAtmosphericScattering
{
	enum BufferType {
		BUF_OPTICAL_DEPTH = 0,
		BUF_SINGLE_SCATTERING,
		BUF_MULTIPLE_SCATTERING,
		BUF_GATHER_INSCATTER,
		BUF_ACCUMULATE_INSCATTER,
		BUF_TOTAL_INSCATTER,
		BUF_PACK_INSCATTER,
		NUM_BUFFERS,
	};

	enum TextureIndex {
		RAYLEIGH = 0,
		MIE,
	};

	struct Desc
	{
		TextureDesc mTextureDesc[NUM_BUFFERS];
		SamplerDesc mSamplerLinearClampDesc;
		BlendStateDesc	mNoBlendDesc;
		BlendStateDesc	mAddBlendDesc;
		Desc()
		{
			mSamplerLinearClampDesc.mBoundaryCondition = SamplerBoundaryCondition::CLAMP;
			mAddBlendDesc.mBlendType = BlendType::ADDITIVE;
			mNoBlendDesc.mBlendType = BlendType::NONE;

			mTextureDesc[BUF_OPTICAL_DEPTH].mHeight = 512;
			mTextureDesc[BUF_OPTICAL_DEPTH].mWidth = 512;
			mTextureDesc[BUF_OPTICAL_DEPTH].mFormat = TextureFormat::R32G32;

			mTextureDesc[BUF_SINGLE_SCATTERING].mWidth = TEX4D_U;
			mTextureDesc[BUF_SINGLE_SCATTERING].mHeight = TEX4D_V;
			mTextureDesc[BUF_SINGLE_SCATTERING].mDepth = TEX4D_W;

			mTextureDesc[BUF_GATHER_INSCATTER].mWidth = TEX4D_U;
			mTextureDesc[BUF_GATHER_INSCATTER].mHeight = TEX4D_V;
			mTextureDesc[BUF_GATHER_INSCATTER].mDepth = TEX4D_W;

			mTextureDesc[BUF_MULTIPLE_SCATTERING].mWidth = TEX4D_U;
			mTextureDesc[BUF_MULTIPLE_SCATTERING].mHeight = TEX4D_V;
			mTextureDesc[BUF_MULTIPLE_SCATTERING].mDepth = TEX4D_W;

			mTextureDesc[BUF_ACCUMULATE_INSCATTER].mWidth = TEX4D_U;
			mTextureDesc[BUF_ACCUMULATE_INSCATTER].mHeight = TEX4D_V;
			mTextureDesc[BUF_ACCUMULATE_INSCATTER].mDepth = TEX4D_W;

			mTextureDesc[BUF_TOTAL_INSCATTER].mWidth = TEX4D_U;
			mTextureDesc[BUF_TOTAL_INSCATTER].mHeight = TEX4D_V;
			mTextureDesc[BUF_TOTAL_INSCATTER].mDepth = TEX4D_W;

			mTextureDesc[BUF_PACK_INSCATTER].mWidth = TEX4D_U;
			mTextureDesc[BUF_PACK_INSCATTER].mHeight = TEX4D_V;
			mTextureDesc[BUF_PACK_INSCATTER].mDepth = TEX4D_W;
		}
	};

	PrecomputedAtmosphericScattering(Device& inDevice, const Desc& inDesc)
		: mDevice(inDevice)
		, mDesc(inDesc)
		, mTotalInscatter(inDevice, inDesc.mTextureDesc[BUF_TOTAL_INSCATTER])
		, mPackInscatter(inDevice, inDesc.mTextureDesc[BUF_PACK_INSCATTER])
		, mSamplerLinearClamp(inDevice, inDesc.mSamplerLinearClampDesc)
		, mNoBlend(inDevice, inDesc.mNoBlendDesc)
		, mAddBlend(inDevice, inDesc.mAddBlendDesc)
	{
		{
			// single scattering + multiple scattering		
			ScreenUpdateContext& ctx = mTotalInscatter.GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_TOTAL_INSCATTER];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mNoBlend;
			ctx.mTextureSamplers[0] = &mSamplerLinearClamp;
		}

		{
			// single scattering + multiple scattering with packing into a single texture
			ScreenUpdateContext& ctx = mPackInscatter.GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_PACK_INSCATTER];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mNoBlend;
			ctx.mTextureSamplers[0] = &mSamplerLinearClamp;
		}

		ResetScatteringParameters(Planet::Earth);
		CompileShaders();
		CreateBuffers();
	}

	void ReleaseBuffers()
	{
		mOpticalDepth = nullptr;
		mSingleScattering = nullptr;
		mInscatterGathering = nullptr;
		mMultipleScattering = nullptr;
		mAccumulateInscatter = nullptr;
	}

	void CreateBuffers()
	{
		assert(mOpticalDepth == nullptr);
		assert(mSingleScattering == nullptr);
		assert(mInscatterGathering == nullptr);
		assert(mMultipleScattering == nullptr);
		assert(mAccumulateInscatter == nullptr);

		// Create buffers
		{
			// optical depth
			mOpticalDepth = std::make_unique< ScreenBuffer2D<1> >(mDevice, mDesc.mTextureDesc[BUF_OPTICAL_DEPTH]);
			ScreenUpdateContext& ctx = mOpticalDepth->GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_OPTICAL_DEPTH];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mNoBlend;
		}
		{
			// inscatter
			mSingleScattering = std::make_unique< ScreenBuffer3D<2> >(mDevice, mDesc.mTextureDesc[BUF_SINGLE_SCATTERING]);
			ScreenUpdateContext& ctx = mSingleScattering->GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_SINGLE_SCATTERING];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mNoBlend;
			ctx.mTextureSamplers[0] = &mSamplerLinearClamp;
		}
		{
			// inscatter gather
			mInscatterGathering = std::make_unique< ScreenBuffer3D<2> >(mDevice, mDesc.mTextureDesc[BUF_GATHER_INSCATTER]);
			ScreenUpdateContext& ctx = mInscatterGathering->GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_GATHER_INSCATTER];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mNoBlend;
			ctx.mTextureSamplers[0] = &mSamplerLinearClamp;
		}
		{
			// multiple scattering
			mMultipleScattering = std::make_unique< ScreenBuffer3D<2> >(mDevice, mDesc.mTextureDesc[BUF_MULTIPLE_SCATTERING]);
			ScreenUpdateContext& ctx = mMultipleScattering->GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_MULTIPLE_SCATTERING];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mNoBlend;
			ctx.mTextureSamplers[0] = &mSamplerLinearClamp;
		}
		{
			// accumulate multiple scattering
			mAccumulateInscatter = std::make_unique< ScreenBuffer3D<2> >(mDevice, mDesc.mTextureDesc[BUF_ACCUMULATE_INSCATTER]);
			ScreenUpdateContext& ctx = mAccumulateInscatter->GetUpdateContext();
			ctx.mPixelShader = &mPixelShaders[BUF_ACCUMULATE_INSCATTER];
			ctx.mVertexShader = &mVertexShader;
			ctx.mBlendState = &mAddBlend;
			ctx.mTextureSamplers[0] = &mSamplerLinearClamp;
		}
	}

	enum class Planet { Earth = 0, Mars };
	void ResetScatteringParameters(Planet inPlanet)
	{
		// ToDo: scattering coeff. calculation (https://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf, Appendix 3)
		auto& param = mPrecomputedParam;
		switch (inPlanet)
		{
		case PrecomputedAtmosphericScattering::Planet::Earth:
			param.mRayleighSctrCoeff = 1e6f * ComputeRayleighScatteringCoefficients();
			param.mRayleighScaleHeight = 7.997f;
			param.mMieSctrCoeff = math::Float3(20.0f, 20.0f, 20.0f);
			param.mMieScaleHeight = 1.2f;
			param.mMieAbsorption = 1.11f;
			break;
		default:
			assert(false);
		}
	}

	void UpdateShaderParameters(ScreenUpdateContext::ShaderParam& inParam, size_t inStartIndex = 0)
	{
		assert(inStartIndex + 2 < inParam.mFloat4.size());
		const auto& param = mPrecomputedParam;
		inParam.mFloat4.at(inStartIndex + 0).x = param.mRayleighSctrCoeff.x * 1e-3f;
		inParam.mFloat4.at(inStartIndex + 0).y = param.mRayleighSctrCoeff.y * 1e-3f;
		inParam.mFloat4.at(inStartIndex + 0).z = param.mRayleighSctrCoeff.z * 1e-3f;
		inParam.mFloat4.at(inStartIndex + 0).w = param.mRayleighScaleHeight;
		inParam.mFloat4.at(inStartIndex + 1).x = param.mMieSctrCoeff.x * 1e-3f;
		inParam.mFloat4.at(inStartIndex + 1).y = param.mMieSctrCoeff.y * 1e-3f;
		inParam.mFloat4.at(inStartIndex + 1).z = param.mMieSctrCoeff.z * 1e-3f;
		inParam.mFloat4.at(inStartIndex + 1).w = param.mMieScaleHeight;
		inParam.mFloat4.at(inStartIndex + 2).x = param.mMieAbsorption;
	}

	void UpdateShaderParameters()
	{
		UpdateShaderParameters(mOpticalDepth->GetUpdateContext().mShaderParam);
		UpdateShaderParameters(mSingleScattering->GetUpdateContext().mShaderParam);
		UpdateShaderParameters(mInscatterGathering->GetUpdateContext().mShaderParam);
		UpdateShaderParameters(mMultipleScattering->GetUpdateContext().mShaderParam);
	}

	void CompileShaders()
	{
		std::string error_message;
		for (auto& ps : mPixelShaders)
			if (!ps.Compile(mDevice, error_message))
				std::cout << error_message << std::endl;
		if (!mVertexShader.Compile(mDevice, error_message))
			std::cout << error_message << std::endl;
	}

	void GeneratePrecomputedTexture()
	{

		ReleaseBuffers();
		CreateBuffers();
		UpdateShaderParameters();

		{
			// optical depth
			mOpticalDepth->Update();
		}
		{
			// single scattering
			ScreenUpdateContext& ctx = mSingleScattering->GetUpdateContext();
			ctx.mInputTexture2Ds[0] = &mOpticalDepth->GetTexture(0);
			mSingleScattering->Update();
		}

		//
		// Multiple scattering
		//
		if(mNumScattering > 1)
		{
			{
				// gather inscatter
				ScreenUpdateContext& ctx = mInscatterGathering->GetUpdateContext();
				ctx.mInputTexture3Ds[RAYLEIGH] = &mSingleScattering->GetTexture(RAYLEIGH);
				ctx.mInputTexture3Ds[MIE] = &mSingleScattering->GetTexture(MIE);
				mInscatterGathering->Update();
			}
			{
				// multiple scattering
				ScreenUpdateContext& ctx = mMultipleScattering->GetUpdateContext();
				ctx.mInputTexture2Ds[0] = &mOpticalDepth->GetTexture(0);
				ctx.mInputTexture3Ds[RAYLEIGH] = &mInscatterGathering->GetTexture(RAYLEIGH);
				ctx.mInputTexture3Ds[MIE] = &mInscatterGathering->GetTexture(MIE);
				mMultipleScattering->Update();
			}
			{
				// accumulate multiple scattering
				ScreenUpdateContext& ctx = mAccumulateInscatter->GetUpdateContext();
				ctx.mInputTexture3Ds[RAYLEIGH] = &mMultipleScattering->GetTexture(RAYLEIGH);
				ctx.mInputTexture3Ds[MIE] = &mMultipleScattering->GetTexture(MIE);
				mAccumulateInscatter->Update();
			}
		}

		for (std::uint32_t i = 2; i < mNumScattering; ++i)
		{
			{
				// gather inscattering
				ScreenUpdateContext& ctx = mInscatterGathering->GetUpdateContext();
				ctx.mInputTexture3Ds[RAYLEIGH] = &mMultipleScattering->GetTexture(RAYLEIGH);
				ctx.mInputTexture3Ds[MIE] = &mMultipleScattering->GetTexture(MIE);
				mInscatterGathering->Update();
			}
			{
				// multiple scattering
				ScreenUpdateContext& ctx = mMultipleScattering->GetUpdateContext();
				ctx.mInputTexture2Ds[0] = &mOpticalDepth->GetTexture(0);
				ctx.mInputTexture3Ds[RAYLEIGH] = &mInscatterGathering->GetTexture(RAYLEIGH);
				ctx.mInputTexture3Ds[MIE] = &mInscatterGathering->GetTexture(MIE);
				mMultipleScattering->Update();
			}
			{
				// accumulate multiple scattering
				ScreenUpdateContext& ctx = mAccumulateInscatter->GetUpdateContext();
				ctx.mInputTexture3Ds[RAYLEIGH] = &mMultipleScattering->GetTexture(RAYLEIGH);
				ctx.mInputTexture3Ds[MIE] = &mMultipleScattering->GetTexture(MIE);
				mAccumulateInscatter->Update();
			}
		}

		//
		// Result
		//
		{
			// single scattering + multiple scattering
			ScreenUpdateContext& ctx = mTotalInscatter.GetUpdateContext();
			ctx.mInputTexture3Ds[RAYLEIGH] = &mSingleScattering->GetTexture(RAYLEIGH);
			ctx.mInputTexture3Ds[MIE] = &mSingleScattering->GetTexture(MIE);
			ctx.mInputTexture3Ds[RAYLEIGH + 2] = &mAccumulateInscatter->GetTexture(RAYLEIGH);
			ctx.mInputTexture3Ds[MIE + 2] = &mAccumulateInscatter->GetTexture(MIE);
			mTotalInscatter.Update();
		}

		{
			// single scattering + multiple scattering with packing into a single texture
			ScreenUpdateContext& ctx = mPackInscatter.GetUpdateContext();
			ctx.mInputTexture3Ds[RAYLEIGH] = &mSingleScattering->GetTexture(RAYLEIGH);
			ctx.mInputTexture3Ds[MIE] = &mSingleScattering->GetTexture(MIE);
			ctx.mInputTexture3Ds[RAYLEIGH + 2] = &mAccumulateInscatter->GetTexture(RAYLEIGH);
			ctx.mInputTexture3Ds[MIE + 2] = &mAccumulateInscatter->GetTexture(MIE);
			mPackInscatter.Update();
		}
	}

	const Texture3D& GetSingleScatteringTexture(TextureIndex index) const { return mSingleScattering->GetTexture(index); }
	const Texture3D& GetMultipleScatteringTexture(TextureIndex index) const { return mMultipleScattering->GetTexture(index); }
	const Texture2D& GetOpticalDepthTexture() const { return mOpticalDepth->GetTexture(0); }
	const Texture3D& GetInscatterGatherTexture(TextureIndex index) const { return mInscatterGathering->GetTexture(index); }
	const Texture3D& GetAccumulateInscatterTexture(TextureIndex index) const { return mAccumulateInscatter->GetTexture(index); }
	const Texture3D& GetTotalInscatterTexture(TextureIndex index) const { return mTotalInscatter.GetTexture(index); }
	const Texture3D& GetPackedTotalInscatterTexture() const { return mPackInscatter.GetTexture(0); }

	PrecomputedSctrParams& GetParam() { return mPrecomputedParam; }

	void SetNumScattering(std::uint32_t n) { mNumScattering = n; }
	std::uint32_t GetNumScattering() const { return mNumScattering; }

protected:
	Device&		mDevice;
	const Desc&	mDesc;
	PrecomputedSctrParams	mPrecomputedParam;
	std::uint32_t	mNumScattering = 6;

	std::array<PixelShader, NUM_BUFFERS>	mPixelShaders = {
		"shader/atmosphere/OpticalDepthPS.hlsl",
		"shader/atmosphere/SingleScatteringPS.hlsl",
		"shader/atmosphere/MultipleScatteringPS.hlsl",
		"shader/atmosphere/GatherInscatterPS.hlsl",
		"shader/atmosphere/AddScatteringPS.hlsl",
		"shader/atmosphere/TotalInscatterPS.hlsl",
		"shader/atmosphere/PackInscatterPS.hlsl",
	};
	VertexShader							mVertexShader = {
		"shader/ScreenVS.hlsl"
	};
	Sampler				mSamplerLinearClamp;
	BlendState			mNoBlend;
	BlendState			mAddBlend;

	std::unique_ptr< ScreenBuffer2D<1> >	mOpticalDepth		= nullptr;
	std::unique_ptr< ScreenBuffer3D<2> >	mSingleScattering	= nullptr;
	std::unique_ptr< ScreenBuffer3D<2> >	mInscatterGathering	= nullptr;
	std::unique_ptr< ScreenBuffer3D<2> >	mMultipleScattering	= nullptr;
	std::unique_ptr< ScreenBuffer3D<2> >	mAccumulateInscatter= nullptr;
	ScreenBuffer3D<2>	mTotalInscatter;
	ScreenBuffer3D<1>	mPackInscatter;
};


//--------------------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------------------
int main()
{
	int exit_code;

	std::unique_ptr<wnd::WindowBase> window( wnd::WindowBase::Create() );
	if (!window->Initialize("Precomputed Atmospheric Scattering v0.1"))
	{
		exit_code = -1;
		return exit_code;
	}

	void* handle = window->GetWindowHandle();
	Device	device(&handle);
	ImGui_ImplDX11_Init(handle, &device.GetD3D11Device(), &device.GetD3D11DeviceContext());

	InputEventHandler input_event_handler(*window);
	InputEventForImGui imgui_input_event(input_event_handler, ImGui::GetIO());
	TouchControlledCamera camera(0.0, 6.0f + EARTH_RADIUS, 0.0f);
	InputEventForCamera camera_input_event(input_event_handler, *window, camera, ImGui::GetIO());

	enum PixelShaderIndices : char {
		PS_ATMOSPHERIC_SCATTERING,
		NUM_PIXELSHADERS,
	};
	std::array<PixelShader, NUM_PIXELSHADERS> pixel_shaders = {
		"shader/atmosphere/AtmosphericScatteringPS.hlsl",
	};
	VertexShader	default_vs("shader/ScreenVS.hlsl");

	std::string error_message;
	for (auto& ps : pixel_shaders)
		if (!ps.Compile(device, error_message))
			std::cout << error_message << std::endl;
	if (!default_vs.Compile(device, error_message))
		std::cout << error_message << std::endl;
	BackBuffer		back_buffer(device);

	SamplerDesc		sampler_clamp_desc;
	SamplerDesc		sampler_wrap_desc;
	sampler_clamp_desc.mBoundaryCondition = SamplerBoundaryCondition::CLAMP;
	sampler_wrap_desc.mBoundaryCondition = SamplerBoundaryCondition::WRAP;
	Sampler			sampler_clamp(device, sampler_clamp_desc);
	Sampler			sampler_wrap(device, sampler_wrap_desc);

	BlendStateDesc	no_blend_desc;
	no_blend_desc.mBlendType = BlendType::NONE;
	BlendState	blend_state_none(device, no_blend_desc);

	// precomputed atmospheric scattering
	PrecomputedAtmosphericScattering::Desc atmosphere_desc;
	PrecomputedAtmosphericScattering atmosphere(device, atmosphere_desc);
	atmosphere.GeneratePrecomputedTexture();

	// primary screen
	std::array<const Texture2D*, 1> primary_screen_textures = { &back_buffer };
	PrimaryScreen primary_screen(device, primary_screen_textures);
	{
		ScreenUpdateContext& ctx = primary_screen.GetUpdateContext();
		ctx.mPixelShader = &pixel_shaders[PS_ATMOSPHERIC_SCATTERING];
		ctx.mVertexShader = &default_vs;
		ctx.mInputTexture3Ds[0] = &atmosphere.GetPackedTotalInscatterTexture();
		ctx.mInputTexture3Ds[1] = &atmosphere.GetTotalInscatterTexture(PrecomputedAtmosphericScattering::RAYLEIGH);
		ctx.mInputTexture3Ds[2] = &atmosphere.GetTotalInscatterTexture(PrecomputedAtmosphericScattering::MIE);
		ctx.mInputTexture2Ds[0] = &atmosphere.GetOpticalDepthTexture();
		ctx.mTextureSamplers[0] = &sampler_clamp;
		ctx.mTextureSamplers[1] = &sampler_wrap;
		ctx.mBlendState = &blend_state_none;
	}

	const wnd::Info& window_info = window->GetInfo();
	const float window_size[2] = { float(window_info.mWindowSize[0]), float(window_info.mWindowSize[1]) };

	PrecomputedSctrParams& precomp_params = atmosphere.GetParam();
	RuntimeSctrParams runtime_params;

	auto	time_start	= std::chrono::system_clock::now();
	auto	time_prev = time_start;
	bool	ui_compilation_requested = false;
	bool	ui_precomputation_requested = false;
	bool	ui_use_vsync = true;
	float	ui_exposure_compensation = -13.5f;
	math::Float2 ui_light_angle(89.0f, 120.0f);
	while (true)
	{
		wnd::WindowBase::UpdateResult result;
		window->Update(result);
		if (result.mState == wnd::WindowBase::UpdateResult::State::Exit)
		{
			exit_code = result.mExitCode;
			break;
		}

		auto time_now = std::chrono::system_clock::now();
		float elapsed_time_sec = static_cast<float>(1e-3 * std::chrono::duration_cast<std::chrono::milliseconds>(time_now - time_start).count());
		float dt = static_cast<float>(1e-3 * std::chrono::duration_cast<std::chrono::milliseconds>(time_now - time_prev).count());
		dt = std::max(1e-3f, dt);
		time_prev = time_now;

		float theta = ui_light_angle.x * math::PI<float> / 180.f;
		float phi = ui_light_angle.y * math::PI<float> / 180.0f;
		math::Float3 light_dir(std::cos(phi) * std::sin(theta), std::cos(theta), std::sin(phi) * std::sin(theta));

		camera.Update(dt);
		const math::Float3x3&	camera_rot = camera.GetRotationMatrix();
		const math::Float3&		camera_pos = camera.GetPosition();

		//-------------------------
		// Set shader parameters
		//-------------------------
		{
			// primary screen
			ScreenUpdateContext& ctx = primary_screen.GetUpdateContext();
			ctx.mShaderParam.mFloat4[0].x = elapsed_time_sec;
			ctx.mShaderParam.mFloat4[0].y = window_size[0];
			ctx.mShaderParam.mFloat4[0].z = window_size[1];
			ctx.mShaderParam.mFloat4[1].x = camera_rot[0][0];
			ctx.mShaderParam.mFloat4[1].y = camera_rot[0][1];
			ctx.mShaderParam.mFloat4[1].z = camera_rot[0][2];
			ctx.mShaderParam.mFloat4[1].w = 0.0f;
			ctx.mShaderParam.mFloat4[2].x = camera_rot[1][0];
			ctx.mShaderParam.mFloat4[2].y = camera_rot[1][1];
			ctx.mShaderParam.mFloat4[2].z = camera_rot[1][2];
			ctx.mShaderParam.mFloat4[2].w = 0.0f;
			ctx.mShaderParam.mFloat4[3].x = camera_rot[2][0];
			ctx.mShaderParam.mFloat4[3].y = camera_rot[2][1];
			ctx.mShaderParam.mFloat4[3].z = camera_rot[2][2];
			ctx.mShaderParam.mFloat4[3].w = 0.0f;
			ctx.mShaderParam.mFloat4[4].x = camera_pos[0];
			ctx.mShaderParam.mFloat4[4].y = camera_pos[1];
			ctx.mShaderParam.mFloat4[4].z = camera_pos[2];
			ctx.mShaderParam.mFloat4[4].w = 0.0f;
			ctx.mShaderParam.mFloat4[5].x = light_dir.x;
			ctx.mShaderParam.mFloat4[5].y = light_dir.y;
			ctx.mShaderParam.mFloat4[5].z = light_dir.z;
			ctx.mShaderParam.mFloat4[6].y = runtime_params.mMieAsymmetry;
			ctx.mShaderParam.mFloat4[6].z = ui_exposure_compensation;
			
			atmosphere.UpdateShaderParameters(ctx.mShaderParam, 8);
		}

		switch (result.mState)
		{
		case wnd::WindowBase::UpdateResult::State::Idle:
			if (ui_compilation_requested)
			{
				for (auto& ps : pixel_shaders)
					if (!ps.Compile(device, error_message))
						std::cout << error_message << std::endl;
				if (!default_vs.Compile(device, error_message))
					std::cout << error_message << std::endl;
				atmosphere.CompileShaders();
				ui_compilation_requested = false;
			}
			
			if (ui_precomputation_requested)
			{
				atmosphere.GeneratePrecomputedTexture();
				ScreenUpdateContext& ctx = primary_screen.GetUpdateContext();
				ctx.mInputTexture3Ds[0] = &atmosphere.GetPackedTotalInscatterTexture();
				ctx.mInputTexture3Ds[1] = &atmosphere.GetTotalInscatterTexture(PrecomputedAtmosphericScattering::RAYLEIGH);
				ctx.mInputTexture3Ds[2] = &atmosphere.GetTotalInscatterTexture(PrecomputedAtmosphericScattering::MIE);
				ctx.mInputTexture2Ds[0] = &atmosphere.GetOpticalDepthTexture();
				ui_precomputation_requested = false;
			}

			ImGui_ImplDX11_NewFrame();
			if (ImGui::Begin("ToyModel"))
			{
				if (ImGui::Button("Compile"))
				{
					ui_compilation_requested = true;
					ui_precomputation_requested = true;
				}
				ImGui::Text("%.2f [s], %.2f ms/frame", elapsed_time_sec, 1000.0f / ImGui::GetIO().Framerate);
				// ImGui::Text("Camera pos = (%.2f, %.2f, %.2f)", camera_pos.x, camera_pos.y, camera_pos.z);

				if (ImGui::TreeNode("Precomputation"))
				{
					if (ImGui::Button("Precompute"))
						ui_precomputation_requested = true;

					int num_scattering = atmosphere.GetNumScattering();
					ImGui::DragInt("Num. Scattering", &num_scattering, 1.0f, 1, 11);
					atmosphere.SetNumScattering(num_scattering);

					ImGui::InputFloat3("Rayleigh Scattering Coeff", (float*)&precomp_params.mRayleighSctrCoeff, 2);
					ImGui::InputFloat("Rayleigh Scale Height", &precomp_params.mRayleighScaleHeight, 0.0f, 0.0f, 2);
					ImGui::InputFloat3("Mie Scattering Coeff", (float*)&precomp_params.mMieSctrCoeff, 2);
					ImGui::InputFloat("Mie Scale Height", &precomp_params.mMieScaleHeight, 0.0f, 0.0f, 2);
					ImGui::InputFloat("Mie Absorption", &precomp_params.mMieAbsorption, 0.0f, 0.0f, 2);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Runtime"))
				{
					const math::Float3& fwd = camera_rot[static_cast<size_t>(Camera::Direction::FORWARD)];
					ImGui::Text("Camera / (%.1f, %.1f, %.1f), (%.2f, %.2f, %.2f)", camera_pos.x, camera_pos.y, camera_pos.z, fwd.x, fwd.y, fwd.z);
					ImGui::SliderFloat("Exposure Compensation", &ui_exposure_compensation, -16.0f, 16.0f);
					// ImGui::RadioButton("View", &ui_camera_mode, 0); ImGui::SameLine();
					// ImGui::RadioButton("Omni", &ui_camera_mode, 1);

					ImGui::Text("Sun");
					ImGui::SliderFloat("Theta", &ui_light_angle.x, 0.0f, 180.0f);
					ImGui::SliderFloat("Phi", &ui_light_angle.y, -180.0f, 180.f);
					
					ImGui::Text("Atmosphere");
					ImGui::SliderFloat("Mie Asymmetry", &runtime_params.mMieAsymmetry, 0.0f, 1.0f);
					ImGui::TreePop();
				}

				ImGui::End();
			}

			primary_screen.Update();
			ImGui::Render();
			device.Present(ui_use_vsync);
			break;
		case wnd::WindowBase::UpdateResult::State::Exit:
			break;
		case wnd::WindowBase::UpdateResult::State::Busy:
			break;
		default:
			break;
		}
	}

	ImGui_ImplDX11_Shutdown();

	return exit_code;
}
