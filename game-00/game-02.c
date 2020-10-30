#include <windows.h>

#include <stdlib.h>
#include <math.h>

#include "palette.h"
#include "sprite.h"

int cxClient, cyClient;

double cx,cy;

double x=0,y=0;

int frame=0;

int pixel_size=4;

void fillRect (HDC hdc,int x,int y,int w,int h,COLORREF color) {
	HBRUSH hBrush;
	RECT rect;

	SetRect(&rect,x,y,x+w,y+h);
	hBrush=CreateSolidBrush(color);
	FillRect(hdc,&rect,hBrush);
	DeleteObject(hBrush);
}     

void drawSprite(HDC hdc,int x,int y,int w,int h,int *pixels,int size,int *palette_colors,int paletteNumColors) {
	COLORREF color;
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			fillRect(hdc,i*size+x,j*size+y,size,size,palette_colors[sprite_pixels[i+j*w]]);
		}
	}
}


LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
		 HDC hdc;
		 static HDC memoryDC;
		 static BITMAP bitmap;
		 HBITMAP hBitmap;

     PAINTSTRUCT ps;

	switch(iMsg) {

		case WM_CREATE:

			hdc=GetDC(hwnd);

			RECT rect;
			if(GetWindowRect(hwnd,&rect)) {
				cxClient=rect.right-rect.left;
				cyClient=rect.bottom-rect.top;
			}

			hBitmap=CreateCompatibleBitmap(hdc,cxClient,cyClient);

			memoryDC=CreateCompatibleDC(hdc);

			ReleaseDC(hwnd,hdc);

			SelectObject(memoryDC,hBitmap);

			DeleteObject(hBitmap);

			cx=((double)cxClient-SPRITE_WIDTH*pixel_size)/2;
			cy=((double)cyClient-SPRITE_HEIGHT*pixel_size)/2;

			return 0 ;

		case WM_SIZE:

			cxClient=LOWORD(lParam);
			cyClient=HIWORD(lParam);

			hdc=GetDC(hwnd);

			hBitmap=CreateCompatibleBitmap(hdc,cxClient,cyClient);

			DeleteDC(memoryDC);

			memoryDC=CreateCompatibleDC(hdc);

			ReleaseDC(hwnd,hdc);

			SelectObject(memoryDC,hBitmap);

			DeleteObject(hBitmap);

			cx=((double)cxClient-SPRITE_WIDTH*pixel_size)/2;
			cy=((double)cyClient-SPRITE_HEIGHT*pixel_size)/2;

			return 0 ;

		case WM_DESTROY:

			DeleteDC(memoryDC);
			PostQuitMessage (0) ;

			return 0 ;

		case WM_PAINT:
		
			SelectObject(memoryDC,GetStockObject(BLACK_BRUSH));
			Rectangle(memoryDC,0,0,cxClient,cyClient);

			x=sin(frame/10.0)*100.0;

			drawSprite(memoryDC,x+cx,y+cy,SPRITE_WIDTH,SPRITE_HEIGHT,sprite_pixels,pixel_size,palette_colors,PALETTE_NUM_COLORS);

			hdc=BeginPaint(hwnd,&ps);
			BitBlt(hdc,0,0,cxClient,cyClient,memoryDC,0,0,SRCCOPY);
			EndPaint(hwnd,&ps);

			frame++;

			InvalidateRect(hwnd,NULL,FALSE);

			return 0 ;

		case WM_ERASEBKGND:
			return 1;
	}

	return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR szCmdLine, int iCmdShow) {
	static TCHAR szAppName[]=TEXT("Game");
	HWND         hwnd;
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if(!RegisterClass(&wndclass)) {
		MessageBox(NULL,TEXT("This program requires Windows NT!"),szAppName,MB_ICONERROR);
		return 0;
	}
     
	hwnd=CreateWindow(	szAppName,TEXT ("Game"),
											WS_OVERLAPPEDWINDOW,
											CW_USEDEFAULT,CW_USEDEFAULT,
											CW_USEDEFAULT,CW_USEDEFAULT,
											NULL,NULL,hInstance,NULL);
     
	ShowWindow(hwnd,iCmdShow);
	UpdateWindow(hwnd);

	while(TRUE) {
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
			if(msg.message==WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
