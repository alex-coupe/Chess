#include "Window.h"
#include <exception>

Window::RegisterWindow Window::RegisterWindow::regWin;

Window::RegisterWindow::RegisterWindow()noexcept
	:
	hInst(GetModuleHandle(nullptr))//Using null as a parameter returns a handle to the calling .exe
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = SetupProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = nullptr;

	RegisterClassEx(&wc);
	//Filling the class structure and registering it
}

//Static functions to provide info later on to the outer Window Class
const char * Window::RegisterWindow::GetName()noexcept
{
	return wClassName;
}

HINSTANCE Window::RegisterWindow::GetInstance()noexcept
{
	return regWin.hInst;
}

Window::RegisterWindow::~RegisterWindow()
{
	UnregisterClass(wClassName, GetInstance());
}

//We can now use a more C++ based constructor that allows the user to specify some parameters for the window
Window::Window(int width, int height, const char* title)
{
	RECT wr;
	wr.left = 200;
	wr.top = 200;
	wr.right = wr.left+width;
	wr.bottom = wr.top+height;
	//Readjusts the window to the size we require independently from the non client region
	AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, false);
	
	//Note the "this" as the last param. This is an lp Param and we are passing our window class to it.
	//This is utilised within the Windows procedure and is important to making WinAPI accept our member function wnd proc
	HWND hWnd = CreateWindow(
		RegisterWindow::GetName(), title, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		wr.left, wr.top, wr.right, wr.bottom, nullptr, nullptr, Window::RegisterWindow::GetInstance(), this
	);
	//Handle to the window is created - some of our static functions from the registration class are utilised
	ShowWindow(hWnd, SW_SHOW);
}
//Destructor follows RAII but cleaning up resources created by the constructor
Window::~Window()
{
	DestroyWindow(hWnd);
}
//Standard Windows message pump
bool Window::ProcessMessage()
{
	MSG msg;
	BOOL gResult;
	while (gResult = GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (gResult == -1)//This means an error with hWnd
	{
		return gResult;
	}
	else
	{
		//This means we received code 0 i.e. WM_CLOSE and the wParam contains the exit code
		return msg.wParam; 
	}
}

/*This is our non-static member function message handler - WinAPI does not recognise C++ style member functions
so we cannot directly plug this into the register window class as our Windows Procedure
*/
LRESULT Window::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
		POINTS pt = MAKEPOINTS(lParam);
		mse.UpdatePos(pt.x, pt.y);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//This is one of our 2 static procedures and this one is assigned when registering the window
LRESULT CALLBACK Window::SetupProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//WM_NCCREATE is called prior to WM_CREATE
	if (uMsg == WM_NCCREATE)
	{
		//Pointer to a struct containing creation data, pulled out from our "this" param in the create window function
		const CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		//User data now stores the pointer to the window
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		//Win Proc changed to our non set-up version - the other static one
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::RedirectProc));
		//Pass the messages on to our member function handler
		return pWnd->HandleMessage(hWnd,uMsg,wParam,lParam);
	}
	//For any other messages coming prior to WM_NCCREATE, handle them the default way
	return DefWindowProc(hWnd,uMsg,wParam, lParam);
}

LRESULT CALLBACK Window::RedirectProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//Get our window pointer back
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	//Pass the messages on to our member function handler
	return pWnd->HandleMessage(hWnd, uMsg, wParam, lParam);
}
