#pragma once
#include <functional>

namespace wnd
{
	struct Info
	{
		int		mWindowSize[2]	= { 0 }; // width, height
	};

	enum struct InputEvent
	{
		MOUSE_MOVE = 0,
		MOUSE_WHEEL,
		MOUSE_BUTTONDOWN,
		MOUSE_BUTTONUP,
		KEY_DOWN,
		KEY_UP,
		KEY_CHAR,
		COUNT,
	};

	struct InputArgument
	{
		std::uint32_t	index;
		std::uint16_t	x;
		std::uint16_t	y;
	};

	using InputEventCallbackFunc = std::function<void(const InputArgument&)>;

	struct WindowBase
	{
	public:
		WindowBase() {}
		static WindowBase* Create();
		virtual ~WindowBase() {}

		struct UpdateResult
		{
			enum struct State { Idle, Busy, Exit };
			State	mState;
			int		mExitCode;
		};
		virtual bool Initialize(const char* inWindowName) = 0;
		virtual void Update(UpdateResult& outResult) = 0;
		virtual void* GetWindowHandle() = 0;
		const Info& GetInfo() { return mInfo; }

		void RegisterInputEventCallbackFunc(InputEvent inEvent, InputEventCallbackFunc inFunc)
		{
			mInputEventCallbackFunc[static_cast<int>(inEvent)] = inFunc;
		}

	protected:
		Info	mInfo;
		InputEventCallbackFunc	mInputEventCallbackFunc[static_cast<int>(InputEvent::COUNT)] = { nullptr };
	};


} // namespace sys

