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

WCHAR wstr[MAX_PATH] = { 0 };
WCHAR wstr0[MAX_PATH] = { 0 };

BOOL LoadImageFileToEdit(HWND hwnd, WCHAR* wstr) {
	BOOL bSuccess = FALSE;
	if (lstrlenW(wstr) == 0)return FALSE;
	lstrcpyW(wstr0, wstr);

	g_hbmBall = mLoadImageFile(wstr);
	if (g_hbmBall == NULL) {
#ifndef DEBUG
		//while (1);
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
		MultiByteToWideChar(CP_UTF8, 0, szFileName, -1, wstr, MAX_PATH);
		LoadImageFileToEdit(hwnd, wstr);
	}
}

volatile HANDLE hFile = NULL;

//https://stackoverflow.com/questions/11140483/how-to-get-list-of-files-with-a-specific-extension-in-a-given-folder
bool OpenNextImage(HWND hwnd) {
	WIN32_FIND_DATA dirFile;
	ZeroMemory(&dirFile, sizeof(dirFile));

	if ((hFile == NULL)||(hFile == INVALID_HANDLE_VALUE)) {
		//this will match all 3 letter file extensions
		hFile = FindFirstFile("*.???", &dirFile);
	}
	if (hFile == INVALID_HANDLE_VALUE)	return FALSE;

	do
	{
		if (!strcmp(dirFile.cFileName, "")) continue;
		if (!strcmp(dirFile.cFileName, ".")) continue;
		if (!strcmp(dirFile.cFileName, "..")) continue;
		/*
		if (gIgnoreHidden)
		{
			if (dirFile.attrib & _A_HIDDEN) continue;
			if (dirFile.name[0] == '.') continue;
		}
		*/

		char* cptr = strrchr(dirFile.cFileName, '.');
		if (cptr == 0)	continue;
		if (cptr[1] == 0) continue;
		if (cptr[2] == 0) continue;
		if (cptr[3] == 0) continue;
		cptr[1] = tolower(cptr[1]);
		cptr[2] = tolower(cptr[2]);
		cptr[3] = tolower(cptr[3]);
		if ((strcmp(cptr, ".png") == 0) ||
			(strcmp(cptr, ".jpg") == 0) ||
			(strcmp(cptr, ".bmp") == 0) ||
			(strcmp(cptr, ".gif") == 0)) {

			char szFileName[MAX_PATH] = "";
			GetFullPathName(dirFile.cFileName, MAX_PATH, szFileName, 0);
			MultiByteToWideChar(CP_UTF8, 0, szFileName, -1, wstr, MAX_PATH);

			if (wcscmp(wstr0, wstr) != 0) {
				LoadImageFileToEdit(hwnd, wstr);
				return TRUE;
			}
		}
	} while (FindNextFile(hFile, &dirFile) != 0);
	FindClose(hFile);
	hFile = NULL;
	return FALSE;
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
				case ID_OPEN:
				case ID_FILE_OPEN:
					hFile = NULL;
					DoFileOpen(hwnd);
				break;
				case ID_NEXT:
					//try twice because function fails if starting from last element in directory.
					if (!OpenNextImage(hwnd))
						OpenNextImage(hwnd);				
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
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
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
	if(hFile)
		FindClose(hFile);
	return (int)Msg.wParam;
}