#include "Render.hpp"

#ifdef RENDER_DIRECTX11
#include <d3d11.h>
#include <d3dcompiler.h>
#include <locale>
#include <codecvt>
#include <typeinfo>
#include <cassert>
#include <sstream>
#include "DirectXUtil.hpp"

namespace detail
{
	void IUnknownDeleter::operator()(IUnknown* r)
	{
		r->Release();
	} 

	static std::hash<std::string>	sStringHash;

	HRESULT CompileShaderFromFile(
		const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
		ID3DBlob** ppOutCodeBlob, ID3DBlob** ppErrorMsgBlob)
	{
		HRESULT hr = S_OK;

		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

		hr = D3DCompileFromFile(szFileName, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
			dwShaderFlags, 0, ppOutCodeBlob, ppErrorMsgBlob);

		return hr;
	}

	ID3D11PixelShader* CompilePixelShader(
		ID3D11Device* inDevice, const WCHAR* inFileName, LPCSTR inEntryPoint, std::string& outErrorMsg)
	{
		HRESULT result = E_FAIL;
		ID3DBlob* blob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		result = CompileShaderFromFile(inFileName, inEntryPoint, "ps_4_0", &blob, &pErrorBlob);
		
		if (FAILED(result))
		{
			if (pErrorBlob != nullptr)
			{
				outErrorMsg = static_cast<const char*>(pErrorBlob->GetBufferPointer());
			}
			else
			{
				std::stringstream ss;
				ss << "undefined error :" << result;
				outErrorMsg = ss.str();
			}
			if (pErrorBlob)
				pErrorBlob->Release();
			return nullptr;
		}
		if (pErrorBlob) pErrorBlob->Release();

		if (FAILED(result))
		{
			MessageBox(NULL,
				"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
			return nullptr;
		}

		// Create a shader from ID3DBlob
		ID3D11PixelShader* pixel_shader = nullptr;
		result = inDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &pixel_shader);
		blob->Release();
		if (FAILED(result))
		{
			return nullptr;
		}
		return pixel_shader;
	}

	ID3D11VertexShader* CompileVertexShader(
		ID3D11Device* inDevice, const WCHAR* inFileName, LPCSTR inEntryPoint, std::string& outErrorMsg)
	{
		HRESULT result = E_FAIL;
		ID3DBlob* blob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		result = CompileShaderFromFile(inFileName, inEntryPoint, "vs_4_0", &blob, &pErrorBlob);

		if (FAILED(result))
		{
			if (pErrorBlob != NULL)
			{
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
				outErrorMsg = static_cast<const char*>(pErrorBlob->GetBufferPointer());
			}
			if (pErrorBlob)
				pErrorBlob->Release();
			return nullptr;
		}
		if (pErrorBlob) pErrorBlob->Release();

		if (FAILED(result))
		{
			MessageBox(NULL,
				"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
			return nullptr;
		}

		// Create a shader from ID3DBlob
		ID3D11VertexShader* vertex_shader = nullptr;
		result = inDevice->CreateVertexShader(
			blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &vertex_shader
			);

		blob->Release();
		if (FAILED(result))
		{
			return nullptr;
		}
		return vertex_shader;
	}
}

//==========================================================================

PixelShader::PixelShader(const std::string inFilePath) : mFilePath(inFilePath) {}
PixelShader::~PixelShader() {}

bool PixelShader::Compile(Device & inDevice, std::string & outMessage)
{
	ID3D11Device& d3d_device = inDevice.GetD3D11Device();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
	std::wstring wfilepath = cv.from_bytes(mFilePath);

	ID3D11PixelShader* ps = nullptr;
	ps = detail::CompilePixelShader(&d3d_device, wfilepath.c_str(), "Entry", outMessage);
	if(ps)
		mUniquePtr = detail::unique_iunknown_ptr<ID3D11PixelShader>(ps);

	return ps != nullptr;
}


//==========================================================================

VertexShader::VertexShader(const std::string inFilePath) : mFilePath(inFilePath) {}
VertexShader::~VertexShader() {}

bool VertexShader::Compile(Device & inDevice, std::string & outMessage)
{
	ID3D11Device& d3d_device = inDevice.GetD3D11Device();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
	std::wstring wfilepath = cv.from_bytes(mFilePath);

	ID3D11VertexShader* ps = nullptr;
	ps = detail::CompileVertexShader(&d3d_device, wfilepath.c_str(), "Entry", outMessage);
	if (ps)
		mUniquePtr = detail::unique_iunknown_ptr<ID3D11VertexShader>(ps);

	return ps != nullptr;
}

//==========================================================================

BlendState::BlendState(Device& inDevice, const BlendStateDesc& inDesc)
{
	ID3D11Device& d3d_device = inDevice.GetD3D11Device();

	D3D11_BLEND_DESC blend_desc;
	blend_desc.IndependentBlendEnable = FALSE;
	blend_desc.AlphaToCoverageEnable = FALSE;
	switch (inDesc.mBlendType)
	{
	case BlendType::NONE:
		for (auto& t : blend_desc.RenderTarget)
			t.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blend_desc.RenderTarget[0].BlendEnable = FALSE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		break;
	case BlendType::ADDITIVE:
		for(auto& t : blend_desc.RenderTarget)
			t.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blend_desc.RenderTarget[0].BlendEnable = TRUE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		break;
	case BlendType::ALPHA:
		for (auto& t : blend_desc.RenderTarget)
			t.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blend_desc.RenderTarget[0].BlendEnable = TRUE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		break;
	default:
		assert(false);
	}

	ID3D11BlendState* bs = nullptr;
	HRESULT result = d3d_device.CreateBlendState(&blend_desc, &bs);
	assert(SUCCEEDED(result));

	mUniquePtr = detail::unique_iunknown_ptr<ID3D11BlendState>(bs);
}

BlendState::~BlendState() {}

//==========================================================================

Sampler::Sampler(Device& inDevice, const SamplerDesc& inDesc)
{
	ID3D11Device&	d3d_device = inDevice.GetD3D11Device();

	D3D11_TEXTURE_ADDRESS_MODE address_mode = D3D11_TEXTURE_ADDRESS_CLAMP;
	switch (inDesc.mBoundaryCondition)
	{
		case SamplerBoundaryCondition::CLAMP:
			address_mode = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case SamplerBoundaryCondition::WRAP:
			address_mode = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		default:
			assert(false);
	}

	ID3D11SamplerState* sampler = nullptr;
	D3D11_SAMPLER_DESC desc =
	{
		D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		address_mode,
		address_mode,
		address_mode,
		0, //FLOAT MipLODBias;
		0, //UINT MaxAnisotropy;
		D3D11_COMPARISON_NEVER, // D3D11_COMPARISON_FUNC ComparisonFunc;
		{ 0.f, 0.f, 0.f, 0.f }, //FLOAT BorderColor[ 4 ];
		-FLT_MAX, //FLOAT MinLOD;
		+FLT_MAX //FLOAT MaxLOD;
	};
	HRESULT result = d3d_device.CreateSamplerState(&desc, &sampler);
	assert(SUCCEEDED(result));

	mUniquePtr = detail::unique_iunknown_ptr<ID3D11SamplerState>(sampler);
}

Sampler::~Sampler(){}

//==========================================================================

Texture2D::Texture2D(){}

Texture2D::Texture2D(Device& inDevice, const TextureDesc& inDesc)
{
	assert(inDesc.mDepth == 1); // depth > 1 is not supported now

	mDesc = inDesc;
	ID3D11Device& d3d_device = inDevice.GetD3D11Device();

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	// Setup the render target texture description.
	desc.Width = inDesc.mWidth;
	desc.Height = inDesc.mHeight;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = GetDXGIFormat(inDesc.mFormat);
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	// Create the render target texture.
	ID3D11Texture2D* texture = nullptr;
	HRESULT result = d3d_device.CreateTexture2D(&desc, NULL, &texture);
	assert(SUCCEEDED(result));
	mUniquePtr = detail::unique_iunknown_ptr<ID3D11Texture2D>(texture);

	// Create render target view.
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = desc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	ID3D11RenderTargetView* rtv;
	result = d3d_device.CreateRenderTargetView(texture, &renderTargetViewDesc, &rtv);
	assert(SUCCEEDED(result));
	mRenderTargetViews.push_back(detail::unique_iunknown_ptr<ID3D11RenderTargetView>(rtv));

	// Create the shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* srv = nullptr;
	result = d3d_device.CreateShaderResourceView(texture, &shaderResourceViewDesc, &srv);
	assert(SUCCEEDED(result));
	mShaderResourceView = detail::unique_iunknown_ptr<ID3D11ShaderResourceView>(srv);
}

Texture2D::~Texture2D(){}

//==========================================================================

Texture3D::Texture3D() {}

Texture3D::Texture3D(Device& inDevice, const TextureDesc& inDesc)
{
	mDesc = inDesc;

	ID3D11Device& d3d_device = inDevice.GetD3D11Device();
	ID3D11DeviceContext& d3d_context = inDevice.GetD3D11DeviceContext();

	// Setup the render target texture description.
	D3D11_TEXTURE3D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = inDesc.mWidth;
	desc.Height = inDesc.mHeight;
	desc.Depth = inDesc.mDepth;
	desc.MipLevels = 1;
	desc.Format = GetDXGIFormat(inDesc.mFormat);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	// Create the render target texture.
	ID3D11Texture3D* texture = nullptr;
	HRESULT result = d3d_device.CreateTexture3D(&desc, NULL, &texture);
	assert(SUCCEEDED(result));
	mUniquePtr = detail::unique_iunknown_ptr<ID3D11Texture3D>(texture);

	// Create render target views
	for (size_t depth = 0; depth < desc.Depth; ++depth)
	{
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format = desc.Format;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		renderTargetViewDesc.Texture3D.MipSlice = 0;
		renderTargetViewDesc.Texture3D.FirstWSlice = depth;
		renderTargetViewDesc.Texture3D.WSize = 1;

		// Create and clear render target views.
		ID3D11RenderTargetView* rtv;
		result = d3d_device.CreateRenderTargetView(texture, &renderTargetViewDesc, &rtv);
		assert(SUCCEEDED(result));
		const float clear_color[4] = { 0.0f };
		d3d_context.ClearRenderTargetView(rtv, clear_color);
		mRenderTargetViews.push_back(detail::unique_iunknown_ptr<ID3D11RenderTargetView>(rtv));
	}

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* srv = nullptr;
	result = d3d_device.CreateShaderResourceView(texture, &shaderResourceViewDesc, &srv);
	assert(SUCCEEDED(result));
	mShaderResourceView = detail::unique_iunknown_ptr<ID3D11ShaderResourceView>(srv);
}

Texture3D::~Texture3D() {}

//==========================================================================

BackBuffer::BackBuffer(Device& inDevice) : Texture2D()
{
	ID3D11Device& d3d_device = inDevice.GetD3D11Device();

	IDXGISwapChain&	swapchain = inDevice.GetSwapChain();
	ID3D11Texture2D* texture = nullptr;
	HRESULT result = swapchain.GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&texture);
	assert(SUCCEEDED(result));
	mUniquePtr = detail::unique_iunknown_ptr<ID3D11Texture2D>(texture);

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);

	// Create render target view.
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = desc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	ID3D11RenderTargetView* rtv;
	result = d3d_device.CreateRenderTargetView(texture, &renderTargetViewDesc, &rtv);
	assert(SUCCEEDED(result));
	mRenderTargetViews.push_back(detail::unique_iunknown_ptr<ID3D11RenderTargetView>(rtv));

	mShaderResourceView = nullptr;
}

BackBuffer::~BackBuffer(){}

//==========================================================================

Device::Device(void** inWindowHandle)
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(HWND(inWindowHandle[0]), &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;	// 60FPS
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = HWND(inWindowHandle[0]);
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swapchain;
	D3D_DRIVER_TYPE         driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       featureLevel = D3D_FEATURE_LEVEL_11_0;
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		driverType = driverTypes[driverTypeIndex];

		hr = D3D11CreateDeviceAndSwapChain(NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &swapchain, &device, &featureLevel, &context);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		assert(false);
	mD3dDevice = detail::unique_iunknown_ptr<ID3D11Device>(device);
	mImmediateContext = detail::unique_iunknown_ptr<ID3D11DeviceContext>(context);
	mSwapChain = detail::unique_iunknown_ptr<IDXGISwapChain>(swapchain);
}

Device::~Device()
{
	if (mImmediateContext) mImmediateContext->ClearState();
}

void Device::Present(bool vSyncEnabled)
{
	IDXGISwapChain& swapchain = GetSwapChain();
	unsigned int sync_interval = vSyncEnabled ? 2 : 0;
	swapchain.Present(sync_interval, 0);
}

//========================================================

#endif // RENDER_DIRECTX11