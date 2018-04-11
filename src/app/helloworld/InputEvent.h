#pragma once
#include <src/thirdparty/imgui/imgui/imgui.h>
#include <src/lib/window/Window.hpp>
#include <src/lib/etc/EventHandler.hpp>

struct InputEventHandler
{
	InputEventHandler(wnd::WindowBase& inWindow)
	{
		for (int i = 0; i < static_cast<int>(wnd::InputEvent::COUNT); ++i)
		{
			auto ev = static_cast<wnd::InputEvent>(i);
			inWindow.RegisterInputEventCallbackFunc(ev, std::bind(&InputEventHandler::OnInputEvent, this, ev, std::placeholders::_1));
		}
	}

	template <typename Observer>
	void RegisterInputEventObserver(wnd::InputEvent&& event, Observer&& observer)
	{
		mInputEventHandler.Register(std::move(event), std::forward<Observer>(observer));
	}

	void OnInputEvent(wnd::InputEvent inEvent, const wnd::InputArgument& inArg)
	{
		mInputEventHandler.Notify(inEvent, inArg);
	}

protected:
	etc::EventHandler<wnd::InputEvent, wnd::InputArgument> mInputEventHandler;
};

//=================================================================

struct InputEventForImGui
{
	InputEventForImGui(InputEventHandler& inEventHandler, ImGuiIO& inImGuiIO) : mImGuiIO(inImGuiIO)
	{
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_BUTTONDOWN, std::bind(&InputEventForImGui::OnMouseDown, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_BUTTONUP, std::bind(&InputEventForImGui::OnMouseUp, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_WHEEL, std::bind(&InputEventForImGui::OnMouseWheel, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_MOVE, std::bind(&InputEventForImGui::OnMouseMove, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_DOWN, std::bind(&InputEventForImGui::OnKeyDown, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_UP, std::bind(&InputEventForImGui::OnKeyUp, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_CHAR, std::bind(&InputEventForImGui::OnChar, this, std::placeholders::_1));
	}

	void OnMouseDown(const wnd::InputArgument& arg)
	{
		mImGuiIO.MouseDown[arg.index] = true;
	}

	void OnMouseUp(const wnd::InputArgument& arg)
	{
		mImGuiIO.MouseDown[arg.index] = false;
	}

	void OnMouseWheel(const wnd::InputArgument&)
	{
	}

	void OnMouseMove(const wnd::InputArgument& arg)
	{
		mImGuiIO.MousePos.x = arg.x;
		mImGuiIO.MousePos.y = arg.y;
	}

	void OnKeyDown(const wnd::InputArgument&)
	{
	}

	void OnKeyUp(const wnd::InputArgument&)
	{
	}

	void OnChar(const wnd::InputArgument&)
	{
	}

private:
	ImGuiIO&				mImGuiIO;
};
