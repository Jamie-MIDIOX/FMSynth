/***************************************************************************
 *
 *   midimain.c
 *
 *   Copyright (c) 1993 by Jamie O'Connell
 *  
 *   Adapted for FMSYNTH - 930411 JWO
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "fmsynth.h"
#include "fmsfunc.h"
#include "sysparm.h"
#include "fmcfg.h"
#include "midic.h"
#include "midiin.h"
#include "midimain.h"
#include "init.h"
#include "fmapi.h"

/***************************************************************************

    internal function prototypes

***************************************************************************/

static void NEAR PASCAL synthNoteOff(MIDIMSG msg);
static void NEAR PASCAL synthNoteOn(MIDIMSG msg);
static void NEAR PASCAL synthPitchBend(MIDIMSG msg);
static void NEAR PASCAL synthControlChange(MIDIMSG msg);
static void NEAR PASCAL synthProgramChange(MIDIMSG msg);
static void NEAR PASCAL synthSysEx(MIDIMSG msg);
static void NEAR PASCAL synthKeyPressure(MIDIMSG msg);
static void NEAR PASCAL synthChannelPressure(MIDIMSG msg);

static short NEAR PASCAL findFreeVox(WORD wChn, WORD wKey); 
static void  NEAR PASCAL voiceOff(short n); 
static void  NEAR PASCAL forceOff(short n); 
static void  NEAR PASCAL doSoundOff(WORD wChn);
static void  NEAR PASCAL doAllNotesOff(WORD wChn);
static void  NEAR PASCAL doVolume(WORD wChn, WORD wAmt);
static void  NEAR PASCAL doPan(WORD wChn, WORD wAmt); 
static void  NEAR PASCAL doModn( WORD wChn, WORD wAmt );
static void  NEAR PASCAL doBend(WORD wChn, WORD wAmt);
static void  NEAR PASCAL doBankChg(WORD wChn, WORD which, WORD wAmt);
static void  NEAR PASCAL doSysEx(BYTE mb);
static void  NEAR PASCAL StoreBuffer(BYTE SXBuf[], short *pBnk, short *pTmb);
static void  NEAR PASCAL StorePercMap(LPBYTE SXBuf, short bufIdx);
static void  NEAR PASCAL StoreParms(WORD Vers, BYTE SXBuf[], short bufIdx);
static void  NEAR PASCAL resetAllCntrl(WORD wChn);
static void  NEAR PASCAL pedal(WORD wChn, short bDown);
static void  NEAR PASCAL setParm(short which, WORD wChan, short nData);
static void  NEAR PASCAL dataEntry(short which, WORD wChan, short nData);
static void  NEAR PASCAL dataIncr(WORD wChan, short nData);
static void  NEAR PASCAL dataDecr(WORD wChan, short nData);
static void  NEAR clearVox(void);
static void  NEAR PASCAL drumMode(BOOL fOn, WORD wChn);
 
static void  NEAR qInit(void);
static void  NEAR PASCAL qAppend(WORD wData); 
static void  NEAR PASCAL qDelete(WORD wData);
static short NEAR PASCAL qNext(BOOL bReset);
static BOOL  NEAR PASCAL qFull(short n);
void   NEAR  PASCAL setID(WORD id);
WORD   NEAR  PASCAL getID(void);

/***************************************************************************

    local data

***************************************************************************/
 
static  VOICE  vox[MAX_VOICES]; 
static  short  nItem;               // Current size of Queue
static  short  nIdx;                // Current index into Queue
static  BYTE   abyQueue[ MAXQUEUE ]; // Note On Queue

static WORD  wParm[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                                                // Registered Parameter Number
static WORD  wParmVal[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                                                // Parameter Value
static PARMFLAG rcv[16]   = {{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
                   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
                   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }};  
                                               // Reg Parm Flags

#define msg_ch         msg.ch /* all messages */

#define msg_note       msg.b1 /* noteoff(0x80),noteon(0x90),keypress(0xA0)*/
#define msg_controller msg.b1 /* controlchange(0xB0) */
#define msg_patch      msg.b1 /* programchange(0xC0) */
#define msg_cpress     msg.b1 /* channelpressure(0xD0) */
#define msg_lsb        msg.b1 /* pitchbend(0xE0) */

#define msg_velocity   msg.b2 /* noteoff(0x80), noteon(0x90) */
#define msg_kpress     msg.b2 /* keypressure(0xA0) */
#define msg_value      msg.b2 /* controlchange(0xB0) */
#define msg_unused     msg.b2 /* programchange(0xC0), channelpressure(0xD0) */
#define msg_msb        msg.b2 /* pitchbend(0xE0) */
  
static  BYTE PercVocMap[18] = {0, 1, 2, 3, 4, 5, 9, 10, 11, 12, 
                        13, 14, 15, 16, 17, 0, 0, 0};
static  BYTE MeloVocMap[18] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
                        11, 12, 13, 14, 15, 16, 17};
static  BYTE VocMap[18] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
                        11, 12, 13, 14, 15, 16, 17};
                        
static  BYTE mHdr[MAXSXHDR] = {0xF0, 0x00, 0x00, 0x5B, 0x7F, 0x01};

/***************************************************************************
 *
 *  MIDI function director array
 *
 *  (x = channel)
 *  0x8x        Note Off
 *  0x9x        Note On                (vel 0 == Note Off)
 *  0xAx        Key Pressure (Aftertouch)
 *  0xBx        Control Change
 *  0xCx        Program Change
 *  0xDx        Channel Pressure (Aftertouch)
 *  0xEx        Pitch Bend Change
 *  0xF0        Sysex
 *  011111sssb  System Common
 *  011110tttb  System Real Time
 *
 *************************************************************************/

static void (NEAR PASCAL * synthmidi [8])(MIDIMSG);

static void (NEAR PASCAL * synthmidi [])() = {
        synthNoteOff,
        synthNoteOn,
        synthKeyPressure,       /* key pressure now currently implemented */
        synthControlChange,
        synthProgramChange,
        synthChannelPressure,   /* channel press now currently implemented */
        synthPitchBend,
        synthSysEx              /* sysex etc. not currently implemented */
        };

/***************************************************************************

    public functions

***************************************************************************/

void FAR synthInit(void) {
    qInit();                    // initialize Note On queue
    clearVox();                 // initialize Vox Structure
    }

void FAR PercMap(void) {
    short ii;
    
    for (ii = 0; ii < 18; ++ii) 
        VocMap[ii] = PercVocMap[ii];
    }

void FAR MeloMap(void) {
    short ii;
 
    for (ii = 0; ii < 18; ++ii) 
        VocMap[ii] = MeloVocMap[ii];
    }                

/**************************************************************************
 *
 *     Process a stream of MIDI messages by calling
 *     the appropriate function based on each status byte.  The function deals
 *     with messages spanning buffers, and with invalid data being passed.
 *
 *     In general, the function steps through the buffer, using the current
 *     state in determining what to look for in checking the next data byte.
 *     This may mean looking for a new status byte, or looking for data
 *     associated with a current status whose data has not been completely
 *     read yet.
 *
 *     The key item to determine the current state of message processing is
 *     the "bCurrentLen" global static byte, which indicates the number of
 *     bytes that are needed to complete the current message being processed.
 *     The current message is stored in the local static "bCurrentStatus", not
 *     to be confused with the global static "status", which is the running
 *     status.  The local static "msg" is used to build the current message,
 *     and is static in order to enable messages to cross buffer boundaries.
 *     The local static "bPosition" determines where in the message buffer to
 *     place the next byte of the message, if any.
 *
 *     The first item in the processing loop is a check for the presence of a
 *     real time message, which can occur anywhere in the data stream, & does
 *     not affect the current state (unless it is possibly a reset command).
 *     Real time messages are ignored for now, including the reset command.
 *     After ignoring them, the loop is continued in case that was the last
 *     byte in the buffer.  If the loop was not continued at this point, the
 *     message sending portion would not function correctly, as "bCurrentLen"
 *     and "bCurrentStatus" is not modified by a real time message.
 *
 *     The next loop item checks to determine if a message is currently being
 *     built.  If "bCurrentLen" is zero, no message is being built, and the
 *     next status byte can be retrieved.  At this point, the current message
 *     position is reset, as any new byte will be the first for this new
 *     message.
 *
 *     If the next byte is a system command, as opposed to a channel command,
 *     it must reset the current running status, and extract the message lengt
 *     from a different message length table (subtracting one for the status
 *     already retrieved).  Even though these messages are eventually ignored,
 *     the actual message buffer is built as normal.  This will enable a
 *     function to be attached to the message function table which would deal
 *     with system messages.  Note that the system message id is placed into
 *     the channel portion of the message, in place of the channel for a norma
 *     message.
 *
 *     If the next byte is not a system command, then it might be either a
 *     new status byte, or a data byte which depends upon the running status.
 *     If it is a new status byte, running status is updated.  If it is not,
 *     and there is not running status, the byte is ignored, and the loop is
 *     continued.  This might be the case when ignoring data after a SYSEX,
 *     or when invalid data occurs.  If a valid status byte is retrieved, or
 *     is already present in the running status, the internal current status
 *     is updated, in case this message spans buffers, and the length of the
 *     message is retrieved from the channel table (subtracting one for the
 *     status byte already retrieved).
 *
 *     At this point, the message may be completely built (i.e., a one byte
 *     message), in which case, it will fall into the message dispatch code.
 *
 *     The next loop item is fallen into if a message is currently being
 *     processed.  It checks the next byte in the buffer to ensure that it
 *     is actually a data byte.  If it is a status byte instead, the current
 *     message is aborted by resetting "bCurrentLen", and the loop is
 *     continued.  Note that running status is not reset in this case.
 *
 *     If however the next byte is valid, it is placed in either the first or
 *     the second data position in the message being built.  The local static
 *     "bPosition" is used to determine which in position to place the data.
 *
 *     The next loop item checks to see if a complete message has been built.
 *     If so, it dispatches the message based on the current command.  It does
 *     not use running status, as that might have been reset for a system
 *     command.  If a function for the particular command is present, the
 *     message is dispatched, else it is ignored.  If the message was not
 *     complete, the next pass through the loop will pick up the next data
 *     byte for the message.
 *
 *     The loop then continues until it is out of data.
 *
 *    lpBuf | Points to a buffer containing the stream of MIDI
 *     data.
 *
 *    dwLength | Contains the length of the data pointed to by
 *     <p>lpBuf<d>.
 *
 *************************************************************************/

static theID = 0;

void NEAR PASCAL setID(WORD id) {
   theID = id;
   }

WORD NEAR PASCAL getID(void) {
   return theID;
   }

void NEAR PASCAL synthMidiData(WORD id, HPBYTE lpBuf, DWORD dwLength) {
    static MIDIMSG msg;
    static BYTE bCurrentStatus;
    static BYTE bPosition;
    BYTE bByte;

    if (_MIDIDisable) // do nothing until turned back on
       return;
    
    setID(id);
    
    for (; dwLength; dwLength--) {
        bByte = *lpBuf++;
        
        if (bByte >= STATUS_TIMINGCLOCK)
            continue;

        if (!bCurrentLen) {
kludge_city:
            bPosition = 0;
            if (bByte >= STATUS_SYSEX) {
                bCurrentStatus = bByte;
                status[id] = 0;
                bCurrentLen = (BYTE)(SYSLENGTH(bCurrentStatus) - 1);
                msg_ch = 0;
                msg.b1 = bByte;
                }
            else {  
                msg_ch = 0;
                msg.b1 = bByte;
                if (bByte >= STATUS_NOTEOFF)
                    status[id] = bByte;
                else if (!status[id]) // Now we call SysX routine
                    goto Kludge2; // Getting messy...
                bCurrentStatus = status[id];
                bCurrentLen    = (BYTE)(MIDILENGTH(status[id]) - 1);
                if (bByte < STATUS_NOTEOFF)
                    goto first_byte;
                msg_ch = FILTERSTATUS(bCurrentStatus);
                }
            }
        else {
            if (bByte >= STATUS_NOTEOFF)
                goto kludge_city;
            if (!bPosition) {
first_byte:
                bPosition++;
                msg.b1 = bByte;
                }
            else
                msg.b2 = bByte;
            --bCurrentLen;
            }
        if (!bCurrentLen) {
Kludge2:
            bByte = (BYTE)((bCurrentStatus >> 4) & 0x07);
            if (*synthmidi[bByte]) 
               (*synthmidi[bByte]) (msg);
            else
               D1("MIDI message type not supported");
            }
        }
    }


/* =================================================================== */

void NEAR PASCAL drumMode(BOOL fOn, WORD wChn) {
    short n;
    
    if (fOn) {
        if (isPercussion()) // already there
            return;
        nNumVox = nVoxPerc;
        PercMap();
        }
    else {
        if (!isPercussion()) // already not
            return;
        nNumVox = nVoxMelo;
        MeloMap();
        }                

    SetPercussion(fOn, FALSE);
    
   /* Turn off any voices currently on for the Percussion channel */
      
   for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
       if (n == LBDRUM || n == LHHSD || n == LTTCY) {
           sndNote(n, vox[n].bKey, 0, vox[n].bChn);
           qDelete(n);
           vox[n].dwTm    = 0L;           
           vox[n].bKey    = NOKEY;
           vox[n].bChn    = 0;
           vox[n].bPatch  = NOPATCH;
           vox[n].bBank   = DFTBANK;  // Patch Bank           
           vox[n].bPedal  = PEDLUP;
           vox[n].bNoteOn = 0;
           }
       }                                       
       
   chanInfo[wChn].bType = fOn ? PERCTYPE : MELOTYPE;    
   sndDrumMode(fOn);
   } 

//=========================================================================
/* Note: MIDI spec says that All Notes Off does NOT overrule pedal.
 * Rather, only like lifting all keys.   So we'll look thru the
 * Note On queue and use voiceOff() which checks the pedal before
 * removing the voice.
 */

void NEAR PASCAL synthAllNotesOff(void) {
    short n;
    
    for (n = qNext(TRUE); n != -1; n = qNext(FALSE))
         voiceOff(n);
    }

/***************************************************************************

    private functions

***************************************************************************/

static void NEAR PASCAL synthNoteOn(MIDIMSG msg) {
    short voice;
     
    if (!isChanOn(msg_ch)) 
        return; // Filter channels
     
    if ((msg_velocity > 127) || (msg_note > 127)) // illegal
        return;

    if (isPercussion() && PercChan(msg_ch)) {    
        sndNote(-1, msg_note, msg_velocity, msg_ch);
        }
    else { // Melodic Note => must obtain a voice
        if (msg_velocity == 0) {          /* 0 velocity means note off */
            synthNoteOff(msg);
            return;
            }                         
        if ((voice = findFreeVox(msg_ch, msg_note)) != -1)        
            sndNote(voice, msg_note, msg_velocity, msg_ch);    
        }
    }
 
/* Given a MIDI channel and a key number, see if that note really is
 * on now.  If so, call voiceOff to check whether pedal is down for
 * this channel.  If so, merely flag that note should expire when
 * pedal lifted, else turn it off right now.
 */

static void NEAR PASCAL synthNoteOff(MIDIMSG msg) {
    short n;
 
    if (!isChanOn(msg_ch)) 
        return; // Filter channels
         
    for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
        if (vox[n].bKey    == msg_note &&
               vox[n].bChn == msg_ch   && 
               vox[n].bNoteOn) {  // Found it
            voiceOff(n);
            return;               // all done
            }
        }
    }

/* =================================================================== */
/* Given a voice, n, check the pedal.   If up, turn it off and perform */
/* housekeeping chores */

static void NEAR PASCAL voiceOff(short n) {  
    vox[n].bNoteOn = 0;              // indicate the note is really off
    vox[n].dwTm    = timeGetTime();      // !!!!!
    if (vox[n].bPedal == PEDLUP) {   // if pedal's on keep sounding note
        qDelete(n);                  // pull voice from queue
        sndNote(n, vox[n].bKey, 0, vox[n].bChn);
        }
    }

/* =================================================================== */

static void NEAR PASCAL forceOff(short n) {
    vox[n].bNoteOn = 0;              // indicate the note is really off
    vox[n].bPedal  = PEDLUP;   
 
    qDelete(n);                      // pull voice from queue
    sndNote(n, vox[n].bKey, 0, vox[n].bChn);
    }
    
/* These functions are commented out because we are not currently supporting
 * channel and key pressure messages in this driver.  Ad Lib had originally
 * interpreted them as volume values, which produces incorrect results.
 * I haven't implemented them because it's not clear from the technical
 * documentation how produce the low-frequency oscillation that is often
 * produced by these messages.  To support the messages, change the
 * entries in the synthmidi array to call these functions again, uncomment
 * these two functions, and define an xSetVoicePressure routine.
 *
 * NOTE: We're going to use the modulation parameter for these routines
 *       JWO 940314
 */

/**************************************************************************

 **************************************************************************/
static void NEAR PASCAL synthChannelPressure(MIDIMSG msg) {
    short n;
    
    if (!isChanOn(msg_ch)) 
        return; // Filter channels

    if (!(isPercussion() && PercChan(msg_ch))) { // No percussion pressure
        for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
            if (vox[n].bChn == (BYTE)msg_ch)
                sndPressure(n, msg_cpress, msg_ch);
            }
        }
    }
 
/**************************************************************************

 **************************************************************************/
static void NEAR PASCAL synthKeyPressure(MIDIMSG msg) {
    short n;
    
    if (!isChanOn(msg_ch)) 
        return;                   // Filter channels

    for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
        if (vox[n].bKey == msg_note  &&
               vox[n].bChn == msg_ch && 
               vox[n].bNoteOn) {  // Found it
            sndPressure(n, msg_kpress, msg_ch);
            return;               // all done
            }
        }
    }

/* ======================================================================= */

static short NEAR PASCAL findFreeVox(WORD wChn, WORD wKey) {
    short n;
    short ii;
    short nCouldUse   = -1;
    short nCouldSteal = -1;    
    BYTE  bPatch = chanInfo[wChn].bPatch;
    BYTE  bBank  = (BYTE)chanInfo[wChn].wBank;
    DWORD dwCur;

    dwCur = timeGetTime();      // !!!!!
    
    for (ii=0; ii < nNumVox; ++ii) {
        n = (short)VocMap[ii];
        if ((vox[n].bNoteOn == 0) && (vox[n].bPedal == PEDLUP)) {
           /* If this voice already set up for the patch, great. Else
            * remember this as okay but needing to have patch set up.
            */
           if (useOldOff()) { // use oldest off voice
              if ((nCouldUse == -1) || (vox[n].dwTm < vox[nCouldUse].dwTm))
                 nCouldUse = n;
              }
           else if ((vox[n].bPatch == bPatch) && (vox[n].bBank == bBank)) 
               goto GOT_ONE;             /* perfect */
           else 
               nCouldUse = n;            /* take from the rear */
           }
        }

    if (nCouldUse != -1) {
        n = nCouldUse;
        goto GOT_ONE;
        }
     
    n = qNext(TRUE);                        // Grab oldest voice
    if ((dwCur - vox[n].dwTm) < (DWORD)wMinWait) // started too recently 
       return -1;
    forceOff(n);                            // Force Note off

GOT_ONE:
    if ((vox[n].bPatch != bPatch) || (vox[n].bBank != bBank)) 
       sndPatch(n, wChn, bBank, bPatch);  

    vox[n].dwTm    = dwCur;                 // current time
    vox[n].bKey    = (BYTE)wKey;
    vox[n].bChn    = (BYTE)wChn;
    vox[n].bPedal  = chanInfo[wChn].bPedal; // in case pedal currently down
    vox[n].bNoteOn = 1;                     // it's turning on
    vox[n].bPatch  = bPatch;
    vox[n].bBank   = bBank;

    qAppend(n); 
    
    return n;
    }

/* ======================================================================= */
/* If lifting up, search for notes that we've marked to expire
 * when pedal is lifted, because their key was released while
 * pedal down.   We do NOT turn off notes whose key is still down.
 */

static void NEAR PASCAL pedal(WORD wChn, short bDown) {
    short n;

    chanInfo[wChn].bPedal = (BYTE)bDown;

    /* If pressing down, notify all notes currently on */

    for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
        if (vox[n].bChn == (BYTE)wChn) {
            if (bDown == PEDLDOWN)  // pedal's turning on
                vox[n].bPedal = PEDLDOWN;
            else {              // pedals coming up -> remove indicators
                vox[n].bPedal = PEDLUP;
                if (vox[n].bNoteOn == 0)    // free up voice
                    voiceOff(n);
                }
            }
        }
    }

/**************************************************************************

 **************************************************************************/

static void NEAR PASCAL synthPitchBend(MIDIMSG msg) {
    WORD wPB = (((WORD)msg_msb) << 7) | msg_lsb;

    /* msb is shifted by 7 because we've redefined the MIDI pitch bend
     * range of 0 - 0x7f7f to 0 - 3fff by concatenating the two
     * 7-bit values in msb and lsb together
     */
    
    if (!isChanOn(msg_ch)) 
        return; // Filter channels
                          
    doBend(msg_ch, wPB);
    }

/**************************************************************************/

static void NEAR PASCAL synthSysEx(MIDIMSG msg) {
    static BOOL  inSysX  = FALSE;
    static BOOL  isOurs  = FALSE;
    static short idx = 0;
    
    if (!inSysX) {
        if (!isOurs) {         // Maybe it is
            if (msg.b1 == mHdr[idx++]) {
                if (idx >= MAXSXHDR) {
                    isOurs = TRUE;
                    inSysX = TRUE;
                    idx    = 0;   // reset 
                    }
                }
            else {
                inSysX = TRUE; // not ours => ignore rest 
                idx = 0;
                }
            }
        }
    else if (isOurs)           // Our message => keep processing
        doSysEx(msg.b1);

    if (msg.b1 == STATUS_EOX) {
        isOurs = FALSE;
        inSysX = FALSE;
        idx    = 0;   // reset 
        }
    }    

/**************************************************************************
 * 
 * Controllers:
 *        0     Bank Change
 *        1     Modulation
 *        6     Data Entry MSB
 *        7     Volume
 *       10     Pan
 *       38     Data Entry LSB
 *       64     Sustain Pedal
 *       92     Tremolo Depth
 *       96     Data Increment
 *       97     Data decrement
 *      100     RPN LSB
 *      101     RPN MSB      
 *      120     All Sound Off     
 *      121     Reset All Controllers
 *      123     All Notes off
 *              
 **************************************************************************/

static void NEAR PASCAL synthControlChange(MIDIMSG msg) {

    if (!isChanOn(msg_ch)) 
        return; // Filter channels
    
    if (msg_value > 127)
        return; // illegal
        
    switch(msg_controller) {      
        case   0: // Bank change
            doBankChg(msg_ch, MSB, msg_value);
            break;
        case   1: // Modulation Wheel
            doModn(msg_ch, msg_value);                     
            break;
        case   6: // Data Entry MSB
            dataEntry(MSB, msg_ch, msg_value);
            break;
        case   7: // volume controller
            doVolume(msg_ch, msg_value);
            break;
        case  10: // Panning controller
            doPan(msg_ch, msg_value);
            break;
        case  32: // Bank change LSB
            doBankChg(msg_ch, LSB, msg_value);
            break;
        case  38: // Data Entry LSB
            dataEntry(LSB, msg_ch, msg_value);
            break;
        case  64: // sustain pedal
            pedal(msg_ch, ((msg_value < 0x40) ? PEDLUP : PEDLDOWN));
            break;
        case  92: // Tremolo Depth
            sndModDepth(msg_value);
            break;
        case  96: // Data increment
            dataIncr(msg_ch, msg_value);
            break;
        case  97: // Data decrement
            dataDecr(msg_ch, msg_value);
            break;
        case 100: // Reg Parm LSB
            setParm(LSB, msg_ch, msg_value);
            break;
        case 101: // Reg Parm MSB
            setParm(MSB, msg_ch, msg_value);
            break;
        // The following are Channel Mode
        case 120: // Sound Off
            doSoundOff(msg_ch);
            break;
        case 121: // Reset Controllers
            resetAllCntrl(msg_ch);
            break;
        case 123: // All Notes Off
            doAllNotesOff(msg_ch);
            break;
        } 
    }

/**************************************************************************

 **************************************************************************/
/* This doesn't actually change until the next note is sounded on the 
 * channel.  In order to support Voyetra and Twelve Tone MIDI files we
 * support the Channel 10 drum mode switch.  NOTE: Not anymore unless the
 * INI file has OldPercSwitch=1
 */

static void NEAR PASCAL synthProgramChange(MIDIMSG msg) { 
    WORD wBank;
    
    if (!isChanOn(msg_ch)) 
        return; // Filter channels
     
    if (msg_patch > 127) // illegal
        return;
        
    if ((msg_ch == PERC_CHAN) && isPercSwitch()) {
        if (msg_patch == 126) {  // This is the only hard coded one (chan 10)
            drumMode(TRUE, PERC_CHAN);       // Go into Percussion mode
            return;
            }
        else if (msg_patch == 127) {
            drumMode(FALSE, PERC_CHAN);
            return;
            }
        } 
        
    // Set up Timbre in buffer 
    chanInfo[msg_ch].bPatch = msg_patch;
    wBank = chanInfo[msg_ch].wBank;
    chanInfo[msg_ch].Timbre = Tmb[wBank].snd[msg_patch];
    }

/* ==================================================================== */
// called to change volume (output level) of channel

static void NEAR PASCAL doVolume(WORD wChn, WORD wAmt) {
    short n;

    chanInfo[wChn].bVolum = (BYTE)wAmt;    // save for future notes

    if (isPercussion() && PercChan(wChn))       // control percussion volume
        sndVolume(-1, wAmt, wChn);
    else {
        for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
            if (vox[n].bChn == (BYTE)wChn)
                sndVolume(n, wAmt, wChn);
            }
        }
    }

/* =================================================================== */
// called to change Pan (stereo Image) of channel

static void NEAR PASCAL doPan(WORD wChn, WORD wAmt) {
    short n;

    chanInfo[wChn].bPan = (BYTE)wAmt;    // save for future notes

    if (isPercussion() && PercChan(wChn)) 
        sndPan(-1, wChn);      
    else {
        for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
            if (vox[n].bChn == (BYTE)wChn)
                sndPan(n, wChn);
            }
        }      
    }

/* =================================================================== */
// called to change Pan (stereo Image) of channel

static void NEAR PASCAL doBend(WORD wChn, WORD wAmt) {
    short n;

    chanInfo[wChn].wBend = wAmt;        // record for future notes

    for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
        if (vox[n].bChn == wChn)
            sndBend(n, wChn);
        }
    }

/* =================================================================== */
// called to turn all sound off - all params unchanged

static void NEAR PASCAL doSoundOff(WORD wChn) {
    short n;
    
    for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
        sndNote(n, vox[n].bKey, 0, vox[n].bChn);
        }
    }

/* =================================================================== */
// called to change Patch Bank for channel

static void NEAR PASCAL doBankChg(WORD wChn, WORD which, WORD wAmt) {
    static WORD wLSB  = 0; // we record the state of controller received
    static WORD wMSB  = 0; // we record the state of controller received
    static WORD wLVal = 0;
    static WORD wHVal = 0;
    WORD wChBit;

    wChBit = 1 << wChn; // get bit in channel position

    switch(which) {
        case LSB:
            wLSB |= wChBit;    // save bit state
            wLVal = wAmt;      // save value
            break;
        case MSB:
            wMSB |= wChBit;
            wHVal = wAmt;
            break;
        }
                                // Our MAX is 0x7F, 0x7F => 16384
    if ((wChBit & wMSB) != 0) { // have High (Low ignored)
        wLSB &= ~wChBit;               // toggle bit off
        wMSB &= ~wChBit;               // toggle off            
            
        if (wHVal < MAXBANK)
            wAmt = wHVal;              // We look only at MSB           
        else
            wAmt = (wHVal >> 4);       // look only a top 3 bits

        if (wAmt >= MAXBANK)
            wAmt = DFTBANK;
        chanInfo[wChn].wBank = wAmt;   // record for future notes
        }
    }

/* =================================================================== */
// called to change modulation level of channel

static void NEAR PASCAL doModn( WORD wChn, WORD wAmt ) {
    short n;

    chanInfo[wChn].bModn = (BYTE)wAmt;  

    if (!(isPercussion() && PercChan(wChn))) { // No percussion modulation
        for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
            if (vox[n].bChn == (BYTE)wChn)
                sndModn(n, wChn);
            }
        }
    }

/* =================================================================== */
static void NEAR PASCAL doAllNotesOff(WORD wChn) {
    short n;
    
    for (n = qNext(TRUE); n != -1; n = qNext(FALSE)) {
         if (vox[n].bChn == (BYTE)wChn)        
              voiceOff(n);
         }
    }

/* =================================================================== */
static void NEAR PASCAL resetAllCntrl(WORD wChan) {
    pedal(wChan, PEDLUP);         // lift pedal
    doBend(wChan, NOBEND);
    doAllNotesOff(wChan);
    doPan(wChan, CTRPAN);      
    doModn(wChan, NOMODN);
    }

/* =================================================================== */
// This is called over and over after  we've determined that a SysX msg
// belongs to us.  We have to parse the message and act on it, deciding
// when it ends.

static void NEAR PASCAL doSysEx(BYTE mb) {
    static WORD  wBnk  = 0;
    static WORD  wTmb  = 0;
    static WORD  wCmd  = 0;
    static WORD  wSize = 0;       // Theoretically could by larger than a
                                  // short, but driver can't hold > 5 Banks.
    static short asIdx  = 0;      // so we can tell where we are in addr-sz
    static short bufIdx = 0;          // so we can tell where we are in buffer
    static BYTE  SXBuf[SXBUFLEN];
    HWND   hwnd;

    if (asIdx < DATASTART) {
        switch(asIdx) {
            case 0:               // just kicking off, get command
                wCmd = mb;
                if ((wCmd == BNKRESET) || (wCmd == DRVRESET))
                    asIdx = DATASTART; // ensure we don't come back in here
                if (wCmd == STATUS_EOX) { // we's done
                    asIdx = 0;
                    bufIdx = 0;            
                    return;
                    }
                break;
            case 1:                
                wBnk = mb;
                break;
            case 2:
                wTmb = (mb & 0x7F); // in range
                break;
            case 3:                 // MSB first
                wSize = (mb & 0x7F); 
                break;                    
            case 4:
                wSize = (WORD)((wSize << 7) | (mb & 0x7F)); // LSB last
                break;
            default:                // impossible
                break;
            }
        ++asIdx;
        }
    else {
        switch(wCmd) {
            case RXDATA: // We're receiving
                if (mb == STATUS_EOX) {
                    switch(wBnk) {
                        case BNKPERCMAP:  // Special Case
                            StorePercMap(SXBuf, bufIdx);
                            break;
                            
                        case SYSPARMNEW:
                        case SYSPARM:
                            StoreParms(wBnk, SXBuf, bufIdx);              
                            break;
                            
                        case 0: case 1: case 2:
                        case 3: case 4: 
                        case BNKPERCNEW:
                        case BNKPERCOLD:    
                            StoreBuffer(SXBuf, &wBnk, &wTmb);
                            break;
                            
                        default:
                            break;
                            }
                    _fmemset(&SXBuf, 0, SXBUFLEN); // Zero out
                    bufIdx = 0;
                    asIdx = 0;
                    }    
                else // only insert Data
                    SXBuf[bufIdx++] = mb;
                break;
                
            case TXDATA: // We're to transmit
                midDumpSysEx(getID(), wBnk, wTmb, wCmd, wSize); // Do it
                if (mb == STATUS_EOX) { // it better!
                    asIdx = 0;
                    bufIdx = 0;            
                    }
                break;
                            
            case DRVRESET:
                SoundBoardInit();
                           // Fall through
                
            case BNKRESET: // We're to reset the Timbres
                if ((hwnd = GethwndTask()) != NULL)
                   PostMessage(hwnd, WM_COMMAND, IDC_TIMBRESET, 0L);
                if (mb == STATUS_EOX) { // it better!
                    asIdx = 0;
                    bufIdx = 0;            
                    }
                break;
            }   
        }    
    }

/* ======================================================================= */
// StorePercMap(BYTE SXBuf[], short bufIdx)
/* ======================================================================= */
void NEAR PASCAL StorePercMap(LPBYTE SXBuf, short bufIdx) {
    short  ii;
    
    for (ii = 0; ii < bufIdx; ) {
        Drm.snd[SXBuf[ii] & 0x7F].dpitch = (SXBuf[ii+1] & 0x7F);
        ii += 2;
        }
    }

/* ======================================================================= */
//
/* ======================================================================= */
void NEAR PASCAL StoreParms(WORD Vers, BYTE SXBuf[], short bufIdx) {
    short ii;
    PARMSTORE ps;             
                 
    // Just run thru

    ps.fSaveSet    = SXBuf[SAVSET];  
    ps.fStereo     = SXBuf[STMODE];
    ps.fPercussion = SXBuf[PRMODE];
    ps.wPercChan   = SXBuf[PRCHAN]; 
    ps.wDftBank    = SXBuf[DFTBNK];
    ps.wModnDepth  = MKVIBR(SXBuf[VIBDPT]) | MKTREM(SXBuf[TRMDPT]); 
    ps.wBendRange  = SXBuf[BNDRNG];
    for (ii = 0; ii < 16; ++ii)
       ps.ChanMap[ii] = (SXBuf[FSTCHN+ii] != 0) ? TRUE : FALSE;
 
    ps.Vers = Vers;   
    if (Vers == SYSPARMNEW && bufIdx > PRCBST) {
        ps.nPercBoost = (short)SXBuf[PRCBST]-64;
        if (bufIdx > FIXPRC)
           ps.fPitchFix  = SXBuf[FIXPRC];
        if (bufIdx > MAPREG)
           ps.fRegShadow = SXBuf[MAPREG];
        }                               
     
    SetParms(&ps);
    
    // ask our task to set it for us
    PostMessage(GethwndTask(), WM_COMMAND, IDC_WRITEPARMS, 0L);
    }

/* ======================================================================= */
//
// Converts and stores Sys Ex buffer, updates Timbre# and maybe bank
//
/* ======================================================================= */

void NEAR PASCAL StoreBuffer(BYTE SXBuf[], short *pBnk, short *pTmb) {
    static SBTIMBRE ltmb; // Local buf
    WORD  maxTmb;
    short nBnk = *pBnk;
    short nTmb = *pTmb;
    
    _fmemset(&ltmb, 0, sizeof(SBTIMBRE)); // Zero out    
        
    ltmb.modchar  = (SXBuf[MDFLG] << 4) | (SXBuf[MDFQM] & 0x0F);
    ltmb.carchar  = (SXBuf[CRFLG] << 4) | (SXBuf[CRFQM] & 0x0F);
    ltmb.modscal  = (SXBuf[MDKSL] << 6) | (SXBuf[MDLEV] & 0x3F);
    ltmb.carscal  = (SXBuf[CRKSL] << 6) | (SXBuf[CRLEV] & 0x3F);
    ltmb.modad    = (SXBuf[MDATK] << 4) | (SXBuf[MDDCY] & 0x0F);
    ltmb.carad    = (SXBuf[CRATK] << 4) | (SXBuf[CRDCY] & 0x0F);
    ltmb.modsr    = (SXBuf[MDSTN] << 4) | (SXBuf[MDREL] & 0x0F);
    ltmb.carsr    = (SXBuf[CRSTN] << 4) | (SXBuf[CRREL] & 0x0F);
    ltmb.modwave  =  SXBuf[MDWAV] & 0x07;
    ltmb.carwave  =  SXBuf[CRWAV] & 0x07;
    ltmb.feedback =  SXBuf[FBCNT] & 0x0F;
    ltmb.percvoc  =  SXBuf[PVOC]  & 0x0F;
    ltmb.dpitch   =  SXBuf[PITCH];        
    if (SXBuf[TRANP] > 63)    // Sign bit?
        ltmb.transpos = (64 - SXBuf[TRANP]); // the rest is negative
    else
        ltmb.transpos = SXBuf[TRANP];

    if (nBnk < MAXBANK) { 
        Tmb[nBnk].snd[nTmb] = ltmb;   // This is the assignment!
        UpdVoxTmb(&ltmb, nBnk, nTmb); // fix up any chan/vox set to this
        maxTmb = MAXTMB;
        }
    else if (nBnk == BNKPERCOLD) {
        Drm.snd[nTmb+FIRSTDRUMNOTE] = ltmb; 
        maxTmb = MAXDRUMTMB;
        }
    else if (nBnk == BNKPERCNEW) {
        Drm.snd[nTmb] = ltmb; 
        maxTmb = MAXTMB;
        }
       
    nTmb = (nTmb+1) % maxTmb;        // Bump up to next
    if (nTmb == 0)
        nBnk = nBnk+1; // May Wrap?
    if (nBnk == MAXBANK)
        nBnk = 0;
    *pBnk = nBnk;
    *pTmb = nTmb;
    }

// ======================================================================= 
// UpdVoxTmb
// ======================================================================= 
void FAR PASCAL UpdVoxTmb(LPTIMBRE ltmb, WORD wBnk, WORD wTmb) {
    short ii, n;
    
    if (wBnk >= MAXBANK)
        return;
    
    for (ii = 0; ii < MAXCHAN; ++ii) { // Update any channels playing this
         if ((chanInfo[ii].wBank  == (BYTE)wBnk) && 
             (chanInfo[ii].bPatch == (BYTE)wTmb)) {
              chanInfo[ii].Timbre = *ltmb; // the next note will pick it up
              for (n = 0; n < MAXVOX; ++n) { 
                   if (vox[n].bChn == (BYTE)ii) // force reload 
                       vox[n].bPatch = NOPATCH; // force reload              
                   }
              }
         }
    }

// ======================================================================= 
// GetPercMap - Exported
// ======================================================================= 

extern void FAR PASCAL _loadds GetPercMap(LPPERCMAP lpPercMap) {
   short ii;
   
   for (ii = 0; ii < NUMDRUMNOTES; ++ii) {
       lpPercMap[ii].patch = ii+FIRSTDRUMNOTE;
       lpPercMap[ii].note = Drm.snd[ii+FIRSTDRUMNOTE].dpitch;
       }
   }

// ======================================================================= 
// SetPercMap - Exported
// ======================================================================= 

extern void FAR PASCAL _loadds SetPercMap(LPPERCMAP lpPercMap) {
    StorePercMap((LPBYTE)lpPercMap, (NUMDRUMNOTES << 1));
    }

// ======================================================================= 
// GetTimbre - Exported
// ======================================================================= 

extern WORD FAR  PASCAL _loadds GetTimbre(WORD wLoc, LPTIMBRE lpTmb, 
                                                         WORD wSrc) {
    WORD wBnk  = HIBYTE(wLoc);
    WORD wTmb  = LOBYTE(wLoc);
    WORD wChan = wLoc;
    
    if (lpTmb == NULL)
        return(ERR_PARAM);
        
    switch(wSrc) {
        case TMB_WORKING_STORAGE:
            if (wChan >= MAXCHAN)
                return(ERR_PARAM);
            *lpTmb = chanInfo[wChan].Timbre;
            return 0;                      

        case TMB_BANK_STORAGE:
            if (wBnk >= MAXBANK)
                return(ERR_PARAM);
            if (wTmb >= MAXTMB)
                return(ERR_PARAM);
            *lpTmb = Tmb[wBnk].snd[wTmb];  // This is the assignment!
            return 0;                     

        case TMB_BANK_PERCUSSION_OLD:
            if (wTmb >= NUMDRUMNOTES)
                return(ERR_PARAM);
            *lpTmb = Drm.snd[wTmb+FIRSTDRUMNOTE];  // This is the assignment!
            return 0;                     

        case TMB_BANK_PERCUSSION_NEW:
            if (wTmb >= MAXTMB)
                return(ERR_PARAM);
            *lpTmb = Drm.snd[wTmb];  // This is the assignment!
            return 0;                     
        }
    return(ERR_PARAM);             // If here, bad source
    }

// ======================================================================= 
// SetTimbre - Exported
// ======================================================================= 

extern WORD FAR  PASCAL _loadds SetTimbre(WORD wLoc, LPTIMBRE lpTmb, 
                                                         WORD wDest) {
    WORD wBnk  = HIBYTE(wLoc);
    WORD wTmb  = LOBYTE(wLoc);
    WORD wChan = wLoc;
    short ii;
    
    if (lpTmb == NULL)
        return(ERR_PARAM);
        
    switch(wDest) {
        case TMB_WORKING_STORAGE:
            if (wChan >= MAXCHAN)
                return(ERR_PARAM);
            chanInfo[wChan].Timbre = *lpTmb;    
            for (ii = 0; ii < MAXVOX; ++ii) { // Update any voices playing 
                if (vox[ii].bChn == (BYTE)wChan) // force reload 
                    vox[ii].bPatch = NOPATCH;    // force reload              
                }
            return 0;                         // Good update
        
        case TMB_BANK_STORAGE:
            if (wBnk >= MAXBANK)
                return(ERR_PARAM);
            if (wTmb >= MAXTMB)
                return(ERR_PARAM);
            Tmb[wBnk].snd[wTmb] = *lpTmb;  // This is the assignment!
            UpdVoxTmb(lpTmb, wBnk, wTmb);  // fix up any chan/vox set to this 
            return 0;                         // Good update

        case TMB_BANK_PERCUSSION_OLD:
            if (wTmb >= NUMDRUMNOTES)
                return(ERR_PARAM);
            Drm.snd[wTmb+FIRSTDRUMNOTE] = *lpTmb;  // This is the assignment!
            return 0;                     

        case TMB_BANK_PERCUSSION_NEW:
            if (wTmb >= MAXTMB)
                return(ERR_PARAM);
            Drm.snd[wTmb] = *lpTmb;  // This is the assignment!
            return 0;                     
        }
    return(ERR_PARAM);             // If here, bad destination
    }

// ======================================================================= 
// GetParm - Exported
// ======================================================================= 
// Currently only one struct length...

extern WORD FAR PASCAL _loadds GetParm(LPSYSPARM lpSysParm, WORD StructLen) {
   if (StructLen != sizeof(SYSPARMS))
      return ERR_PARAM;
  
   GetParms((LPBYTE)lpSysParm, SYSPARMNEW, StructLen);
   lpSysParm->PercBoost -= 64;
   return 0;
   }

// ======================================================================= 
// SetParm - Exported
// ======================================================================= 

extern WORD FAR PASCAL _loadds SetParm(LPSYSPARM lpSysParm, WORD StructLen) {
   BYTE tmp;
   static BYTE Buf[sizeof(SYSPARMS)];
   
   tmp = (StructLen > PRCBST) ? (BYTE)(lpSysParm->PercBoost+64) : 64;
   _fmemcpy(Buf, lpSysParm, StructLen);
   Buf[PRCBST] = tmp;
   StoreParms(SYSPARMNEW, Buf, StructLen);    
   return 0;
   }

/* ======================================================================= */
// Registered Parameter Routines
/* ======================================================================= */
// Select the Registered Parameter Number.  The Number is not selected
// initially, until both the MSB and LSB have been recieved.

static void NEAR PASCAL setParm(short which, WORD wChan, short nData) {
    nData &= 0x07F;                     // ensure byte size

    switch( which ) {
        case LSB:
            wParm[wChan] &= 0x03F80;   // retain only upper byte
            wParm[wChan] |= nData;                     // insert LSB
            rcv[wChan].bLSB = 1;
            break;
        case MSB:
            wParm[wChan] &= 0x07F;      // retain only lower 7 bits
            wParm[wChan] |= (nData << 7);      // insert MSB in top byte
            rcv[wChan].bMSB = 1;
            break;
        }
    }

/* ======================================================================= */
// The Data Entry controller value causes the value to be sent to the
// Sound IPD if a Param has been selected

static void NEAR PASCAL dataEntry(short which, WORD wChan, short nData) {
    nData &= 0x07F;             // ensure byte size

    if (rcv[wChan].bLSB && rcv[wChan].bMSB) { // if have both halves => send
        switch( which ) {
            case LSB:
                wParmVal[wChan] &= 0x03F80;    // retain only upper byte
                wParmVal[wChan] |= nData;      // insert LSB
                break;
            case MSB:
                wParmVal[wChan] &= 0x07F;        // retain only lower 7 bits
                wParmVal[wChan] |= (nData << 7); // insert MSB in top 7 bits
                break;
            }
        sndParm(wParm[wChan], wParmVal[wChan], wChan);
        }
    }

/* ======================================================================= */
// The Data Increment controller value causes the incremented value to be
// sent to the Sound IPD if a Param has been selected

static void NEAR PASCAL dataIncr(WORD wChan, short nData) {
    nData &= 0x07F;             // ensure byte size

    if (rcv[wChan].bLSB && rcv[wChan].bMSB) { // if have both halves => send
        wParmVal[wChan] += nData;
        sndParm(wParm[wChan], wParmVal[wChan], wChan);
        }
    }

/* ======================================================================= */
// The Data Decrement controller value causes the decremented value to be
// sent to the Sound IPD if a Param has been selected

static void NEAR PASCAL dataDecr(WORD wChan, short nData) {
    nData &= 0x07F;             // ensure byte size

    if (rcv[wChan].bLSB && rcv[wChan].bMSB) { // if have both halves => send
        wParmVal[wChan] -= nData;
        sndParm(wParm[wChan], wParmVal[wChan], wChan);
        }
    }

/* ======================================================================= */
// initialize VOX structure and Patch Array

static void NEAR clearVox(void) {
    short n;

    for (n=0; n < MAX_VOICES; ++n) {
        vox[n].dwTm     = 0L;
        vox[n].bKey     = NOKEY;
        vox[n].bChn     = 0;
        vox[n].bPatch   = NOPATCH;
        vox[n].bBank    = DFTBANK;  // Patch Bank        
        vox[n].bPedal   = PEDLUP;
        vox[n].bNoteOn  = 0;
        }
    }

/* ======================================================================= */
// Queue Routines - this queue is used to determine which notes are on and
//       the relative age of a note => the oldest is first in the queue
//       This is not a queue in the strictest sense, because values can be
//       removed in places other than the front, and the list can be searched,
//       but "queue" seems to be most descriptive.

static void NEAR qInit(void) {
    nIdx = nItem = 0;
    }

/* ======================================================================= */
// Add data to the end of the Queue

static void NEAR PASCAL qAppend(WORD wData) {
    if (nItem < MAXQUEUE)
        abyQueue[nItem++] = (BYTE)wData;
    }

/* ======================================================================= */
// Delete an Item (with value wData) from the queue and move the rest up

static void NEAR PASCAL qDelete(WORD wData) {
    _asm {
        push    ds
        push    es
        push    si
        push    di
        push    cx
        push    dx

        cmp     nItem,0 // ensure something to delete
        jle     NotFound
        mov     cx,nItem
        dec     nItem   // removing an entry

        mov     di,ds           
        mov     es,di
        mov     dx,OFFSET abyQueue
        mov     di,dx           // save offset in dx for later
        mov     ax,wData
        cld
        repne   scasb           // Locate wData in Queue
        jcxz    NotFound
        mov     si,di
        dec     di              // all set up for move
        add     dx,nIdx         // see if nIdx is greater or equal to di

        cmp     di,dx           // NOTE: by removing an item from the queue, it
        jg      DoMove          // may be necessary to adjust nIdx into queue
        dec     nIdx            // keep referencing the same item after move
        }
DoMove:
     _asm {
        rep     movsb           // move ds:si -> es:di
        }

NotFound:
    _asm {
        pop     dx
        pop     cx
        pop     di
        pop     si
        pop     es
        pop     ds
        }
    }

/* ======================================================================= */
// Return the value of the next Item in the queue. If bReset != 0, the
// queue index is reset to the first Item.  If we are at the end of the
// queue, the value -1 is returned.

static short NEAR PASCAL qNext(BOOL bReset) {
    if (bReset)
        nIdx = 0;               // reset static index
    if (nIdx < nItem)
        return (short)abyQueue[nIdx++];
    return -1;                 // end of queue reached
    }

/* ======================================================================= */
// Add data to the end of the Queue

static BOOL NEAR PASCAL qFull(short n) {
    return(nItem >= n);
    }
