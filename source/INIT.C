/****************************************************************************
 *
 *   init.c
 *
 *   Copyright (c) 1993 by Jamie O'Connell
 * 
 *   Adapted for FM Synth: 930404 JWO
 * 
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#define  DEFINE_STR
#include "fmstring.h"
#include "fmsynth.h"
#include "sysparm.h"
#include "init.h"
#include "midimain.h"
#include "fmsfunc.h"
#include "fmsyntha.h"
#include "fmapi.h"

#define DEF286WRITEDELAY     8       /* 25MHz 286 */
#define DEF386WRITEDELAY    14       /* 50MHz 386 */
#define DEF486WRITEDELAY    41       /* 50MHz 486 */
#define DELAYMULT            7       /* second is 7 times first */

#define DEF286SHORTDELAY    5       /* 25MHz 286 */
#define DEF386SHORTDELAY    9       /* 50MHz 386 */
#define DEF486SHORTDELAY    16      /* 50MHz 486 */

extern int  FAR  PASCAL LibMain(HANDLE hInstance, WORD wHeapSize, 
                                LPSTR lpCmdLine);

/***************************************************************************

    internal function prototypes

***************************************************************************/

static int  NEAR SoundColdInit(void);
static void NEAR SoundChut(BYTE voice);
static WORD NEAR GetMinWait(void);
static WORD NEAR GetMinPercWait(void);
static BOOL NEAR TestHardware(void);
static void NEAR InitVoxInfo(void);
static void NEAR InitRegMap(void);
static void NEAR InitFM(void);
static void NEAR ResetFM(void);
static void PASCAL NEAR DrvLoadInitialize(void);
static void NEAR setDrums(BOOL fOn, WORD wChn);

/*************************************************************************

    public data

*************************************************************************/
HINSTANCE ghInstTask   = 0;
BOOL      fhaveTask    = FALSE;
BOOL      _MIDIDisable = FALSE;
HINSTANCE ghInstance   = 0;               /* our global instance */
HINSTANCE _hModule     = 0;               /* module handle - CTL3D */
WORD      wWriteDelay1 = DEF386WRITEDELAY;
WORD      wWriteDelay2 = DEF386WRITEDELAY * 7;
WORD      wPort = 0x388;                /* address of sound chip */
WORD      wFMChipAddr;                  // The current Port to write
WORD      wLeftFMChip;
WORD      wRiteFMChip;
WORD      wChip;
WORD      wMinWait;                // Minimum wait bewteen noteon for vox
WORD      wMinPercWait;            // Minimum wait bewteen noteon for drums
BOOL      fEnabled;                     /* are we enabled? */
short     nVoxMelo;
short     nVoxPerc;                     // Melodic voices in Percussion mode
short     nNumVox;                      // current number voices
                                         /* Allow for several Banks */
TMBARY    Tmb[MAXBANK];                  /* patch data  */
PERCARY   Drm;

FMVOXINFO   voxInfo[MAXVOX];           // Holds hardware specific stuff 
CHANNELINFO chanInfo[MAXCHAN];

BYTE      LRegMap[246];                // Used to shadow chip registers
BYTE      RRegMap[246];                // Right chip
BOOL      fShadow;

#ifdef DEBUG
WORD      wDebugLevel;                  /* debug level */
#endif

/*************************************************************************

    strings

*************************************************************************/

/*non-localized strings */
// All undoc go in [Special] section of FMSYNTH.INI

char BCODE aszSpecialSect[]   = "Special";
char BCODE aszWriteDelay1[]   = "WriteDelay1";  // undoc
char BCODE aszWriteDelay2[]   = "WriteDelay2";  // undoc
char BCODE aszOneInstance[]   = "OneInstance";  // undoc'd
char BCODE aszTestHw[]        = "TestHardware"; // undoc
char BCODE aszMinWait[]       = "MinWait";      // undoc
char BCODE aszMinPercWait[]   = "MinPercWait";  // undoc

char BCODE aszFailMsg[]       = "FM Chip Not found";
char BCODE aszTaskName[]      = "FMTASK.EXE";
char BCODE aszTskKey[]        = "FMTask";


/***************************************************************************

    Constant Data 
    
***************************************************************************/


/***************************************************************************

    local data

***************************************************************************/

static BOOL   fInit = FALSE;                /* have we initialized yet? */

/* format of drumkit.bin file */
typedef struct drumfilepatch_tag {
    BYTE key;                   /* the key to map */
    BYTE patch;                 /* the patch to use */
    BYTE note;                  /* the note to play  */
    } DRUMFILEPATCH, *NPDRUMFILEPATCH, FAR *LPDRUMFILEPATCH;

/***************************************************************************

    public functions

***************************************************************************/

WORD FAR GetChipSet(void) {
    return GetPrivateProfileInt(aszDriverName, aszCardType, 
                                ONEOPL2, aszSysProfileNm); 
    }
        
WORD FAR GetBaseAddr(void) {
    char PortBuf[16];
    LPSTR lpPB = PortBuf;
    
    GetPrivateProfileString(aszDriverName, aszBasePort, DEF_PORT, lpPB, 16,
                            aszSysProfileNm);
    wPort = StrToHex(lpPB);
    return wPort;
    }

BOOL NEAR TestHardware(void) {
    return (GetPrivateProfileInt(aszSpecialSect, aszTestHw,
                                 1, aszProfileNm) == 1); 
    }

//****************************************************************************

#define DFTMINWAIT         300 // empirical, but nice

WORD NEAR GetMinWait(void) {
    return GetPrivateProfileInt(aszSpecialSect, aszMinWait, 
                                       DFTMINWAIT, aszProfileNm);
    }

//****************************************************************************

#define DFTMINPERCWAIT     50 // empirical, but nice

WORD NEAR GetMinPercWait(void) {
    return GetPrivateProfileInt(aszSpecialSect, aszMinPercWait, 
                                DFTMINPERCWAIT, aszProfileNm);
    }


/***************************************************************************
 Perform Hex conversion on string
***************************************************************************/

WORD FAR PASCAL StrToHex(LPSTR asz) {
    UINT ii;
    WORD wResult = 0;
    UINT slen    = lstrlen(asz);
    
    for (ii = 0; ii < slen; ++ii) {
        wResult <<= 4; // times 16                     
        if (asz[ii] >= '0' && asz[ii] <= '9')
            wResult += (WORD)(asz[ii] - '0'); 
        else if (asz[ii] >= 'a' && asz[ii] <= 'f')
            wResult += (asz[ii] - 'a' + 10); 
        else if (asz[ii] >= 'A' && asz[ii] <= 'F')
            wResult += (asz[ii] - 'A' + 10); 
        else
            break;
        }         
    return wResult;
    }
 
/****************************************************************************
 *
 * int | BoardInstalled | Checks to see if the board is installed.
 *
 * Returns a nonzero value if the board is installed and zero otherwise.
 ***************************************************************************/

BOOL FAR BoardInstalled(void) {
BYTE t1, t2, i;

    if (!TestHardware())           //  See if we should test...
        return TRUE;
    
    D1("BoardInstalled");
    SetChipAddr(LOCHIP);                    // Access left Chip
    BlindOutput(4, 0x60);             /* mask T1 & T2 */
    BlindOutput(4, 0x80);             /* reset IRQ */
    t1 = inport();                  /* read status register */
    BlindOutput(2, 0xFF);             /* set timer - 1 latch */
    BlindOutput(4, 0x21);             /* unmask & start T1 */
    for (i = 0; i < 400; i++) {     /* 100 uSec delay for timer1 overflow */
        t2 = inport();              /* a delay of some sort...(stupid) */
        }
    t2 = inport();                  /* read status register */
    BlindOutput(4, 0x60);
    BlindOutput(4, 0x80);
    
    return (((t1 & 0xE0) == 0) && ((t2 & 0xE0) == 0xC0));
    }

/****************************************************************************
 *
 * int | SoundColdInit | Must be called for start-up initialization.
 *
 * Returns nonzero value if the board is installed and zero otherwise.
 ***************************************************************************/
 /*
  *     char strBuf[40];
  *     LPSTR lpDStr = strBuf;
  *
  *     wsprintf(lpDStr, "OPL3 - Stereo: %d, Base: %X", fStereo, wPort);
  *     WritePrivateProfileString(aszDriverName, "MyDebug", lpDStr, 
  *                            aszProfileNm);
  */
    
static int NEAR SoundColdInit(void) {
    int hardware;
    DWORD dwWinFlags;
    BOOL  fShortWait;
    BOOL  fOPL3;
    BOOL  fPro;

#ifdef DEBUG
    /* update debug level - default is 1 */
    wDebugLevel = GetProfileInt(aszMMDebug, aszFMSynth, 1);
#endif

    D1("SoundColdInit");

    MIDIOff();
    wLeftFMChip = GetBaseAddr();
    wRiteFMChip = wLeftFMChip + RCHIPOFS; // Do this even for single chip OPL2
                                        // All cards we know have second
                                        // chip Address at base+2
    InitRegMap();
    
    // Right off the Bat we figure out the differences in operation
    
    wChip = GetChipSet();
    
    switch(wChip) {
    case ONEOPL2: 
        fPro    = FALSE;
        fOPL3   = FALSE;
        fShortWait = FALSE;
        break;
        
    case TWOOPL2:
        fOPL3 = FALSE;
        fPro  = TRUE;
        fShortWait = FALSE;
        break;
        
    case ONEOPL3:
        fOPL3 = TRUE; 
        fPro  = TRUE;
        fShortWait = TRUE;         // OPL3 can write faster
        break;
        }
    
    if (fShortWait) {
        dwWinFlags = GetWinFlags();
        if (dwWinFlags & WF_CPU286) 
            wWriteDelay1 = DEF286SHORTDELAY;
                else if (dwWinFlags & WF_CPU386)
            wWriteDelay1 = DEF386SHORTDELAY;
                else if (dwWinFlags & WF_CPU486)
            wWriteDelay1 = DEF486SHORTDELAY;
                wWriteDelay2 = wWriteDelay1 << 1;       // Use twice as much
        }
    
    DrvLoadInitialize();     
    SetPercussion(FALSE, FALSE);
    SetOPL3(fOPL3);
    SetPro(fPro);
    
    if (hardware = BoardInstalled()) {
        CopyFreqs();
        SoundBoardInit();
        }

    MIDIOn();
    return hardware;
    }

/***************************************************************************
 *
 *  void | DrvLoadInitialize | Initializes the write delay time.  
 *  LibEntry has already initialized the global wWriteDelay according to 
 *  the type of CPU present.
 *                
 ***************************************************************************/

// Special -- for tuning -- debugging
static void PASCAL NEAR DrvLoadInitialize(void) {
    WORD wDefDelay;

    wDefDelay = wWriteDelay1;
    wWriteDelay1 = GetPrivateProfileInt(aszSpecialSect, aszWriteDelay1,
        wDefDelay, aszProfileNm); 
    wDefDelay = wWriteDelay2;
    wWriteDelay2 = GetPrivateProfileInt(aszSpecialSect, aszWriteDelay2,
        wDefDelay, aszProfileNm);
    }

/****************************************************************************
 *
 * void | SoundBoardInit | Initializes the chip.
 *
 ***************************************************************************/

BOOL FAR PASCAL _loadds SoundFullInit(void) {
    if (!LoadPatches(0))             /* load the melodic patches */
         return FALSE;                /* keep fInit set to FALSE */        
    if (!LoadDrumPatches())          /* load the drum kit information */
         return FALSE;               /* keep fInit set to FALSE */
    if (!SoundColdInit()) {          /* if we can't find a card */
         MessageBox(NULL, aszFailMsg, aszDriverName, 
                                   MB_SYSTEMMODAL+MB_OK);
         return FALSE;           /* keep fInit set to FALSE */
         }
    LoadExtraBanks();
    return TRUE;
    }

/****************************************************************************
 *
 * void | SoundBoardInit | Initializes the chip.
 *
 ***************************************************************************/
        
void FAR PASCAL _loadds SoundBoardInit(void) {

    D1("SoundBoardInit");
    
    setRunTime(GetStereo());
    GetModnDepth();    
    InitFM();
    GetCurrentBank();
    InitVoxInfo();
    InitChannelInfo();  
    synthInit();
    setDrums(GetPercussion(), GetPercChannel());
    GetPercSwitch();
    sndParm(BENDRNG, (GetBendRange() << 7), (WORD)-1); // Init from current
    GetChanMap();
    GetPercBoost();
    GetUseOldest();
    
    // SetBnkInit(TRUE);
    wMinWait     = GetMinWait();
    wMinPercWait = GetMinPercWait();
    }

/****************************************************************************
 *
 * SoundWarmInit does minimum necessary to re-init the chip
 *
 * There is no return value.
 ***************************************************************************/

void FAR SoundWarmInit(void) {
    WORD ii;
    
    D1("SoundWarmInit");

    InitRegMap();    
    InitFM();
    InitVoxInfo();
    synthInit();
    setDrums(isPercussion(), thePercChannel());
    for (ii = 0; ii < MAXCHAN; ++ii)
       sndParm(BENDRNG, (theBendRange(ii) << 7), ii);
                                             // Init from current
    }

//****************************************************************************

void FAR PASCAL setRunTime(BOOL fStereo) {
    BOOL  fPercWriteBoth;

    nVoxMelo = NVOX_MELOMODE;
    nVoxPerc = NVOX_PERCMODE;
    
    switch(wChip) {
    case ONEOPL2: 
        fStereo = FALSE;
        fPercWriteBoth = FALSE;
        break;
        
    case TWOOPL2:
        if (fStereo) 
            fPercWriteBoth = TRUE; // Write Percussion sounds to both chips
        else {
            fPercWriteBoth = FALSE;
            nVoxMelo = NVOX_MELOPRO; // More voices if not stereo panning
            nVoxPerc = NVOX_PERCPRO;
            }
        break;
        
    case ONEOPL3:
        if (!fStereo) {
            nVoxMelo = NVOX_MELOPRO; // More voices if not stereo panning
            nVoxPerc = NVOX_PERCPRO;
            }
        fPercWriteBoth = FALSE;
        break;
        }
    
    nNumVox = nVoxMelo;    
    SetPercBoth(fPercWriteBoth);
    SetStereo(fStereo, FALSE);
    }
    
//****************************************************************************

void NEAR setDrums(BOOL fOn, WORD wChn) {
   GetFixedPitch();

   if (fOn) {
       nNumVox = nVoxPerc;
       chanInfo[wChn].bType = PERCTYPE;
       PercMap();
       }
   else {
       nNumVox = nVoxMelo;       
       chanInfo[wChn].bType = MELOTYPE;    
       MeloMap();
       }

   sndDrumMode(fOn);
   }

//****************************************************************************

void NEAR InitVoxInfo(void) {
    static SBTIMBRE BlankTimbre = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                                    0, 0, 0, 0 };
    int ii;
    WORD wDBank = theCurrentBank();   // Current Bank
         
    for (ii = 0; ii < MAXVOX; ++ii) {
        voxInfo[ii].wPitch = 0;         // Pitch Info
        voxInfo[ii].bNote  = 0;         // MIDI Note Number
        voxInfo[ii].wBank  = wDBank;
        voxInfo[ii].bPatch = NOPATCH;   // Current Timbre
        voxInfo[ii].bVeloc = 0;         // Velocity
        voxInfo[ii].bVolum = 0;         // Overall Note Volume
        voxInfo[ii].bModn  = 0;         // Modulation Control
        voxInfo[ii].bSpkrB = 0;         // Speaker Bits (OPL3)
        voxInfo[ii].bLCLev = 0;         // Left Chip Carrier Level
        voxInfo[ii].bLMLev = 0;         // Left Chip Modulator Level
        voxInfo[ii].bRCLev = 0;         // Right Chip Carrier Level
        voxInfo[ii].bRMLev = 0;         // Right Chip Modulator Level
        voxInfo[ii].Tmb    = BlankTimbre; // Empty Params
        } 
    }
    
//  ************************************************************************

void FAR InitChannelInfo(void) {
    int ii;
    WORD wDBank = theCurrentBank();   // Current Bank
         
    for (ii = 0; ii < MAXCHAN; ++ii) { 
        chanInfo[ii].wBank   = wDBank;  // Patch Bank
        chanInfo[ii].bPatch  = DFTPATCH; // Current Timbre    
        chanInfo[ii].bVolum  = DFTVOLUME;   
        chanInfo[ii].wBend   = MIDBEND;  
        chanInfo[ii].bPan    = CTRPAN;
        chanInfo[ii].bModn   = NOMODN;
        chanInfo[ii].bPedal  = NOPEDAL;     
        chanInfo[ii].bType   = MELOTYPE;
        chanInfo[ii].Timbre  = Tmb[DFTBANK].snd[DFTPATCH];       
        } 
    }
    
// Load User defined Banks ***************************************************

void FAR LoadExtraBanks(void) {
    short ii;
    char aszBankPath[80];
    
    // Now See if we Load any external Banks
    
    for (ii = 1; ii < MAXBANK; ++ii) 
        Tmb[ii] = Tmb[0]; // Copy Base Bank throughout
    
    for (ii = 0; ii < MAXBANK; ++ii) {
        GetBankPath(ii, aszBankPath, sizeof(aszBankPath));
        if (lstrcmp(aszBankPath, aszInternal) != 0) 
            LoadBank(aszBankPath, ii);
        }
    // Percussion
    GetBankPath(PRCBANK, aszBankPath, sizeof(aszBankPath));
    if (lstrcmp(aszBankPath, aszInternal) != 0) 
       LoadDrumBank(aszBankPath);
    }

// Routine to Allocate and Load a Patch Bank *************************

BOOL FAR PASCAL LoadBank(LPCSTR lpBankPath, short idx) {
    static BYTE Sig[4];
    LPTMBARY lpBnk;
    int      dfh;
    int      nread;
                               
    if ((dfh = _lopen(lpBankPath,OF_READ)) == -1)
       return FALSE;
       
    nread = _lread(dfh, &Sig, sizeof(Sig));
    
    if ((nread != sizeof(Sig)) || (memcmp(&Sig, IBKSIG, sizeof(Sig)) != 0)) {
        _lclose(dfh);
        return FALSE;
        }
    
    lpBnk = &Tmb[idx];
    nread = _lread(dfh, lpBnk, sizeof(TMBARY)); // read directly into array
    _lclose(dfh);
    
    memcpy(&Sig, "JWO\0", sizeof(Sig));
    return TRUE;
    }

// Routine to Allocate and Load a Percussion Patch Bank ***********

BOOL FAR  PASCAL LoadDrumBank(LPCSTR lpBankPath) {
    static BYTE Sig[4];
    LPPERCARY lpPrcBnk;
    int       dfh;
    int       nread;
                               
    if ((dfh = _lopen(lpBankPath,OF_READ)) == -1)
       return FALSE;
       
    nread = _lread(dfh, &Sig, sizeof(Sig));
    
    if ((nread != sizeof(Sig)) || (memcmp(&Sig, IBKSIG, sizeof(Sig)) != 0)) {
        _lclose(dfh);
        return FALSE;
        }
    
    lpPrcBnk = &Drm;
    nread = _lread(dfh, lpPrcBnk, sizeof(PERCARY)); // read into array
    _lclose(dfh);
    
    memcpy(&Sig, "JWO\0", sizeof(Sig));
    return TRUE;
    }


//***********************************************************************
// InitFM 
//***********************************************************************

void NEAR InitFM(void) {
    BYTE ii;
    WORD thebits;
        
    SetChipAddr(LOCHIP);
    for (ii = 1; ii <= NUMREGS; ++ii)
        BlindOutput(ii, 0);
    BlindOutput(4, 0x60);              // Mask Timers
        
    thebits = theModnDepth();    
    setPercBits(thebits);
    sndModDepth(thebits >> 1);  // Clear Percussion, set Depth    
    if (isOPL3()) {
        SetChipAddr(HICHIP);    // Access Upper Chip
        BlindOutput(4, 0);        // Turn off any 4OP voices
        BlindOutput(5, 1);        // Set "New" bit          
        for (ii = 32; ii <= NUMREGS; ++ii) 
            BlindOutput(ii, 0);   // Clear rest of Right Regs
        }
    else { // We don't set Wave Select for OPL3
        BlindOutput(WAVSELREG, WAVSEL);
        if (isPro()) {
           SetChipAddr(HICHIP);    // Access Upper Chip
           for (ii = 1; ii <= NUMREGS; ++ii)
               BlindOutput(ii, 0);
           }
        }
        
    SetChipAddr(LOCHIP);
    }

//***********************************************************************
// InitRegMap
//***********************************************************************
 
void NEAR InitRegMap(void) {
    memset(LRegMap, 0, sizeof(LRegMap));
    memset(RRegMap, 0, sizeof(RRegMap));    

    fShadow = GetRegShadow();
    }
 
//***********************************************************************
// ResetFM 
//***********************************************************************

void NEAR ResetFM(void) {
    if (isOPL3()) {
        SetChipAddr(HICHIP);    // Access Upper Chip
        BlindOutput(5, 0);        // Set "New" bit          
        BlindOutput(4, 0);        // Turn off any 4OP voices
        }
    }

/****************************************************************************
 *
 * int | LoadPatches | Reads the patch set from the BANK resource and
 *     builds the <p patches> array. Loads the bank indexed by idx
 *
 ***************************************************************************/
 
int FAR PASCAL LoadPatches(short idx) {
    HANDLE hResInfo;
    HANDLE hResData;
    LPSTR  lpRes;
    int    iPatches;
    DWORD  dwResSize;
    LPTIMBRE   lpBankTimbre;
    LPTIMBRE   lpPatchTimbre;
    LPIBKFMT   lpBankHdr;

    /* find resource and get its size */
    hResInfo = FindResource(ghInstance, MAKEINTRESOURCE(DEFAULTBANK), 
                            MAKEINTRESOURCE(RT_BANK));
    if (!hResInfo) {
        D1("Default bank resource not found");
        return 0;
        }
    dwResSize = (DWORD)SizeofResource(ghInstance, hResInfo);

    /* load and lock resource */
    hResData = LoadResource(ghInstance, hResInfo);
    if (!hResData) {
        D1("Bank resource not loaded");
        return 0;
        }
    lpRes = LockResource(hResData);
    if (!lpRes) {
        D1("Bank resource not locked");
        return 0;
        }

    /* read the bank resource, loading patches as we find them */

    D1("loading patches");
    lpBankHdr  = (LPIBKFMT)lpRes;
    
    for (iPatches = 0; iPatches < MAXTMB; iPatches++) {
        lpBankTimbre   = &lpBankHdr->snd[iPatches];
        lpPatchTimbre  = &Tmb[idx].snd[iPatches];
        *lpPatchTimbre = *lpBankTimbre;
        }

    UnlockResource(hResData);
    FreeResource(hResData);
    
    return iPatches;
    }

/****************************************************************************
 *
 * int | LoadDrumPatches | Reads the drum kit patch set from the 
 *     DRUMKIT resource and builds the <p drumpatch> array. 
 *
 * Each entry of the <t drumpatch> array (representing a key number
 *     from the "drum patch") consists of a patch number and note number
 *     from some other patch.
 *
 * Returns the number of patches loaded, or 0 if an error occurs.
 ***************************************************************************/

int FAR PASCAL LoadDrumPatches(void) {
    HANDLE hResInfo;
    HANDLE hResData;
    LPSTR  lpRes;
    int    iPatches;
    DWORD  dwResSize;
    LPTIMBRE   lpBankTimbre;
    LPTIMBRE   lpPatchTimbre;
    LPIBKFMT   lpBankHdr;
     
        /* find resource and get its size */
    hResInfo = FindResource(ghInstance, MAKEINTRESOURCE(DEFAULTDRUMBANK), 
                            MAKEINTRESOURCE(RT_DRUMBANK));
    if (!hResInfo) {
        D1("Percussion bank resource not found");
        return 0;
        }
    dwResSize = (DWORD)SizeofResource(ghInstance, hResInfo);

    /* load and lock resource */
    hResData = LoadResource(ghInstance, hResInfo);
    if (!hResData) {
        D1("Percussion resource not loaded");
        return 0;
        }
    lpRes = LockResource(hResData);
    if (!lpRes) {
        D1("Percussion resource not locked");
        return 0;
        }

    /* read the bank resource, loading patches as we find them */

    D1("loading percussion patches");
    lpBankHdr  = (LPIBKFMT)lpRes;
    
    for (iPatches = 0; iPatches < MAXDRUMTMB; iPatches++) {
        lpBankTimbre   = &lpBankHdr->snd[iPatches];
        lpPatchTimbre  = &Drm.snd[iPatches];
        *lpPatchTimbre = *lpBankTimbre;
        }

    UnlockResource(hResData);
    FreeResource(hResData);
  
    return iPatches;
    }

/***************************************************************************

    public functions

***************************************************************************/

/****************************************************************************
 *
 *  BOOL | Enable | Enables the card.  If we haven't yet enabled in
 *     this session, it will do a cold restart of the card and load the
 *     patches; otherwise it will do a warm restart.
 *
 *  Returns TRUE if successful and false otherwise.
 ***************************************************************************/

BOOL FAR PASCAL Enable(void) {
    static char aszMsg[60];
    
    D1("Enable");
    if (!fInit) {                        /* if we haven't initialized yet */
        if (!SoundFullInit())
            goto Problem;
        }
    else {                               /* we've already initialized */
        SoundWarmInit();                 /* so do a warm restart */
        }
        
    fInit = TRUE;                      /* say that we have initialized once */
    fEnabled = TRUE;                     /* say that we're enabled */

Problem:
    return fInit;
    }

/****************************************************************************
 *
 * void | Disable | Since this function is called either when a
 *     Windows session ends or when we switch to a DOS box (in 286 mode),
 *     we'll reset the card in preparation for someone else to use it.
 *
 ***************************************************************************/

void FAR PASCAL Disable(void) {
    D1("Disable");

    if (fInit)                 /* if we have a card */
        ResetFM();              /* reset card to be good */
    fEnabled = FALSE;           /* say that we're not enabled */
    }

extern void FAR PASCAL _loadds MIDIOff(void) {
   _MIDIDisable = TRUE;
   }

extern void FAR PASCAL _loadds MIDIOn(void) {
   _MIDIDisable = FALSE;
   }

/****************************************************************************
 *
 * int | LibMain | Library initialization code.
 *
 * HANDLE | hInstance | Our instance handle.
 *
 * WORD | wHeapSize | The heap size from the .def file.
 *
 * LPSTR | lpCmdLine | The command line.
 *
 ***************************************************************************/

int FAR PASCAL LibMain(HANDLE hInstance, WORD wHeapSize, LPSTR lpCmdLine) { 
    DWORD dwWinFlags;
    char  NulStr[1] = {'\0'};
    UINT  ShowNorm  = SW_SHOWNORMAL;
    char  TaskPath[80];
    
    struct _LOADPARMS {
       WORD      segEnv;         /* child environment  */
       LPSTR     lpszCmdLine;    /* child command tail */
       UINT FAR* lpShow;         /* how to show child  */
       UINT FAR* lpReserved;     /* must be NULL       */
       } LoadParms;

#ifdef DEBUG
    /* get debug level - default is 1 */
    wDebugLevel = GetProfileInt(aszMMDebug, aszFMSynth, 1);
#endif
    
    D1("LibMain");

    _hModule   = hInstance;         // for CTL3D

    ghInstance = hInstance;         /* save our instance */
    dwWinFlags = GetWinFlags();
  
    if (dwWinFlags & WF_CPU286) 
        wWriteDelay1 = DEF286WRITEDELAY;
    else if (dwWinFlags & WF_CPU386)
        wWriteDelay1 = DEF386WRITEDELAY;
    else if (dwWinFlags & WF_CPU486)
        wWriteDelay1 = DEF486WRITEDELAY;
    wWriteDelay2 = wWriteDelay1 * 7; // Second delay is much longer

    GetPrivateProfileString(aszAppSect, aszTskKey, aszTaskName, TaskPath, 
                                        sizeof(TaskPath), aszProfileNm);
       
          
    LoadParms.segEnv      = 0;         /* child environment  */
    LoadParms.lpszCmdLine = NulStr;    /* child command tail */
    LoadParms.lpShow      = &ShowNorm; /* how to show child  */
    LoadParms.lpReserved  = NULL;      /* must be NULL       */
    ghInstTask = LoadModule(aszTaskName, &LoadParms);
    fhaveTask = (ghInstTask >= 32);
       
    return 1;                       /* exit ok */
    }
