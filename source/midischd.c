/****************************************************************************
 *
 *   midiin.c
 *
 *   Copyright (c) 1993 by Jamie O'Connell
 *
 *   Adapted 930602 by Jamie O'Connell for FM Synth Driver
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <memory.h>
#include "fmstring.h"
#include "fmsynth.h"
#include "fmsyntha.h"
#include "fmvers.h"
#include "sysparm.h"
#include "midic.h"
#include "midiin.h"

static SYNTHALLOC       gMidiInClient[MAXCLIENT]; /* input client info */
static MIDIINMSGCLIENT  gMIMC[MAXCLIENT];         /* MIDI input msg client */
static BOOL             fMIDIStarted[MAXCLIENT];
static BYTE             openInAry[MAXCLIENT];
static WORD             nAllocIn = 0;

/*****************************************************************************

    internal function prototypes

 ****************************************************************************/

static void   NEAR PASCAL midFreeQ(WORD id);
static void   NEAR PASCAL midGetDevCaps(LPBYTE lpCaps, WORD wSize);
static void   NEAR PASCAL midStart(WORD id);
static void   NEAR PASCAL midStop(WORD id);
static void   NEAR PASCAL midBufferFill(WORD id, LPBYTE lpsx, WORD wLen);
static LPBYTE NEAR PASCAL midMakeHdr(WORD wCmd, WORD Addr1, 
                                     WORD Addr2, WORD wSize);
static short  NEAR PASCAL midCnvtTmb(LPBYTE SXBuf, LPWORD pBnk, LPWORD pTmb);
static short  NEAR PASCAL midCnvtPMap(LPBYTE SXBuf, WORD wBnk, WORD wTmb);
static WORD   NEAR PASCAL verifySize(WORD wSize, WORD wBnk);

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midFreeQ | Free all buffers in the MIQueue.
 *
 * @comm Currently this is only called after sending off any partially filled
 *     buffers, so all buffers here are empty.  The timestamp value is 0 in
 *     this case.
 *
 * @rdesc There is no return value.
 ***************************************************************************/ 

static void NEAR PASCAL midFreeQ(WORD id) {
    LPMIDIHDR   lpH, lpN;
    DWORD       dwTime;

    lpH = gMIMC[id].lpmhQueue;              /* point to top of the queue */
    gMIMC[id].lpmhQueue = NULL;             /* mark the queue as empty */
    gMIMC[id].dwCurData = 0L;
    
    dwTime = timeGetTime() - gMIMC[id].dwRefTime;

    while (lpH) {
        lpN = lpH->lpNext;
        lpH->dwFlags |= MHDR_DONE;
        lpH->dwFlags &= ~MHDR_INQUEUE;
        lpH->dwBytesRecorded = 0;
        midiCallback(&gMidiInClient[id], MIM_LONGDATA, (DWORD)lpH, dwTime);
        lpH = lpN;
        }
    }

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midAddBuffer | This function adds a buffer to the list of
 *      wave input buffers.
 *
 * @parm LPWAVEHDR | lpWIHdr | Far pointer to a wave input data header.
 *
 * @rdesc The return value is an MMSYS error code (0L if success).
 *
 * @comm We assume that the header and block are both page locked when they
 *      get here.  That is, it has been 'prepared.'
 ***************************************************************************/ 

static DWORD NEAR PASCAL midAddBuffer(WORD id,  LPMIDIHDR lpmh) {
    LPMIDIHDR   lpN;

    /* check if it's been prepared */
    if (!(lpmh->dwFlags & MHDR_PREPARED))
        return MIDIERR_UNPREPARED;

    /* check if it's in our queue already */
    if (lpmh->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    /* add the buffer to our queue */
    lpmh->dwFlags |= MHDR_INQUEUE;
    lpmh->dwFlags &= ~MHDR_DONE;

    /* sanity */
    lpmh->dwBytesRecorded = 0;
    lpmh->lpNext = NULL;

    CritEnter();
        {
        if ( lpN = gMIMC[id].lpmhQueue ) {
            while ( lpN->lpNext && (lpN = lpN->lpNext) )
                ;
             lpN->lpNext = lpmh;
            }
        else 
            gMIMC[id].lpmhQueue = lpmh;
        }
    CritLeave();

    /* return success */
    return ( 0L );
    }

/****************************************************************************
 * @doc INTERNAL
 * 
 * @api WORD | midSendPartBuffer | This function is called from midStop().
 *     It looks at the buffer at the head of the queue and, if it contains
 *     any data, marks it as done as sends it back to the client.
 * 
 * @rdesc The return value is the number of bytes transfered. A value of zero
 *     indicates that there was no more data in the input queue.
 ***************************************************************************/ 

void NEAR PASCAL midSendPartBuffer(WORD id) {
    if ( gMIMC[id].lpmhQueue && gMIMC[id].dwCurData ) {
		  DWORD     dwMsgTime;	
        LPMIDIHDR lpH = gMIMC[id].lpmhQueue;
        gMIMC[id].lpmhQueue = gMIMC[id].lpmhQueue->lpNext;
        gMIMC[id].dwCurData = 0L;
        lpH->dwFlags |= MHDR_DONE;
        lpH->dwFlags &= ~MHDR_INQUEUE;
        dwMsgTime = timeGetTime() - gMIMC[id].dwRefTime;
        midiCallback(&gMidiInClient[id], MIM_LONGERROR, (DWORD)lpH,
                                                    dwMsgTime);
        }
    }

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midGetDevCaps | Get the capabilities of the port.
 *
 * @parm LPBYTE | lpCaps | Far pointer to a MIDIINCAPS structure.
 *
 * @parm WORD | wSize | Size of the MIDIINCAPS structure.
 *
 * @rdesc There is no return value.
 ****************************************************************************/

static void NEAR PASCAL midGetDevCaps(LPBYTE lpCaps, WORD wSize) {
    MIDIINCAPS mc;

    mc.wMid = MM_MICROSOFT;
    mc.wPid = MM_SNDBLST_MIDIIN;
    mc.vDriverVersion = DRIVER_VERSION;
    lstrcpy(mc.szPname, aszInputName);
    _fmemcpy(lpCaps, &mc, min(wSize, sizeof(MIDIINCAPS)));
    }

WORD NEAR PASCAL getNextOpenIn(void) {
   short ii;
   
   for (ii = 0; ii < MAXCLIENT; ++ii) {
      if (openInAry[ii] == 0) {
         openInAry[ii] = 1;
         return ii;
         }
      }
   return (WORD)-1;
   }
 
/****************************************************************************

    This function conforms to the standard MIDI input driver message proc

****************************************************************************/

DWORD FAR PASCAL _loadds midMessage(WORD devId, UINT msg, DWORD dwUser, 
                                    DWORD dwParam1, DWORD dwParam2) {
short ii;
WORD  id;

    if ( !fEnabled ) {
        if ( msg == MIDM_INIT ) {
            D1("MIDM_INIT");
            return 0L;
            }

        D1("midMessage called while disabled");
        return ( (msg == MIDM_GETNUMDEVS) ? 0L : MMSYSERR_NOTENABLED );
        }

    /* this driver only supports one device */
    if (devId != 0) {
        D1("invalid midi device id");
        return MMSYSERR_BADDEVICEID;
        }

    id = LOWORD(dwUser); // Most messages use this
     
    switch ( msg ) {
        case MIDM_GETNUMDEVS:
            D1("MIDM_GETNUMDEVS");
            return 1L;

        case MIDM_GETDEVCAPS:
            D1("MIDM_GETDEVCAPS");
            midGetDevCaps((LPBYTE)dwParam1, (WORD)dwParam2);
            return 0L;

        case MIDM_OPEN:
            D1("MIDM_OPEN");

            if (nAllocIn == 0) {
               for (ii = 0; ii < MAXCLIENT; ++ii)
                  openInAry[ii] = 0;
               }
               
            if (++nAllocIn > MAXCLIENT) {
               --nAllocIn;
               return MMSYSERR_ALLOCATED;
               }
            
            id = getNextOpenIn();             
            *((DWORD FAR *)dwUser) = MAKELONG(id, 0); // remember our id
            
            fMIDIStarted[id] = FALSE;            

            /* allocate structure containing info about my client (global */
            /* because I only allow one client to access midi input). */
            
            gMidiInClient[id].dwCallback = 
                              ((LPMIDIOPENDESC)dwParam1)->dwCallback;
            gMidiInClient[id].dwInstance = 
                              ((LPMIDIOPENDESC)dwParam1)->dwInstance;
            gMidiInClient[id].hMidi   = ((LPMIDIOPENDESC)dwParam1)->hMidi;
            gMidiInClient[id].dwFlags = dwParam2;
            
            /* initialize queue stuff */
            gMIMC[id].dwCurData = 0;
            gMIMC[id].lpmhQueue = 0;

            /* NOTE: we must initialize reference time in case someone adds */
            /* longdata buffers after opening, then resets the midi stream */
            /* without starting midi input.  Otherwise, midFreeQ would give */
            /* inconsistent timestamps */
            gMIMC[id].dwRefTime = timeGetTime();

            /* notify client */
            midiCallback(&gMidiInClient[id], MIM_OPEN, 0L, 0L);

            return 0L;

        case MIDM_CLOSE:
            D1("MIDM_CLOSE");

            if ( gMIMC[id].lpmhQueue )
                return MIDIERR_STILLPLAYING;

            /* just in case they started input without adding buffers */
            midStop(id);

            /* notify client */
            midiCallback(&gMidiInClient[id], MIM_CLOSE, 0L, 0L);
            openInAry[id] = 0; // free up slot

            /* get out */
            if (nAllocIn > 0)
                --nAllocIn;
            return 0L;

        case MIDM_ADDBUFFER:
            D1("MIDM_ADDBUFFER");
            
            /* attempt to add the buffer */
            return midAddBuffer(id, (LPMIDIHDR)dwParam1);

        case MIDM_START:
            D1("MIDM_START");
            
            /* initialize all the parsing status variables */
            gMIMC[id].fSysEx     = 0;
            gMIMC[id].bStatus    = 0;
            gMIMC[id].bBytesLeft = 0;
            gMIMC[id].bBytePos   = 0;
            gMIMC[id].dwShortMsg = 0;
            gMIMC[id].dwMsgTime  = 0;
            gMIMC[id].dwRefTime  = 0;
            gMIMC[id].dwCurData  = 0;

            /* get a new reference time */
            gMIMC[id].dwRefTime = timeGetTime();

            midStart(id);
            return 0L;

        case MIDM_STOP:
            D1("MIDM_STOP");
            midStop(id);
            return 0L;

        case MIDM_RESET:
            D1("MIDM_RESET");
            
            /* stop if it is started and release all buffers */
            midStop(id);
            midFreeQ(id);
            return 0L;

        default:
            return MMSYSERR_NOTSUPPORTED;
        }

    /* should never get here */

    return MMSYSERR_NOTSUPPORTED;
    }

/****************************************************************************
 * midStart
 ***************************************************************************/ 

void NEAR PASCAL midStart(WORD id) {
    fMIDIStarted[id] = TRUE;
    }

/****************************************************************************
 * midStop
 ***************************************************************************/ 

void NEAR PASCAL midStop(WORD id) {
    if (fMIDIStarted[id]) {
        fMIDIStarted[id] = FALSE;
        midSendPartBuffer(id);    // send any unfinished headers back
        }
    }


/****************************************************************************
 * ScheduleCallBack
 ***************************************************************************/ 
 
        dwMsgTime = timeGetTime() - gMIMC[id].dwRefTime;     
        midiCallback(
        			&gMidiInClient[id], 
        			MIM_LONGDATA, 
        			(DWORD)lpmh,                  	                             
             	dwMsgTime
             	);

/****************************************************************************
 * midDumpSysEx
 ***************************************************************************/ 

void FAR PASCAL midDumpSysEx(WORD id, WORD wBank, WORD wTmb, 
                                      WORD wCmd, WORD wSize) {
     static BYTE sxBuf[SXBUFLEN];
     WORD   ii;
     WORD   nWr;

     if (!fEnabled) 
          return;
     
     // format and dump the header

     if (wCmd == TXDATA) {
         wSize = verifySize(wSize, wBank);
         midBufferFill(id, midMakeHdr(RXDATA, wBank, wTmb, wSize), FULLSXHDR);
     
         ii = 0;
         while (ii < wSize) {
             _fmemset(&sxBuf[0], 0, SXBUFLEN); // Zero out
             nWr = 0;
             switch(wBank) {
                 case 0: case 1: case 2: case 3:
                 case 4: case BNKPERCOLD: case BNKPERCNEW: 
                    nWr = midCnvtTmb(&sxBuf[0], &wBank, &wTmb);
                    break;
                 case BNKPERCMAP:
                    nWr = midCnvtPMap(&sxBuf[0], wBank, wTmb);
                    break;
                 case SYSPARM: 
                 case SYSPARMNEW:
                    nWr = GetParms(&sxBuf[0], wBank, wSize);
                    break;
                 default:
                    break;
                 }
             if (nWr == 0)
                 break;
             midBufferFill(id, &sxBuf[0], nWr);
             ii += nWr;
             }
         }
     sxBuf[0] = STATUS_EOX; // end of message
     midBufferFill(id, &sxBuf[0], 1);     
     }

/****************************************************************************
 * verifySize
 ***************************************************************************/ 

WORD NEAR PASCAL verifySize(WORD wSize, WORD wBank) {    
    WORD nSize = 0;
    WORD maxSz;
    WORD mapXtra;
    
    mapXtra = (2 * NUMDRUMNOTES);
    
    switch(wBank) {
        case 0: case 1: case 2: case 3: case 4: // can only be in this block
            maxSz = (MAXBANK - wBank) * SXTMBLEN * MAXTMB;
            nSize = wSize - (wSize % SXTMBLEN);
            if (nSize > maxSz)
                nSize = maxSz;
            break;
        case BNKPERCOLD: // Can only be this or this and perc map
            maxSz = (SXTMBLEN * MAXDRUMTMB);
            if (wSize > maxSz) {
                if (wSize >= (maxSz+mapXtra))
                    nSize = maxSz+mapXtra;
                else
                    nSize = maxSz;
                }
            else
                nSize = wSize - (wSize % SXTMBLEN);            
            break;
        case BNKPERCMAP:
            if (wSize >= mapXtra)
                nSize = mapXtra;
            else 
                nSize = 0;
            break;
        case SYSPARM:            
            nSize = SXTMBLEN;
            break;
        case SYSPARMNEW:            
            if (wSize == sizeof(SYSPARMS))
               nSize = wSize;
            else
               nSize = SXTMBLEN+1; // vers 2.0
            break;
        case BNKPERCNEW: // Can only be this 
            maxSz = (SXTMBLEN * MAXTMB);
            if (wSize > maxSz) 
                nSize = maxSz;
            else
                nSize = wSize - (wSize % SXTMBLEN);            
            break;
        }
    return nSize;
    } 

/****************************************************************************
 * midBufferFill
 ***************************************************************************/ 

void NEAR PASCAL midBufferFill(WORD id, LPBYTE lpsx, WORD wLen) {
    LPMIDIHDR   lpmh;
    BYTE        bByte;
    WORD        ii;
    DWORD       dwMsgTime;
    
    /* if no buffers, nothing happens */
    if (!(lpmh = gMIMC[id].lpmhQueue)) 
        return;

    for (ii = 0; ii < wLen; ++ii) {
        bByte = lpsx[ii];
         
        /* if the long message is being terminated, only save eox byte */
        if ((bByte < STATUS_FIRST) || (bByte == STATUS_EOX) || 
            (bByte == STATUS_SYSEX)) {
    
            /* write the data into the long message buffer */
            *((HPSTR)(lpmh->lpData) + gMIMC[id].dwCurData++) = bByte;

            /* if !(end of sysex or buffer full), return */
            if ((bByte != STATUS_EOX) && 
                 (gMIMC[id].dwCurData < lpmh->dwBufferLength))
                continue;
            }

        /* send client back the data buffer */
        D1("bufferdone");
        gMIMC[id].lpmhQueue   = gMIMC[id].lpmhQueue->lpNext;
        lpmh->dwBytesRecorded = gMIMC[id].dwCurData;
        gMIMC[id].dwCurData   = 0L;
        lpmh->dwFlags        |= MHDR_DONE;
        lpmh->dwFlags        &= ~MHDR_INQUEUE;
        ScheduleCallBack(&gMidiInClient[id], (DWORD)lpmh);
        if (!(lpmh = gMIMC[id].lpmhQueue)) 
            return;
        }
   }

/****************************************************************************
 * midMakeHdr
 ***************************************************************************/ 

LPBYTE NEAR PASCAL midMakeHdr(WORD wCmd, WORD Addr1, WORD Addr2, WORD wSize) {
    short       ii;
    static  BYTE SxTxHdr[FULLSXHDR] = {0xF0, 0x00, 0x00, 0x5B, 0x7F, 0x01,
                                       RXDATA, 00, 00, 00, 00};
    ii = 6; // start of address
    SxTxHdr[ii++] = (BYTE)wCmd;                                  
    SxTxHdr[ii++] = (BYTE)Addr1;
    SxTxHdr[ii++] = (BYTE)Addr2;
    SxTxHdr[ii++] = (BYTE)(wSize >> 7);
    SxTxHdr[ii++] = (BYTE)(wSize & 0x7F);    
    return (LPBYTE)&SxTxHdr[0];
    }

/****************************************************************************
 * midCnvtTmb
 ***************************************************************************/ 

static short NEAR PASCAL  midCnvtTmb(LPBYTE SXBuf, LPWORD pBnk, LPWORD pTmb) {
   	static SBTIMBRE ltmb; // Local buf
	short nBnk = *pBnk;
	short nTmb = *pTmb;
    
    if (nBnk < MAXBANK)
        ltmb = Tmb[nBnk].snd[nTmb];
    else if (nBnk == BNKPERCOLD) // Percussion bank Obsolete
        ltmb = Drm.snd[nTmb+FIRSTDRUMNOTE];
    else if (nBnk == BNKPERCNEW) // Percussion bank New
        ltmb = Drm.snd[nTmb];
    else // Bad!
        return 0;
    
    // Just run thru
        
    SXBuf[MDATK] = ltmb.modad   >> 4;
    SXBuf[MDDCY] = ltmb.modad    & 0x0F;        
	 SXBuf[MDSTN] = ltmb.modsr   >> 4;
    SXBuf[MDREL] = ltmb.modsr    & 0x0F;
	 SXBuf[MDFLG] = ltmb.modchar >> 4;
    SXBuf[MDFQM] = ltmb.modchar  & 0x0F;
	 SXBuf[MDKSL] = ltmb.modscal >> 6;
    SXBuf[MDLEV] = ltmb.modscal  & 0x3F;    
	 SXBuf[MDWAV] = ltmb.modwave  & 0x07;
	 SXBuf[FBCNT] = ltmb.feedback & 0x0F;
    
	 SXBuf[CRATK] = ltmb.carad   >> 4;
    SXBuf[CRDCY] = ltmb.carad    & 0x0F;
	 SXBuf[CRSTN] = ltmb.carsr   >> 4;
    SXBuf[CRREL] = ltmb.carsr    & 0x0F;    
	 SXBuf[CRFLG] = ltmb.carchar >> 4;
    SXBuf[CRFQM] = ltmb.carchar  & 0x0F;
	 SXBuf[CRKSL] = ltmb.carscal >> 6;
    SXBuf[CRLEV] = ltmb.carscal  & 0x3F;    
	 SXBuf[CRWAV] = ltmb.carwave  & 0x07;
    SXBuf[PVOC]  = ltmb.percvoc;
    SXBuf[PITCH] = ltmb.dpitch;    
    if (ltmb.transpos < 0) // only less than zero for ofs > 63
        SXBuf[TRANP] = (BYTE)((short)(64 - ltmb.transpos) & 0x7F);    
    else
        SXBuf[TRANP] = (BYTE)ltmb.transpos;

    switch(nBnk) {
        case 0:
        case 1:
        case 2:
        case 3:
            if (++nTmb >= MAXTMB) {
                nTmb = 0;
                ++nBnk;
                }
            break;
        case 4:
            if (++nTmb >= MAXTMB) {
                nTmb = 0;
                nBnk = 0;
                }
            break;
        case BNKPERCOLD:
            if (++nTmb >= NUMDRUMNOTES) {
                nTmb = 0;
                nBnk = BNKPERCMAP; // Don't return
                }
            break;
        case BNKPERCNEW:
            if (++nTmb >= MAXTMB) {
                nTmb = 0;
                nBnk = 0; // Don't return
                }
            break;
        default:
            break;
        }
	*pBnk = nBnk;
	*pTmb = nTmb;
    return SXTMBLEN;
	}

/****************************************************************************
 * midCnvtPMap
 ***************************************************************************/ 

static short NEAR PASCAL midCnvtPMap(LPBYTE SXBuf, WORD wBnk, WORD wTmb) {
   	static short nCt = 0;
    short  ii;
    BYTE   idx;

    for (ii = 0; ii < SXBUFLEN; ++ii) {
        if (nCt < NUMDRUMNOTES) {
            idx = (BYTE)nCt + FIRSTDRUMNOTE;
            SXBuf[ii]   = idx; 
            SXBuf[++ii] = Drm.snd[idx].dpitch;
            ++nCt;
            }
        else {
            nCt = 0; // reset
            break;
            }
        }
    return ii;
    }

/****************************************************************************
 * GetParms
 ***************************************************************************/ 
// fills buffer with system parms

short FAR PASCAL GetParms(LPBYTE SXBuf, WORD wloc, WORD wSz) {
    short ii;
    
    // Just run thru
    SXBuf[STMODE] = isStereo();
    SXBuf[PRMODE] = isPercussion();
    SXBuf[PRCHAN] = isPercussion() ? (BYTE)thePercChannel() : 0;
    SXBuf[DFTBNK] = (BYTE)theCurrentBank();
    SXBuf[VIBDPT] = (BYTE)VIBRBIT(theModnDepth());
    SXBuf[TRMDPT] = (BYTE)TREMBIT(theModnDepth());
    SXBuf[BNDRNG] = (BYTE)theBendRange((WORD)-1);
    SXBuf[SAVSET] = isSaveSet();
    for (ii = 0; ii < 16; ++ii)
         SXBuf[FSTCHN+ii] = isChanOn(ii);

    ii = LSTCHN+1; // for version 1.x
    if (wloc == SYSPARMNEW) {
        SXBuf[PRCBST] = isPercussion() ? (BYTE)thePercBoost()+64 : 64;
        ii = PRCBST+1;
        if (wSz > MAPREG) { // ver 2.10
           SXBuf[FIXPRC] = (BYTE)isPitchFixed();
           SXBuf[MAPREG] = (BYTE)isRegShadow();
           ii = MAPREG+1;
           }
        }
    
    return ii;
    }

