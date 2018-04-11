#include "FullScreenTriangle.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cassert>

namespace detail
{
	static std::hash<std::string>	sStringHash;

	inline void UpdateConstantBuffer(ID3D11DeviceContext *pDeviceCtx, ID3D11Buffer *pCB, const void *pData, size_t DataSize)
	{
		D3D11_MAPPED_SUBRESOURCE MappedData;
		pDeviceCtx->Map(pCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedData);
		memcpy(MappedData.pData, pData, DataSize);
		pDeviceCtx->Unmap(pCB, 0);
	}
}

enum ConstantBufferSlot
{
	SLOT_RESERVED_PARAM	= 0,
	SLOT_TEX3D_PARAM,
};

//================================================================================

template<typename T> ScreenUpdaterBase<T>::ScreenUpdaterBase(const ScreenCreationContext<T>& inContext) : mDevice(inContext.mDevice)
{}

template<typename T> void ScreenUpdaterBase<T>::DrawFullscreenTriangle(std::uint32_t inScreenWidth, std::uint32_t inScreenHeight)
{
	ID3D11DeviceContext& d3d_context = mDevice.GetD3D11DeviceContext();

	// Setup viewport
	D3D11_VIEWPORT vp;
	vp.Width = float(inScreenWidth);
	vp.Height = float(inScreenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	d3d_context.RSSetViewports(1, &vp);

	// Draw fullscreen triangle
	d3d_context.IASetInputLayout(NULL);
	d3d_context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3d_context.Draw(3, 0);
}

template<typename T> void ScreenUpdaterBase<T>::SetRenderTechnique(const ScreenUpdateContext & inContext)
{
	ID3D11DeviceContext& d3d_context = mDevice.GetD3D11DeviceContext();

	// Samplers
	for (size_t i = 0; i < inContext.mTextureSamplers.size(); ++i)
	{
		if (!inContext.mTextureSamplers[i]) continue;
		ID3D11SamplerState*	sampler = inContext.mTextureSamplers[i]->Get();
		d3d_context.PSSetSamplers(i, 1, &sampler);
	}

	// Shaders
	ID3D11PixelShader*	pixel_shader = inContext.mPixelShader->Get();
	ID3D11VertexShader*	vertex_shader = inContext.mVertexShader->Get();
	d3d_context.PSSetShader(pixel_shader, nullptr, 0);
	d3d_context.VSSetShader(vertex_shader, nullptr, 0);

	// Blend state
	float blending_coeff[] = { 0.0f, 0.0f, 0.0f, 0.0f }; // R, G, B, A
	uint32_t sample_mask = 0xffffffff;
	if(inContext.mBlendState)
	{
		ID3D11BlendState* blend_state = inContext.mBlendState->Get();
		d3d_context.OMSetBlendState(blend_state, blending_coeff, sample_mask);
	}
}

template<typename T> void ScreenUpdaterBase<T>::SetShaderResources(const ScreenUpdateContext & inContext)
{
	ID3D11DeviceContext& d3d_context = mDevice.GetD3D11DeviceContext();
	assert(inContext.mInputTexture2Ds.size() == ScreenUpdateContext::sNumInputTextures);
	for (size_t i = 0; i < inContext.mInputTexture2Ds.size(); ++i)
	{
		const Texture2D* texture = inContext.mInputTexture2Ds[i];
		if (!texture)
			continue;
		ID3D11ShaderResourceView* srv = texture->GetShaderResourceView();
		if (srv)
			d3d_context.PSSetShaderResources(i, 1, &srv);
	}

	assert(inContext.mInputTexture3Ds.size() == ScreenUpdateContext::sNumInputTextures);
	for (size_t i = 0; i < inContext.mInputTexture3Ds.size(); ++i)
	{
		const Texture3D* texture = inContext.mInputTexture3Ds[i];
		if (!texture)
			continue;
		ID3D11ShaderResourceView* srv = texture->GetShaderResourceView();
		if (srv)
			d3d_context.PSSetShaderResources(i + ScreenUpdateContext::sNumInputTextures, 1, &srv);
	}
}

template<typename T> void ScreenUpdaterBase<T>::UnbindResources()
{
	ID3D11DeviceContext& d3d_context = mDevice.GetD3D11DeviceContext();
	ID3D11ShaderResourceView* const pSRV[2 * ScreenUpdateContext::sNumInputTextures] = { nullptr };
	d3d_context.PSSetShaderResources(0, 2 * ScreenUpdateContext::sNumInputTextures, pSRV);
}

//================================================================================

ScreenUpdater::ScreenUpdater(const ScreenCreationContext<Texture2D>& inContext) : ScreenUpdaterBase(inContext)
{
	ID3D11Device& d3d_device = mDevice.GetD3D11Device();

	for (size_t i = 0; i < inContext.mResourceTextures.size(); ++i)
		if (inContext.mResourceTextures[i])
			mResourceTextures[i] = inContext.mResourceTextures[i];

	// Create render target view (assign back buffer for now)
	HRESULT result;
	ID3D11RenderTargetView* rtv;
	const Texture2D* texture2d = inContext.mResourceTextures[0];
	assert(texture2d);
	ID3D11Texture2D* texture_resource = texture2d->Get();
	assert(texture_resource);

	// Create render target view from the texture resource
	result = d3d_device.CreateRenderTargetView(texture_resource, nullptr, &rtv);
	assert(SUCCEEDED(result));

	// create a constant buffer
	D3D11_BUFFER_DESC CBDesc =
	{
		sizeof(ScreenUpdateContext::ShaderParam),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE, //UINT CPUAccessFlags
		0, //UINT MiscFlags;
		0, //UINT StructureByteStride;
	};
	ID3D11Buffer* buffer;
	result = d3d_device.CreateBuffer(&CBDesc, NULL, &buffer);
	assert(SUCCEEDED(result));

	mRenderTargetViews[0].push_back(detail::unique_iunknown_ptr<ID3D11RenderTargetView>(rtv));
	mConstantBuffer = detail::unique_iunknown_ptr<ID3D11Buffer>(buffer);
}

ScreenUpdater::~ScreenUpdater()
{
}

void ScreenUpdater::Update(const ScreenUpdateContext & inContext)
{
	assert(mRenderTargetViews[0].size() == 1);

	ID3D11DeviceContext&	d3d_context = mDevice.GetD3D11DeviceContext();
	IDXGISwapChain&		swapchain = mDevice.GetSwapChain();

	ID3D11Buffer*				constant_buffer = mConstantBuffer.get();
	ID3D11RenderTargetView*		rtv = mResourceTextures[0]->GetRenderTargetViews()[0].get();
	assert(constant_buffer);

	// Update constant buffer
	const ScreenUpdateContext::ShaderParam& reserved_param = inContext.mShaderParam;
	detail::UpdateConstantBuffer(&d3d_context, constant_buffer, &reserved_param, sizeof(reserved_param));
	d3d_context.PSSetConstantBuffers(SLOT_RESERVED_PARAM, 1, &constant_buffer);

	d3d_context.OMSetRenderTargets(1, &rtv, NULL);

	// Clear the back buffer 
	float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
	d3d_context.ClearRenderTargetView(rtv, ClearColor);

	// Set shader resource view
	SetShaderResources(inContext);

	// Set samplers and shaders
	SetRenderTechnique(inContext);

	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	HRESULT hr = swapchain.GetDesc(&swap_chain_desc);
	assert(SUCCEEDED(hr));
	DrawFullscreenTriangle(swap_chain_desc.BufferDesc.Width, swap_chain_desc.BufferDesc.Height);

	// Unbind resources
	UnbindResources();
}

//================================================================================

ScreenBufferUpdater2D::ScreenBufferUpdater2D(const ScreenCreationContext<Texture2D>& inContext) : ScreenUpdaterBase(inContext)
{
	for(size_t i = 0; i < inContext.mResourceTextures.size(); ++i)
		if(inContext.mResourceTextures[i])
			mResourceTextures[i] = inContext.mResourceTextures[i];
	ID3D11Texture2D* texture = mResourceTextures[0]->Get();
	assert(texture);

	ID3D11Device& d3d_device = mDevice.GetD3D11Device();
	HRESULT result;

	// Setup the description of the render target view.
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// create a constant buffer
	D3D11_BUFFER_DESC CBDesc =
	{
		sizeof(ScreenUpdateContext::ShaderParam),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE, //UINT CPUAccessFlags
		0, //UINT MiscFlags;
		0, //UINT StructureByteStride;
	};
	ID3D11Buffer* buffer;
	result = d3d_device.CreateBuffer(&CBDesc, NULL, &buffer);
	assert(SUCCEEDED(result));
	mConstantBuffer = detail::unique_iunknown_ptr<ID3D11Buffer>(buffer);
}

ScreenBufferUpdater2D::~ScreenBufferUpdater2D()
{
}

void ScreenBufferUpdater2D::Update(const ScreenUpdateContext & inContext)
{
	assert(mRenderTargetViews.size() <= sMaxNumResourceTextures);
	ID3D11DeviceContext&	d3d_context = mDevice.GetD3D11DeviceContext();

	ID3D11Buffer*				constant_buffer = mConstantBuffer.get();
	ID3D11RenderTargetView*		rtv = mResourceTextures[0]->GetRenderTargetViews()[0].get();
	ID3D11Texture2D*			texture = mResourceTextures[0]->Get();

	// Update constant buffer
	const ScreenUpdateContext::ShaderParam& reserved_param = inContext.mShaderParam;
	detail::UpdateConstantBuffer(&d3d_context, constant_buffer, &reserved_param, sizeof(reserved_param));
	d3d_context.PSSetConstantBuffers(SLOT_RESERVED_PARAM, 1, &constant_buffer);

	// Set render targets
	d3d_context.OMSetRenderTargets(1, &rtv, NULL);

	// Clear the back buffer 
	float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
	d3d_context.ClearRenderTargetView(rtv, ClearColor);

	// Set shader resource view
	SetShaderResources(inContext);

	// Set samplers and shaders
	SetRenderTechnique(inContext);

	D3D11_TEXTURE2D_DESC texture_desc;
	texture->GetDesc(&texture_desc);
	DrawFullscreenTriangle(texture_desc.Width, texture_desc.Height);

	// Unbind resources
	UnbindResources();
}

//================================================================================
struct Tex3DShaderParams
{
	float			wq[2]; // Used when pre-computing inscattering look-up table
	std::uint32_t	mDepthSlice;
	float			mPadding;
};
static_assert(sizeof(Tex3DShaderParams) % 16 == 0, "sizeof(Tex3DShaderParams) is not multiple of 16");

ScreenBufferUpdater3D::ScreenBufferUpdater3D(const ScreenCreationContext<Texture3D>& inContext) : ScreenUpdaterBase(inContext)
{
	ID3D11Device& d3d_device = mDevice.GetD3D11Device();
	ID3D11DeviceContext& d3d_context = mDevice.GetD3D11DeviceContext();
	HRESULT result;

	for (size_t i = 0; i < inContext.mResourceTextures.size(); ++i)
	{
		if (!inContext.mResourceTextures[i])
			continue;
		mResourceTextures[i] = inContext.mResourceTextures[i];
		ID3D11Texture3D* texture = mResourceTextures[i]->Get();
		D3D11_TEXTURE3D_DESC textureDesc;
		texture->GetDesc(&textureDesc);
		mRenderTargetViews[i].reserve(textureDesc.Depth);

		// Setup the render target view.
		for (size_t depth = 0; depth < textureDesc.Depth; ++depth)
		{
			// Clear render target views.
			ID3D11RenderTargetView* rtv = mResourceTextures[i]->GetRenderTargetViews()[depth].get();
			const float clear_color[4] = { 0.0f };
			d3d_context.ClearRenderTargetView(rtv, clear_color);
		}
	}

	// create a constant buffers
	D3D11_BUFFER_DESC CBDesc =
	{
		sizeof(ScreenUpdateContext::ShaderParam),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE, //UINT CPUAccessFlags
		0, //UINT MiscFlags;
		0, //UINT StructureByteStride;
	};
	ID3D11Buffer* buffer;
	result = d3d_device.CreateBuffer(&CBDesc, nullptr, &buffer);
	assert(SUCCEEDED(result));
	mConstantBuffer = detail::unique_iunknown_ptr<ID3D11Buffer>(buffer);

	CBDesc.ByteWidth = sizeof(Tex3DShaderParams);
	result = d3d_device.CreateBuffer(&CBDesc, nullptr, &buffer);
	assert(SUCCEEDED(result));
	mTex3dConstantBuffer = detail::unique_iunknown_ptr<ID3D11Buffer>(buffer);
}

ScreenBufferUpdater3D::~ScreenBufferUpdater3D() {}

void ScreenBufferUpdater3D::Update(const ScreenUpdateContext & inContext)
{
	ID3D11DeviceContext&	d3d_context = mDevice.GetD3D11DeviceContext();
	ID3D11Buffer*		constant_buffer = mConstantBuffer.get();
	ID3D11Buffer*		tex3d_constant_buffer = mTex3dConstantBuffer.get();

	ID3D11Texture3D* texture = mResourceTextures[0]->Get();
	D3D11_TEXTURE3D_DESC texture_desc;
	texture->GetDesc(&texture_desc);

	const int num_3d_slices = texture_desc.Depth / inContext.mNum4dSlices;
	assert(num_3d_slices > 0);

	// Set samplers and shaders
	SetRenderTechnique(inContext);

	// Update constant buffer
	const ScreenUpdateContext::ShaderParam& reserved_param = inContext.mShaderParam;
	detail::UpdateConstantBuffer(&d3d_context, constant_buffer, &reserved_param, sizeof(reserved_param));
	d3d_context.PSSetConstantBuffers(SLOT_RESERVED_PARAM, 1, &constant_buffer);
	
	for (size_t depth = 0; depth < texture_desc.Depth; ++depth)
	{
		// Set render target view
		size_t num_rtvs_to_draw = 0;
		ID3D11RenderTargetView* rtvs_buffer[sMaxNumResourceTextures] = { nullptr };
		for (size_t i = 0; i < sMaxNumResourceTextures; ++i)
		{
			if (!mResourceTextures[i])
				continue;
			const auto& rtvs = mResourceTextures[i]->GetRenderTargetViews();
			if (rtvs.size() == 0)
				continue;
			assert(rtvs.size() == texture_desc.Depth);
			rtvs_buffer[i] = rtvs[depth].get();
			num_rtvs_to_draw++;
		}
		d3d_context.OMSetRenderTargets(num_rtvs_to_draw, rtvs_buffer, NULL);

		// Set shader resource view
		SetShaderResources(inContext);

		// Update the constant buffer of tex3d
		Tex3DShaderParams tex3d_params;
		tex3d_params.mDepthSlice = depth;
		UINT uiW = depth % num_3d_slices;
		UINT uiQ = depth / num_3d_slices;
		tex3d_params.wq[0] = ((float)uiW + 0.5f) / (float)num_3d_slices;
		tex3d_params.wq[1] = ((float)uiQ + 0.5f) / (float)inContext.mNum4dSlices;
		assert(0 < tex3d_params.wq[0] && tex3d_params.wq[0] < 1);
		assert(0 < tex3d_params.wq[1] && tex3d_params.wq[1] < 1);
		detail::UpdateConstantBuffer(&d3d_context, tex3d_constant_buffer, &tex3d_params, sizeof(tex3d_params));
		d3d_context.PSSetConstantBuffers(SLOT_TEX3D_PARAM, 1, &tex3d_constant_buffer);

		DrawFullscreenTriangle(texture_desc.Width, texture_desc.Height);
	}

	// Unbind resources
	UnbindResources();
}
