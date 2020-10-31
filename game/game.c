#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "palette.h"

#define CANVAS_NAME "canvas"
#define FONT_NAME "font"
#define LOG_FILE "logs.txt"

#define GET_HIGH_BIT(x) ((1 << 15) & (x))

BOOL quit=FALSE;

int cxClient=0, cyClient=0;

int gridX,gridY,gridWidth,gridHeight,gridSize;
COLORREF gridColor;
BOOL gridShow;

int paletteX,paletteY,paletteGridSize,paletteColorIndex;
COLORREF paletteGridColor;

int thumbX,thumbY,thumbPixelSize;
COLORREF thumbGridColor;

int mouseX=0,mouseY=0;
BOOL mouseLeftButtonDown=FALSE,mouseRightButtonDown=FALSE;

int exitKeyPressed;
int saveKeyPressed;
int loadKeyPressed;

BOOL hold=FALSE;
int idx=0;

int *canvas=NULL;
int canvasWidth,canvasHeight,canvasNumFrames;

int *font=NULL;
int fontWidth,fontHeight,fontNumFrames;

FILE *logfout;

char *strupr(char *s) {
	char *i=s;
	while(*i) {
		*i=toupper(*i);
		i++;
	}
	return s;
}

char *strlwr(char *s) {
	char *i=s;
	while(*i) {
		*i=tolower(*i);
		i++;
	}
	return s;
}

void saveSprite(char *name,int *canvas,int w,int h,int frames) {
	char filename[256];
	strcpy(filename,name);
	strcat(filename,".cvs");
	FILE *fout=fopen(filename,"w");
	fprintf(fout,"#ifndef %s_H\n",strupr(name));
	fprintf(fout,"#define %s_H\n\n",strupr(name));
	fprintf(fout,"#define %s_WIDTH %d\n",strupr(name),w);
	fprintf(fout,"#define %s_HEIGHT %d\n",strupr(name),h);
	fprintf(fout,"#define %s_NUM_FRAMES %d\n\n",strupr(name),frames);
	fprintf(fout,"int %s_pixels [] = {\n",strlwr(name));
	for(int k=0;k<frames;k++) {
		if(k=0) fprintf(fout,"\n");
		fprintf(fout,"\n/* %d */\n\n",k);
		for(int j=0;j<h;j++) {
			for(int i=0;i<w;i++) {
				fprintf(fout,"%d,",canvas[i+j*w+k*w*h]);
			}
			fprintf(fout,"\n");
		}
		fprintf(fout,"\n");
	}
	fprintf(fout,"};\n\n");
	fprintf(fout,"#endif\n");
	fclose(fout);
}

BOOL loadSprite(char *name,int **canvas,int *w,int *h,int *frames) {
	int line=0;
	char filename[256];
	strcpy(filename,name);
	strcat(filename,".cvs");
	FILE *fin=fopen(filename,"r");
	if(!fin) return FALSE;
	
	line++;
	fscanf(fin,"#ifndef %*s\n");
	
	line++;
	fscanf(fin,"#define %*s\n\n");
	
	line++;
	if(fscanf(fin,"#define %*s %d\n",w)!=1) {
		fprintf(logfout,"Error Line %d: invalid width\n",line);
		return FALSE;
	}
	
	line++;
	if(fscanf(fin,"#define %*s %d\n",h)!=1) {
		fprintf(logfout,"Error Line %d: invalid height\n",line);
		return FALSE;
	}
	
	line++;
	if(fscanf(fin,"#define %*s %d\n\n",frames)!=1) {
		fprintf(logfout,"Error Line %d: invalid num frames\n",line);
		return FALSE;
	}
	
	line++;
	fscanf(fin,"int %*s [] = {\n");
	
	(*canvas)=malloc(sizeof(int)*((*w)*(*h)*(*frames)));
	if(!(*canvas)) return FALSE;
	
	for(int k=0;k<(*frames);k++) {
		fscanf(fin,"\n/* %*d */\n\n");
		line+=3;
		for(int j=0;j<(*h);j++) {
			for(int i=0;i<(*w);i++) {
				int idx=0;
				if(fscanf(fin,"%d,",&idx)!=1) {
					fprintf(logfout,"Error Line %d: data error\n",line);
					free(*canvas);
					(*canvas)=NULL;
					return FALSE;
				}
				(*canvas)[i+j*(*w)+k*(*w)*(*h)]=idx;
			}
			line++;
			fscanf(fin,"\n");
		}
		line++;
		fscanf(fin,"\n");
	}
	fscanf(fin,"};\n\n");
	fscanf(fin,"#endif\n");
	fclose(fin);
	fflush(logfout);
	return TRUE;
}

double sgn(double x) { return x<0?-1:x>0?1:0; }

BOOL inrect(int x,int y,int rx,int ry,int rw,int rh) {
	return x>=rx && x<rx+rw && y>=ry && y<ry+rh;
}

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

void drawGrid(HDC hdc,int x,int y,int w,int h,int size,COLORREF color,BOOL show,int frame,int *canvas,COLORREF *paletteColors,int paletteLength,int paletteTransparentIndex) {
	drawRect(hdc,x,y,w*size+2,h*size+2,color);
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			int idx=canvas[i+j*w+frame*h*w];
			if(idx!=paletteTransparentIndex) {
				fillRect(hdc,x+i*size+1,y+j*size+1,size,size,paletteColors[idx]);
			}
			if(gridShow) { 
				drawRect(hdc,x+i*size+1,y+j*size+1,size,size,color);	
			}
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

void drawSprite(HDC hdc,int x,int y,int w,int h,int size,int frame,int *canvas,COLORREF *paletteColors,int paletteNumColors,int paletteTransparentIndex) {
	for(int j=0;j<h;j++) {
		for(int i=0;i<w;i++) {
			int idx=canvas[i+j*w+frame*w*h];
			if(idx!=paletteTransparentIndex) {
				fillRect(hdc,i*size+x,j*size+y,size,size,paletteColors[idx]);
			}
		}
	}
}

void drawThumbnail(HDC hdc,int x,int y,int w,int h,int size,int frame,int *canvas,COLORREF color,COLORREF *paletteColors,int paletteNumColors,int paletteTransparentIndex) {
	drawRect(hdc,x,y,(w+2)*size,(h+2)*size,color);
	drawSprite(hdc,x,y,w,h,size,frame,canvas,paletteColors,paletteNumColors,paletteTransparentIndex);
}

void drawText(HDC hdc,int x,int y,char *text,int size,int *font,int w,int h,int n,COLORREF color) {
	int cx=x,cy=y;
	for(int k=0;k<strlen(text);k++) {
		for(int j=0;j<h;j++) {
			for(int i=0;i<w;i++) {
				unsigned char l=text[k];
				int idx=font[i+j*w+l*w*h];
				if(idx!=0) {
					fillRect(hdc,i*size+cx,j*size+cy,size,size,color);
				}
			}
		}
		cx+=w*size;
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

			logfout=fopen(LOG_FILE,"a");

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

			if(!loadSprite(FONT_NAME,&font,&fontWidth,&fontHeight,&fontNumFrames)) {
				exit(-1);
			}

			if(!loadSprite(CANVAS_NAME,&canvas,&canvasWidth,&canvasHeight,&canvasNumFrames)) {
				canvasWidth=32;
				canvasHeight=32;
				canvasNumFrames=1;
				canvas=malloc(sizeof(int)*(canvasWidth*canvasHeight*canvasNumFrames));
				for(int i=0;i<canvasWidth*canvasHeight*canvasNumFrames;i++) {
					canvas[i]=0;
				}
			}

			gridSize=14;
			gridX=0;
			gridY=0;
			gridWidth=canvasWidth;
			gridHeight=canvasHeight;
			gridColor=palette_colors[3];
			gridShow=FALSE;

			paletteGridSize=32;
			paletteX=0;
			paletteY=cyClient-paletteGridSize;
			paletteColorIndex=0;
			paletteGridColor=palette_colors[3];
			
			thumbPixelSize=2;
			thumbX=cxClient-gridWidth*thumbPixelSize;
			thumbY=0;
			thumbGridColor=palette_colors[3];
						
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
			drawGrid(backdc,gridX,gridY,gridWidth,gridHeight,gridSize,gridColor,gridShow,0,canvas,palette_colors,palette_LENGTH,palette_TRANSPARENT_INDEX);
			drawPalette(backdc,paletteX,paletteY,paletteGridSize,paletteGridColor,palette_colors,palette_LENGTH,paletteColorIndex,4);
			drawThumbnail(backdc,thumbX,thumbY,gridWidth,gridHeight,thumbPixelSize,0,canvas,thumbGridColor,palette_colors,palette_LENGTH,palette_TRANSPARENT_INDEX);

//			drawText(backdc,0,0,"Hello World",1,font,fontWidth,fontHeight,fontNumFrames,palette_colors[3]);
			
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
					if(x>=0 && x<palette_LENGTH && y==0) {
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

			SHORT exitKeyPressed = GET_HIGH_BIT(GetAsyncKeyState(VK_ESCAPE));						
			SHORT saveKeyPressed = GET_HIGH_BIT(GetAsyncKeyState('S'));						
			SHORT loadKeyPressed = GET_HIGH_BIT(GetAsyncKeyState('L'));						
			SHORT gridKeyPressed = GET_HIGH_BIT(GetAsyncKeyState('G'));						

			if(exitKeyPressed) {
				quit=TRUE;
			}

			if(saveKeyPressed) {
				if(!hold) {
					hold=TRUE;
					saveSprite(CANVAS_NAME,canvas,gridWidth,gridHeight,1);
				} 
			}

			if(loadKeyPressed) {
				if(!hold) {
					hold=TRUE;
					loadSprite(CANVAS_NAME,&canvas,&canvasWidth,&canvasHeight,&canvasNumFrames);
					gridWidth=canvasWidth;
					gridHeight=canvasHeight;
				} 
			}

			if(gridKeyPressed) {
				if(!hold) {
					hold=TRUE;
					gridShow=!gridShow;
				} 
			}

							
			if(	!mouseLeftButtonDown && 
					!mouseRightButtonDown &&
					!exitKeyPressed &&
					!saveKeyPressed &&
					!loadKeyPressed &&
					!gridKeyPressed) {
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
			fclose(logfout);
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

	while(GetMessage(&msg,NULL,0,0) && !quit) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
