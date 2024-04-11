#include <windows.h>
#include "resource.h"
//adapted from http://www.winprog.org/tutorial/
//gdi stuff from https://www.codeproject.com/questions/842393/can-loadimage-windows-function-be-used-to-load-oth
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#define WIN32_LEAN_AND_MEAN
const char g_szClassName[] = "smallImageViewerClass";
const LPCSTR file_open_save_filter = "Image Files(*.bmp,*.png,*.jpg)\0*.bmp;*.png;*.jpg\0All Files(*.*)\0*.*\0\0";
const LPCSTR lpstrDefExt = "bmp";	//default extension when file browser is opened.

HBITMAP g_hbmBall = NULL;
ULONG_PTR gdiplusToken;

// BMP, GIF, JPEG, PNG, TIFF, Exif, WMF, and EMF
HBITMAP mLoadImageFile(const WCHAR* filename)
{
	HBITMAP result = NULL;
	Gdiplus::Bitmap bitmap(filename, false);
	bitmap.GetHBITMAP(0, &result);
	return result;
}

WCHAR wstr[MAX_PATH];

BOOL LoadImageFileToEdit(HWND hwnd, LPCTSTR pszFileName) {
	BOOL bSuccess = FALSE;

	MultiByteToWideChar(CP_UTF8, 0, pszFileName, -1, wstr, MAX_PATH);

	g_hbmBall = mLoadImageFile(wstr);
	if (g_hbmBall == NULL) {
#ifndef DEBUG
		while (1);
#endif
		return FALSE;
	}
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return bSuccess;
}

void DoFileOpen(HWND hwnd)
{
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = file_open_save_filter;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = lpstrDefExt;

	if(GetOpenFileName(&ofn))
	{
		LoadImageFileToEdit(hwnd, szFileName);
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
		{
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;			
			Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
			g_hbmBall = NULL;
		}
		break;
		/*
		case WM_SIZE:
		{
			HWND hEdit;
			RECT rcClient;

			GetClientRect(hwnd, &rcClient);

			hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
			SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
		}
		*/
		break;
		case WM_PAINT: {
			if (g_hbmBall) {
				BITMAP bm;
				PAINTSTRUCT ps;

				HDC hdc = BeginPaint(hwnd, &ps);

				HDC hdcMem = CreateCompatibleDC(hdc);
				HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_hbmBall);

				GetObject(g_hbmBall, sizeof(bm), &bm);

				BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

				SelectObject(hdcMem, hbmOld);
				DeleteDC(hdcMem);

				EndPaint(hwnd, &ps);
			}
		}
		break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_FILE_NEW:
				break;
				case 0:	//this is the wParam that gets passed when Open is clicked in top menu.
				case ID_FILE_OPEN:
					DoFileOpen(hwnd);
				break;
				case ID_FILE_SAVEAS:
				break;
			}
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
	wc.lpszClassName = g_szClassName;
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
#ifndef DEBUG
		while (1);
#endif
		return 0;
	}

	hwnd = CreateWindowEx(
		0,
		g_szClassName,
		"small Image Viewer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 480, 320,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
#ifndef DEBUG
		while (1);
#endif
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return (int)Msg.wParam;
}