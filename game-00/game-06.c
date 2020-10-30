#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "palette.h"
#include "sprite.h"



int cxClient=0, cyClient=0;

int gridX,gridY,gridWidth,gridHeight,gridSize;
COLORREF gridColor;

int paletteX,paletteY,paletteGridSize,paletteColorIndex;
COLORREF paletteGridColor;

int thumbX,thumbY,thumbPixelSize;

int mouseX=0,mouseY=0;
BOOL mouseLeftButtonDown=FALSE,mouseRightButtonDown=FALSE;

BOOL hold=FALSE;
int idx=-1;

int *canvas=NULL;

double sgn(double x) { return x<0?-1:x>0?1:0; }

void fillRect (HDC hdc,int x,int y,int w,int h,COLORREF color) {
	HBRUSH hBrush=CreateSolidBrush(color);
	RECT rect;
	SetRect(&rect,x,y,x+w,y+h);
	FillRect(hdc,&rect,hBrush);
	DeleteObject(hBrush);
}

void drawRect(HDC hdc,int x,int y,int w,int h,COLORREF color) {
	HBRUSH hBrush=CreateSolidBrush(color);
	FrameRect(hdc,&(RECT){x,y,x+w,y+h},hBrush);
	DeleteObject(hBrush);
}

void drawLine(HDC hdc,int x0,int y0,int x1,int y1,COLORREF color) {
	HPEN hPen=CreatePen(PS_SOLID,1,color);
	SelectObject(hdc,hPen);
	MoveToEx(hdc,x0,y0,NULL);
	LineTo(hdc,x1,y1);
}

void drawGrid(HDC hdc,int x,int y,int w,int h,int size,COLORREF color) {
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			HPEN hPen=CreatePen(PS_SOLID,1,color);
			HBRUSH hBrush=CreateSolidBrush(palette_colors[canvas[i+j*w]]);
			SelectObject(hdc,hPen);
			SelectObject(hdc,hBrush);
			Rectangle(hdc,x+i*size,y+j*size,i*size+size,j*size+size);
			DeleteObject(hPen);
			DeleteObject(hBrush);
		}
	}
}

void drawPalette(HDC hdc,int x,int y,int size,COLORREF color,COLORREF *palette_colors,int paletteNumColors,int paletteColorIndex,int gap) {
	for(int i=0;i<paletteNumColors;i++) {
		fillRect(hdc,x+i*size+gap*2,y+gap*2,size-gap*4,size-gap*4,palette_colors[i]);
	}
	drawRect(hdc,x+paletteColorIndex*size+gap,y+gap,size-gap*2,size-gap*2,color);
	drawRect(hdc,x,y,paletteNumColors*size,size,color);
}

void drawCanvas(HDC hdc,int x,int y,int w,int h,int size,int *canvas,int *palette_colors,int paletteNumColors,int paletteTransparentColor) {
	COLORREF color;
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			int idx=canvas[i+j*w];
			if(idx!=paletteTransparentColor) {
				fillRect(hdc,i*size+x,j*size+y,size,size,palette_colors[idx]);
			}
		}
	}
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	static HDC backdc;
	static BITMAP bitmap;
	HBITMAP hBitmap;

	PAINTSTRUCT ps;

	switch(msg) {

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

			gridSize=16;
			gridX=0;
			gridY=0;
			gridWidth=16;
			gridHeight=16;
			gridColor=palette_colors[3];

			paletteGridSize=32;
			paletteX=0;
			paletteY=cyClient-paletteGridSize;
			paletteColorIndex=0;
			paletteGridColor=palette_colors[3];
			
			thumbPixelSize=4;
			thumbX=cxClient-gridWidth*thumbPixelSize;
			thumbY=0;
			
			canvas=malloc(sizeof(int)*(gridWidth*gridHeight));

			for(int j=0;j<gridHeight;j++) {
				for(int i=0;i<gridWidth;i++) {
					canvas[i+j*gridWidth]=0;
				}
			}

			break;

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

			paletteY=cyClient-paletteGridSize;

			thumbX=cxClient-gridWidth*thumbPixelSize;

			break;
			
		case WM_PAINT:
		
			fillRect(backdc,0,0,cxClient,cyClient,RGB(0,0,0));
			drawGrid(backdc,gridX,gridY,gridWidth,gridHeight,gridSize,gridColor);
			drawPalette(backdc,paletteX,paletteY,paletteGridSize,paletteGridColor,palette_colors,PALETTE_NUM_COLORS,paletteColorIndex,4);
			
			drawRect(backdc,thumbX,thumbY,(gridWidth+2)*thumbPixelSize,(gridHeight+2)*thumbPixelSize,palette_colors[3]);
			drawCanvas(backdc,thumbX+thumbPixelSize,thumbY+thumbPixelSize,gridWidth,gridHeight,thumbPixelSize,canvas,palette_colors,PALETTE_NUM_COLORS,4);

			hdc=BeginPaint(hwnd,&ps);
			BitBlt(hdc,0,0,cxClient,cyClient,backdc,0,0,SRCCOPY);
			EndPaint(hwnd,&ps);

			InvalidateRect(hwnd,NULL,FALSE);						

			if(mouseLeftButtonDown) {

				{
					int x=(mouseX-gridX)/gridSize;
					int y=(mouseY-gridY)/gridSize;
					if(x>=0 && x<gridWidth && y>=0 && y<gridHeight) {
						if(!hold) {
							hold=TRUE;
							if(canvas[x+y*gridWidth]==paletteColorIndex) {
								idx=0;
							} else {
								idx=paletteColorIndex;
							}
						} else {
							canvas[x+y*gridWidth]=idx;
						}
					}
				}
				
				{
					int x=(mouseX-paletteX)/paletteGridSize;
					int y=(mouseY-paletteY)/paletteGridSize;
					if(x>=0 && x<PALETTE_NUM_COLORS && y==0) {
						if(!hold) {
							hold=TRUE;
						} else {
							paletteColorIndex=x;
						}
					}
				}
			
			}
			
			if(mouseRightButtonDown) {
				int x=(mouseX-gridX)/gridSize;
				int y=(mouseY-gridY)/gridSize;
				if(x>=0 && x<gridWidth && y>=0 && y<gridHeight) {
					if(!hold) {
						hold=TRUE;
					} else {
						paletteColorIndex=canvas[x+y*gridWidth];
					}
				}
			}
							
			if(!mouseLeftButtonDown && !mouseRightButtonDown) {
				hold=FALSE;
			}
									
			break;

		case WM_MOUSEMOVE:
			mouseX=LOWORD(lParam);
			mouseY=HIWORD(lParam);
			break;

		case WM_LBUTTONDOWN: 
			mouseX=LOWORD(lParam);
			mouseY=HIWORD(lParam);
			mouseLeftButtonDown=TRUE;
			break;
		
		case WM_LBUTTONUP:
			mouseX=LOWORD(lParam);
			mouseY=HIWORD(lParam);
			mouseLeftButtonDown=FALSE;
			break;
		
		case WM_RBUTTONDOWN: 
			mouseX=LOWORD(lParam);
			mouseY=HIWORD(lParam);
			mouseRightButtonDown=TRUE;
			break;
		
		case WM_RBUTTONUP:
			mouseX=LOWORD(lParam);
			mouseY=HIWORD(lParam);
			mouseRightButtonDown=FALSE;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		
		default:
			return DefWindowProc(hwnd,msg,wParam,lParam);	
	
	}

	return 0;

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
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
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

	while(GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
