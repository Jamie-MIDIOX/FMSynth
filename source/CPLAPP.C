/****************************************************************************
 *
 *   CLPAPP.C
 *
 *   Copyright (c) 1993 by Jamie O'Connell
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <cpl.h>

#include "fmstring.h"
#include "fmsynth.h"
#include "fmcfg.h"

#ifndef BCODE
#define BCODE _based(_segname("_CODE"))
#endif
                   
/*non-localized strings */

char BCODE aszAppletName[] = "FM Synth";
char BCODE aszDesc[] = 
              "Allows run-time configuration of the FM Synth MIDI driver";

static BOOL   BnkInit = TRUE;
static BOOL   fStereo;
static BOOL   fEnable;
static BOOL   fPercMode;
static BOOL   fVib;
static BOOL   fTrem;
static WORD   wVibDepth;
static WORD   wVal;
static BOOL   btrans;
static BOOL   fModesw;     
static BOOL   fSave;          
static BOOL   fPerc;                    
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
static BOOL   imRunning = FALSE;

/***************************************************************************
 * Control Panel Applet -- Exported from driver
 *
 * Copyright 930416 by Jamie O'Connell
 ***************************************************************************/

static WORD NEAR CalcNVox(void);
static void InitOpenName(HWND hDlg, LPOPENFILENAME lpOFN, LPSTR lpCur);

LONG FAR PASCAL _loadds CPlApplet(HWND hCpl, UINT wMessage, 
                              LPARAM lParam1, LPARAM lParam2) {
    LPNEWCPLINFO lpCPI;
    LONG         liRet;
    
    switch (wMessage) {

        case CPL_INIT:
            D1("CPL_INIT");

            /*
               Sent to the applet when it is loaded. Always the first
               message received by the applet.

               lParam1 is 0L.
               lParam2 is 0L.
                
               Return 0L to fail the load. (that's what we do...)
            */
                        
            liRet = (LONG)GetPrivateProfileInt(aszAppSect, aszLoad, TRUE, 
                                                           aszProfileNm);
            return liRet;
            
        case CPL_GETCOUNT:
            D1("CPL_GETCOUNT");

            /* We tell Control Panel the number of applets

               lParam1 is 0L.
               lParam2 is 0L.
            */

            return (LONG)1;   // We only have one 

        case CPL_INQUIRE:
            D1("CPL_INQUIRE!");
             
            /*
             Obsolete
             */

            return 0L; // Return not handled

        case CPL_NEWINQUIRE:
            D1("CPL_NEWINQUIRE");
             
            /*
             Fill in info
             */
                         
            lpCPI = (LPNEWCPLINFO)lParam2;
            lpCPI->dwSize = sizeof(NEWCPLINFO);
            lpCPI->dwHelpContext = 0;
            lpCPI->lData = 0;
            lpCPI->hIcon = LoadIcon(ghInstance, aszIconName);
            lstrcpy(lpCPI->szName, aszAppletName);
            lstrcpy(lpCPI->szInfo, aszDesc);
            lstrcpy(lpCPI->szHelpFile, aszHelpPath);
                         
            return 0L;
            
        case CPL_SELECT:
            D1("CPL_SELECT");
            return 0L;

        case CPL_DBLCLK:
            D1("CPL_DBLCLK");
            DoFMCtlDlg(hCpl);
            return 0L;
            
        case CPL_STOP:
            D1("CPL_STOP");
            return 0L;  
            
        case CPL_EXIT:
            D1("CPL_EXIT");
            return 0L;

        default:
            return 0L;
        }                  
    }

