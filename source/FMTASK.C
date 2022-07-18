//--------------------------------------------------------------------------
// FMTASK.C
// 
// Copyright (c) 1994 by Jamie O'Connell
// 940331 Jamie O'Connell - broke out task from driver
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <mmsystem.h>

#include "fmsynth.h"
#include "fmvers.h"
#include "fmcfg.h"
#include "fmapi.h"

#include "resfmtsk.h"

// Sent by subsequent instance
#define WM_ACTIVATECFG WM_USER + 23

/*non-localized strings */
#define BCODE _based(_segname("_CODE"))

char BCODE aszProdName[]   = "FMSYNTH";
char BCODE aszProfileNm[]  = "FMSYNTH.INI";
char BCODE aszTaskCls[]    = "FMTask";
char BCODE szAppName[]     = "FMTASK";   // Name of App  
char BCODE aszCtlDlg[]     = "DoFMCtlDlg";
char BCODE aszFMCfg[]      = "FMCFG.DLL";
char BCODE aszAppSect[]    = "Application";
char BCODE aszCfgKey[]     = "FMCfg";
char BCODE aszLoad[]       = "LoadCPL";
char BCODE aszMMCPL[]      = "MMCPL";
char BCODE aszControlNm[]  = "CONTROL.INI";

//---------------------------------------------------------------------------
// Global Variables...
//---------------------------------------------------------------------------

HINSTANCE hInstance;              // Global instance handle for application
HWND    hwndTask;
HWND    hwndFirst;
char    RSBuf[256];
char    CfgPath[80];

//---------------------------------------------------------------------------
// Function declarations
//---------------------------------------------------------------------------

static void ErrorBox(LPSTR msg);
static LPSTR GetRStr(short id, LPSTR buf, UINT cb);
LONG FAR PASCAL _export TaskWndProc(HWND wnd, 
                                   WORD message, 
                                   WORD wParam, 
                                   DWORD lParam);

//---------------------------------------------------------------------------
// WinMain
//---------------------------------------------------------------------------

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hInstPrev,
                                   LPSTR lpstrCmdLine, int cmdShow) {
   WNDCLASS wclass;
        MSG      msgTask;

        
   if (hInstPrev) { // previous instance => activate
      hwndFirst = FindWindow(szAppName, NULL);
      PostMessage(hwndFirst, WM_ACTIVATECFG, 0, 0);
      return 0;        
      }

   // Set the global instance variable
        hInstance = hInst;

   // Make a class for the Hidden TASK window

   wclass.style            = NULL;
   wclass.lpfnWndProc      = (WNDPROC)TaskWndProc;
   wclass.cbClsExtra       = 0;
   wclass.cbWndExtra       = 0;
   wclass.hInstance        = hInst;
   wclass.hIcon            = NULL;
   wclass.hCursor          = NULL;
   wclass.hbrBackground    = NULL;
   wclass.lpszMenuName     = NULL;
   wclass.lpszClassName    = (LPSTR)aszTaskCls;
 
   if (!RegisterClass((LPWNDCLASS)&wclass)) {
      ErrorBox(GetRStr(IDS_TASKREG, RSBuf, sizeof(RSBuf))); 
      return 0;
      }  

   hwndTask = CreateWindow(
                  aszTaskCls,
                  szAppName,
                  WS_OVERLAPPEDWINDOW,  // (WS_CHILD) not visible
                  0, 0, 0, 0,           // no position or dimensions 
                  NULL,                 // parent 
                  NULL,                 // no menu 
                  hInst,
                  NULL);

   if (hwndTask == NULL) {
      ErrorBox(GetRStr(IDS_TASKCRT, RSBuf, sizeof(RSBuf))); 
      return FALSE;
      }  

        // Main message "pump"
   while (GetMessage((LPMSG) &msgTask, NULL, 0, 0)) {
      TranslateMessage((LPMSG) &msgTask);
      DispatchMessage((LPMSG) &msgTask);
      }

   return(0);
   }

/************************************************************************
** LONG FAR PASCAL
** TaskWndProc(
**   HWND wnd,          Handle of window
**   WORD message,      Message received from Windows
**   WORD wParam,       word parameter
**   DWORD lParam);     long parameter
**
**    Handle messages from the main window.
**
*/
LONG FAR PASCAL _export TaskWndProc(HWND wnd, 
                                   WORD message, 
                                   WORD wParam, 
                                   DWORD lParam) {
   HINSTANCE hLib;
   FARPROC   CtlFn;
   LPSTR     pCfg;

   switch (message) {
      case WM_CREATE:
         // decide whether or not to Load CPanel applet
         GetPrivateProfileString(aszAppSect, aszCfgKey, aszFMCfg, CfgPath, 
                                            sizeof(CfgPath), aszProfileNm);
         pCfg = ((BOOL)GetPrivateProfileInt(aszAppSect, aszLoad, TRUE, 
                                            aszProfileNm)) ? CfgPath : NULL;
         // Will either Add or Remove Entry...
         WritePrivateProfileString(aszMMCPL, aszProdName, pCfg, aszControlNm);
         StoreTask(wnd);
         return 0L;

      case WM_ACTIVATECFG:          // in response to new activation
         if ((hLib = LoadLibrary(CfgPath)) >= HINSTANCE_ERROR) {
            if ((CtlFn = (FARPROC)GetProcAddress(hLib, aszCtlDlg)) != NULL)
               CtlFn(wnd);
            FreeLibrary(hLib);
            }
         return 0L;
      
      case WM_COMMAND:
         switch(wParam) {
            case IDC_TIMBRESET:
               ResetTimbres();
               break;
               
            case IDC_WRITEPARMS:
               WriteParms();
               break;
            }
         break;
         
      case WM_CLOSE:
         DestroyWindow(wnd);
         return 0L;
   
      case WM_DESTROY:          
         PostQuitMessage(0);
         return 0L;

     case WM_SYSCOMMAND:
         break;
        
      default:
         break;
      }

   return DefWindowProc(wnd, message, wParam, lParam);
   }  

/* -------------------------------------------------------------------- */    
void ErrorBox(LPSTR msg) {
   MessageBox(hwndTask, msg, "Error",
                     MB_OK | MB_ICONSTOP | MB_TASKMODAL);
   }

/* -------------------------------------------------------------------- */    

LPSTR GetRStr(short id, LPSTR buf, UINT cb) { 
   
   LoadString(hInstance, id, buf, cb);
   return buf;
   }
