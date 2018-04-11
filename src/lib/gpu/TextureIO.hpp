#pragma once
//
//	References
//		https://github.com/Microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexD3D11.cpp
//
#include "Render.hpp"

// Forward declarations
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Resource;

struct ImageMetaData
{
	size_t			width;
	size_t			height;
	size_t			depth;
	size_t			mipLevels;
	// size_t	arraySize;
	TextureFormat	format;
	TextureFlagSet	flagSet;
};

struct Image
{
	Image(const ImageMetaData& inDesc);
private:
	const ImageMetaData&	mDesc;
	size_t					mWidth;
	size_t					mHeight;
	size_t					mRowPitch;
	size_t					mSlicePitch;
	uint8_t*				mPixels;
};

std::unique_ptr<Image> CaptureTexture(const Device& inDevice, const Texture2D& inTexture2D);