#include <windows.h>
#include "guifunc.h"

static LRESULT CALLBACK GtkSepProc(HWND hwnd,UINT msg,WPARAM wParam,
                                   LPARAM lParam) {
   PAINTSTRUCT ps;
   HPEN oldpen,dkpen,ltpen;
   RECT rect;
   HDC hDC;
   if (msg==WM_PAINT) {
      if (GetUpdateRect(hwnd,NULL,TRUE)) {
         BeginPaint(hwnd,&ps);
         GetClientRect(hwnd,&rect);
         hDC=ps.hdc;
         ltpen=CreatePen(PS_SOLID,0,(COLORREF)GetSysColor(COLOR_3DHILIGHT));
         dkpen=CreatePen(PS_SOLID,0,(COLORREF)GetSysColor(COLOR_3DSHADOW));

         if (rect.right > rect.bottom) {
            oldpen=SelectObject(hDC,dkpen);
            MoveToEx(hDC,rect.left,rect.top,NULL);
            LineTo(hDC,rect.right,rect.top);

            SelectObject(hDC,ltpen);
            MoveToEx(hDC,rect.left,rect.top+1,NULL);
            LineTo(hDC,rect.right,rect.top+1);
         } else {
            oldpen=SelectObject(hDC,dkpen);
            MoveToEx(hDC,rect.left,rect.top,NULL);
            LineTo(hDC,rect.left,rect.bottom);

            SelectObject(hDC,ltpen);
            MoveToEx(hDC,rect.left+1,rect.top,NULL);
            LineTo(hDC,rect.left+1,rect.bottom);
         }

         SelectObject(hDC,oldpen);
         DeleteObject(ltpen); DeleteObject(dkpen);
         EndPaint(hwnd,&ps);
      }
      return TRUE;
   } else return DefWindowProc(hwnd,msg,wParam,lParam);
}

void RegisterSepClass(HINSTANCE hInstance) {
  WNDCLASS wc;
  wc.style         = CS_HREDRAW|CS_VREDRAW;
  wc.lpfnWndProc   = GtkSepProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = NULL;
  wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(1+COLOR_BTNFACE);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = "WC_GTKSEP";
  RegisterClass(&wc);
}

static BOOL CALLBACK enumFunc(HWND hWnd,LPARAM lParam) {
  HFONT GuiFont;
  GuiFont=(HFONT)lParam;
  SendMessage(hWnd,WM_SETFONT,(WPARAM)GuiFont,MAKELPARAM(FALSE,0));
  return TRUE;
}

void SetGuiFont(HWND hWnd) {
  HFONT GuiFont;
  GuiFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
  if (GuiFont) EnumChildWindows(hWnd,enumFunc,(LPARAM)GuiFont);
}
