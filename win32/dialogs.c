#include <windows.h>
#include <commctrl.h>
#include "contid.h"

#define NUMDIALOGS 4

HWND mainDlg[NUMDIALOGS];

int CurrentDialog;

HINSTANCE hInst=NULL;

BOOL CALLBACK enumFunc(HWND hWnd,LPARAM lParam) {
  HFONT GuiFont;
  GuiFont=(HFONT)lParam;
  SendMessage(hWnd,WM_SETFONT,(WPARAM)GuiFont,MAKELPARAM(FALSE,0));
  return TRUE;
}

void ShowNewDialog(int NewDialog) {
  RECT DeskRect,OurRect;
  int newX,newY;
  HWND hWnd;
  if (NewDialog<0 || NewDialog>=NUMDIALOGS) return;

  hWnd=mainDlg[NewDialog];
  if (GetWindowRect(hWnd,&OurRect) &&
      GetWindowRect(GetDesktopWindow(),&DeskRect)) {
    newX = (DeskRect.left+DeskRect.right+OurRect.left-OurRect.right)/2;
    newY = (DeskRect.top+DeskRect.bottom+OurRect.top-OurRect.bottom)/2;
    SetWindowPos(hWnd,HWND_TOP,newX,newY,0,0,SWP_NOSIZE);
  }
  ShowWindow(hWnd,SW_SHOW);

  if (CurrentDialog!=-1) ShowWindow(mainDlg[CurrentDialog],SW_HIDE);
  CurrentDialog=NewDialog;
}

LRESULT CALLBACK GtkSepProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
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

void ConditionalExit(HWND hWnd) {
  if (MessageBox(hWnd,"This will exit without installing any new software on "
                 "your machine.\nAre you sure you want to quit?\n\n(You can "
                 "continue the installation at a\nlater date simply by "
                 "running this program again.)","Exit Install",
                 MB_YESNO|MB_ICONQUESTION)==IDYES) {
    PostQuitMessage(0);
  }
}

BOOL CALLBACK MainDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam)==BN_CLICKED && lParam) {
        switch(LOWORD(wParam)) {
          case BT_CANCEL: ConditionalExit(hWnd); break;
          case BT_NEXT:   ShowNewDialog(CurrentDialog+1); break;
          case BT_BACK:   ShowNewDialog(CurrentDialog-1); break;
          case BT_FINISH: PostQuitMessage(0); break;
//        case BT_BROWSE: SelectInstDir(); break;
        }
      }
      break;
    case WM_CLOSE:
      ConditionalExit(hWnd);
      return TRUE;
  }
  return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam,int nCmdShow) {
  MSG msg;
  HWND mainWin;
  DWORD err;
  LPVOID lpMsgBuf;
  WNDCLASS wc;
  int i;
  BOOL Handled;
  HFONT GuiFont;
//InitCommonControls();

  hInst = hInstance;

  if (!hPrevInstance) {
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

  for (i=0;i<NUMDIALOGS;i++) {
    mainDlg[i] = CreateDialog(hInst,MAKEINTRESOURCE(i+1),NULL,MainDlgProc);
  }

  GuiFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
  if (GuiFont) for (i=0;i<NUMDIALOGS;i++) {
    EnumChildWindows(mainDlg[i],enumFunc,(LPARAM)GuiFont);
  }
  CurrentDialog=-1;
  ShowNewDialog(0);

/*if (!mainWin) {
    err=GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf,0,NULL);
    MessageBox(NULL,lpMsgBuf,"Error",MB_OK | MB_ICONSTOP);
    LocalFree(lpMsgBuf);
    return 0;
  }*/

  while (GetMessage(&msg,NULL,0,0)) {
    Handled=FALSE;
    for (i=0;i<NUMDIALOGS && !Handled;i++) {
      Handled=IsDialogMessage(mainDlg[i],&msg);
    }
    if (!Handled) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}
