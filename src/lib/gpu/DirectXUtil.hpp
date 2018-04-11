#pragma once
#include <d3d11.h>
#include "GPU.hpp"

DXGI_FORMAT GetDXGIFormat(TextureFormat inFormat)
{
	switch (inFormat)
	{
	case TextureFormat::R32:
		return DXGI_FORMAT_R32_FLOAT;
	case TextureFormat::R32G32:
		return DXGI_FORMAT_R32G32_FLOAT;
	case TextureFormat::R32G32B32:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case TextureFormat::R32G32B32A32:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	default:
		assert(false);
	};
	return DXGI_FORMAT_UNKNOWN;
}