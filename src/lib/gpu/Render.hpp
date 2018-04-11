#pragma once
#include "GPU.hpp"
#include <memory>
#include <string>
#include <vector>	// std::vector

#define RENDER_DIRECTX11

struct IUnknown;
namespace detail
{
	struct IUnknownDeleter
	{
		void operator()(IUnknown* r);
	};

	template<typename T>
	using unique_iunknown_ptr = std::unique_ptr<T, IUnknownDeleter>;
}

struct Device;
struct ResourcePool;

//=============================================================

//=============================================================
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11PixelShader;
struct ID3D11VertexShader;
struct ID3D11SamplerState;
struct ID3D11BlendState;
struct ID3D11Texture2D;
struct ID3D11Texture3D;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

struct GPU_EXPORT Device
{
	Device(void** inWindowHandle);
	~Device();
	void Present(bool vSyncEnabled);

	ID3D11Device& GetD3D11Device() { return *mD3dDevice; }
	ID3D11DeviceContext& GetD3D11DeviceContext() { return *mImmediateContext; }
	IDXGISwapChain& GetSwapChain() { return *mSwapChain; }

	ID3D11Device& GetD3D11Device() const { return *mD3dDevice; }
	ID3D11DeviceContext& GetD3D11DeviceContext() const { return *mImmediateContext; }
	IDXGISwapChain& GetSwapChain() const { return *mSwapChain; }
private:
	detail::unique_iunknown_ptr<ID3D11Device>			mD3dDevice = nullptr;
	detail::unique_iunknown_ptr<ID3D11DeviceContext>	mImmediateContext = nullptr;
	detail::unique_iunknown_ptr<IDXGISwapChain>			mSwapChain = nullptr;
};

//=============================================================

struct GPU_EXPORT PixelShader
{
	PixelShader(const std::string inFilePath);
	~PixelShader();
	ID3D11PixelShader* Get() const { return mUniquePtr.get(); }
	bool Compile(Device& inDevice, std::string& outMessage);
protected:
	detail::unique_iunknown_ptr<ID3D11PixelShader> mUniquePtr = nullptr;
	std::string	mFilePath;
};

//=============================================================

struct GPU_EXPORT VertexShader
{
	VertexShader(const std::string inFilePath);
	~VertexShader();
	ID3D11VertexShader* Get() const { return mUniquePtr.get(); }
	bool Compile(Device& inDevice, std::string& outMessage);
protected:
	detail::unique_iunknown_ptr<ID3D11VertexShader> mUniquePtr = nullptr;
	std::string	mFilePath;
};

//=============================================================

enum struct SamplerBoundaryCondition
{
	CLAMP,
	WRAP,
};

struct GPU_EXPORT SamplerDesc
{
	SamplerBoundaryCondition	mBoundaryCondition = SamplerBoundaryCondition::CLAMP;
};

struct GPU_EXPORT Sampler
{
	Sampler(Device& inDevice, const SamplerDesc& inDesc);
	~Sampler();
	ID3D11SamplerState* Get() const { return mUniquePtr.get(); }
protected:
	detail::unique_iunknown_ptr<ID3D11SamplerState> mUniquePtr = nullptr;
};

//=============================================================

enum struct BlendType
{
	NONE,
	ADDITIVE,
	ALPHA
};

struct GPU_EXPORT BlendStateDesc
{
	BlendType	mBlendType = BlendType::NONE;
};

struct GPU_EXPORT BlendState
{
	BlendState(Device& inDevice, const BlendStateDesc& inDesc);
	~BlendState();
	ID3D11BlendState* Get() const { return mUniquePtr.get(); }
protected:
	detail::unique_iunknown_ptr<ID3D11BlendState> mUniquePtr = nullptr;
};

//=============================================================


struct Texture
{
	const auto GetShaderResourceView() const { return mShaderResourceView.get(); }
	const auto& GetRenderTargetViews() const { return mRenderTargetViews; }
	const auto& GetTextureDesc() const { return mDesc; }
protected:
	TextureDesc mDesc;
	detail::unique_iunknown_ptr<ID3D11ShaderResourceView>				mShaderResourceView = nullptr;
	std::vector<detail::unique_iunknown_ptr<ID3D11RenderTargetView>>	mRenderTargetViews;
};

struct GPU_EXPORT Texture2D : Texture
{
	Texture2D();
	Texture2D(Device& inDevice, const TextureDesc& inDesc);
	virtual ~Texture2D();
	ID3D11Texture2D* Get() const { return mUniquePtr.get(); }
protected:
	detail::unique_iunknown_ptr<ID3D11Texture2D> mUniquePtr;
};

struct GPU_EXPORT Texture3D : Texture
{
	Texture3D();
	Texture3D(Device& inDevice, const TextureDesc& inDesc);
	virtual ~Texture3D();
	ID3D11Texture3D* Get() const { return mUniquePtr.get(); }
protected:
	detail::unique_iunknown_ptr<ID3D11Texture3D> mUniquePtr;
};

//=============================================================

struct GPU_EXPORT BackBuffer : Texture2D
{
	BackBuffer(Device& inDevice);
	virtual ~BackBuffer();
};

//=============================================================
