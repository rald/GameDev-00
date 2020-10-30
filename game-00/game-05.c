#include <windows.h>

#include <stdlib.h>
#include <math.h>

#include "palette.h"
#include "sprite.h"



#define GET_HIGH_BIT(x) ((1<<15) & (x))

BOOL quit=FALSE;

int cxClient, cyClient;

double cx,cy;

double x=0,y=0;

double vx=0,vy=0;

int frame=0;

int pixel_size=4;

POINT mouse;

double sgn(double x) { return x<0?-1:x>0?1:0; }

void fillRect (HDC hdc,int x,int y,int w,int h,COLORREF color) {
	HBRUSH hBrush;
	RECT rect;

	SetRect(&rect,x,y,x+w,y+h);
	hBrush=CreateSolidBrush(color);
	FillRect(hdc,&rect,hBrush);
	DeleteObject(hBrush);
}     

void drawSprite(HDC hdc,int x,int y,int w,int h,int size,int frame,int *pixels,int *palette_colors,int paletteNumColors) {
	COLORREF color;
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			int idx=sprite_pixels[i+j*w+frame*w*h];
			if(idx!=0) {
				fillRect(hdc,i*size+x,j*size+y,size,size,palette_colors[idx]);
			}
		}
	}
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	static HDC backdc;
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

			backdc=CreateCompatibleDC(hdc);

			ReleaseDC(hwnd,hdc);

			SelectObject(backdc,hBitmap);

			DeleteObject(hBitmap);

			cx=((double)cxClient-SPRITE_WIDTH*pixel_size)/2;
			cy=((double)cyClient-SPRITE_HEIGHT*pixel_size)/2;

			x=cx; y=cy;

			return 0 ;

		case WM_SIZE:

			cxClient=LOWORD(lParam);
			cyClient=HIWORD(lParam);

			hdc=GetDC(hwnd);

			hBitmap=CreateCompatibleBitmap(hdc,cxClient,cyClient);

			DeleteDC(backdc);

			backdc=CreateCompatibleDC(hdc);

			ReleaseDC(hwnd,hdc);

			SelectObject(backdc,hBitmap);

			DeleteObject(hBitmap);

			cx=((double)cxClient-SPRITE_WIDTH*pixel_size)/2;
			cy=((double)cyClient-SPRITE_HEIGHT*pixel_size)/2;

			x=cx; y=cy;

			return 0 ;

		case WM_DESTROY:

			DeleteDC(backdc);
			PostQuitMessage(0);

			return 0 ;

		case WM_PAINT:
		
			GetCursorPos(&mouse);
			ScreenToClient(hwnd,&mouse);
		
			SelectObject(backdc,GetStockObject(BLACK_BRUSH));
			Rectangle(backdc,0,0,cxClient,cyClient);

			SelectObject(backdc,GetStockObject(WHITE_PEN));
			MoveToEx(backdc,x+SPRITE_WIDTH*pixel_size/2,y+SPRITE_HEIGHT*pixel_size/2,NULL);
			LineTo(backdc,mouse.x,mouse.y);

			drawSprite(backdc,x,y,SPRITE_WIDTH,SPRITE_HEIGHT,pixel_size,frame%SPRITE_NUM_FRAMES,sprite_pixels,palette_colors,PALETTE_NUM_COLORS);


			hdc=BeginPaint(hwnd,&ps);
			BitBlt(hdc,0,0,cxClient,cyClient,backdc,0,0,SRCCOPY);
			EndPaint(hwnd,&ps);

			InvalidateRect(hwnd,NULL,FALSE);

			SHORT escapeKeyState = GET_HIGH_BIT(GetAsyncKeyState(VK_ESCAPE));
			SHORT upKeyState = GET_HIGH_BIT(GetAsyncKeyState(VK_UP));
			SHORT downKeyState = GET_HIGH_BIT(GetAsyncKeyState(VK_DOWN));
			SHORT leftKeyState = GET_HIGH_BIT(GetAsyncKeyState(VK_LEFT));
			SHORT rightKeyState = GET_HIGH_BIT(GetAsyncKeyState(VK_RIGHT));
			
			BOOL keypressed=FALSE;
			
			if(escapeKeyState)    { quit=TRUE; }
			if(upKeyState)    { vy--; keypressed=TRUE; }
			if(downKeyState)  { vy++; keypressed=TRUE; }
			if(leftKeyState)  { vx--; keypressed=TRUE; }
			if(rightKeyState) { vx++; keypressed=TRUE; }

			if(!keypressed) { vx-=sgn(vx); vy-=sgn(vy); };

			if(x<0) { x=0; vx=0; }
			if(y<0) { y=0; vy=0; }
			if(x>=cxClient-SPRITE_WIDTH*pixel_size) { x=cxClient-SPRITE_WIDTH*pixel_size-1; vx=0; }
			if(y>=cyClient-SPRITE_HEIGHT*pixel_size) { y=cyClient-SPRITE_HEIGHT*pixel_size-1; vy=0; }

			x+=vx;
			y+=vy;			

			if(vx || vy) frame++;

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

	while(!quit) {
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
			if(msg.message==WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
