/****************************************************************************

   PROGRAM: xlmain.cpp

	DESCRIPTION: An OpenGL/OpenAL shell for Win32

****************************************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>
#include "../common/common.h"

/****************************************************************************
	Globals
****************************************************************************/
HDC g_hDC; // have global handle to device context for OpenGL
HGLRC g_hRC; // for OpenGL
AVEnvironment g_Env; // audio/visual environment object

AudioEnvPtr Object::audioObject = NULL; // static pointer to an audio object used by Object::Move

/****************************************************************************
	Constants
****************************************************************************/
const char APPNAME[] = "XL Demo"; // application name
const float STRIDE = 0.9f; // movement scaling factor
const float PI = 3.1415926535f;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM); 
void EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC );
void DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC );  
double TimeDiff();
            
/****************************************************************************

    FUNCTION: WinMain

    PURPOSE: initializes window, processes message loop

****************************************************************************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASSEX wndclass;

	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon (hInstance, "PROGICON");
	wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = APPNAME;
	wndclass.hIconSm = LoadIcon (hInstance, "PROGICON");

	RegisterClassEx (&wndclass);

	hwnd = CreateWindow(APPNAME, APPNAME,
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT,
						400, 300,
						NULL, NULL, hInstance, NULL);
	
	ShowWindow (hwnd, iCmdShow);
	UpdateWindow (hwnd);

   TimeDiff(); // init TimeDiff function

   // init AV environment
   g_Env.Init();

   // ** MAIN LOOP **
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0)) return msg.wParam;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
      }

      // change AV state
	  float td = (float) TimeDiff();
	  if (td != 0) { g_Env.SetFPS(1/td); }
      g_Env.DrawBuffer(td);
      g_Env.PlaceCamera();
      SwapBuffers(g_hDC);
	}

	return msg.wParam;
}


/****************************************************************************

    FUNCTION: WndProc

    PURPOSE:  Processes messages

****************************************************************************/

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{                
	static int iCurrentX, iCurrentY;

	static HINSTANCE hInstance;

	static BOOL bResultDisplayed = FALSE;
	int iTestTime = 100;

    switch (iMsg) {
		case WM_CREATE:
			hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
         EnableOpenGL( hwnd, &g_hDC, &g_hRC ); // enable OpenGL for the window
			return 0;

		case WM_SIZE:
			iCurrentX = (int) LOWORD(lParam);
			iCurrentY = (int) HIWORD(lParam);

         g_Env.ChangeView(iCurrentX, iCurrentY);
			return 0;

      case WM_KEYDOWN:
			// Translate/Rotate based on keystroke
			if(wParam == VK_UP)
         {
            g_Env.Step(0, STRIDE);
         }
	
			if(wParam == VK_DOWN)
         {
            g_Env.Step(PI, STRIDE);
         }
				
			if(wParam == VK_LEFT)
         {
				g_Env.Turn(-0.15f * STRIDE);
         }

			if(wParam == VK_RIGHT)
			{
            g_Env.Turn(0.15f * STRIDE);
         }
			if(wParam == 69) // E
			{
				g_Env.IncrementEnv();
			}

			if(wParam == 70) // F
			{
			   g_Env.ToggleFPS();	
			}

			if(wParam == 90) // Z
			{
			   g_Env.Step((- PI/2), STRIDE);	
			}

			if(wParam == 88) // X
			{
				g_Env.Step((PI/2), STRIDE);
			}
			break;

		case WM_PAINT:   
			return 0;

		case WM_DESTROY:
			DisableOpenGL(hwnd, g_hDC, g_hRC);
			PostQuitMessage(0);
			return 0;

		default:
			return (DefWindowProc(hwnd, iMsg, wParam, lParam));
    }
    return 0;
}

void EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC )
{
  PIXELFORMATDESCRIPTOR pfd;
  int iFormat;

  // get the device context (DC)
  *hDC = GetDC( hWnd );

  // set the pixel format for the DC
  ZeroMemory( &pfd, sizeof( pfd ) );
  pfd.nSize = sizeof( pfd );
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW |
    PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 16;
  pfd.iLayerType = PFD_MAIN_PLANE;
  iFormat = ChoosePixelFormat( *hDC, &pfd );
  SetPixelFormat( *hDC, iFormat, &pfd );

  // create and enable the render context (RC)
  *hRC = wglCreateContext( *hDC );
  wglMakeCurrent( *hDC, *hRC );

}

void DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC )
{
  wglMakeCurrent( NULL, NULL );
  wglDeleteContext( hRC );
  ReleaseDC( hWnd, hDC );
}

double TimeDiff()
{
   static clock_t OldTime;
   clock_t NewTime;
   double returnval;

   NewTime = clock();
   returnval = (double)(NewTime - OldTime) / CLOCKS_PER_SEC;
   OldTime = NewTime;
   return returnval;
}

