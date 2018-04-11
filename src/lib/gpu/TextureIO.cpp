#include "TextureIO.hpp"
#include <d3d11.h>
#include <cassert>

Image::Image(const ImageMetaData& inDesc)
	: mDesc(inDesc)
{
	size_t mipLevels = inDesc.mipLevels;

	size_t dimension = 1;
	dimension += inDesc.height	> 1 ? 1 : 0;
	dimension += inDesc.depth	> 1 ? 1 : 0;

	assert(mipLevels == 1); // ToDo
	switch (dimension)
	{
	case 1:
		assert(false); // ToDo
		break;

	case 2:
		mWidth = inDesc.width;
		mHeight = inDesc.height;
		break;

	case 3:
		assert(false); // ToDo
		break;

	default:
		assert(false);
	}
}

//==========================================================

std::unique_ptr<Image> CaptureTexture(const Device& inDevice, const Texture2D& inTexture2D)
{
	HRESULT hr;

	ID3D11Texture2D* src_texture = inTexture2D.Get();
	assert(src_texture);

	D3D11_TEXTURE2D_DESC desc;
	src_texture->GetDesc(&desc);

	detail::unique_iunknown_ptr<ID3D11Texture2D> staging_texture = nullptr;
	if (desc.SampleDesc.Count > 1)
	{
		// detail::unique_iunknown_ptr<ID3D11Texture2D> staging_texture = nullptr;
		assert(false); // todo
	}
	else if ((desc.Usage == D3D11_USAGE_STAGING) && (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ))
	{
		// Handle case where the source is already a staging texture we can use directly
		assert(false); // todo
	}
	else
	{
		desc.BindFlags = 0;
		desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.Usage = D3D11_USAGE_STAGING;

		ID3D11Texture2D* raw_staging_texture = staging_texture.get();
		hr = inDevice.GetD3D11Device().CreateTexture2D(&desc, 0, &raw_staging_texture);
		assert(SUCCEEDED(hr));
		assert(staging_texture);

		inDevice.GetD3D11DeviceContext().CopyResource(raw_staging_texture, src_texture);
	}

	ImageMetaData meta_data;
	meta_data.width = desc.Width;
	meta_data.height = desc.Height;
	meta_data.depth = 1;
	meta_data.mipLevels = desc.MipLevels;
	meta_data.flagSet = static_cast<std::uint8_t>(TextureFlags::NONE);
	meta_data.format = inTexture2D.GetTextureDesc().mFormat;
	if (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
		meta_data.flagSet.set(static_cast<std::uint8_t>(TextureFlags::CUBEMAP));

	std::unique_ptr<Image> outImage = std::make_unique<Image>(meta_data);

	// (Under construction)

	return nullptr;
}