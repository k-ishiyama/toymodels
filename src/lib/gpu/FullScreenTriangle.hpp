#pragma once
#include "Render.hpp"
#include <type_traits>	// std::conditional
#include <vector>		// std::vector
#include <array>		// std::array

template<typename T> struct ScreenUpdaterBase;

template<typename T> struct GPU_EXPORT ScreenCreationContext
{
	ScreenCreationContext(const Device& inDevice) : mDevice(inDevice) {}
	const Device&	mDevice;
	std::array<const T*, ScreenUpdaterBase<T>::sMaxNumResourceTextures>	mResourceTextures = { nullptr }; // Used to generate render targets
};

struct GPU_EXPORT ScreenUpdateContext
{
	static const size_t sNumInputTextures = 4;
	static const size_t sNumTextureSamplers = 2;

	struct GPU_EXPORT /* alignas(16) */ ShaderParam
	{
		struct float4 { float x, y, z, w; };
		std::array<float4, 16> mFloat4;	// float4 x 16, shader parameter for "screen updater" system
	};
	static_assert( sizeof(ShaderParam) % 16 == 0, "sizeof(""ShaderParam"") is not multiple of 16" );
	ShaderParam				mShaderParam;
	const PixelShader*		mPixelShader	= nullptr;
	const VertexShader*		mVertexShader	= nullptr;
	const BlendState*		mBlendState		= nullptr;
	std::array<const Sampler*, sNumTextureSamplers>	mTextureSamplers = { nullptr };
	std::array<const Texture2D*, sNumInputTextures>	mInputTexture2Ds = { nullptr };
	std::array<const Texture3D*, sNumInputTextures>	mInputTexture3Ds = { nullptr };
	std::uint32_t			mNum4dSlices = 1;	// length of the 4th dimension. the division factor of the depth of 3d texture
};

struct ID3D11Buffer;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

//------------------------------------------------------------------

template<typename T> struct ScreenUpdaterBase
{
	static const size_t sMaxNumResourceTextures = 2;

	ScreenUpdaterBase(const ScreenCreationContext<T>& inContext);
	virtual ~ScreenUpdaterBase() = default;
	virtual void Update(const ScreenUpdateContext& inContext) = 0;

protected:
	void DrawFullscreenTriangle(std::uint32_t inScreenWidth, std::uint32_t inScreenHeight);
	void SetRenderTechnique(const ScreenUpdateContext &inContext);
	void SetShaderResources(const ScreenUpdateContext &inContext);
	void UnbindResources();

	const Device& mDevice;
	std::array<const T*, sMaxNumResourceTextures> mResourceTextures = { nullptr };
	detail::unique_iunknown_ptr<ID3D11Buffer> mConstantBuffer = nullptr;
	std::array<std::vector<detail::unique_iunknown_ptr<ID3D11RenderTargetView>>, sMaxNumResourceTextures> mRenderTargetViews;
};

struct GPU_EXPORT ScreenUpdater : ScreenUpdaterBase<Texture2D>
{
	ScreenUpdater(const ScreenCreationContext<Texture2D>& inContext);
	virtual ~ScreenUpdater();
	virtual void Update(const ScreenUpdateContext& inContext);
};

//------------------------------------------------------------------

struct GPU_EXPORT ScreenBufferUpdater2D : ScreenUpdaterBase<Texture2D>
{
	ScreenBufferUpdater2D(const ScreenCreationContext<Texture2D>& inContext);
	virtual ~ScreenBufferUpdater2D();
	virtual void Update(const ScreenUpdateContext& inContext);
};

//------------------------------------------------------------------

struct GPU_EXPORT ScreenBufferUpdater3D : ScreenUpdaterBase<Texture3D>
{
	ScreenBufferUpdater3D(const ScreenCreationContext<Texture3D>& inContext);
	virtual ~ScreenBufferUpdater3D();
	virtual void Update(const ScreenUpdateContext& inContext);
protected:
	detail::unique_iunknown_ptr<ID3D11Buffer> mTex3dConstantBuffer = nullptr;
};