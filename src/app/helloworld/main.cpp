//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
#define USE_IMGUI 0

#include <iostream>
#include <memory>
#include <chrono>

#include <src/lib/math/Math.hpp>
#include <src/lib/window/Window.hpp>
#include <src/lib/window/Log.hpp>
#include <src/lib/gpu/Render.hpp>
#include <src/lib/gpu/FullScreenTriangle.hpp>
#if USE_IMGUI
#include <src/lib/etc/EventHandler.hpp>
#include <src/thirdparty/imgui/imgui/imgui.h>
#include <src/thirdparty/imgui/imgui/examples/directx11_example/imgui_impl_dx11.h>
#include "InputEvent.h"
#endif
#include <shader/atmosphere/AtmosphereConstants.h>

int main()
{
	int exit_code;

	std::unique_ptr<wnd::WindowBase> window( wnd::WindowBase::Create() );
	if (!window->Initialize("Hello, world"))
	{
		exit_code = -1;
		return exit_code;
	}

	void* handle = window->GetWindowHandle();
	Device	device(&handle);
#if USE_IMGUI
	ImGui_ImplDX11_Init(handle, &device.GetD3D11Device(), &device.GetD3D11DeviceContext());

	InputEventHandler input_event_handler(*window);
	InputEventForImGui imgui_input_event(input_event_handler, ImGui::GetIO());
#endif

	std::string error_message;
	PixelShader		default_ps("shader/helloworld/ScreenPS.hlsl");
	PixelShader		offscr2d_ps("shader/helloworld/OffScreen2dPS.hlsl");
	PixelShader		offscr3d_ps("shader/helloworld/OffScreen3dPS.hlsl");
	VertexShader	default_vs("shader/ScreenVS.hlsl");
	if (!default_ps.Compile(device, error_message))
		std::cout << error_message << std::endl;
	if (!offscr2d_ps.Compile(device, error_message))
		std::cout << error_message << std::endl;
	if (!offscr3d_ps.Compile(device, error_message))
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

	// off screen 2d
	std::unique_ptr<ScreenBufferUpdater2D> offscreen2d;
	TextureDesc	offscr2d_texture_desc;
	offscr2d_texture_desc.mHeight = 128;
	offscr2d_texture_desc.mWidth = 128;
	Texture2D offscr2d_texture(device, offscr2d_texture_desc);
	ScreenUpdateContext offscr2d_update_context;
	{
		ScreenCreationContext<Texture2D> creation_context(device);
		creation_context.mResourceTextures[0] = &offscr2d_texture;
		offscreen2d = std::make_unique<ScreenBufferUpdater2D>(creation_context);

		offscr2d_update_context.mPixelShader = &offscr2d_ps;
		offscr2d_update_context.mVertexShader = &default_vs;
		offscr2d_update_context.mTextureSamplers[0] = &sampler_clamp;
		offscr2d_update_context.mTextureSamplers[1] = &sampler_wrap;
	}

	// off screen 3d
	std::unique_ptr<ScreenBufferUpdater3D> offscreen3d;
	TextureDesc	offscr3d_texture_desc;
	offscr3d_texture_desc.mHeight = 32;
	offscr3d_texture_desc.mWidth = 32;
	offscr3d_texture_desc.mDepth = 32;
	Texture3D	offscr3d_texture0(device, offscr3d_texture_desc);
	Texture3D	offscr3d_texture1(device, offscr3d_texture_desc);
	ScreenUpdateContext offscr3d_update_context;
	{
		ScreenCreationContext<Texture3D> creation_context(device);
		creation_context.mResourceTextures[0] = &offscr3d_texture0;
		creation_context.mResourceTextures[1] = &offscr3d_texture1;
		offscreen3d = std::make_unique<ScreenBufferUpdater3D>(creation_context);

		offscr3d_update_context.mPixelShader = &offscr3d_ps;
		offscr3d_update_context.mVertexShader = &default_vs;
		offscr3d_update_context.mTextureSamplers[0] = &sampler_clamp;
		offscr3d_update_context.mTextureSamplers[1] = &sampler_wrap;
	}

	// primary screen
	std::unique_ptr<ScreenUpdater> screen;
	ScreenUpdateContext update_context;
	{
		ScreenCreationContext<Texture2D> creation_context(device);
		creation_context.mResourceTextures[0] = &back_buffer;
		screen = std::make_unique<ScreenUpdater>(creation_context);

		update_context.mPixelShader = &default_ps;
		update_context.mVertexShader = &default_vs;
		update_context.mInputTexture2Ds[0] = &offscr2d_texture;
		update_context.mInputTexture3Ds[0] = &offscr3d_texture0;
		update_context.mInputTexture3Ds[1] = &offscr3d_texture1;
		update_context.mTextureSamplers[0] = &sampler_clamp;
		update_context.mTextureSamplers[0] = &sampler_wrap;
	}

	const wnd::Info& window_info = window->GetInfo();
	const float window_size[2] = { float(window_info.mWindowSize[0]), float(window_info.mWindowSize[1]) };
	std::cout << "window size : " << window_size[0] << ", " << window_size[1] << std::endl;

	auto time_start	= std::chrono::system_clock::now();
	bool ui_precomputation_requested = false;
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

		switch (result.mState)
		{
		case wnd::WindowBase::UpdateResult::State::Idle:
			if (ui_precomputation_requested)
			{
				if (!default_ps.Compile(device, error_message))
					std::cout << error_message << std::endl;
				if (!offscr2d_ps.Compile(device, error_message))
					std::cout << error_message << std::endl;
				if (!offscr3d_ps.Compile(device, error_message))
					std::cout << error_message << std::endl;
				if (!default_vs.Compile(device, error_message))
					std::cout << error_message << std::endl;
				ui_precomputation_requested = false;
			}
#if USE_IMGUI
			ImGui_ImplDX11_NewFrame();
			if (ImGui::Begin("ToyModel"))
			{
				if (ImGui::Button("Compile"))
					ui_precomputation_requested = true;
				ImGui::Text("%.2f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::Text("Time = %.2f [s]", elapsed_time_sec);
				ImGui::End();
			}
#endif

			update_context.mShaderParam.mFloat4[0].x = elapsed_time_sec;
			update_context.mShaderParam.mFloat4[0].y = window_size[0];
			update_context.mShaderParam.mFloat4[0].z = window_size[1];
			offscr2d_update_context.mShaderParam.mFloat4[0].x = elapsed_time_sec;
			offscr2d_update_context.mShaderParam.mFloat4[0].y = window_size[0];
			offscr2d_update_context.mShaderParam.mFloat4[0].z = window_size[1];
			offscr3d_update_context.mShaderParam.mFloat4[0].x = elapsed_time_sec;
			offscr3d_update_context.mShaderParam.mFloat4[0].y = window_size[0];
			offscr3d_update_context.mShaderParam.mFloat4[0].z = window_size[1];

			offscreen3d->Update(offscr3d_update_context);
			offscreen2d->Update(offscr2d_update_context);
			screen->Update(update_context);
#if USE_IMGUI
			ImGui::Render();
#endif
			device.Present(true);
			break;
		case wnd::WindowBase::UpdateResult::State::Exit:
			break;
		case wnd::WindowBase::UpdateResult::State::Busy:
			break;
		default:
			break;
		}
	}
#if USE_IMGUI
	ImGui_ImplDX11_Shutdown();
#endif
	return exit_code;
}
