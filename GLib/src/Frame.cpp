#include "../include/GLib.h"

namespace GLib
{
	bool Frame::repaint = false;
	bool Frame::close = false;

	HWND Frame::hwnd;

	void Frame::init(std::string name, int width, int height)
	{
		place = D2D1::RectF(0, 0, width, height);

		if (!SUCCEEDED(createDeviceIndependentResources()))
		{
			MessageBox(NULL, TEXT("Error Create Device Independent Resources"), TEXT("ERROR"), MB_OK);
			return;
		}

		WNDCLASS wc;

		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hIcon = NULL;// LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
		wc.hInstance = (HINSTANCE)::GetModuleHandle(NULL);;
		wc.lpfnWndProc = Frame::wndProc;
		wc.lpszClassName = "WIN_CLASS";
		wc.lpszMenuName = 0;
		wc.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClass(&wc))
		{
			MessageBox(NULL, TEXT("Error registering class"), TEXT("ERROR"), MB_OK);
			return;
		}

		hwnd = CreateWindow("WIN_CLASS",
			"AI Cells 2",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			(HWND)NULL,
			(HMENU)NULL,
			GetModuleHandle(NULL),
			this);

		//delete window border
		LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
		lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		SetWindowLong(hwnd, GWL_STYLE, lStyle);

		LONG lExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		SetWindowLong(hwnd, GWL_EXSTYLE, lExStyle);

		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		if (!hwnd)
		{
			MessageBox(NULL, TEXT("Call to CreateWindow failed!"), TEXT("ERROR"), MB_OK);
			return;
		}

		if (!SUCCEEDED(createDeviceResources()))
		{
			MessageBox(NULL, TEXT("Error Create Device Resources"), TEXT("ERROR"), MB_OK);
			return;
		}

		repaint = 1;
		close = 0;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hwnd;
	}

	void Frame::runMessageLoop()
	{
		MSG msg;

		while (true)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) //TODO: Check PM_REMOVE and ">0"
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (GetForegroundWindow() != hwnd)
			{
				Sleep(10);
			}
			if (repaint == 1 || continues)
			{
				InvalidateRect(hwnd, NULL, FALSE);
				repaint = 0;
			}
			updateControl();
			if (close == 1)
			{
				break;
			}
		}
	}

	void Frame::askRepaint()
	{
		repaint = 1;
	}
	void Frame::showWindow(int mode)
	{
		ShowWindow(hwnd, mode);
	}

	void Frame::closeWindow()
	{
		close = true;
	}

	HWND Frame::getHWND()
	{
		return hwnd;
	}

	LRESULT Frame::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_CREATE)
		{
			LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
			Frame* frame = (Frame*)pcs->lpCreateParams;
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)frame);
			return DefWindowProc(hwnd, message, wParam, lParam);
		}

		Frame* frame = (Frame*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (frame == nullptr)
		{
			return DefWindowProc(hwnd, message, wParam, lParam);
		}

		if (message == WM_PAINT)
		{
			frame->rt->BeginDraw();
			frame->rt->SetTransform(D2D1::Matrix3x2F::Identity());
			frame->rt->Clear(D2D1::ColorF(D2D1::ColorF::WhiteSmoke));
			frame->renderControl(frame->rt, &frame->writer, &frame->color, 0, 0, frame->place);
			frame->rt->EndDraw();
			ValidateRect(hwnd, NULL);
		}
		else if (message == WM_CLOSE)
		{
			frame->close = 1;
		}
		else if (message == WM_MOUSEMOVE)
		{
			if (!frame->mouseTracked)
			{
				TrackMouseEvent(&frame->tme);
				frame->mouseTracked = true;
			}
		}
		else if (message == WM_MOUSELEAVE || message == WM_NCMOUSELEAVE)
		{
			frame->mouseTracked = false;
		}
		else if (message == WM_MOUSEHOVER || message == WM_NCMOUSEHOVER)
		{
			TrackMouseEvent(&frame->tme);
			frame->mouseTracked = true;
		}

		frame->mouseEventControl(frame, hwnd, message, wParam, lParam);
		frame->winEventControl(frame, hwnd, message, wParam, lParam);

		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	HRESULT Frame::createDeviceIndependentResources()
	{
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &Direct2dFactory);
		return hr;
	}
	HRESULT Frame::createDeviceResources()
	{
		HRESULT hr = S_OK;

		RECT rc;
		GetClientRect(hwnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		hr = Direct2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, size), &rt);

		color.init(rt);
		writer.init(rt);

		return hr;
	}
}