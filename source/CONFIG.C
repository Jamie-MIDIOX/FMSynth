/****************************************************************************
 *
 *   config.c
 *
 *   Copyright (c) 1993 by Jamie O'Connell
 *
 *   Adapted for FM Synth 1993 Jamie O'Connell
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "fmstring.h"
#include "fmsynth.h"
#include "fmcfg.h"
#include "init.h"
#include "config.h"

static char _based(_segname("_CODE")) aszHexFormat[] = "%X";
static char _based(_segname("_CODE")) aszIntFormat[] = "%d";
static char _based(_segname("_CODE")) aszCmdFmt[] = "%s\\%s";
static char _based(_segname("_CODE")) aszAskMapper[]
                       = "OK to append a new MIDI Mapper setup?\n"
                         "(doing so will not harm your existing setup)";
static char _based(_segname("_CODE")) aszMapperCfg[] = "ADDMAPFM.EXE";
static char _based(_segname("_CODE")) aszWindows[]   = "Windows";
static char _based(_segname("_CODE")) aszRun[]       = "Run";

BOOL fConfig = FALSE;

/*****************************************************************************

    internal function prototypes

 ****************************************************************************/

static void RegisterStartup(void);

static int NEAR PASCAL PortToId(WORD wBPort) {
    switch(wBPort) {
        case 0x220:  return IDC_220;
        case 0x228:  return IDC_228;
        case 0x240:  return IDC_240;
        case 0x388:  return IDC_388;
        default:     return IDC_OTHR;
        }                              
    }

static WORD NEAR PASCAL IdToPort(HWND hDlg, int id) {
char   szBuffer[20];
WORD   wResult;

    switch(id) {
        case IDC_220:  return 0x220;
        case IDC_228:  return 0x228;
        case IDC_240:  return 0x240;
        case IDC_388:  return 0x388;
        case IDC_OTHR: 
             GetDlgItemText(hDlg, IDC_EPORT, 
                           (LPSTR)szBuffer, sizeof(szBuffer));
             wResult = StrToHex(szBuffer);
             if (wResult > 0)
                 return wResult;
        default:       
             return (WORD)-1;
        }
    }

static int NEAR PASCAL ChipToId(BYTE bInt) {
    switch(bInt) {
        case 1:  return IDC_1OPL2;
        case 2:  return IDC_2OPL2;
        case 3:  return IDC_1OPL3;
        default: return -1;
        }
    }

static BYTE NEAR PASCAL IdToChip(int id) {
    switch(id) {
        case IDC_1OPL2:  return 1;
        case IDC_2OPL2:  return 2;
        case IDC_1OPL3:  return 3;
        default:     return (BYTE)-1;
        }
    }

/***************************************************************************/

static void NEAR PASCAL ConfigErrorMsgBox(HWND hDlg, WORD wStringId) {
   char szErrorBuffer[MAX_ERR_STRING];    /* buffer for error messages */

    LoadString(ghInstance, wStringId, szErrorBuffer, sizeof(szErrorBuffer));
    MessageBox(hDlg, szErrorBuffer, aszProductName, MB_OK|MB_ICONEXCLAMATION);
    }

/***************************************************************************/

void FAR PASCAL ConfigRemove(void) {
    WritePrivateProfileString(aszDriverName, NULL, NULL, aszSysProfileNm);
    }

/****************************************************************************
 *
 ***************************************************************************/
 
int FAR PASCAL Config(HWND hWnd, HANDLE hInstance) {
    short iRslt;
    
    iRslt = DialogBox(hInstance, MAKEINTATOM(DLG_CONFIG), 
                      hWnd, (DLGPROC)ConfigDlgProc);
    return iRslt; 
    }

/****************************************************************************
 *
 ***************************************************************************/

static DWORD NEAR PASCAL GetPortAndChip(HWND hDlg)
{
WORD wNewPort   = (WORD)-1;      /* new port chosen by user in config box */
BYTE bNewChip   = (BYTE)-1;      /* new card type chosen */
int  id;

    for (id = IDC_FIRSTPORT; id <= IDC_LASTPORT; id++)
        if (IsDlgButtonChecked(hDlg, id)) {
            wNewPort = IdToPort(hDlg, id);
            break;
            }

    for (id = IDC_FIRSTCHIP; id <= IDC_LASTCHIP; id++)
        if (IsDlgButtonChecked(hDlg, id)) {
            bNewChip = IdToChip(id);
            break;
            }

    return MAKELONG(wNewPort, bNewChip);
    }

/****************************************************************************
 *     Returns DRV_RESTART if the user has changed settings, which will
 *     cause the drivers applet which launched this to give the user a
 *     message about having to restart Windows for the changes to take
 *     effect.  If the user clicks on "Cancel" or if no settings have changed,
 *     DRV_CANCEL is returned.
 ***************************************************************************/

int FAR PASCAL _loadds ConfigDlgProc(HWND hDlg, WORD msg, 
                                     WORD wParam, LONG lParam) {
     static char aszPathBuf[80];
     static char aszMsgBuf[80];
     static char aszHelpFile[80];     
     char    buf[20];            /* buffer to write profile string into */
     DWORD   dw;                 /* return value from GetPortAndInt */
     WORD    wNewPort;           /* new port chosen by user in config box */
     BYTE    bNewChip;           /* new interrupt chosen */
     int     id;  
     HWND    hEdit;

     switch (msg) {
        case WM_INITDIALOG:
            fConfig  = TRUE;
            bNewChip = (BYTE)GetChipSet();
            wNewPort = GetBaseAddr(); // this also sets wPort

            id = PortToId(wNewPort);
            CheckRadioButton(hDlg, IDC_FIRSTPORT, IDC_LASTPORT, id);
            if (id == IDC_OTHR) {
                wsprintf(buf, aszHexFormat, wNewPort);                
                SetDlgItemText(hDlg, IDC_EPORT, (LPSTR)buf);
                }
            id = ChipToId(bNewChip);
            CheckRadioButton(hDlg, IDC_FIRSTCHIP, IDC_LASTCHIP, id);
            break;
            
       case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    Disable();
                    dw = GetPortAndChip(hDlg);
                    wNewPort = LOWORD(dw);
                    bNewChip = (BYTE)HIWORD(dw);

                    /*  verify settings - if this fails, DO NOT WRITE INI */
                    /*  SETTINGS! */
                    
                    wWriteDelay1 = 40; // set these prior (looong delay)
                    wWriteDelay2 = 280;
                    wFMChipAddr  = wNewPort;
                    wPort        = wNewPort;
                    wLeftFMChip  = wNewPort;
                    wRiteFMChip  = wNewPort + RCHIPOFS;
                    if (!BoardInstalled()) {                    
                        ConfigErrorMsgBox(hDlg, IDS_ERRBADPORT);
                        wLeftFMChip = GetBaseAddr();
                        wNewPort    = wLeftFMChip;
                        wRiteFMChip = wNewPort + RCHIPOFS;
                        id = PortToId(wNewPort);
                        CheckRadioButton(hDlg, IDC_FIRSTPORT, 
                                         IDC_LASTPORT, id);
                        if (id == IDC_OTHR) {
                            wsprintf(buf, aszHexFormat, wNewPort);
                            SetDlgItemText(hDlg, IDC_EPORT, (LPSTR)buf);
                            }
                        break;
                        }

                    /* settings seem valid, so write them out */
                    wsprintf(buf, aszHexFormat, wNewPort);
                    WritePrivateProfileString(aszDriverName, aszBasePort, 
                                              buf, aszSysProfileNm);
                    wsprintf(buf, aszIntFormat, bNewChip);
                    WritePrivateProfileString(aszDriverName, aszCardType, 
                                              buf, aszSysProfileNm);

                    fConfig = FALSE;
                    
                    if (MessageBox(hDlg, aszAskMapper, aszProductName, 
                                   MB_YESNO|MB_ICONQUESTION) == IDYES) 
                        RegisterStartup();  
                    EndDialog(hDlg, DRVCNF_RESTART);
                    break;

                case IDCANCEL:
                    fConfig = FALSE;                    
                    EndDialog(hDlg, DRVCNF_CANCEL);
                    break;

                case IDC_220:
                case IDC_228:
                case IDC_240:
                case IDC_388:
                    CheckRadioButton(hDlg, IDC_FIRSTPORT, 
                                     IDC_LASTPORT, wParam);
                    break;

                case IDC_OTHR:
                    CheckRadioButton(hDlg, IDC_FIRSTPORT, 
                                     IDC_LASTPORT, IDC_OTHR);
                    hEdit = GetDlgItem(hDlg, IDC_EPORT);
                    SetFocus(hEdit);
                    SendMessage(hEdit, EM_SETSEL, 
                                0, (LPARAM)(MAKELONG(0, 32767)));
                    break;
                
                case IDC_1OPL2:
                case IDC_2OPL2:
                case IDC_1OPL3:
                    CheckRadioButton(hDlg, IDC_FIRSTCHIP, 
                                     IDC_LASTCHIP, wParam);
                    break;

                case IDC_HELP:
                    WinHelp(hDlg, aszHelpPath, HELP_KEY, (DWORD)aszInstall);
                    break;

                default:
                    break;
                }
            break;

        default:
            return FALSE;
        }

    return TRUE;
    }

void RegisterStartup(void) {
   char RunTxt[256];
   
   GetProfileString(aszWindows, aszRun, "\0", RunTxt, sizeof(RunTxt));
   if (RunTxt[0] != '\0')
      strcat(RunTxt, " ");
   strcat(RunTxt, aszMapperCfg);
   WriteProfileString(aszWindows, aszRun, RunTxt);
   }
   

