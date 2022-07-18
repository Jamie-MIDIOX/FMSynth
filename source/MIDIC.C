/*****************************************************************************
 *
 *   midic.c
 *
 *   Copyright (c) 1993 by Jamie O'Connell
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "fmstring.h"
#include "fmsynth.h"
#include "fmsyntha.h"
#include "fmsfunc.h"
#include "midimain.h"
#include "sysparm.h"
#include "init.h"
#include "fmvers.h"

typedef struct {
   WORD  id;
   BOOL  isOpen;
   } OPINF;

BYTE status[MAXCLIENT];    /* don't make assumptions about initial status */
BYTE bCurrentLen;

BOOL fhaveMix = FALSE;
HINSTANCE hInstMix = 0;

/*****************************************************************************

    internal function prototypes

*****************************************************************************/

static void FAR PASCAL GetSynthCaps(LPBYTE lpCaps, WORD wSize);
#pragma alloc_text(_TEXT, GetSynthCaps)
static WORD NEAR PASCAL getNextOpen(void);

/***************************************************************************

    local data

***************************************************************************/

static SYNTHALLOC gClient[MAXCLIENT]; /* client information */
static short nAllocated    = 0;   /* have we already been allocated? */
static BOOL  fSynthEntered = 0;   /* reentrancy check */

static OPINF oi[MAXCLIENT]; 

#define QSIZE         80
static QMSG  qMsg[QSIZE];
static short qInTx  = 0;
static short qOutTx = 0;
static short nQ     = 0;

//----------------------------------------------------------
#if 0

static WORD    wMixID = 0;
static HMIXER  hMixer = 0;

static FARPROC MixOpen		 = NULL;
static FARPROC MixClose 	 = NULL;

static FARPROC MixGetControl = NULL;
static FARPROC MixSetControl = NULL;
static FARPROC MixGetCaps	  = NULL;
static FARPROC MixGetNumDevs = NULL;

#endif
//----------------------------------------------------------

#define FIXED_DS()  ((HGLOBAL)HIWORD((DWORD)(LPVOID)(&nAllocated)))
#define FIXED_CS()  ((HGLOBAL)HIWORD((DWORD)(LPVOID)modMessage))

BYTE gbMidiLengths[] = {
        3,      /* STATUS_NOTEOFF */
        3,      /* STATUS_NOTEON */
        3,      /* STATUS_POLYPHONICKEY */
        3,      /* STATUS_CONTROLCHANGE */
        2,      /* STATUS_PROGRAMCHANGE */
        2,      /* STATUS_CHANNELPRESSURE */
        3,      /* STATUS_PITCHBEND */
        };

BYTE gbSysLengths[] = {
        1,      /* STATUS_SYSEX */
        2,      /* STATUS_QFRAME */
        3,      /* STATUS_SONGPOINTER */
        2,      /* STATUS_SONGSELECT */
        1,      /* STATUS_F4 */
        1,      /* STATUS_F5 */
        1,      /* STATUS_TUNEREQUEST */
        1,      /* STATUS_EOX */
        };

/****************************************************************************
 *
 * void | midiCallback | This calls DriverCallback, which calls the
 *     client's callback or window if the client has requested notification.
 *
 * NPSYNTHALLOC | pClient | Pointer to the SYNTHALLOC structure.
 *
 * WORD | msg | The message to send.
 *
 * DWORD | dw1 | Message-dependent parameter.
 *
 * DWORD | dw2 | Message-dependent parameter.
 *
 ***************************************************************************/

void FAR PASCAL midiCallback(NPSYNTHALLOC pClient, WORD msg, 
                                     DWORD dw1, DWORD dw2) {
    /* dwFlags contains midi driver specific flags in the LOWORD */
    /* and generic driver flags in the HIWORD */

    if (pClient->dwCallback)
        DriverCallback(pClient->dwCallback,      /* client's callback DWORD */
                       HIWORD(pClient->dwFlags), /* callback flags */
                       pClient->hMidi,           /* handle to device */
                       msg,                      /* the message */
                       pClient->dwInstance,      /* client's instance data */
                       dw1,                      /* first DWORD */
                       dw2);                     /* second DWORD */
    }

/****************************************************************************
 * INTERNAL
 *
 * void | GetSynthCaps | Get the capabilities of the synth.
 *
 * LPBYTE | lpCaps | Far pointer to a MIDICAPS structure.
 *
 * WORD | wSize | Size of the MIDICAPS structure.
 *
 ***************************************************************************/

static void FAR PASCAL GetSynthCaps(LPBYTE lpCaps, WORD wSize) {
    MIDIOUTCAPS mc;    /* caps structure we know about */
    LPBYTE      mp;    /* place in client's buffer */
    WORD        w;     /* number of bytes to copy */
    WORD        wChanMap;
    short       ii;
    
//----------------------------------------------------------    
#if 0
    MIXERLINEINFO LI;
    
    if (haveMixer) {
        if (MixGetNumDevs == NULL) { // Fill in all of them
            MixGetNumDevs = GetProcAddress(hInstMix, MIXGETNUMDEVS);
            MixGetCaps	  = GetProcAddress(hInstMix, MIXGETCAPS);
            MixOpen		  = GetProcAddress(hInstMix, MIXOPEN);
            MixClose 	  = GetProcAddress(hInstMix, MIXCLOSE);
            MixGetControl = GetProcAddress(hInstMix, MIXGETCONTROL);
            MixSetControl = GetProcAddress(hInstMix, MIXSETCONTROL);
            }
        wMixID = (WORD)MixGetNumDevs();
        MixOpen((LPHMIXER)&hMixer, wMixID, ...
        MixGetCaps(wMixID, (LPMIXERCAPS)&MVM, (WORD)sizeof(MIXERCAPS));
        }

#endif
//----------------------------------------------------------

    mc.wMid = 0;                              //  MM_MICROSOFT;
    mc.wPid = 0;                              //  MM_ADLIB;
    mc.wTechnology = MOD_FMSYNTH;
    mc.wVoices = 16;
    mc.wNotes  = nNumVox;
    wChanMap = 0;
    for (ii = 0; ii < 16; ++ii) { 
       if (isChanOn(ii))
          wChanMap |= ((WORD)1 << ii); 
       }      
    mc.wChannelMask   = wChanMap;                       /* all channels */
    mc.vDriverVersion = DRIVER_VERSION;
    mc.dwSupport = 0L;
    lstrcpy(mc.szPname, aszProductName);

    /* copy as much as will fit into client's buffer */
    w = min(wSize, sizeof(MIDIOUTCAPS));
    mp = (LPBYTE)&mc;
    while (w--) 
       *lpCaps++ = *mp++;
    }

WORD NEAR PASCAL getNextOpen(void) {
   short ii;
   
   for (ii = 0; ii < MAXCLIENT; ++ii) {
      if (!oi[ii].isOpen) {
         oi[ii].isOpen = TRUE;
         return oi[ii].id;
         }
      }
   return (WORD)-1;
   }

/****************************************************************************

    This function conforms to the standard MIDI output driver message proc
    modMessage, which is documented in the multimedia DDK.

               synthAllNotesOff(); // we were doing at close last one
               sndDrumMode(FALSE);

 ***************************************************************************/

DWORD FAR PASCAL _loadds modMessage(WORD devId, WORD msg, DWORD dwUser, 
                                    DWORD dwParam1, DWORD dwParam2) {
static DWORD dwMBuf;          /* hold event temporarily */
LPMIDIHDR    lpHdr;           /* header of long message buffer */
LPSTR        lpBuf;           /* current spot in long message buffer */
DWORD        dwLength;        /* length of data being processed */
BOOL         isLongMsg;
short        ii;
WORD         id;
OPINF  NEAR  *oip;

    /* has the whole card been enabled? */
    if (!fEnabled) {
        if ( msg == MODM_INIT ) {
            D1("MIDM_INIT");
            return 0L;
            }
        D1("modMessage called while disabled");
        if (msg == MODM_GETNUMDEVS)
            return 0L;
        else
            return MMSYSERR_NOTENABLED;
		}

    /* this driver only supports 1 device (but multiple opens)  */
    if (devId != 0) {               
        D1("invalid midi device id");
        return MMSYSERR_BADDEVICEID;
        }

    switch (msg) {
        case MODM_GETNUMDEVS:   
            D1("MODM_GETNUMDEVS");
            return 1L;

        case MODM_GETDEVCAPS:
            D1("MODM_GETDEVCAPS");
            GetSynthCaps((LPBYTE)dwParam1, (WORD)dwParam2);
            return 0L;

        case MODM_OPEN:
            D1("MODM_OPEN");
  
            /* check if allocated - init if not */
            if (nAllocated == 0) {
                SoundWarmInit();
                for (ii = 0; ii < MAXCLIENT; ++ii) {
                    oi[ii].isOpen = FALSE;
                    oi[ii].id     = ii;
                    }
                }            
            
            if (++nAllocated > MAXCLIENT) {
               --nAllocated;
               return MMSYSERR_ALLOCATED;
               }
            
            if ((id = getNextOpen()) == (WORD)-1) 
               return MMSYSERR_ALLOCATED;
            
            /* !!! fix for 3.0 286p mode  */
            if (!(GlobalPageLock(FIXED_DS()) && 
                  GlobalPageLock(FIXED_CS()))) 
                return MMSYSERR_NOMEM;
 
            oip = &oi[id];
            *((DWORD FAR *)dwUser) = MAKELONG(oip, 0); // remember our id
            
            /* save client information */
            gClient[id].dwCallback = ((LPMIDIOPENDESC)dwParam1)->dwCallback;
            gClient[id].dwInstance = ((LPMIDIOPENDESC)dwParam1)->dwInstance;
            gClient[id].hMidi      = ((LPMIDIOPENDESC)dwParam1)->hMidi;
            gClient[id].dwFlags    = dwParam2;
   
            bCurrentLen = 0;
            status[id]  = 0;

            /* notify client */
            midiCallback(&gClient[id], MOM_OPEN, 0L,  0L);
            return 0L;
            
        case MODM_CLOSE:
            D1("MODM_CLOSE");
            oip = (OPINF *)LOWORD(dwUser);
            id  = oip->id;

            /* get out */
            if (nAllocated > 0)
                --nAllocated;

            midiCallback(&gClient[id], MOM_CLOSE, 0L,  0L);
            oi[id].isOpen = FALSE; // free up slot

            GlobalPageUnlock(FIXED_DS());
            GlobalPageUnlock(FIXED_CS());
            return 0L;
            
        case MODM_RESET:
            D1("MODM_RESET");

            /* we don't need to return all long buffers since we've */
            /* implemented MODM_LONGDATA synchronously */
            // synthAllNotesOff(); Per 12 Tone Recommendation
            return 0L;

        case MODM_LONGDATA:
            D1("MODM_LONGDATA");

            /* check if it's been prepared */
            lpHdr = (LPMIDIHDR)dwParam1;
            if (!(lpHdr->dwFlags & MHDR_PREPARED)) {
                return MIDIERR_UNPREPARED;
                }
            isLongMsg = TRUE;
            goto MIDICOMMON;
            
        case MODM_DATA:
            D4("MODM_DATA");
            isLongMsg = FALSE;
            lpBuf = (LPBYTE)&dwParam1;
            if (*lpBuf == STATUS_TIMINGCLOCK || 
                *lpBuf == STATUS_ACTIVESENSING)
               return 0L;
MIDICOMMON:
            CritEnter();  // don't interrupt this part
            oip = (OPINF *)LOWORD(dwUser);
            id  = oip->id;

            if (fSynthEntered) {
                D1("MODM_DATA or MODM_LONGDATA reentered!");
                if (++nQ > QSIZE) { // just couldn't hack it
                    --nQ;
                    CritLeave();
                    return MIDIERR_NOTREADY;
                    }
                
                qMsg[qInTx].devId    = (BYTE)id;
                qMsg[qInTx].isLong   = (BYTE)isLongMsg;                
                qMsg[qInTx].dwParam1 = dwParam1;             
                                
                if (++qInTx == QSIZE)
                   qInTx = 0;   // wrap
                CritLeave();
                return 0L;      // just go back -- we'll handle later
                }

             fSynthEntered = TRUE;
             CritLeave();
             
             // Guaranteed to be a single thread now
             
NextMsg:
             if (!isLongMsg) {
                lpBuf = (LPBYTE)&dwParam1;
                if (*lpBuf >= STATUS_TIMINGCLOCK)
                    dwLength = 1;
                else {
                    bCurrentLen = 0;
                    if (ISSTATUS(*lpBuf)) {
                        if (*lpBuf >= STATUS_SYSEX)
                            dwLength = SYSLENGTH(*lpBuf);
                        else
                            dwLength = MIDILENGTH(*lpBuf);
                        }
                    else {               // use running
                        if (!status[id]) // error
                            goto GetNext;
                        dwLength = MIDILENGTH(status[id]) - 1;
                        }
                    }
                synthMidiData(id, lpBuf, dwLength);                    
                }
            else { // long message (sysex)
                lpHdr = (LPMIDIHDR)dwParam1;
                synthMidiData(id, lpHdr->lpData, lpHdr->dwBufferLength);

                /* return buffer to client */
                lpHdr->dwFlags |= MHDR_DONE;
                status[id] = STATUS_EOX;
                midiCallback(&gClient[id], MOM_DONE, dwParam1,  0L);
                }
GetNext:
            CritEnter(); // once again block
               
            if (nQ > 0) {
               id        = qMsg[qOutTx].devId;
               isLongMsg = (BOOL)qMsg[qOutTx].isLong;
               dwParam1  = qMsg[qOutTx].dwParam1;
                
               if (++qOutTx == QSIZE)
                  qOutTx = 0;
               --nQ;               
               CritLeave();               
               goto NextMsg;
               }

            fSynthEntered = FALSE;
            CritLeave();
            return 0L;

        default:
            return MMSYSERR_NOTSUPPORTED;           
        }

    return MMSYSERR_NOTSUPPORTED;
    }
