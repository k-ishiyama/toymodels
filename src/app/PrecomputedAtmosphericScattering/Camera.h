#pragma once
#pragma once
#include <iostream>
#include <sstream>
#include <bitset>
#include <src/lib/window/Window.hpp>
#include <src/lib/etc/EventHandler.hpp>
#include <src/lib/math/Math.hpp>
#include <src/lib/math/SO3.hpp>
#include "InputEvent.h"

struct Camera
{
	Camera(float x, float y, float z)
	{
		mCameraOrientation.x = 0.0f;
		mCameraOrientation.y = 0.0f;
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
		UpdateCameraRotation();
	}

	virtual ~Camera() {}

	const math::Float3x3&	GetRotationMatrix() const { return mRotationMatrix; }
	const math::Float3&		GetPosition() const { return mPosition; }
	void Rotate(float dPhi, float dTheta) { mCameraOrientation.x += dPhi; mCameraOrientation.y += dTheta; }

	enum struct Direction { RIGHT = 0, UP, FORWARD };
	void Move(float dx, Direction dir)
	{
		int idx = static_cast<int>(dir);
		math::Float3 forward(mRotationMatrix[0][idx], mRotationMatrix[1][idx], mRotationMatrix[2][idx]);
		mPosition = mPosition + forward * dx;
	}

protected:
	math::Float3x3	mRotationMatrix;
	math::Float3	mPosition;
	math::Float2	mCameraOrientation;

	void UpdateCameraRotation()
	{
		float& phi = mCameraOrientation.x;
		float& theta = mCameraOrientation.y;
		phi = std::fmodf(phi, 2.0f * math::PI<float>);
		theta = std::fmodf(theta, math::PI<float>);
		mRotationMatrix = math::RotationXYZ(theta, phi, 0.0f);
	}
};



struct TouchControlledCamera : Camera
{
	TouchControlledCamera(float x, float y, float z) : Camera(x, y, z) {}
	virtual ~TouchControlledCamera() {}

	void Touch(float x, float y) { mTouchPos[0] = x; mTouchPos[1] = y; }
	void Right(float dx) { mDirectionalPad[0] = dx; }
	void Forward(float dy) { mDirectionalPad[1] = dy; }

	void Update(const float dt)
	{
		const float dcx = mTouchPos[0] - mPrevTouchPos[0];
		const float dcy = mTouchPos[1] - mPrevTouchPos[1];

		if (mToggleBitSet[static_cast<std::uint8_t>(Toggle::ROTATE)])
			Rotate(mRotationSpeed * dcx * dt, mRotationSpeed * dcy * dt);

		if (mToggleBitSet[static_cast<std::uint8_t>(Toggle::UP_RIGHT)])
		{
			Move(dcy * mTranslationSpeed * dt, Direction::UP);
			Move(dcx * mTranslationSpeed * dt, Direction::RIGHT);
		}

		if (mToggleBitSet[static_cast<std::uint8_t>(Toggle::FORWARD)])
			Move(dcy * mTranslationSpeed * dt, Direction::FORWARD);

		Move(mDirectionalPad[0] * mTranslationSpeed * dt, Direction::RIGHT);
		Move(mDirectionalPad[1] * mTranslationSpeed * dt, Direction::FORWARD);

		// store the cursor position
		mPrevTouchPos[0] = mTouchPos[0];
		mPrevTouchPos[1] = mTouchPos[1];

		UpdateCameraRotation();
	}

	enum struct Toggle : std::uint8_t
	{
		NONE = 0,
		ROTATE = 1,
		UP_RIGHT = 2,
		FORWARD = 3,
	};

	void ClearToggle() { mToggleBitSet &= 0; }

	void SwitchToggle(Toggle inToggle, bool isEnable)
	{
		std::uint8_t i = static_cast<std::uint8_t>(inToggle);
		if (isEnable)
			mToggleBitSet.set(i);
		else
			mToggleBitSet.reset(i);
	}

protected:
	float			mDirectionalPad[2] = { 0.0f };
	float			mTouchPos[2] = { 0.0f };
	float			mPrevTouchPos[2] = { 0.0f };
	float			mRotationSpeed = 1.5e2f;
	float			mTranslationSpeed = 5e2f;
	std::bitset<8>	mToggleBitSet;
};

//==========================================================================

struct InputEventForCamera
{
	InputEventForCamera(InputEventHandler& inEventHandler, wnd::WindowBase& inWindow, TouchControlledCamera& inCamera, const ImGuiIO& inImGuiIO)
		: mWindow(inWindow), mCamera(inCamera), mImGuiIO(inImGuiIO)
	{
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_BUTTONDOWN, std::bind(&InputEventForCamera::OnMouseDown, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_BUTTONUP, std::bind(&InputEventForCamera::OnMouseUp, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_MOVE, std::bind(&InputEventForCamera::OnMouseMove, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_DOWN, std::bind(&InputEventForCamera::OnKeyDown, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_UP, std::bind(&InputEventForCamera::OnKeyUp, this, std::placeholders::_1));
	}

	void OnMouseDown(const wnd::InputArgument& arg)
	{
		// if (!mIsAltKeyDown) return;
		if (mImGuiIO.WantCaptureMouse) return;
		switch (arg.index)
		{
		case 0: // LEFT
			mCamera.SwitchToggle(TouchControlledCamera::Toggle::ROTATE, true);
			break;
		case 1: // RIGHT
			mCamera.SwitchToggle(TouchControlledCamera::Toggle::FORWARD, true);
			break;
		case 2: // MIDDLE
			mCamera.SwitchToggle(TouchControlledCamera::Toggle::UP_RIGHT, true);
			break;
		default:
			break;
		}
	}

	void OnMouseUp(const wnd::InputArgument& arg)
	{
		// if (!mIsAltKeyDown) return;
		switch (arg.index)
		{
		case 0: // LEFT
			mCamera.SwitchToggle(TouchControlledCamera::Toggle::ROTATE, false);
			break;
		case 1: // RIGHT
			mCamera.SwitchToggle(TouchControlledCamera::Toggle::FORWARD, false);
			break;
		case 2: // MIDDLE
			mCamera.SwitchToggle(TouchControlledCamera::Toggle::UP_RIGHT, false);
			break;
		default:
			break;
		}
	}

	void OnMouseMove(const wnd::InputArgument& arg)
	{
		auto mouse_pos_x = arg.x;
		auto mouse_pos_y = arg.y;
		float nx = static_cast<float>(mouse_pos_x) / mWindow.GetInfo().mWindowSize[0];
		float ny = static_cast<float>(mouse_pos_y) / mWindow.GetInfo().mWindowSize[1];
		nx = nx < 0.0f ? 0.0f : (nx > 1.0f ? 1.0f : nx);
		ny = ny < 0.0f ? 0.0f : (ny > 1.0f ? 1.0f : ny);
		mCamera.Touch(nx, ny);
	}

	void OnKeyDown(const wnd::InputArgument& arg)
	{
		switch (arg.index)
		{
		case 0x12:	// Alt
			mIsAltKeyDown = true;
			break;
		case 0x41:	// A
		case 0x25:	// Left arrow
			mCamera.Right(-0.2f);
			break;
		case 0x44:	// D
		case 0x27:	// Right arrow
			mCamera.Right(0.2f);
			break;
		case 0x53:	// S
		case 0x28:	// Down arrow
			mCamera.Forward(-0.2f);
			break;
		case 0x57:	// W
		case 0x26:	// Up arrow
			mCamera.Forward(0.2f);
			break;
		default:
			break;
		}
	}

	void OnKeyUp(const wnd::InputArgument& arg)
	{
		switch (arg.index)
		{
		case 0x12:	// Alt
			mIsAltKeyDown = false;
			break;
		case 0x41:	// A
		case 0x44:	// D
		case 0x25:	// Left arrow
		case 0x27:	// Right arrow
			mCamera.Right(0.0f);
			break;
		case 0x53:	// S
		case 0x57:	// W
		case 0x28:	// Down arrow
		case 0x26:	// Up arrow
			mCamera.Forward(0.0f);
			break;
		default:
			break;
		}
	}

private:
	TouchControlledCamera & mCamera;
	wnd::WindowBase&		mWindow;
	const ImGuiIO&			mImGuiIO;
	bool					mIsAltKeyDown = false;
};


struct InputEventForShader
{
	InputEventForShader(InputEventHandler& inEventHandler, wnd::WindowBase& inWindow) : mWindow(inWindow)
	{
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_BUTTONDOWN, std::bind(&InputEventForShader::OnMouseDown, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_BUTTONUP, std::bind(&InputEventForShader::OnMouseUp, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::MOUSE_MOVE, std::bind(&InputEventForShader::OnMouseMove, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_DOWN, std::bind(&InputEventForShader::OnKeyDown, this, std::placeholders::_1));
		inEventHandler.RegisterInputEventObserver(wnd::InputEvent::KEY_UP, std::bind(&InputEventForShader::OnKeyUp, this, std::placeholders::_1));
	}

	void OnMouseDown(const wnd::InputArgument& arg)
	{
		switch (arg.index)
		{
		case 0: // LEFT
			mIsMouseButtonDown = true;
			break;
		case 1: // RIGHT
			break;
		case 2: // MIDDLE
			break;
		default:
			break;
		}
	}

	void OnMouseUp(const wnd::InputArgument& arg)
	{
		switch (arg.index)
		{
		case 0: // LEFT
			mIsMouseButtonDown = false;
			break;
		case 1: // RIGHT
			break;
		case 2: // MIDDLE
			break;
		default:
			break;
		}
	}

	void OnMouseMove(const wnd::InputArgument& arg)
	{
		if (!mIsMouseButtonDown) return;
		mMouseX = arg.x;
		mMouseY = arg.y;
	}

	void OnKeyDown(const wnd::InputArgument&)
	{
	}

	void OnKeyUp(const wnd::InputArgument&)
	{
	}

	auto GetMouseX() const { return mMouseX; }
	auto GetMouseY() const { return mMouseY; }
private:
	wnd::WindowBase&		mWindow;
	std::uint16_t			mMouseX = 0;
	std::uint16_t			mMouseY = 0;
	bool					mIsMouseButtonDown = false;
};
