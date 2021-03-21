#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

TCHAR szClassName[] = TEXT("Window");
HWND hList;

BOOL GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
{
	BOOL bSetScaling = FALSE;
	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		HMODULE hShcore = LoadLibrary(TEXT("SHCORE"));
		if (hShcore)
		{
			typedef HRESULT __stdcall GetDpiForMonitor(HMONITOR, int, UINT*, UINT*);
			GetDpiForMonitor* fnGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor*>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (fnGetDpiForMonitor)
			{
				UINT uDpiX, uDpiY;
				if (SUCCEEDED(fnGetDpiForMonitor(hMonitor, 0, &uDpiX, &uDpiY)) && uDpiX > 0 && uDpiY > 0)
				{
					*pnX = uDpiX;
					*pnY = uDpiY;
					bSetScaling = TRUE;
				}
			}
			FreeLibrary(hShcore);
		}
	}
	if (!bSetScaling)
	{
		HDC hdc = GetDC(NULL);
		if (hdc)
		{
			*pnX = GetDeviceCaps(hdc, LOGPIXELSX);
			*pnY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
			bSetScaling = TRUE;
		}
	}
	if (!bSetScaling)
	{
		*pnX = DEFAULT_DPI;
		*pnY = DEFAULT_DPI;
		bSetScaling = TRUE;
	}
	return bSetScaling;
}

WNDPROC DefaultEditWndProc;
LRESULT CALLBACK EditProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CHAR:
		if (wParam == '[')
		{
			int nTextHeight = 0;
			{
				HDC hdc = GetDC(hWnd);
				SIZE size = {};
				HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
				HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
				GetTextExtentPoint32(hdc, L"[", 1, &size);
				SelectObject(hdc, hOldFont);
				nTextHeight = size.cy;
				ReleaseDC(hWnd, hdc);
			}
			if(nTextHeight)
			{
				POINT point;
				GetCaretPos(&point);
				ClientToScreen(hWnd, &point);
				SetWindowLongPtr(hList, GWLP_USERDATA, (LONG_PTR)hWnd);
				{
					const int nItemHeight = (int)SendMessage(hList, LB_GETITEMHEIGHT, 0, 0);
					const int nItemCount = min((int)SendMessage(hList, LB_GETCOUNT, 0, 0), 16);
					SetWindowPos(hList, 0, point.x, point.y + nTextHeight, 256, nItemHeight * nItemCount + GetSystemMetrics(SM_CYEDGE) * 2, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				SendMessage(hList, LB_SETCURSEL, 0, 0);
				ShowWindow(hList, SW_SHOW);
				return CallWindowProc(DefaultEditWndProc, hWnd, msg, wParam, lParam);
			}
		} else {
			ShowWindow(hList, SW_HIDE);
		}
		break;
	case WM_KILLFOCUS:
		if ((HWND)wParam != hList) {
			ShowWindow(hList, SW_HIDE);
		}
		break;
	default:
		break;
	}
	return CallWindowProc(DefaultEditWndProc, hWnd, msg, wParam, lParam);
}

WNDPROC DefaultListWndProc;
LRESULT CALLBACK ListProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CHAR:
		if (wParam == VK_RETURN) {
			const int nIndex = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
			if (nIndex != LB_ERR) {
				const int nTextLength = (int)SendMessage(hWnd, LB_GETTEXTLEN, nIndex, 0);
				if (nTextLength != LB_ERR) {
					LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, (nTextLength + 1) * sizeof(WCHAR));
					SendMessage(hWnd, LB_GETTEXT, nIndex, (LPARAM)lpszText);
					HWND hEdit = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);
					if (hEdit) {
						SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)lpszText);
						SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)L"]");
					}
					GlobalFree(lpszText);
				}
			}
			ShowWindow(hWnd, SW_HIDE);
		}
		else if (wParam == VK_ESCAPE) {
			ShowWindow(hWnd, SW_HIDE);
		}
		else {
			return CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
		}
		break;
	case WM_KILLFOCUS:
		ShowWindow(hWnd, SW_HIDE);
		{
			HWND hEdit = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (hEdit && IsWindow(hEdit) && IsWindowVisible(hEdit)) {
				SetFocus(hEdit);
			}
		}
		break;
	default:
		break;
	}
	return CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit;
	static HFONT hFont;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	switch (msg)
	{
	case WM_CREATE:
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		DefaultEditWndProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc1);
		hList = CreateWindowEx(WS_EX_TOPMOST | WS_EX_CLIENTEDGE | WS_EX_NOACTIVATE, L"LISTBOX", NULL, WS_POPUP | WS_VSCROLL, 0, 0, 0, 0, 0, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		DefaultListWndProc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListProc1);
		{
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"abc");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"def");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"ghi");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"jkl");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"mno");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"pqr");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"stu");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"vwx");
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"yz");
		}
		SendMessage(hWnd, WM_APP, 0, 0);
		break;
	case WM_SETFOCUS:
		SetFocus(hEdit);
		break;
	case WM_APP:
		GetScaling(hWnd, &uDpiX, &uDpiY);
		DeleteObject(hFont);
		hFont = CreateFontW(-POINT2PIXEL(20), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, L"Yu Gothic UI");
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(hList, WM_SETFONT, (WPARAM)hFont, 0);
		break;
	case WM_MOVE:
		ShowWindow(hList, SW_HIDE);
		break;
	case WM_SIZE:
		MoveWindow(hEdit, POINT2PIXEL(10), POINT2PIXEL(10), LOWORD(lParam) - POINT2PIXEL(20), HIWORD(lParam) - POINT2PIXEL(20), TRUE);
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandle(TEXT("user32.dll"));
			if (hModUser32)
			{
				typedef BOOL(WINAPI*fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DPICHANGED:
		SendMessage(hWnd, WM_APP, 0, 0);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Window"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
