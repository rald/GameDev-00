#include <windows.h>
#include <stdlib.h>           // for the rand function

#include "sprite.h"

int cxClient, cyClient ;

void fillRect (HWND hwnd,int x,int y,int w,int h,COLORREF color)
{
     HBRUSH hBrush ;
     HDC    hdc ;
     RECT   rect ;
     
     if (cxClient == 0 || cyClient == 0)
          return ;
     
     SetRect (&rect, x, y, x+w, y+h) ;
     
     hBrush = CreateSolidBrush (color) ;

     hdc = GetDC (hwnd) ;
     FillRect (hdc, &rect, hBrush) ;
     ReleaseDC (hwnd, hdc) ;
     DeleteObject (hBrush) ;
}     

void drawSprite(HWND hwnd,int w,int h,int *pixels,int size) {
	COLORREF color;
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			if(pixels[i+j*w]==1) {
				fillRect(hwnd,i*size,j*size,size,size,RGB(0,255,0));
			}
		}
	}
}


LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{

		 PAINTSTRUCT  ps ;

     switch (iMsg)
     {
     case WM_SIZE:
          cxClient = LOWORD (lParam) ;
          cyClient = HIWORD (lParam) ;
          return 0 ;
          
     case WM_DESTROY:
          PostQuitMessage (0) ;
          return 0 ;

     case WM_PAINT:
          drawSprite(hwnd,SPRITE_WIDTH,SPRITE_HEIGHT,sprite_pixels,4);
          return 0 ;

     }
     return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
     static TCHAR szAppName[] = TEXT ("RandRect") ;
     HWND         hwnd ;
     MSG          msg ;
     WNDCLASS     wndclass ;
     
     wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
     wndclass.lpfnWndProc   = WndProc ;
     wndclass.cbClsExtra    = 0 ;
     wndclass.cbWndExtra    = 0 ;
     wndclass.hInstance     = hInstance ;
     wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
     wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
     wndclass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH) ;
     wndclass.lpszMenuName  = NULL ;
     wndclass.lpszClassName = szAppName ;
     
     if (!RegisterClass (&wndclass))
     {
          MessageBox (NULL, TEXT ("This program requires Windows NT!"),
                      szAppName, MB_ICONERROR) ;
          return 0 ;
     }
     
     hwnd = CreateWindow (szAppName, TEXT ("Random Rectangles"),
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL, NULL, hInstance, NULL) ;
     
     ShowWindow (hwnd, iCmdShow) ;
     UpdateWindow (hwnd) ;

		 while (TRUE)
     {
          if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
          {
               if (msg.message == WM_QUIT)
                    break ;
               
               TranslateMessage (&msg) ;
               DispatchMessage (&msg) ;
          }
     }
     return msg.wParam ;
}

