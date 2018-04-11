#include "Window.hpp"
#include "Log.hpp"
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
#include <sstream>

#if _UNICODE
#define _T(x) L##x
#else
#define _T(x) x
#endif

namespace wnd
{
	struct Window_win32 : WindowBase
	{
	public:
		Window_win32() {};
		virtual ~Window_win32() {};
		virtual void Update(UpdateResult& outResult);
		// Register class and create window
		virtual bool Initialize(const char* inWindowName);
		virtual void* GetWindowHandle();

	protected:
		// Called every time the application receives a message
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			PAINTSTRUCT ps;
			HDC hdc;

			InputArgument input_arg;
			InputEventCallbackFunc callback = nullptr;
			switch (message)
			{
			case WM_SIZE:
				break;

			case WM_PAINT:
				hdc = BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
				break;

			case WM_DESTROY:
				PostQuitMessage(0);
				break;

			case WM_LBUTTONDOWN:
				input_arg.index = 0;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_BUTTONDOWN)];
				SetCapture(hWnd);
				break;

			case WM_LBUTTONUP:
				input_arg.index = 0;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_BUTTONUP)];
				ReleaseCapture();
				break;

			case WM_RBUTTONDOWN:
				input_arg.index = 1;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_BUTTONDOWN)];
				SetCapture(hWnd);
				break;

			case WM_RBUTTONUP:
				input_arg.index = 1;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_BUTTONUP)];
				ReleaseCapture();
				break;

			case WM_MBUTTONDOWN:
				input_arg.index = 2;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_BUTTONDOWN)];
				SetCapture(hWnd);
				break;

			case WM_MBUTTONUP:
				input_arg.index = 2;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_BUTTONUP)];
				ReleaseCapture();
				break;

			case WM_MOUSEMOVE:
				input_arg.x = static_cast<std::uint16_t>(GET_X_LPARAM(lParam));
				input_arg.y = static_cast<std::uint16_t>(GET_Y_LPARAM(lParam));
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_MOVE)];
				break;

			case WM_MOUSEWHEEL:
				input_arg.index = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : 0;
				callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::MOUSE_WHEEL)];
				return 0;


			case WM_KEYDOWN:
				if(wParam < 256)
				{
					input_arg.index = wParam;
					callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::KEY_DOWN)];
				}
				break;

			case WM_KEYUP:
				if (wParam < 256)
				{
					input_arg.index = wParam;
					callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::KEY_UP)];
				}
				break;

			case WM_SYSKEYDOWN:
				if (wParam < 256)
				{
					input_arg.index = wParam;
					callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::KEY_DOWN)];
				}
				SetCapture(hWnd);
				break;

			case WM_SYSKEYUP:
				if (wParam < 256)
				{
					input_arg.index = wParam;
					callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::KEY_UP)];
				}
				ReleaseCapture();
				break;

			case WM_CHAR:
				if (wParam > 0 && wParam < 0x10000)
				{
					input_arg.index = wParam;
					callback = mInputEventCallbackFunc[static_cast<int>(InputEvent::KEY_CHAR)];
				}
				break;

			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}

			if (callback)
				callback(input_arg);

			return 0;
		}

		static LRESULT CALLBACK sBaseWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			Window_win32* pTargetWnd = (Window_win32*)GetProp(hWnd, _T("Window_win32"));

			if (!pTargetWnd)
			{
				if ((uMsg == WM_CREATE) || (uMsg == WM_NCCREATE))
					pTargetWnd = (Window_win32*)((LPCREATESTRUCT)lParam)->lpCreateParams;
				else if (uMsg == WM_INITDIALOG)
					pTargetWnd = (Window_win32*)lParam;
			}
			else
			{
				LRESULT	lResult = pTargetWnd->WndProc(hWnd, uMsg, wParam, lParam);
				return lResult;

			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		HWND  mWindowHandle = NULL;
		HINSTANCE	mInstance = NULL;
	};

void Window_win32::Update(UpdateResult& outResult)
{
	outResult.mState	= UpdateResult::State::Idle;
	outResult.mExitCode = 0;
	
	MSG msg = { 0 };
	BOOL msg_result = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
	if (msg_result)
	{
		if (WM_QUIT == msg.message)
		{
			outResult.mState	= UpdateResult::State::Exit;
			outResult.mExitCode = (int)msg.wParam;
			return;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		outResult.mState = UpdateResult::State::Busy;
	}
}

bool Window_win32::Initialize(const char* inWindowName)
{
	HINSTANCE hInstance = GetModuleHandle(NULL); // handle of this app's instance
	// LPTSTR CommandLineParams = GetCommandLine(); // command line params

	STARTUPINFO StartupInfo;
	GetStartupInfo(&StartupInfo); // initial state of this window
	// int nCmdShow = StartupInfo.wShowWindow; // Notice(VS2017) : https://stackoverflow.com/questions/36953886/startupinfo-wshowwindow-is-0-when-running-from-visual-studio
	int nCmdShow = SW_SHOWNORMAL;

	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = this->sBaseWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "WindowClass";
	wcex.hIconSm = NULL;
	if (!RegisterClassEx(&wcex))
		return false;

	// Create window
	mInstance = hInstance;
	// RECT rc = { 0, 0, 480, 320 };
 	RECT rc = { 0, 0, 640, 480 };
	// RECT rc = { 0, 0, 1280, 960 };

	mInfo.mWindowSize[0] = rc.right - rc.left;
	mInfo.mWindowSize[1] = rc.bottom - rc.top;

	const bool is_resizable = false;
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	if (!is_resizable)
		dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	AdjustWindowRect(&rc, dwStyle, FALSE);

	mWindowHandle = CreateWindow("WindowClass", inWindowName,
		dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!mWindowHandle)
		return false;

	ShowWindow(mWindowHandle, nCmdShow);

	// Combine the window handle and WindowBase object
	SetProp(mWindowHandle, _T("Window_win32"), (HANDLE)this);

	return true;
}

void* Window_win32::GetWindowHandle()
{
	return mWindowHandle;
}

WindowBase* WindowBase::Create()
{
	return new Window_win32();
}

} // namespace wnd