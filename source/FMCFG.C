//--------------------------------------------------------------------------
// FMCFG.C
// 
// Copyright (c) 1994 by Jamie O'Connell
// 940402 Jamie O'Connell - This is now DLL
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>

#define  DEFINE_STR
#include "fmstring.h"
#include "fmsynth.h"
#include "fmvers.h"
#include "ctl3d.h"
#include "mscrdll.h"
#include "spinedit.h"
#include "fmcfg.h"
#include "fmapi.h"

/*non-localized strings */

char BCODE aszMenuOT[]     = "&Stay On Top";
char BCODE BnkNm[MAXBANK+1][10] = {"Bank1", "Bank2", "Bank3", "Bank4", 
                                  "Bank5", "PercBank"};
char BCODE aszIBKPair[30]  = "IBK Bank Files (*.ibk)\0*.ibk\0";
char BCODE aszTBTitle[]    = "IBK Bank File";
char BCODE aszIBK[]        = "ibk";
char BCODE aszCmdFmt[]     = "%s %s\\%s";
char BCODE aszVersFmt[]    = "FM Synth MIDI Driver  Vers. %d.%02d";
char BCODE aszOnTop[]      = "StayOnTop";

//---------------------------------------------------------------------------
// Global Variables...
//---------------------------------------------------------------------------

HINSTANCE ghInstance = NULL;       // Global instance handle for application
HWND      ghDlg  = NULL;
BOOL      fOnTop = FALSE;
HMENU     hSysMenu;
RECT      rPos;
// DLGPROC lpfnAbtDlg;
DLGPROC lpfnDlgProc;

static BOOL   BnkInit = TRUE;
static BOOL   fStereo;
static BOOL   fEnable;
static BOOL   fPercMode;
static BOOL   fVib;
static BOOL   fTrem;
static WORD   wVibDepth;
static WORD   wTrmDepth;
static WORD   wVal;
static BOOL   btrans;
static BOOL   fModesw;     
static BOOL   fSave;          
static BOOL   fPerc;
static BOOL   fPitchFixed;          
static HANDLE hBLBox;
static HANDLE hBrws;
static HANDLE hRmv;
static WORD   wChipType;
static WORD   wPercChan;
static WORD   wBendRange;
static short  nVox;
static short  nPercBoost;
static char   BnkPath[MAXBANK+1][128];
static BOOL   BnkMod[MAXBANK+1];
static char   FNBuf[128];
static BOOL   ChanMap[16];
static SYSPARMS SysParm;

#ifdef DEBUG
   WORD wDebugLevel = 1;
#endif

//---------------------------------------------------------------------------
// Function declarations
//---------------------------------------------------------------------------

BOOL FAR PASCAL  FMCtlDlg(HWND hDlg, UINT wMsg, 
                              WPARAM wParam, LPARAM lParam);
static WORD NEAR CalcNVox(void);
static void InitOpenName(HWND hDlg, LPOPENFILENAME lpOFN, LPSTR lpCur);
static WORD NEAR GetChipType(void);
static BOOL NEAR GetOnTop(void);
static BOOL NEAR isOnTop(void);
static void NEAR SetOnTop(BOOL flag, BOOL bWrite);

// BOOL FAR PASCAL _export 
//         AbtDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int CALLBACK LibMain(HINSTANCE hinst, WORD wDataSeg, 
                     WORD cbHeapSize, LPSTR lpszCmdLine);

//************************************************************
int CALLBACK LibMain(HINSTANCE hinst, WORD wDataSeg, 
                     WORD cbHeapSize, LPSTR lpszCmdLine) {
   ghInstance = hinst;
   if (FRegisterControl(hinst)) 
       return hinst;
   return 0;
   }

/*
 *  WEP - Generic for a DLL.  Doesn't do a whole lot.
 */
int FAR PASCAL WEP(WORD wParam) {
    return TRUE;
    }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

int FAR PASCAL  DoFMCtlDlg(HWND hParent) {
   static BOOL reenter = FALSE;
   
   if (!reenter) {
      reenter = TRUE;
      DialogBox(ghInstance, MAKEINTATOM(DLG_FMCTL), hParent, FMCtlDlg);
      reenter = FALSE;
      }
   else {
      if (IsIconic(ghDlg))
          ShowWindow(ghDlg, SW_RESTORE); // Restore
      else
          SetActiveWindow(ghDlg);        // Activate
      }
   return(0);
   }


//===========================================================================
// AbtDlg
//
// Dialog Callback
//
//===========================================================================
#if 0

BOOL FAR PASCAL _export AbtDlg(HWND hwnd, UINT msg, 
                               WPARAM wParam, LPARAM lParam) {
   switch(msg) {
      case WM_INITDIALOG:
         SetDlgItemText(hwnd, IDVERSTR, szVerStr);
         return(TRUE);

      case WM_CLOSE:
         EndDialog(hwnd, 0);
         return(TRUE);

        case WM_DLGBORDER:
        //
        // Don't draw the 3-D frame.
        //
            *(int FAR*)(lParam) = CTL3D_NOBORDER;
            break;
            
      case WM_COMMAND:
         switch(wParam) {
            case IDOK:
               EndDialog(hwnd, 0);
               return(TRUE);
            }
         break;
      }
   return(FALSE);
   }


#endif

//==================================================================
                                
WORD NEAR CalcNVox(void) {
    short iVal;
    
    iVal = fPercMode ? NVOX_PERCMODE : NVOX_MELOMODE;
    
    switch(GetChipType()) {
        case TWOOPL2:
        case ONEOPL3:        
            if (!fStereo)
                iVal = fPercMode ? NVOX_PERCPRO : NVOX_MELOPRO;
            break;
        
        default:
            break;
        }
    return iVal;
    }
    
//==================================================================
// little thing to reset Bank list

void FAR SetBnkInit(BOOL flag) {
    BnkInit = flag;
    }

BOOL FAR PASCAL  FMCtlDlg(HWND hDlg, UINT wMsg, 
                                WPARAM wParam, LPARAM lParam) {
     static char aszPathBuf[80];
     static char aszCmdLine[128];     
     static char aszVersStr[64];
     static HMENU hSysMenu = NULL;
     static HICON hIcon = NULL;
     static HICON hOldIcon = NULL;     
     short  ii;
     short  idx;
     OPENFILENAME ofn;
     LPOPENFILENAME lpOFN = &ofn;
     BOOL fNewState;
     
     switch (wMsg) {
        case WM_INITDIALOG:
            Ctl3dRegister(ghInstance);
            Ctl3dSubclassDlgEx(hDlg, CTL3D_ALL);
            ghDlg = hDlg;
            hIcon = LoadIcon(ghInstance, aszIconName);
            hOldIcon = SetClassWord(hDlg, GCW_HICON, hIcon);
            hSysMenu = GetSystemMenu(hDlg, FALSE);            
            AppendMenu(hSysMenu, MF_SEPARATOR, 0, (LPSTR) NULL);
            AppendMenu(hSysMenu, MF_STRING, IDM_ONTOP, aszMenuOT);
            if (GetOnTop()) // initially set
               PostMessage(hDlg, WM_SYSCOMMAND, IDM_ONTOP, 0L);
 
            wsprintf(aszVersStr, aszVersFmt, DRIVER_MAJOR, DRIVER_MINOR);
            SetDlgItemText(hDlg, IDC_VERSTR, aszVersStr);
                         
            GetParm(&SysParm, sizeof(SYSPARMS));            

            // Set up the Stereo Button 
            
            wChipType = GetChipType();
            if (wChipType > ONEOPL2) {
                fStereo = SysParm.fStereo; 
                fEnable = TRUE;
                }
            else 
                fStereo = fEnable = FALSE;
            CheckDlgButton(hDlg, IDC_STEREO, (WORD)fStereo);
            EnableWindow(GetDlgItem(hDlg, IDC_STEREO), fEnable);
            
            // Set up the Percussion Button
            fPercMode = SysParm.fPercussion; 
            CheckDlgButton(hDlg, IDC_PERCUSSION, (WORD)fPercMode);
            wPercChan = SysParm.PerChannel; // Show Common form (1 based)
            EnableWindow(GetDlgItem(hDlg, IDC_PERCHAN), fPercMode);
            nPercBoost = (short)SysParm.PercBoost;
            EnableWindow(GetDlgItem(hDlg, IDC_PERCBOOST), fPercMode);
            fPitchFixed = SysParm.fPitchFixed;
            CheckDlgButton(hDlg, IDC_FIXEDPITCH, (WORD)fPitchFixed);

            // Spin edit controls
            if (!SetupSpinEdit(hDlg, ghInstance, IDC_PERCHAN, 
                     ID_PCHN_SPIN, 1, 16, wPercChan+1, FALSE))
               break;
            if (!SetupSpinEdit(hDlg, ghInstance, IDC_PERCBOOST, 
                     ID_PBST_SPIN, (WORD)-63, 63, (WORD)nPercBoost, FALSE))
               break;
            
            // Set up the Vibrato Buttons
            wVibDepth = SysParm.fVibDepth;
            wTrmDepth = SysParm.fTremDepth;
            CheckDlgButton(hDlg, IDC_VIB, wVibDepth);
            CheckDlgButton(hDlg, IDC_TREM, wTrmDepth);
             
            // Set up Bend Range
            if (!SetupSpinEdit(hDlg, ghInstance, IDC_BEND, 
                         ID_BND_SPIN, 0, 24, SysParm.BendRange, FALSE))
               break;
            
            // Show Number Voices
            SetDlgItemInt(hDlg, IDC_NVOC, CalcNVox(), FALSE);
            
            // Get Current Banks and fill list box
            hBLBox = GetDlgItem(hDlg, IDC_BNKLBOX);
            SendMessage(hBLBox, LB_RESETCONTENT, 0, 0);
            for (ii = 0; ii < MAXBANK+1; ++ii) {
                BnkMod[ii] = FALSE;
                if (BnkInit) 
                   GetBankPath(ii, BnkPath[ii], 128);  
                SendMessage(hBLBox, LB_ADDSTRING, 0, 
                                     (LONG)(LPSTR)BnkNm[ii]);
                }
            BnkInit = FALSE; // don't get again, unless reset
            CheckDlgButton(hDlg, IDC_SAVESET, SysParm.fSaveSettings);
            // Get current State of MIDI channels
            for (ii = 0; ii < 16; ++ii) 
                CheckDlgButton(hDlg, IDC_CHAN1+ii, (WORD)SysParm.ChanMap[ii]);
            break;
            // SendMessage(hinLBox, LB_SETSEL, TRUE, MAKELPARAM(i, 0));
        
        case WM_DLGBORDER:
            if (IsIconic(hDlg)) // don't draw around Icon
               *(int FAR *)(lParam) = CTL3D_NOBORDER;
            return FALSE;       // not handled    
            
        case WM_SYSCOLORCHANGE:
            Ctl3dColorChange();
            break;

        case WM_SYSCOMMAND:
            switch (wParam) { 
               case IDM_ONTOP:                         
                  fNewState = (GetMenuState(hSysMenu, IDM_ONTOP, MF_BYCOMMAND)
                               & MF_CHECKED) ? FALSE : TRUE;
                  // Toggle the state of the item. 
                  CheckMenuItem(hSysMenu, IDM_ONTOP, MF_BYCOMMAND | 
                                     (fNewState ? MF_CHECKED : MF_UNCHECKED));
                  SetWindowPos(hDlg, (fNewState ? 
                                  HWND_TOPMOST : HWND_NOTOPMOST),
                                  0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE));
                  SetOnTop(fNewState, FALSE); // just set - don't write  
                  break; // out of switch
                     
               case SC_MAXIMIZE:
                  break; // ignore this command
                     
               default:                    
                  return FALSE; // Not handled
               }
            break;

        case WM_COMMAND:
            switch (wParam) {                      
                case IDC_STEREO:
                     if (fEnable) // Toggle Value
                         fStereo = !IsDlgButtonChecked(hDlg, IDC_STEREO);
                     CheckDlgButton(hDlg, IDC_STEREO, (fStereo ? 1:0));
                     break;
                     
                case IDC_PERCUSSION:
                     fPercMode = !IsDlgButtonChecked(hDlg, IDC_PERCUSSION);
                     EnableWindow(GetDlgItem(hDlg, IDC_PERCHAN), fPercMode);
                     EnableWindow(GetDlgItem(hDlg, IDC_PERCBOOST), fPercMode);
                     CheckDlgButton(hDlg, IDC_PERCUSSION, (WORD)fPercMode); 
                     break;

                case IDC_PERCHAN:
                case IDC_PERCBOOST:
                case IDC_VIB:
                case IDC_TREM:
                case IDC_BEND:
                case IDC_FIXEDPITCH:
                case IDC_REGSHADOW:
                     break;

                case IDC_BNKLBOX:
                     if (HIWORD(lParam) == LBN_SELCHANGE) {
                        hBrws = GetDlgItem(hDlg, IDC_BROWSE);
                        if (!IsWindowEnabled(hBrws))
                            EnableWindow(hBrws, TRUE);
                        hRmv = GetDlgItem(hDlg, IDC_REMOVE);
                        if (!IsWindowEnabled(hRmv))
                            EnableWindow(hRmv, TRUE);
                        idx = (short)SendMessage(LOWORD(lParam), 
                                                 LB_GETCURSEL, 0, 0L);
                        EnableWindow(GetDlgItem(hDlg, IDC_DFTBANK), 
                                                   (idx != PRCBANK));
                        SetDlgItemText(hDlg, IDC_BNKPATH, BnkPath[idx]);
                        CheckDlgButton(hDlg, IDC_DFTBANK, 
                                     (WORD)(idx == (short)SysParm.DftBank)); 
                        }
                     else if (HIWORD(lParam) == LBN_DBLCLK) 
                        SendMessage(hDlg, WM_COMMAND, IDC_BROWSE, 0L); 
                     break;
                
                case IDC_BROWSE:
                     // do file open 
                    hBLBox = GetDlgItem(hDlg, IDC_BNKLBOX);
                    idx = (short)SendMessage(hBLBox, LB_GETCURSEL, 0, 0L);
                    lstrcpy(FNBuf, BnkPath[idx]); // save for compare
                    if (lstrcmp(BnkPath[idx], aszInternal) == 0)
                        *BnkPath[idx] = '\0';     // Blow it away
                    InitOpenName(hDlg, lpOFN, BnkPath[idx]);
                    GetOpenFileName(lpOFN);
                    if (*BnkPath[idx] == '\0')    // fill it back in
                        lstrcpy(BnkPath[idx], aszInternal);
                    else
                        AnsiLower(BnkPath[idx]);
                    if (lstrcmp(FNBuf, BnkPath[idx]) != 0) {
                        BnkMod[idx] = TRUE;       // Indicate Modified
                        SetDlgItemText(hDlg, IDC_BNKPATH, BnkPath[idx]);
                        }
                    break;
                     
                case IDC_REMOVE: 
                    hBLBox = GetDlgItem(hDlg, IDC_BNKLBOX);
                    idx = (short)SendMessage(hBLBox, LB_GETCURSEL, 0, 0L);
                    lstrcpy(BnkPath[idx], aszInternal);
                    BnkMod[idx] = TRUE;
                    SetDlgItemText(hDlg, IDC_BNKPATH, BnkPath[idx]);
                    break;
                    
                case IDC_DFTBANK:
                    // See if we change the current Bank...
                    if (HIWORD(lParam) == BN_CLICKED) {
                        if (IsDlgButtonChecked(hDlg, IDC_DFTBANK)) 
                           CheckDlgButton(hDlg, IDC_DFTBANK, FALSE);
                        else {
                           hBLBox = GetDlgItem(hDlg, IDC_BNKLBOX);
                           idx = (short)SendMessage(hBLBox,LB_GETCURSEL,0,0L);
                           idx %= MAXBANK;
                           SysParm.DftBank = (BYTE)idx;
                           CheckDlgButton(hDlg, IDC_DFTBANK, TRUE);
                           }
                        }
                    break;
                    
                case IDC_RESET:  // Hard reset of driver                     
                    SoundFullInit(); // fall through
                 
                case IDC_SET:    // !!! Whole thing needs to change !!!
                    fSave   = IsDlgButtonChecked(hDlg, IDC_SAVESET);
                    fStereo = IsDlgButtonChecked(hDlg, IDC_STEREO);
                    fPerc   = IsDlgButtonChecked(hDlg, IDC_PERCUSSION);
                    wPercChan = GetDlgItemInt(hDlg, IDC_PERCHAN, 
                                              &btrans, FALSE)-1;
                    nPercBoost = GetDlgItemInt(hDlg, IDC_PERCBOOST, 
                                              &btrans, TRUE);
                    wVibDepth = IsDlgButtonChecked(hDlg, IDC_VIB);
                    wTrmDepth = IsDlgButtonChecked(hDlg,  IDC_TREM);
                    fPitchFixed = IsDlgButtonChecked(hDlg, IDC_FIXEDPITCH);
                    
                    SetOnTop(isOnTop(), fSave); // remember last setting
                    fModesw = (SysParm.fStereo != fStereo);
                    SysParm.fSaveSettings = (BYTE)fSave;
                    SysParm.fStereo       = (BYTE)fStereo;
                    SysParm.fPercussion   = (BYTE)fPerc;
                    SysParm.PerChannel    = (BYTE)wPercChan; 
                    SysParm.PercBoost     = (char)nPercBoost;
                    SysParm.fPitchFixed   = (BYTE)fPitchFixed;
                    SysParm.fRegShadow    = (BYTE)FALSE;
                    SysParm.fVibDepth     = (BYTE)wVibDepth; 
                    SysParm.fTremDepth    = (BYTE)wTrmDepth;                 
                    SysParm.BendRange     = 
                        (BYTE)GetDlgItemInt(hDlg, IDC_BEND, &btrans, FALSE);
                    MIDIOff();
                    for (ii = 0; ii < MAXBANK; ++ii) {
                        if (BnkMod[ii]) {
                            UpdBank(ii, BnkPath[ii], fSave);
                            BnkMod[ii] = FALSE;
                            }
                        }
                        
                    // Check the percussion Bank                     
                    if (BnkMod[PRCBANK]) { 
                        UpdBank(PRCBANK, BnkPath[PRCBANK], fSave);
                        BnkMod[PRCBANK] = FALSE;
                        }
 
                    for (ii = 0; ii < 16; ++ii) {
                         ChanMap[ii] = 
                            (BOOL)IsDlgButtonChecked(hDlg, IDC_CHAN1+ii);
                         SysParm.ChanMap[ii] = ChanMap[ii];
                         }

                    // Will do all the updating
                    SetParm(&SysParm, sizeof(SYSPARMS));
                    MIDIOn();
                    break;

                case IDC_TEST:       // Make some noise! 
                    // Right in here!!!                    
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
                
                case IDC_HELP:
                    WinHelp(hDlg, aszHelpPath, HELP_KEY, (DWORD)aszControl);  
                    break;

                default:
                    return FALSE; // not handled
                } 
                
            // Show Number Voices
            SetDlgItemInt(hDlg, IDC_NVOC, CalcNVox(), FALSE);
            break;
        
        case WM_DESTROY:
            WinHelp(hDlg, aszHelpPath, HELP_QUIT, 0L);
            SetClassWord(hDlg, GCW_HICON, hOldIcon);
            DestroyIcon(hIcon);
            GetSystemMenu(hDlg, TRUE); // Revert
            Ctl3dUnregister(ghInstance);
            D1("WM_DESTROY - Task Dialog");
            break;
         
        default:
            return FALSE; // didn't handle
        }
   
    return TRUE; // Handled
    }

//=========================================================================

void InitOpenName(HWND hDlg, LPOPENFILENAME lpOFN, LPSTR lpCur) {

    lpOFN->lStructSize = sizeof(OPENFILENAME);
    lpOFN->hwndOwner   = hDlg; 
    lpOFN->hInstance   = ghInstance;
    lpOFN->lpstrFilter = aszIBKPair;
    lpOFN->lpstrCustomFilter = NULL;
    lpOFN->nMaxCustFilter  = 0;
    lpOFN->nFilterIndex    = 0;
    lpOFN->lpstrFile       = lpCur; 
    lpOFN->nMaxFile        = 128;
    lpOFN->lpstrFileTitle  = NULL;       
    lpOFN->nMaxFileTitle   = 0;
    lpOFN->lpstrInitialDir = NULL;
    lpOFN->lpstrTitle      = aszTBTitle;
    lpOFN->Flags = (OFN_FILEMUSTEXIST | OFN_SHOWHELP | OFN_HIDEREADONLY);    
    lpOFN->nFileOffset     = 0;
    lpOFN->nFileExtension  = 0;
    lpOFN->lpstrDefExt     = aszIBK;        
    lpOFN->lCustData       = NULL;
    lpOFN->lpfnHook        = NULL;
    lpOFN->lpTemplateName  = NULL;
    
    return;
    }                   
    
//****************************************************************************

BOOL NEAR  GetOnTop(void) {
    return (fOnTop = (BOOL)GetPrivateProfileInt(aszAppSect, aszOnTop, 
                                      TRUE, aszProfileNm));
    }

BOOL NEAR isOnTop(void) {
    return fOnTop;
    }

//*************************************************************************
 
void NEAR SetOnTop(BOOL flag, BOOL bWrite) {
    if (bWrite) 
       WritePrivateProfileString(aszAppSect, aszOnTop, 
                                 (flag ? "1":"0"), aszProfileNm);
    fOnTop = flag;
    }

WORD NEAR GetChipType(void) {
    return GetPrivateProfileInt(aszDriverName, aszCardType, 
                                ONEOPL2, aszSysProfileNm); 
    }

