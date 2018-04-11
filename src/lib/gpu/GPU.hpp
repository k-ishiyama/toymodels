#pragma once
#include "GPUExports.hpp"
#include <bitset>

enum struct TextureFormat
{
	R32,
	R32G32,
	R32G32B32,
	R32G32B32A32,
};

enum struct TextureFlags : std::uint8_t
{
	NONE = 0,
	CUBEMAP,
};
using TextureFlagSet = std::bitset<8>;

struct GPU_EXPORT TextureDesc
{
	size_t	mHeight = 256;
	size_t	mWidth = 256;
	size_t	mDepth = 1;
	TextureFormat	mFormat = TextureFormat::R32G32B32A32;
};
