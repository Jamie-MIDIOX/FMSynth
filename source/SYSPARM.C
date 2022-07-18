// SysParm -- Module to handle system Parameters
// Copyright (c) 1993 by Jamie O'Connell

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include "fmstring.h"
#include "fmsynth.h"
#include "fmsfunc.h"
#include "midimain.h"
#include "init.h"
#include "sysparm.h"

static PARMSTORE parmStore;
    
static HWND      hwndTask    = NULL;
static BOOL      fSaveSet    = TRUE;
static BOOL      useOldest   = FALSE;
static BOOL      fOPL3       = FALSE;
static BOOL      fPro        = FALSE;
static BOOL      fStereo     = FALSE;
static BOOL      fPercussion = FALSE;
static BOOL      fPercSw     = FALSE;
static BOOL      fRegShadow  = FALSE;
static BOOL      fPitchFix   = FALSE;
static WORD      wModnDepth  = DFTDEPTH; 
static WORD      wDftBank    = DFTBANK;
static WORD      wPercChan   = PERC_CHAN;
static WORD      wBendRange[MAXCHAN]  = {DFTRANGE, DFTRANGE, DFTRANGE,
                     DFTRANGE, DFTRANGE, DFTRANGE, DFTRANGE, DFTRANGE, 
                     DFTRANGE, DFTRANGE, DFTRANGE, DFTRANGE, DFTRANGE,
                     DFTRANGE, DFTRANGE, DFTRANGE};            // Bend Range
static BOOL      ChanMap[MAXCHAN];

//****************************************************************************

BOOL FAR  GetStereo(void) {
    return (fStereo = (BOOL)GetPrivateProfileInt(aszModeSect, aszStereoMode, 
                                                 FALSE, aszProfileNm));
    }

//****************************************************************************

BOOL FAR  GetPercussion(void) {
    return (fPercussion = (BOOL)GetPrivateProfileInt(aszModeSect, 
                                      aszPercusMode, TRUE, aszProfileNm));
    }

//****************************************************************************

WORD FAR  GetPercChannel(void) {
    return (wPercChan = (GetPrivateProfileInt(aszModeSect, 
                          aszPercChannel, PERC_CHAN+1, aszProfileNm) - 1));
    }
    
//****************************************************************************

short FAR  GetPercBoost(void) {
    return (nPercBoost = GetPrivateProfileInt(aszModeSect, 
                          aszPercBoost, DFTBOOST, aszProfileNm));
    }

//****************************************************************************

WORD FAR  GetPercSwitch(void) {
    return (fPercSw = (BOOL)GetPrivateProfileInt(aszModeSect, 
                          aszOldPercSw, FALSE, aszProfileNm));
    }

//****************************************************************************
// Do Shadow Reg?
WORD FAR  GetRegShadow(void) {
    return (fRegShadow = (BOOL)GetPrivateProfileInt(aszModeSect, 
                          aszRegShadow, TRUE, aszProfileNm));
    }

//****************************************************************************
// Fixed percussion Pitch?

WORD FAR  GetFixedPitch(void) {
    return (fPitchFix = (BOOL)GetPrivateProfileInt(aszModeSect, 
                          aszFixPitch, TRUE, aszProfileNm));
    }

//****************************************************************************

WORD FAR  GetBendRange(void) {
    WORD iniRange;
    WORD ii;
    
    iniRange = GetPrivateProfileInt(aszRangeSect, 
                             aszBendRange, DFTRANGE, aszProfileNm);
    for (ii = 0; ii < MAXCHAN; ++ii)
         wBendRange[ii] = iniRange;
    return iniRange;
    }

//****************************************************************************
// Default is Heavy Vibrato, Light Tremolo

WORD FAR  GetModnDepth(void) {
    wModnDepth  = MKTREM(GetPrivateProfileInt(aszRangeSect, aszTremolo, 
                                                  0, aszProfileNm));
    wModnDepth |= MKVIBR(GetPrivateProfileInt(aszRangeSect, aszVibrato, 
                                                  1, aszProfileNm));
    return wModnDepth;
    }

//****************************************************************************
// Get Pathname (if Any)

void FAR PASCAL _loadds GetBankPath(short idx, LPSTR aszPath, WORD len) {
    char aszBankNr[16];

    if (idx < MAXBANK)
       wsprintf(aszBankNr, "Bank%d", idx+1);
    else if (idx == PRCBANK)
       lstrcpy(aszBankNr, aszPercBank);
    else
       return;
   
    GetPrivateProfileString(aszBankSect, aszBankNr, aszInternal, 
                            aszPath, len, aszProfileNm);
    }

//*************************************************************************  

void FAR  GetChanMap(void) {
    static char aszChanMap[40];
    char *npc = aszChanMap;
    short idx;
    
    for (idx = 0; idx < 16; ++idx)
        ChanMap[idx] = FALSE;
    
    GetPrivateProfileString(aszChanSect, aszChanNm, aszDFTChan, 
                            aszChanMap, sizeof(aszChanMap), aszProfileNm);
    npc = strtok(aszChanMap, ","); // Parse off digits
    while(npc != NULL) {
        if ((idx = atoi(npc)) > 0)
            ChanMap[idx-1] = TRUE;
        npc = strtok(NULL, ",");
        }
    }

//****************************************************************************

WORD FAR  GetCurrentBank(void) {
    return (wDftBank = (WORD)(GetPrivateProfileInt(aszBankSect, aszDftBank, 
                                       DFTBANK, aszProfileNm)));
    }


//*************************************************************************

void FAR PASCAL SetUseOldest(BOOL flag, BOOL bWrite) {
    if (bWrite) 
       WritePrivateProfileString(aszModeSect, aszUseOld, 
                                 (flag ? "1":"0"), aszProfileNm);
    useOldest = flag;
    }
 
//****************************************************************************

BOOL FAR GetUseOldest(void) {
    short tmp;
    
    tmp = GetPrivateProfileInt(aszModeSect, aszUseOld, 3, aszProfileNm);
    if (tmp == 3) // defaulted
       SetUseOldest(TRUE, TRUE);
    else
       useOldest = (BOOL)tmp;       
    return useOldest;
    }

//*************************************************************************

BOOL FAR isOPL3(void) {
    return fOPL3;
    }

BOOL FAR isPro(void) {
    return fPro;
    }

BOOL FAR isStereo(void) {
    return fStereo;
    }

BOOL FAR useOldOff(void) {
    return useOldest;
    }
 
BOOL FAR isPercussion(void) {
    return fPercussion;
    }

BOOL FAR isPercSwitch(void) {
    return fPercSw;
    }

BOOL FAR isPercBoth(void) {
    return fPercWriteBoth;
    }

BOOL FAR isPitchFixed(void) {
    return fPitchFix;
    }

BOOL FAR isRegShadow(void) {
    return fRegShadow;
    }

BOOL FAR isChanOn(WORD wChan) {
     return ChanMap[wChan];
     }
     
WORD FAR  theBendRange(WORD wChan) {
    WORD Range;
    BYTE most;
    BYTE count[25];
    WORD ii;
   
    if (wChan == (WORD)-1) {
       _fmemset(&count[0], 0, 25);
       for (ii = 0; ii < MAXCHAN; ++ii) 
          count[wBendRange[ii]]++; // find the most frequent
       most  = 0;
       Range = 0;
       for (ii = 0; ii < 25; ++ii) {
          if (count[ii] > most) {
             most  = count[ii];
             Range = ii;
             }
          }
       }
    else
       Range = wBendRange[wChan];

    return Range;
    }

WORD FAR  theModnDepth(void) {
    return wModnDepth;
    }

WORD FAR  thePercChannel(void) {
    return wPercChan;
    }

WORD FAR  theCurrentBank(void) {
    return wDftBank;
    }

short FAR thePercBoost(void) {
    return nPercBoost;
    }

//*************************************************************************

void FAR PASCAL SetOPL3(BOOL flag) {
    fOPL3 = flag;
    }

void FAR PASCAL SetPro(BOOL flag) {
    fPro = flag;
    }

void FAR PASCAL SetPercBoth(BOOL fVal) {
    fPercWriteBoth = fVal;
    }

//*************************************************************************

BOOL FAR PASCAL SetStereo(BOOL flag, BOOL bWrite) {
    BOOL Chg = (flag != fStereo);
    if (bWrite) 
       WritePrivateProfileString(aszModeSect, aszStereoMode, 
                                 (flag ? "1":"0"), aszProfileNm);
    fStereo = flag;
    return Chg;
    }

//*************************************************************************

void FAR PASCAL SetPercussion(BOOL flag, BOOL bWrite) {
    if (bWrite)
        WritePrivateProfileString(aszModeSect, aszPercusMode, 
                                 (flag ? "1":"0"), aszProfileNm);
    fPercussion = flag;
    }
    
//*************************************************************************

void FAR PASCAL SetFixedPitch(BOOL flag, BOOL bWrite) {
    if (bWrite)
        WritePrivateProfileString(aszModeSect, aszFixPitch, 
                                 (flag ? "1":"0"), aszProfileNm);
    fPitchFix = flag;
    } 
    
//*************************************************************************

void FAR PASCAL SetRegShadow(BOOL flag, BOOL bWrite) {
    if (bWrite)
        WritePrivateProfileString(aszModeSect, aszRegShadow, 
                                 (flag ? "1":"0"), aszProfileNm);
    fRegShadow = flag;
    }

//*************************************************************************

void FAR PASCAL SetPercChannel(WORD wChan, BOOL bWrite) {
    char aszChan[4];
    
    if (bWrite) {
        wsprintf(aszChan, "%d", wChan+1); // Store Common form 
        WritePrivateProfileString(aszModeSect, aszPercChannel, 
                                  aszChan, aszProfileNm);
        }
    wPercChan = wChan;
    }

//*************************************************************************

void FAR PASCAL SetPercBoost(short nBoost, BOOL bWrite) {
    char aszBoost[4];
    
    if (nBoost > MAXBOOST)
       nBoost = MAXBOOST;
    if (nBoost < MINBOOST)
       nBoost = MINBOOST;
       
    if (bWrite) {
        wsprintf(aszBoost, "%d", nBoost); // Store Common form 
        WritePrivateProfileString(aszModeSect, aszPercBoost, 
                                  aszBoost, aszProfileNm);
        }
    nPercBoost = nBoost;
    }
    
//*************************************************************************

void FAR PASCAL SetModnDepth(WORD wDepth, BOOL bWrite) {
    if (bWrite) {
        WritePrivateProfileString(aszRangeSect, aszVibrato, 
                             (VIBRBIT(wDepth)?"1":"0"), aszProfileNm);
        WritePrivateProfileString(aszRangeSect, aszTremolo, 
                             (TREMBIT(wDepth)?"1":"0"), aszProfileNm);
        }
    wModnDepth = wDepth;
    }

//*************************************************************************

void FAR PASCAL SetBendRange(WORD wChan, WORD wRange, BOOL bWrite) {
    char aszBend[4];
    WORD ii;
    
    if (wRange > 24)
       wRange = 24;
    
    if (bWrite) {
        wsprintf(aszBend, "%d", wRange); 
        WritePrivateProfileString(aszRangeSect, aszBendRange, 
                                  aszBend, aszProfileNm);
        }                             
    if (wChan == (WORD)-1) { // set them all
        for (ii = 0; ii < MAXCHAN; ++ii)
            wBendRange[ii] = wRange;
        }
    else  
        wBendRange[wChan] = wRange;
    }

//*************************************************************************

void FAR PASCAL SetCurrentBank(WORD wBank, BOOL bWrite) {
    char aszVal[4];
    
    if (bWrite) {
        wsprintf(aszVal, "%d", wBank); 
        WritePrivateProfileString(aszBankSect, aszDftBank, 
                                  aszVal, aszProfileNm);
        }                             
    wDftBank = wBank;
    }

//****************************************************************************
// Get Pathname (if Any)

void FAR PASCAL SetBankPath(short idx, LPSTR aszPath) {
    char aszBankNr[16];

    if (idx < MAXBANK)
       wsprintf(aszBankNr, "Bank%d", idx+1);
    else if (idx == PRCBANK)
       lstrcpy(aszBankNr, aszPercBank);
    else
       return;

    WritePrivateProfileString(aszBankSect, aszBankNr, aszPath, aszProfileNm);
    }

//*************************************************************************  
void FAR PASCAL SetChanMap(LPINT ChanAry, BOOL bWrite) {
    static char aszChanMap[40];
    short ii;
    short slen;
    
    aszChanMap[0] = '\0'; // Zero First
    for (ii = 0; ii < 16; ++ii) {
        ChanMap[ii] = ChanAry[ii];
        if (ChanAry[ii]) { 
            slen = lstrlen(aszChanMap);
            wsprintf(&aszChanMap[slen], "%d,", ii+1); // Display common form
            }
        } 
    slen = lstrlen(aszChanMap);
    if (slen > 0)
        aszChanMap[slen-1] = '\0'; // Remove last comma    
                            // [Channel] ChanMap=1,2,3,4,5,6,7,8,9,10
    if (bWrite)                                 
        WritePrivateProfileString(aszChanSect, aszChanNm, 
                                  aszChanMap, aszProfileNm);
    }

//****************************************************************************

BOOL FAR  GetSaveSet(void) {
    return (fSaveSet = (BOOL)GetPrivateProfileInt(aszAppSect, aszSaveSet, 
                                      TRUE, aszProfileNm));
    }

BOOL FAR isSaveSet(void) {
    return fSaveSet;
    }

//*************************************************************************

void FAR PASCAL SetSaveSet(BOOL flag, BOOL bWrite) {
    if (bWrite) 
       WritePrivateProfileString(aszAppSect, aszSaveSet, 
                                 (flag ? "1":"0"), aszProfileNm);
    fSaveSet = flag;
    }

//************************************************************************* 
// Exported

void FAR PASCAL _loadds UpdBank(short ii, LPSTR lpszName, BOOL fSave) {  
   short jj;
   
   if (fSave) // only write if saving
       SetBankPath(ii, lpszName);
    
   if (ii == PRCBANK) {
      if (lstrcmp(lpszName, aszInternal) == 0)
          LoadDrumPatches();
      else 
          LoadDrumBank(lpszName);
      }
   else {
      if (lstrcmp(lpszName, aszInternal) == 0)
          LoadPatches(ii);
      else 
          LoadBank(lpszName, ii);
      for (jj = 0; jj < MAXTMB; ++jj) 
          UpdVoxTmb(&Tmb[ii].snd[jj], ii, jj);
      }
   }

//************************************************************************* 
void FAR PASCAL _loadds ResetTimbres(void) {  
    short ii, jj;
    
    LoadPatches(0);
    LoadDrumPatches();    
    LoadExtraBanks();
    
    for (ii = 0; ii < MAXBANK; ++ii) {
       for (jj = 0; jj < MAXTMB; ++jj) 
           UpdVoxTmb(&Tmb[ii].snd[jj], ii, jj);
       }
    }            
//************************************************************************* 
// simply saves the passed in state of the parameters and then updates
// without save.  A save may be indicated later by the Task.
// 
void FAR PASCAL SetParms(LPARMSTORE lpPS) {
    LPARMSTORE lpLocPS = &parmStore;
    
    *lpLocPS = *lpPS;        
    }

//************************************************************************* 
// Designed to be called by the Task -- write out the current state
// 
void FAR PASCAL _loadds WriteParms(void) {
    BOOL  fSave;
    BOOL  fModSw;
    short newBank;
    
    fSave    = parmStore.fSaveSet;
    fModSw   = SetStereo(parmStore.fStereo, fSave);
    SetPercussion(parmStore.fPercussion, fSave);
    SetPercChannel(parmStore.wPercChan, fSave);
    newBank  = parmStore.wDftBank; // maybe changed
    SetCurrentBank(newBank, fSave);
    updDftBank(newBank);           // force change
    SetModnDepth(parmStore.wModnDepth, fSave);
    SetBendRange((WORD)-1, parmStore.wBendRange, fSave);
    SetSaveSet(parmStore.fSaveSet, fSave);
    SetChanMap(&parmStore.ChanMap[0], fSave);

    if (parmStore.Vers == SYSPARMNEW) {
        SetPercBoost(parmStore.nPercBoost, fSave);
        SetFixedPitch(parmStore.fPitchFix, fSave);
        SetRegShadow(parmStore.fRegShadow, fSave);
        }   
    if (fModSw) { // do hard or soft reset
        setRunTime(isStereo());
        InitChannelInfo();  
        }
    SoundWarmInit();
    }

//************************************************************************* 

void FAR PASCAL _loadds StoreTask(HWND hwnd) {   
   hwndTask = hwnd;
   }

HWND FAR GethwndTask(void) {
  return hwndTask;
  }
