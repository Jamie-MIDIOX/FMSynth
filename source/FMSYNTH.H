/****************************************************************************
 *
 *   fmsynth.h
 *
 *   Copyright (c) 1993 Jamie O'Connell. All Rights Reserved.
 *
 ***************************************************************************/

#ifndef _FMSYNTH_H_
#define _FMSYNTH_H_

/* this way we get more symbols for debug */

#ifdef DEBUG
//    #define static
#endif

#define IDC_FIRSTPORT   0x201
#define IDC_220         0x202
#define IDC_228         0x203
#define IDC_240         0x204
#define IDC_388         0x205
#define IDC_OTHR        0x206
#define IDC_EPORT       0x207
#define IDC_LASTPORT    0x207

#define IDC_PORTGRP     0x301
#define IDC_CHIPGRP     0x302
                             
#define IDC_FIRSTCHIP   0x101
#define IDC_1OPL2       0x101
#define IDC_2OPL2       0x102
#define IDC_1OPL3       0x103
#define IDC_LASTCHIP    0x103

#define MAX_ERR_STRING  250   /* max length of string table errors */

#define DEF_PORT        "388" /* default port address (aszHex) */
#define DLG_CONFIG      42    /* dialog box resource id */
#define IDM_ONTOP       156   /* System Menu */
 
/*  Error strings... */

#define IDS_ERRTWODRIVERS       1
#define IDS_ERRMCANOTSUPPORTED  2
#define IDS_ERROLDVDMAD         3
#define IDS_ERRBADPORT          4
#define IDS_ERRBADVERSION       5
#define IDS_ERRBADINT           6

#define IDS_ERRBADCONFIG        16
#define IDS_WARNPROCARD         17
#define IDS_WARNTHUNDER         18

/* Client Info */

#define MAXCLIENT       8

#define LCHIPOFS        0
#define RCHIPOFS        2  // All sound cards: Right chip is offset 2 from base
#define VOCPERCHIP      9  // Number Voices on a "Chip"
#define REGSPERVOICE   11  // Registers making up a Timbre

#define NVOX_MELOMODE   9
#define NVOX_PERCMODE   6
#define NVOX_MELOPRO   18
#define NVOX_PERCPRO   15

#define NUMCHANNELS    16                /* number of MIDI channels */

// Hardware Stuff

#define ONEOPL2         1
#define TWOOPL2         2
#define ONEOPL3         3

#define WAVSEL         0x20
#define MONOBIT        ((BYTE)0x30)
#define LSTBIT         ((BYTE)0x10)
#define RSTBIT         ((BYTE)0x20)
#define LOCHIP         0        // Voice Numbers which will trigger change
#define HICHIP         VOCPERCHIP
#define NUMREGS        0xF5

#define MAXVOX         18            // Max Voices on card
#define MAXTMB        128            /* max patch number (>127 for drums) */
#define MAXDRUMTMB    128            /* #Drum Patches (was 47) */
#define MAXBANK         5
#define MAXVOLUME    0x7f
#define MAXDRUM         5
#define MAXCHAN        16 
                         
#define MAXBOOST       63       // Max Percussion Boost
#define MINBOOST      -64       // Min Percussion Boost

#define PRCBANK         5       // 6th Bank is percussion
#define PERCMASK     0xE0       // Masks AM-VIB-Perc
#define TRAM         0x80       // Tremolo bit
#define VBFM         0x40       // Vibrato bit
#define DEEP         0xC0       // Deep Modulation
#define MODINIT      0xFF       // Mark as different
#define MODNMASK     0x3F       // Mask off Modulation

#define FNUMREG      0xA0       // FNum register
#define KEYBLKREG    0xB0       // Key On/Blk register
#define WAVSELREG    0x01       // Wave Select register
#define PERCREG      0xBD       // AM-VIB depth/Percussion

#define PERCVOC         6

#define KEYON        0x20       // note on value in MSB of FNum
#define PERCON       0x20       // percussion bit
#define PERCOFF      0xC0       // turn off percussion
#define OCTVMASK     0x01C      // octave position in MSB of FNum
#define FREQMASK     0x3FF      // word mask for FNUM
#define WKEYON       0x2000     // note on value in MSB of FNum (WORD size)
#define WKEYOFF      0x1FFF     // note off mask in MSB of FNum (WORD size)

#define LBDRUM          6       // bass drum          voice# 6
#define LHHSD           7       // HighHat - SnareDrum voice# 7
#define LTTCY           8       // TomTom  - Cymbal    voice# 8

#define RBDRUM         15       // bass drum          voice# 15
#define RHHSD          16       // HighHat - SnareDrum voice# 16
#define RTTCY          17       // TomTom  - Cymbal    voice# 17

#define BDPITCH        44       // this is variable, 43 sounds good
#define SDPITCH        43       // must be 7 above TomTom?
#define TTPITCH        36       // the other 4 percus. change with this pitch

// Obsolete, but supported...
#define FIRSTDRUMNOTE  35
#define LASTDRUMNOTE   81
#define NUMDRUMNOTES   (LASTDRUMNOTE - FIRSTDRUMNOTE + 1)

#define MAXPITCH     0x3fff         /* maximum pitch bend value */
#define MIDBEND      0x2000         /* mid pitch bend value (no shift) */
#define NR_STEP_PITCH    25         /* steps per half-tone for pitch bend */
#define MID_C            60         /* MIDI standard mid C */
#define CHIP_MID_C       48         /* sound chip mid C */

#define BENDRNG           0             // Registered Parm number
#define FINETUNE              1         // Reg. Parm. number
#define CRSTUNE               2         // Reg. Parm. number

#define RT_BANK         256
#define RT_DRUMBANK     258
#define DEFAULTBANK     1
#define DEFAULTDRUMBANK 1
                            
#define MUTETYPE        0x00                            
#define MELOTYPE        0x01                    
#define PERCTYPE        0x02

/****************************************************************************/

#define ISSTATUS(bData)         ((bData) & 0x80)
#define FILTERCHANNEL(bStatus)  ((BYTE)((bStatus) & 0xf0))
#define FILTERSTATUS(bStatus)   ((BYTE)((bStatus) & 0x0f))

#define STATUS_FIRST            0x80

#define STATUS_NOTEOFF          0x80
#define STATUS_NOTEON           0x90
#define STATUS_POLYPHONICKEY    0xa0
#define STATUS_CONTROLCHANGE    0xb0
#define STATUS_PROGRAMCHANGE    0xc0
#define STATUS_CHANNELPRESSURE  0xd0
#define STATUS_PITCHBEND        0xe0

#define STATUS_SYS              0xf0
#define STATUS_SYSEX            0xf0
#define STATUS_QFRAME           0xf1
#define STATUS_SONGPOINTER      0xf2
#define STATUS_SONGSELECT       0xf3
#define STATUS_F4               0xf4
#define STATUS_F5               0xf5
#define STATUS_TUNEREQUEST      0xf6
#define STATUS_EOX              0xf7
#define STATUS_TIMINGCLOCK      0xf8
#define STATUS_F9               0xf9
#define STATUS_START            0xfa
#define STATUS_CONTINUE         0xfb
#define STATUS_STOP             0xfc
#define STATUS_FD               0xfd
#define STATUS_ACTIVESENSING    0xfe
#define STATUS_SYSTEMRESET      0xff

// SysEx Misc

#define DATASTART           5
#define SXTMBLEN           24
#define SXBUFLEN           32
#define MAXSXHDR            6
#define FULLSXHDR          11
#define BNKPERCOLD       0x10 // location of perc timbres
#define BNKPERCMAP       0x11 // location of perc timbre map
#define BNKPERCNEW       0x12 // location of version 2.0 perc bank
#define SYSPARM          0x20 // location of system parameters
#define SYSPARMNEW       0x22 // location of system parameters

#define TXDATA           0x11 // Driver is to transmit data
#define RXDATA           0x12 // Driver is to receive data
                              // or Driver is sending data 
#define DRVRESET         0x15 // Driver is to reset to power-on state
#define BNKRESET         0x16 // Driver is to reset Banks

/****************************************************************************

    definitions of sound chip parameters

 ***************************************************************************/

#define DFTBOOST        10        /* Default drum voulme boost */

/* percussive voice numbers (0-5 are melodic voices, 12 op): */
#define BD              6         /* bass drum (2 op) */
#define SD              7         /* snare drum (1 op Car) */
#define TOM             8         /* tom-tom (1 op Mod) */
#define CYMB            9         /* cymbal (1 op Car) */
#define HIHAT           10        /* hi-hat (1 op Mod) */

#define FIRSTDRUMVOC    (BD)

#define vBASSDRUM       0         // index into structure
#define vSNARE          1
#define vTOMTOM         2 
#define vCYMBAL         3
#define vHIHAT          4

/* In percussive mode, the last 4 voices (SD TOM HH CYMB) are created
 * using melodic voices 7 & 8. A noise generator uses channels 7 & 8
 * frequency information for creating rhythm instruments. Best results
 * are obtained by setting TOM two octaves below mid C and SD 7 half-tones
 * above TOM. In this implementation, only the TOM pitch may vary, with the
 * SD always set 7 half-tones above.
 */
#define TOM_PITCH        36      /* best frequency, in range of 0 to 95 */
#define TOM_TO_SD        7       /* 7 half-tones between voice 7 & 8 */
#define SD_PITCH         (TOM_PITCH + TOM_TO_SD)

// FM MIDI Stuff **************************************************

#define KIND(n) ((n) & 0xF0)
#define CHAN(n) ((n) & 0x0F)

// all voices initialized to patch 0

#define DFTPATCH        (0x00)
#define DFTBANK         (0x00)
#define DFTRANGE        (0x02)
#define DFTVOLUME       (0x7F)
#define CTRPAN          (0x3F) // Center Position
#define NOPATCH         (0xFF)
#define NOKEY           (0xFF)
#define NOMODN          (0x00)  
#define NOPEDAL         (0x00)
#define DFTDEPTH        (0x40) // Heavy Vibrato, Light Tremolo bit
 
#define PEDLUP          (0x00)          // pedal up
#define PEDLDOWN        (0x01)
#define DFTVOL          (0x7F)          // default volume (127)
#define DMODDEPTH       (0x32)         // Light Tremolo - Deep Vibrato
#define NOBEND          (0x2000)        // midpoint
#define PERC_CHAN       9               /*MIDI channel for percussion (0..15)*/

#define LSB 1
#define MSB 2
 
#define SBISIG   "SBI\x1A"     /* 4 chars long */
#define IBKSIG   "IBK\x1A"     /* 4 chars long */

/* Packed Timbre Parameters */

#define MODCHAR         0
#define CARCHAR         1
#define MODSCAL         2
#define CARSCAL         3
#define MODAD           4
#define CARAD           5
#define MODSR           6
#define CARSR           7
#define MODWAVE         8
#define CARWAVE         9
#define FEEDBACK       10
#define FREQNUM        11
#define KEYBLK         12

/* SysEx Timbre Parameter index */

#define MDATK               0
#define MDDCY               1
#define MDSTN               2
#define MDREL               3
#define MDFLG               4
#define MDFQM               5
#define MDKSL               6
#define MDLEV               7
#define MDWAV               8
#define FBCNT               9
#define CRATK              10
#define CRDCY              11
#define CRSTN              12
#define CRREL              13
#define CRFLG              14
#define CRFQM              15
#define CRKSL              16
#define CRLEV              17
#define CRWAV              18
#define PVOC               19
#define TRANP              20
#define PITCH              21

// The System Parameters 

#define STMODE              0
#define PRMODE              1
#define PRCHAN              2
#define DFTBNK              3
#define VIBDPT              4
#define TRMDPT              5
#define BNDRNG              6
#define SAVSET              7
#define FSTCHN              8
#define LSTCHN             23
#define PRCBST             24
#define FIXPRC             25
#define MAPREG             26

// Voice Allocation

/****************************************************************************

    typedefs

 ***************************************************************************/


typedef struct {        /* 16 Bytes each */
        BYTE modchar;   /* AM, VIB, SUS, KSR, MUL */
        BYTE carchar;
        BYTE modscal;   /* KSL, TL */
        BYTE carscal;
        BYTE modad;     /* Attack/Decay */
        BYTE carad;
        BYTE modsr;     /* Sustain/Release */
        BYTE carsr;
        BYTE modwave;   /* Wave Select */
        BYTE carwave;
        BYTE feedback;  /* FB, Connection *(inverse of Adlib)* <- not true? */
        BYTE percvoc;   /* Percussion voice number */
        char transpos;  // amount to transpose voice -31 to 31
        BYTE dpitch;    /* drum pitch */
        BYTE rsv[2];    /* unsused - so far */
        } SBTIMBRE;     

/* SBI Instrument file */

typedef struct {
        char     sig[4];             /* signature: "SBI\x1A"  */
        char     name[32];           /* instrument name */
        SBTIMBRE snd;                /* instrument parameters */
        } SBIFMT;

/* IBK Instrument Bank File */

typedef struct {                     /* 3204 Bytes (0x0C83) */
        char     sig[4];             /* signature: "IBK\x1A"  */
        SBTIMBRE snd[MAXTMB];           /* Instrument block */
        char     name[MAXTMB][9];       /* name block */
        } IBKFMT;


typedef struct {
        SBTIMBRE snd[MAXTMB];
        } TMBARY;         

typedef struct {
        SBTIMBRE snd[MAXDRUMTMB];
        } PERCARY;         

typedef BYTE NEAR * NPBYTE;
typedef WORD NEAR * NPWORD;
typedef BYTE huge *HPBYTE;

typedef SBTIMBRE FAR *LPTIMBRE;
typedef IBKFMT   FAR *LPIBKFMT;
typedef TMBARY   FAR *LPTMBARY;
typedef PERCARY  FAR *LPPERCARY;

typedef struct tMIDIMSGCLIENT {
    BYTE        fSysEx;
    BYTE        bStatus;

    BYTE        bBytesLeft;
    BYTE        bBytePos;

    DWORD       dwShortMsg;
    DWORD       dwMsgTime;
    DWORD       dwRefTime;

    DWORD       dwCurData;
    LPMIDIHDR   lpmhQueue;
    } MIDIINMSGCLIENT, NEAR *NPMIDIINMSGCLIENT, 
      *PMIDIINMSGCLIENT, FAR *LPMIDIINMSGCLIENT;

/****************************************************************************

    bank file support

 ***************************************************************************/
/* Obsolete!!! but we still use... */
        
typedef struct drumpatch_tag {
    BYTE patch;                 /* the patch to use */
    BYTE note;                  /* the note to play  */
    } DRUMPATCH;

/* Our Reentrancy Queue Struct */
    
typedef struct _tagQMSG {
   BYTE  devId;    // which device
   BYTE  isLong;   // whether or not long message
   DWORD dwParam1; // The msg or SysEx header
   } QMSG;
    
/* client information structure */
typedef struct synthalloc_tag {
    HANDLE       hMidi;         /* handle for our parent in MMSYSTEM */
    DWORD        dwCallback;    /* callback for client */
    DWORD        dwInstance;    /* DWORD of reference data from client */
    DWORD        dwFlags;       /* allocation flags */
    } SYNTHALLOC, NEAR *NPSYNTHALLOC;

typedef struct _MIDIMSG {
    BYTE ch;                    /* channel number */
    BYTE b1;                    /* first data byte */
    BYTE b2;                    /* second data byte */
    BYTE pad;                   /* not used */
    } MIDIMSG, FAR *LPMIDIMSG;

/****************************************************************************

    Instance Structures

 ***************************************************************************/

typedef struct _FMVOXINFO {     // Info For a Voice
    WORD    wPitch;             // Pitch Info
    WORD    wBank;              // Current Bank
    BYTE    bPatch;             // Current Timbre
    BYTE    bNote;              // MIDI Note Number
    BYTE    bVeloc;             // Velocity
    BYTE    bVolum;             // Overall Note Volume
    BYTE    bModn;              // Modulation Control
    BYTE    bSpkrB;             // Speaker Bits (OPL3)
    BYTE    bLCLev;             // Left Chip Carrier Level
    BYTE    bLMLev;             // Left Chip Modulator Level
    BYTE    bRCLev;             // Right Chip Carrier Level
    BYTE    bRMLev;             // Right Chip Modulator Level
    SBTIMBRE Tmb;               // Current Params        
    } FMVOXINFO, FAR *LPVOXINFO;

typedef struct _PERCVOXINFO {   // Info For a Percussion Voice
    DWORD   dwTm;                // When this note was sounded
    WORD    wPitch;             // Pitch Info
    BYTE    bNote;              // MIDI Note Number
    BYTE    bPatch;             // Current Timbre
    BYTE    bVeloc;             // Velocity
    BYTE    bVolum;             // Overall Note Volume
    BYTE    bSpkrB;             // Speaker Bits (OPL3)
    BYTE    bCLev;              // Carrier Chip Level
    BYTE    bMLev;              // Modulator Chip Level
    BYTE    bPad;               // Keep alignment
    SBTIMBRE Tmb;               // Current Params
    } PERCVOXINFO, FAR *LPPRCVOXINFO;

typedef struct _CHANNELINFO {     // One of these per MIDI Channel
    WORD     wBank;               // Current Patch Bank
    BYTE     bPatch;              // Current Patch
    BYTE     bVolum;              // Volume
    WORD     wBend;               // Bend Amount
    BYTE     bPan;                // Panning Control
    BYTE     bModn;               // Modulation Control
    BYTE     bPedal;              // Sustain Pedal Control
    BYTE     bType;               // Type of channel (Melodic / Rhythm)
    SBTIMBRE Timbre;              // Local Copy
    } CHANNELINFO, FAR *LPCHANNELINFO;
    
typedef struct _VOICE {  
    DWORD dwTm;           // Time this note was sounded
    BYTE  bBank;         // Last Bank number this voice used        
    BYTE  bKey;          // MIDI key number if note on here, else -1 (0xFF)
    BYTE  bChn;          // MIDI channel
    BYTE  bNoteOn;       // Note ON switch
    BYTE  bPedal;        // Note has had key released, held on only by pedal
    BYTE  bPatch;        // Last patch number this voice used
    } VOICE;

#define MAX_VOICES      18
#define MAXQUEUE    (MAX_VOICES+1)

 // Registered Parameter Variables

typedef struct _PARMFLAG {      // Keeps track of RPN reception
    BYTE  bMSB : 1,             // Received Parm Number MSB?
          bLSB : 1;             // Received Parm Number LSB?
    } PARMFLAG;


// Special Macros for test... Critical Sections

#define VIBRBIT(a) (((a) >> 6) & 1)
#define TREMBIT(a) (((a) >> 7) & 1)
#define MKVIBR(a)  (((a) & 1) << 6)
#define MKTREM(a)  (((a) & 1) << 7)

/* System Parameter Struct */

typedef struct _tagSYSPARM {
        BYTE    fStereo;      /* 1 = on, 0 = off */
        BYTE    fPercussion;  /* 1 = on, 0 = off */
        BYTE    PerChannel;   /* if Percussion mode = 1: channel = 0-15 */
                              /* (i.e. MIDI channel 10 is value 9)      */
        BYTE    DftBank;      /* 0-4 (Bank1 - Bank5) */
        BYTE    fVibDepth;    /* 0 = light, 1 = deep */
        BYTE    fTremDepth;   /* 0 = light, 1 = deep */
        BYTE    BendRange;    /* 0-12 semitones (1 octave) */
        BYTE    fSaveSettings;/* 1 = Save the settings in the INI file */
        BYTE    ChanMap[16];  /* 1 = on, 0 = off -- one byte per channel */
        char    PercBoost;    /* signed: -64 - +63 -- 0 is no boost */
        BYTE    fPitchFixed;  /* if on, we use parallel pitched percussion */ 
        BYTE    fRegShadow;   /* if on, we do chip register shadowing */
        } SYSPARMS, FAR *LPSYSPARM;

/* External Declarations */
        
extern HINSTANCE   ghInstance;
extern BOOL        _MIDIDisable;
extern WORD        wFMChipAddr;
extern WORD        wLeftFMChip;
extern WORD        wRiteFMChip;

extern WORD        wPort;              /* card's port address */
extern WORD        wWriteDelay1;
extern WORD        wWriteDelay2; 
extern WORD        wMinWait;          // between noteon's for voice
extern WORD        wMinPercWait;      // between noteon's for Percussion

extern WORD        wDebugLevel;             
extern BOOL        fEnabled;           /* are we enabled? */
extern short       nVoxMelo; 
extern short       nVoxPerc;           /* Melodic voices in Percussion mode */
extern short       nNumVox;            /* current number voices */

extern TMBARY      Tmb[];              /* melodic patch information */
extern PERCARY     Drm;                /* Percussion patch information */

extern FMVOXINFO   voxInfo[];
extern PERCVOXINFO drmInfo[];
extern CHANNELINFO chanInfo[];
extern BOOL        ChanMap[];
        
//*******************************
// SetChipAddr 
//*******************************

#define SetChipAddr(nVox) (wFMChipAddr = ((nVox) < VOCPERCHIP) ? \
                           wLeftFMChip : wRiteFMChip)
   

/****************************************************************************

       Debug output

 ***************************************************************************/

#ifdef DEBUG
   #define D1(sz) if (wDebugLevel >= 1) (OutputDebugStr("\r\nFMSynth: "),OutputDebugStr(sz))
   #define D2(sz) if (wDebugLevel >= 2) (OutputDebugStr(" "),OutputDebugStr(sz))
   #define D3(sz) if (wDebugLevel >= 3) (OutputDebugStr(" "),OutputDebugStr(sz))
   #define D4(sz) if (wDebugLevel >= 4) (OutputDebugStr(" "),OutputDebugStr(sz))

#else
   #define D1(sz) 0
   #define D2(sz) 0
   #define D3(sz) 0
   #define D4(sz) 0
#endif


#endif // #IFNDEF _FMSYNTH_H_

