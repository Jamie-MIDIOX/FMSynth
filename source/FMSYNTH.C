/****************************************************************************
 *
 *   fmsynth.c
 *
 *   Copyright (c) 1993 Jamie O'Connell.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "fmsynth.h"
#include "fmsyntha.h"
#include "sysparm.h"
#include "fmsfunc.h"

/***************************************************************************

    internal function prototypes

***************************************************************************/

static void  NEAR PASCAL WriteTimbre(short nVox, SBTIMBRE *pTmb);
static void  NEAR PASCAL OutBoth(BYTE reg, BYTE val, BOOL BothChip);
static void  NEAR PASCAL WriteFNum(short nVox, WORD wFNum); 
static WORD  NEAR PASCAL GetFNum(BYTE bNote, BOOL bKeyOn);
static void  NEAR PASCAL CoarsePan(short nVox, WORD wChan);
static WORD  NEAR PASCAL CalcBend(short nVox, WORD wChan);
static short NEAR PASCAL TuneNote(short iNote);
static void  NEAR PASCAL WriteModn(short nVox, WORD wChan);
static void  NEAR PASCAL WritePressure(short nVox, BYTE bAmt, WORD wChan);
static void  NEAR PASCAL WriteVolume(short nVox, WORD wVel, WORD wChan);
static void  NEAR PASCAL WritePercTimbre(short nVox, SBTIMBRE *pTmb, 
                                         BOOL fInit);
static void  NEAR PASCAL WritePercVolume(short nVox, WORD wVel, WORD wChan);
static WORD  NEAR PASCAL WritePercPitch(short nPercVoc, WORD wKey);
static void  NEAR SetupPerc(void);

// static void  NEAR PASCAL VolumeOff(short nVox, WORD wChan);

/***************************************************************************

    public data

***************************************************************************/
 
short nPercBoost;
BOOL  fPercWriteBoth = FALSE;

/***************************************************************************

    local data

***************************************************************************/

static BYTE      bPercBits   = DFTDEPTH;
static short     nCoarseTune = 0;
static PERCVOXINFO drmInfo[MAXDRUM];           // Percussion Info

// Local Constants

// Default drum patches

static BYTE DftDrm[5] = {0, 3, 12, 16, 7};
 
static BOOL drmWriteReg[5][REGSPERVOICE] =
        {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
         {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1},  
         {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},  
         {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1},  
         {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}};  
        
static short drmVocMap[5] = {6, 7, 8, 8, 7}; // Logical to physical Map
                                      // bass drum           voice# 6
                                      // SnareDrum  High-Hat voice# 7
                                      // TomTom  - Cymbal    voice# 8
        
static BYTE DPMask[5] = {
        0x10, 0x09, 0x06, 0x06, 0x09 };

static BYTE invDPMask[5] = {            // Drum-Pair Masks: Mask off both
        0xEF, 0xF6, 0xF9, 0xF9, 0xF6 }; // drums on a voice

static BYTE percMask[5] = {
        0x10, 0x08, 0x04, 0x02, 0x01 }; 
        
static BYTE invPMask[5] = {
        0xEF, 0xF7, 0xFB, 0xFD, 0xFE };

static BYTE regAddrMap[18][13] = 
       {{0x20,0x23,0x40,0x43,0x60,0x63,0x80,0x83,0xE0,0xE3,0xC0,0xA0,0xB0},
        {0x21,0x24,0x41,0x44,0x61,0x64,0x81,0x84,0xE1,0xE4,0xC1,0xA1,0xB1},
        {0x22,0x25,0x42,0x45,0x62,0x65,0x82,0x85,0xE2,0xE5,0xC2,0xA2,0xB2},
        {0x28,0x2B,0x48,0x4B,0x68,0x6B,0x88,0x8B,0xE8,0xEB,0xC3,0xA3,0xB3},
        {0x29,0x2C,0x49,0x4C,0x69,0x6C,0x89,0x8C,0xE9,0xEC,0xC4,0xA4,0xB4},
        {0x2A,0x2D,0x4A,0x4D,0x6A,0x6D,0x8A,0x8D,0xEA,0xED,0xC5,0xA5,0xB5},
        {0x30,0x33,0x50,0x53,0x70,0x73,0x90,0x93,0xF0,0xF3,0xC6,0xA6,0xB6},
        {0x31,0x34,0x51,0x54,0x71,0x74,0x91,0x94,0xF1,0xF4,0xC7,0xA7,0xB7},
        {0x32,0x35,0x52,0x55,0x72,0x75,0x92,0x95,0xF2,0xF5,0xC8,0xA8,0xB8},

    // RIGHT CHIP
        {0x20,0x23,0x40,0x43,0x60,0x63,0x80,0x83,0xE0,0xE3,0xC0,0xA0,0xB0},
        {0x21,0x24,0x41,0x44,0x61,0x64,0x81,0x84,0xE1,0xE4,0xC1,0xA1,0xB1},
        {0x22,0x25,0x42,0x45,0x62,0x65,0x82,0x85,0xE2,0xE5,0xC2,0xA2,0xB2},
        {0x28,0x2B,0x48,0x4B,0x68,0x6B,0x88,0x8B,0xE8,0xEB,0xC3,0xA3,0xB3},
        {0x29,0x2C,0x49,0x4C,0x69,0x6C,0x89,0x8C,0xE9,0xEC,0xC4,0xA4,0xB4},
        {0x2A,0x2D,0x4A,0x4D,0x6A,0x6D,0x8A,0x8D,0xEA,0xED,0xC5,0xA5,0xB5},
        {0x30,0x33,0x50,0x53,0x70,0x73,0x90,0x93,0xF0,0xF3,0xC6,0xA6,0xB6},
        {0x31,0x34,0x51,0x54,0x71,0x74,0x91,0x94,0xF1,0xF4,0xC7,0xA7,0xB7},
        {0x32,0x35,0x52,0x55,0x72,0x75,0x92,0x95,0xF2,0xF5,0xC8,0xA8,0xB8}};

//==========================================================================
// VELOCITY Mapping Table - gives a logarithmic response and maps the
// 128 velocities into a magnitude of 0 - 63 for the FM chip.

static BYTE velocTbl[128] =
       {0x00, 0x08, 0x0A, 0x0F, 0x13, 0x16, 0x18, 0x1A,
        0x1C, 0x1D, 0x1E, 0x20, 0x21, 0x22, 0x23, 0x24,
        0x24, 0x25, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29,
        0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2D,
        0x2D, 0x2E, 0x2E, 0x2E, 0x2F, 0x2F, 0x2F, 0x30,
        0x30, 0x30, 0x31, 0x31, 0x31, 0x32, 0x32, 0x32,
        0x32, 0x33, 0x33, 0x33, 0x33, 0x34, 0x34, 0x34,
        0x34, 0x35, 0x35, 0x35, 0x35, 0x36, 0x36, 0x36,
        0x36, 0x36, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37,
        0x38, 0x38, 0x38, 0x38, 0x38, 0x39, 0x39, 0x39,
        0x39, 0x39, 0x39, 0x39, 0x3A, 0x3A, 0x3A, 0x3A,
        0x3A, 0x3A, 0x3A, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B,
        0x3B, 0x3B, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C,
        0x3C, 0x3C, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
        0x3D, 0x3D, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E,
        0x3E, 0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F};

/****************************************************************************
 ***************************************************************************/


static WORD masterFreqs[] = 
                  {0x158, 0x16D, 0x183, 0x19A, 0x1B2, 0x1CC, 0x1E7, 
                   0x204, 0x223, 0x244, 0x266, 0x28A};

static BYTE freqDiff[] = 
                  {0x14, 0x15, 0x16, 0x17, 0x18, 0x1A, 0x1B, 0x1D, 0x1F,
                   0x21, 0x22, 0x24, 0x26}; // Note 13 entries - used for
                                            // tuning up AND down
static WORD freqNums[12];                   // local copy (modified by tuning)

/***************************************************************************

    public functions

***************************************************************************/

void FAR PASCAL sndNote(short nVox, WORD wKey, WORD wVel, WORD wChan) {
        
    WORD  wPercPatch;
    WORD  wPercNote;
    short nPercVoc;
    short nOfs;
    short nKey;    
    DWORD dwCur;
    WORD  wSnare;
    BOOL  fixed;

#ifdef DEBUG
    char buf[64];
    wsprintf(buf, "Voice: %d, Note: %2X, Vel: %2X, Chan: %d\r\n", nVox, 
             wKey, wVel, wChan);
    D2(buf);
#endif

    if (nVox == -1) {                   // PERCUSSION Note
        wKey &= 0x7F;
        wVel &= 0x7F;
        wPercPatch = wKey;
        wPercNote  = Drm.snd[wPercPatch].dpitch;
        nPercVoc   = (short)Drm.snd[wPercPatch].percvoc - FIRSTDRUMVOC; 
        if ((nPercVoc < 0) || (nPercVoc > 4))
            return;      // Use the stored Voice number
        if (wVel != 0) { // for Note Off we just ignore 
            dwCur = timeGetTime(); 
            if ((dwCur - drmInfo[nPercVoc].dwTm) < (DWORD)wMinPercWait)
                return;                     // too short
            drmInfo[nPercVoc].dwTm = dwCur;   // record
            SetChipAddr(LBDRUM);
            if ((bPercBits & DPMask[nPercVoc]) != 0) { // 1 of pair still on 
                bPercBits &= invDPMask[nPercVoc];
                OutBoth(PERCREG, bPercBits, fPercWriteBoth);
                }
            if (drmInfo[nPercVoc].bPatch != (BYTE)wPercPatch) { 
                WritePercTimbre(nPercVoc, &Drm.snd[wPercPatch], FALSE);
                drmInfo[nPercVoc].bPatch = (BYTE)wPercPatch;
                } 
            if (drmInfo[nPercVoc].bNote != (BYTE)wPercNote) {
               fixed = isPitchFixed();
               if (!fixed || nPercVoc == vTOMTOM || 
                                      nPercVoc ==  vBASSDRUM) 
                  drmInfo[nPercVoc].wPitch = 
                            WritePercPitch(nPercVoc, wPercNote);              
               if (fixed && nPercVoc == vTOMTOM) { // change Snare
                  wSnare = wPercNote + TOM_TO_SD;
                  drmInfo[vSNARE].wPitch = 
                            WritePercPitch(vSNARE, wSnare);
                  drmInfo[vSNARE].bNote = (BYTE)wSnare;                  
                  }
               drmInfo[nPercVoc].bNote = (BYTE)wPercNote; 
               }
            WritePercVolume(nPercVoc, wVel, wChan); // Volume may change  
            drmInfo[nPercVoc].bVeloc = (BYTE)wVel;
            bPercBits |= percMask[nPercVoc];
            OutBoth(PERCREG, bPercBits, fPercWriteBoth);
            }     
        else { // trun off
            SetChipAddr(LBDRUM);
            drmInfo[nPercVoc].dwTm = 0L;    // show not on
            bPercBits &= invPMask[nPercVoc];
            OutBoth(PERCREG, bPercBits, fPercWriteBoth);
            }            
        }
    else {                      // Normal melodic voice 
        SetChipAddr(nVox);   
        wKey = TuneNote(wKey);  // Possible tuning adjustment
        nOfs = (short)chanInfo[wChan].Timbre.transpos; // another adjust
        nKey = (short)wKey + nOfs;
        while(nKey > 127)
            nKey -= 12;
        while(nKey < 0)
            nKey += 12;
#ifdef DEBUG
        wsprintf(buf, "Transpose: %d, Note: %2X, New Note: %2X\r\n", nOfs, 
             wKey, nKey);
        D2(buf);
#endif
        voxInfo[nVox].bNote  = (BYTE)nKey;
        voxInfo[nVox].bVeloc = (BYTE)wVel;
        if (wVel > 0) {
            wVel &= 0x7F;
            WriteVolume(nVox, wVel, wChan); 
            CoarsePan(nVox, wChan);
            WriteModn(nVox, wChan);
            voxInfo[nVox].wPitch = GetFNum((BYTE)nKey, TRUE);            
            }  
        else {
            voxInfo[nVox].wPitch = GetFNum((BYTE)nKey, FALSE);
            }
        WriteFNum(nVox, CalcBend(nVox, wChan));
        }
    }

//=========================================================================
// sndPatch
//
// nVox is our physical voice number, or -1 (see below).
// In mpInit() we passed in the number of simultaneous voices we can
// render.  mp takes responsibility for managing them and telling us
// which to turn on/off.
//
// wPatch is a number from 0 to 127 representing a patch (timbre).
//
// When nVox is -1, then a patch change has been received and this is
// simply an "advance warning" that the patch may be specified "for
// real" for a specific voice.  So if a device requires a lot of time
// to set up a voice, this may give it a chance before a note-on.
// (Some clients, like Cakewalk, allow starting patches which are sent
// before playback starts.)
//
//=========================================================================

void FAR PASCAL sndPatch(short nVox, WORD wChan, WORD wBank, WORD wPatch) { 
    SBTIMBRE *pTmb;
    
    if (nVox != -1) { // actually set up nVox to render this patch
        SetChipAddr(nVox);
        wPatch &= 0x7F;
        pTmb    = &chanInfo[wChan].Timbre;
        voxInfo[nVox].wBank  = wBank;
        voxInfo[nVox].bPatch = (BYTE)wPatch;
        voxInfo[nVox].bLCLev = 0;
        voxInfo[nVox].bLMLev = 0;
        voxInfo[nVox].bRCLev = 0;
        voxInfo[nVox].bRMLev = 0;
        voxInfo[nVox].bSpkrB = 0;
        voxInfo[nVox].bModn  = MODINIT;
        WriteTimbre(nVox, pTmb);
        }
    }

//===========================================================================
// sndDrumMode
//
// This function will be called to signal switch in or out of drum mode.
// Should assume NOT in drum mode until we are called.
//
//==========================================================================

void FAR PASCAL sndDrumMode(BOOL fOn) {
    SetChipAddr(LBDRUM);
   
    bPercBits  &= PERCOFF; // turn off no matter what
    OutBoth(PERCREG, bPercBits, fPercWriteBoth);
    
    if (fOn) { // Enter drum mode
        SetupPerc();
        bPercBits  = (bPercBits | PERCON) & PERCMASK;
        OutBoth(PERCREG, bPercBits, fPercWriteBoth);        
        }
                         //    SetPercussion(fOn, FALSE);
    }

//===========================================================================
// SendVolume: Calculates and writes the total output volume to FM chip.
// The Volume is made from three components: The timbre volume, the
// note velocity, and the controller volume.  We keep track of current
// settings in order to avoid redundant writes.
//
// nVox is voice number, or -1 if percussion.
//
// wAmt is MIDI volume controller amount from 0 to 127.
// This routine immediately changes the volume of a voice if it is on.
//
// This routine always forces the writing of both operators if Percussion
// mode is on and the voice# > 6 (the upper 4 percussive sounds)
// The volume of all 3 percussion voices (5 instruments) are changed
//===========================================================================

void FAR PASCAL sndVolume(short nVox, WORD wAmt, WORD wChan) {
    short ii;
    
    wAmt &= 0x7F;
    if (nVox == -1) {           // Percussion Volume
        for (ii = 0; ii < MAXDRUM; ++ii) 
            drmInfo[ii].bVolum = (BYTE)wAmt;         // Overall Note Volume
        }
    else {                       // Melodic
        SetChipAddr(nVox);
        WriteVolume(nVox, voxInfo[nVox].bVeloc, wChan);
        }
    }  

//===========================================================================
// sndModn
//
// nVox is voice number.
//
// wAmt is MIDI Modulation wheel amount from 0 to 127 (MSB)
//
//===========================================================================

void FAR PASCAL sndModn(short nVox, WORD wChan) {

    if (nVox != -1) {
        SetChipAddr(nVox);  
        WriteModn(nVox, wChan);
        }
    }

//===========================================================================
// sndPressure
//
// nVox is voice number.
//
// wAmt is MIDI Modulation wheel amount from 0 to 127 (MSB)
//
// This is a lot like Modulation, but uses Amt and channel info
//
//===========================================================================

void FAR PASCAL sndPressure(short nVox, WORD wAmt, WORD wChan) {

    if (nVox != -1) {
        SetChipAddr(nVox);  
        WritePressure(nVox, (BYTE)wAmt, wChan);
        }
    }
 
//==========================================================================
// sndPan
//
// nVox is voice number.
//
// wAmt is MIDI Pan Controller: 0 = Hard Left, 40h = Center, 7F = Hard Right
//
// This function will just return, unless we're in stereo mode
//
//===========================================================================

void FAR PASCAL sndPan(short nVox, WORD wChn) {
    if (nVox != -1) {
        SetChipAddr(nVox);
        if (isStereo()) 
            WriteVolume(nVox, voxInfo[nVox].bVeloc, wChn);
        else if (isOPL3())  // Maybe OPL3
            CoarsePan(nVox, wChn);
        }
    }
     

//==========================================================================
// sndParm
//
// wParm is the Registered Parameter Number.
//
// wPval is MIDI Parameter value from 0 to 16383, where 8192 is center.
//
// Bend Range:   We use only the (rounded) MSB to decide the number of
//               semitones to bend.  There is currently an upper limit
//               of 24 semitones (2 octaves).
//
// Fine Tuning:  There are only 15 discrete steps between middle C and C#.
//               So again, we use only the (rounded) MSB of the parameter
//               value.
//
// Coarse Tuning: There are only 64 discrete steps in either direction -
//               so we use only the (rounded) MSB and move in 1/2 steps.
//
//===========================================================================

void FAR PASCAL sndParm(WORD wParm, WORD wPval, WORD wChan) {
    WORD wValue;
    short ii;
    
    wValue = (wPval + 127) >> 7; // Round off and dump LSB
    switch(wParm) {
        case BENDRNG:     // We bend +-24 semitones (for each channel!!)
            if (wValue > 24)
                wValue = 24; // Max is 24
            SetBendRange(wChan, wValue, FALSE);
            break;
        case FINETUNE:    // Fine Tuning
            CopyFreqs();
            if (wValue > 0x40) { // tuning up
                wValue -= 0x40;
                for (ii = 0; ii < 12; ++ii) 
                    freqNums[ii] += ((wValue * freqDiff[ii+1]) >> 6);
                }
            else if (wValue < 0x40) { // tuning down
                wValue = 0x40 - wValue;
                for (ii = 0; ii < 12; ++ii) 
                    freqNums[ii] += ((wValue * freqDiff[ii]) >> 6);
                }
            break;
        case CRSTUNE:   // Coarse Tuning
            nCoarseTune = (short)wValue - 0x40;
            break;
        }
    }
      
//==========================================================================
// sndBend
//
// nVox is voice number.
//
// wAmt is MIDI pitch wheel bend amount from 0 to 16383, where 8192 is
// center (no bend).
//
//==========================================================================

void FAR PASCAL sndBend(short nVox, WORD wChan) {
    if (nVox != -1) {
        SetChipAddr(nVox);
        WriteFNum(nVox, CalcBend(nVox, wChan));       
        }
    }

//==========================================================================

void FAR PASCAL setPercBits(WORD thebits) {
    bPercBits = (BYTE)thebits;
    }

//==========================================================================

void FAR PASCAL updDftBank(WORD theBank) {
    short ii;
    BYTE  thePatch;
    
    for (ii = 0; ii < 16; ++ii) {
         chanInfo[ii].wBank = theBank;
         thePatch = chanInfo[ii].bPatch;
         if (thePatch < MAXTMB)
             chanInfo[ii].Timbre = Tmb[theBank].snd[thePatch];
         }             // the next note will pick it up
         
    }

//==========================================================================
    
void FAR PASCAL sndModDepth(WORD wDepth) {
    SetChipAddr(0);
    wDepth &= 0x60; // look only at bits 5-6
    wDepth <<= 1;   // move into pos 6-7
    bPercBits = (bPercBits & MODNMASK) | (BYTE)wDepth;
    OutBoth(PERCREG, bPercBits, (isPro() && !isOPL3()));
    SetModnDepth(wDepth, FALSE);
    }
    
/***************************************************************************
 ***************************************************************************/

void FAR PASCAL CopyFreqs(void) {
    short ii;    
    
    for (ii = 0; ii < 12; ++ii) // Get Copy of Frequency table
        freqNums[ii] = masterFreqs[ii];
    }


/***************************************************************************

    private functions

***************************************************************************/

//=============================================================================
// CoarsePan: Do Low level Speaker Bit for OPL3 "Cheap" panning

static void NEAR PASCAL CoarsePan(short nVox, WORD wChn) {
    SBTIMBRE *pTmb;
    BYTE bPan;
    BYTE bSBit;
    
    if (!isOPL3() || isStereo())
        return;
    pTmb = &chanInfo[wChn].Timbre;
    bPan = chanInfo[wChn].bPan;
    SetChipAddr(nVox);
    if (bPan < 32)         // Treat anything between 32 - 95 as center
        bSBit = LSTBIT;
    else if (bPan > 95)
        bSBit = RSTBIT;
    else
        bSBit = MONOBIT;                                              
    voxInfo[nVox].bSpkrB = bSBit;
    SndOutput(regAddrMap[nVox][FEEDBACK], (BYTE)(pTmb->feedback|bSBit));
    }

//==========================================================================
// WriteVolume: Calculate and write the velocity and volume info to FM chip.
// we scale velocity: newvol = (vel/4)+32
//              then newvol = (newvol*(vol+1))/128
//              then newvol = newvol*(64-oldvol)/64
//              and  newvol = 63 - newvol
//===========================================================================

void NEAR PASCAL WriteVolume(short nVox, WORD wVel, WORD wChan) {
    SBTIMBRE *pTmb;
    WORD      work;      
    WORD      rawvol;
    WORD      wRPan;
    WORD      wLPan;
    WORD      wVol;
    
    
    pTmb = &chanInfo[wChan].Timbre;
     
    rawvol   = (WORD)velocTbl[wVel] * velocTbl[chanInfo[wChan].bVolum];
    rawvol >>= 6; // Divide by 64
    work     = rawvol * (63 - (pTmb->carscal & 0x3F)); // Level is Attenuation
    work   >>= 6; // again by 64
    
    if (isStereo()) { // Stereo - must do Pan
        wRPan  = chanInfo[wChan].bPan;
        wLPan  = (127 - wRPan);
        wVol   = (WORD)(velocTbl[wRPan] * work);
        wVol >>= 6; // divide by 64;
        wVol   = (63 - wVol) | (pTmb->carscal & 0xC0);
        voxInfo[nVox].bRCLev = (BYTE)wVol;
        wFMChipAddr = wRiteFMChip;
        SndOutput(regAddrMap[nVox][CARSCAL], (BYTE)wVol);  
        
        wVol   = (WORD)(velocTbl[wLPan] * work);
        wVol >>= 6; // divide by 64;
        wVol   = (63 - wVol) | (pTmb->carscal & 0xC0);
        voxInfo[nVox].bLCLev = (BYTE)wVol;
        wFMChipAddr = wLeftFMChip;
        SndOutput(regAddrMap[nVox][CARSCAL], (BYTE)wVol);  
        }
    else {  // Just write the carrier level to one position
        wVol = (63 - work) | (pTmb->carscal & 0xC0);
        voxInfo[nVox].bLCLev = (BYTE)wVol;
        SndOutput(regAddrMap[nVox][CARSCAL], (BYTE)wVol);  
        }
    
    // We check the AddSyn value to decide whether to write the modulator
    if ((pTmb->feedback & 1) == 0)  // Write both (otherwise just carrier)
        return;
        
    work   = rawvol * (63 - (pTmb->modscal & 0x3F)); // Level is Attenuation
    work >>= 6; // again by 64
    
    if (isStereo()) { // Stereo - must do Pan
        wVol   = (WORD)(velocTbl[wRPan] * work);
        wVol >>= 6; // divide by 64;
        wVol   = (63 - wVol) | (pTmb->modscal & 0xC0);
        voxInfo[nVox].bRMLev = (BYTE)wVol;
        wFMChipAddr = wRiteFMChip;
        SndOutput(regAddrMap[nVox][MODSCAL], (BYTE)wVol);  
        
        wVol   = (WORD)(velocTbl[wLPan] * work);
        wVol >>= 6; // divide by 64;
        wVol   = (63 - wVol) | (pTmb->modscal & 0xC0);
        voxInfo[nVox].bLMLev = (BYTE)wVol;
        wFMChipAddr = wLeftFMChip;
        SndOutput(regAddrMap[nVox][MODSCAL], (BYTE)wVol);  
        }
    else {  // Just write the carrier level to one position
        wVol = (63 - work) | (pTmb->modscal & 0xC0);
        voxInfo[nVox].bLMLev = (BYTE)wVol;
        SndOutput(regAddrMap[nVox][MODSCAL], (BYTE)wVol);  
        }
    }        

//==========================================================================
// WritePercVolume: Calculate and write the percussion velocity and volume 
// we scale velocity: newvol = (vel/4)+32
//              then newvol = (newvol*(vol+1))/128
//              then newvol = newvol*(64-oldvol)/64
//              and  newvol = 63 - newvol
//===========================================================================

// Calculate and write the percussion volume parameters
                                                         
void NEAR PASCAL WritePercVolume(short nPercVoc, WORD wVel, WORD wChan) {
    SBTIMBRE *pTmb;
    short     nWork;      
    WORD      rawvol;
    WORD      wVol;
    short     nPhysVoc;
    
    pTmb     = &Drm.snd[drmInfo[nPercVoc].bPatch];
    nPhysVoc = drmVocMap[nPercVoc];

    rawvol   = (WORD)velocTbl[wVel] * chanInfo[wChan].bVolum;
    rawvol >>= 7; // Divide by 128
    
    if (drmWriteReg[nPercVoc][CARSCAL]) { 
        nWork = rawvol * (63 - (pTmb->carscal & 0x3F)); // Level is Atten
        nWork = (nWork >> 6) + nPercBoost;
        if (nWork > 63) 
           nWork = 63;
        else if (nWork < 0)
           nWork = 0; 
        wVol = (63 - nWork) | (pTmb->carscal & 0xC0); 
        drmInfo[nPercVoc].bCLev = (BYTE)wVol; 
        OutBoth(regAddrMap[nPhysVoc][CARSCAL], (BYTE)wVol, fPercWriteBoth);  
        }
        
    if (drmWriteReg[nPercVoc][MODSCAL]) { 
        nWork = rawvol * (63 - (pTmb->modscal & 0x3F)); // Level is Atten
        nWork = (nWork >> 6) + nPercBoost;
        if (nWork > 63) 
           nWork = 63;
        else if (nWork < 0)
           nWork = 0; 
        wVol = (63 - nWork) | (pTmb->modscal & 0xC0); 
        drmInfo[nPercVoc].bMLev = (BYTE)wVol; 
        OutBoth(regAddrMap[nPhysVoc][MODSCAL], (BYTE)wVol, fPercWriteBoth);  
        }
    }

// ==========================================================================
// WritePercPitch: Calculate and write the modulation. 
// For Percussion the Key is always on
// ==========================================================================

WORD NEAR PASCAL WritePercPitch(short nPercVoc, WORD wNote) {   
    WORD  wFNum;      
    short nPhysVoc;
    
    nPhysVoc = drmVocMap[nPercVoc];
    wFNum    = GetFNum((BYTE)wNote, FALSE);  // KeyOn must be OFF! 
    
    OutBoth(regAddrMap[nPhysVoc][FREQNUM], LOBYTE(wFNum), fPercWriteBoth);
    OutBoth(regAddrMap[nPhysVoc][KEYBLK],  HIBYTE(wFNum), fPercWriteBoth); 
    return wFNum;
   }

//==========================================================================
static void NEAR PASCAL WritePercTimbre(short nPercVoc, SBTIMBRE *pTmb,
                                        BOOL fInit) {
    BYTE *bVal;
    BYTE  bFB;
    short ii;
    short nPhysVoc;
    
    bVal = (BYTE *)pTmb;
    nPhysVoc = drmVocMap[nPercVoc];
    SetChipAddr(nPhysVoc); 
    
    for (ii = 0; ii < REGSPERVOICE-1; ++ii) { 
        if (drmWriteReg[nPercVoc][ii]) 
            OutBoth(regAddrMap[nPhysVoc][ii], bVal[ii], fPercWriteBoth);
        }
    
    if (drmWriteReg[nPercVoc][FEEDBACK]) {
        bFB = bVal[FEEDBACK];
        if (isOPL3())
             bFB |= MONOBIT;
        SndOutput(regAddrMap[nPhysVoc][FEEDBACK], bFB);
        }
    }

// ==========================================================================
// WriteModn: Calculate and write the modulation. 
// For FM Cards this is an on/off proposition
// ==========================================================================

void NEAR PASCAL WriteModn(short nVox, WORD wChan) { 
    BOOL fModulate;
    SBTIMBRE *pTmb;                                 
    BYTE bModn;
    BYTE bModVT;
    BYTE bCarVT;
               
    bModn = chanInfo[wChan].bModn;           
    fModulate = (bModn >= 0x40);
    
    if (bModn != voxInfo[nVox].bModn) {   
        pTmb  = &chanInfo[wChan].Timbre;
        voxInfo[nVox].bModn = bModn;
        
        bCarVT = pTmb->carchar;
        bModVT = pTmb->modchar;
        
        if (fModulate) {
            bCarVT |= DEEP;
            bModVT |= DEEP;
            }  
                                
        OutBoth(regAddrMap[nVox][MODCHAR], bModVT, isStereo());
        OutBoth(regAddrMap[nVox][CARCHAR], bCarVT, isStereo());
        }     
    }

// ==========================================================================
// WritePressure: Calculate and write the Pressure. 
// For FM Cards this is an on/off proposition
// ==========================================================================

void NEAR PASCAL WritePressure(short nVox, BYTE bAmt, WORD wChan) { 
    BOOL fPress;
    SBTIMBRE *pTmb;                                 
    BYTE bModVT;
    BYTE bCarVT;
               
    fPress = (bAmt >= 0x40);
    
    if (bAmt != voxInfo[nVox].bModn) {   
        pTmb  = &chanInfo[wChan].Timbre;
        voxInfo[nVox].bModn = bAmt;
        bCarVT = pTmb->carchar;
        bModVT = pTmb->modchar;
        if (fPress) {
           bCarVT |= DEEP;
           bModVT |= DEEP;
           }  
        OutBoth(regAddrMap[nVox][MODCHAR], bModVT, isStereo());
        OutBoth(regAddrMap[nVox][CARCHAR], bCarVT, isStereo());
        }     
    }

//=========================================================================
// Calculate Frequency Params
//============================================================================
// GetFnum: Calculate FM chip parameters given a MIDI Note#.
// Output: AH = KeyOn, Block Num, Upper 2 bits of Fnumber => Addr (BxH)
//         AL = Lower 8 bits of FNum Parameter => Addr AxH (x = opr slot)
//============================================================================

static WORD NEAR PASCAL GetFNum(BYTE bNote, BOOL bKeyOn) {
    WORD wOctv;
    WORD wFNum;
    
    wOctv = bNote / 12;
    
    if (wOctv > 0)
        --wOctv;            // Drop Octave (FM Chip uses 48 as MID C)
    if (wOctv > 7)
        wOctv = 7;          // Keep in range
    wFNum = freqNums[bNote % 12] | (wOctv << 10);
    if (bKeyOn) 
        wFNum |= WKEYON;
    else
        wFNum &= ~WKEYON;
    return wFNum;
    }
                     
//==========================================================================
// This routine writes the frequency of a voice to the FM chip. It also
// Effectively turns the note on or off.

static void NEAR PASCAL WriteFNum(short nVox, WORD wFNum) {
    BOOL fBoth;
    
    fBoth  = isStereo();
    OutBoth(regAddrMap[nVox][FREQNUM], LOBYTE(wFNum), fBoth);
    OutBoth(regAddrMap[nVox][KEYBLK],  HIBYTE(wFNum), fBoth); 
    }              

//===========================================================================
// CalcBend: Calculate FM chip parameters given a frequency and a
//          bend amt.

static WORD NEAR PASCAL CalcBend(short nVox, WORD wChan) {
   WORD wBend;
   WORD wFNum; 
   WORD wFreq;
   WORD wNewFreq; 
   WORD wAmt;
   BYTE bOctv;
   BYTE bNote;
   long lDiff;
   BYTE bRange;
   short idx;
   
   wFNum = voxInfo[nVox].wPitch;
   wBend = chanInfo[wChan].wBend;
      
   if (wBend == MIDBEND) // Nothing to do
       return wFNum;
        
   if ((bRange = (BYTE)theBendRange(wChan)) == 0)
       return wFNum;
   
   bOctv = (HIBYTE(wFNum) & OCTVMASK) >> 2;
   wFreq = wFNum & FREQMASK;
   bNote = voxInfo[nVox].bNote;
   
   if (wBend > MIDBEND) { // Bending Up
       wAmt   = (wBend - MIDBEND);
       idx    = (bNote + bRange) % 12;
       wNewFreq  = freqNums[idx];
       if (wNewFreq <= wFreq)
          wNewFreq <<= 1;      // Jump up an octave
       if (bRange > 12)
          wNewFreq <<= 1;      // Jump up an octave for this too
       lDiff = (long)(wNewFreq - wFreq) * wAmt;
       wNewFreq = (WORD)(lDiff >> 13) + wFreq;
       while (wNewFreq > 0x3FF) { // Max    
           if (bOctv < 7) {     // Can adjust
               ++bOctv;
               wNewFreq >>= 1;
               }
           else
               wNewFreq = 0x3FF; // just set to Max
           }
       }
    else { // Bend Down
       wAmt   = (MIDBEND - wBend);
       idx    = (bNote - bRange) % 12;
       wNewFreq  = freqNums[idx];
       if (wNewFreq >= wFreq)
          wNewFreq >>= 1;      // Jump down an octave
       if (bRange > 12)
          wNewFreq >>= 1;      // Jump down an octave again          
       lDiff = (long)(wFreq - wNewFreq) * wAmt;
       wNewFreq = wFreq - (WORD)(lDiff >> 13);
       while (wNewFreq < freqNums[0]) { // Min    
           if (bOctv > 0) {     // Can adjust
               --bOctv;
               wNewFreq <<= 1;  // Up an octave
               }
           else
               wNewFreq = freqNums[0]; // just set to Min
           } 
       }
 
   wFNum = (wFNum & WKEYON) | (WORD)(bOctv << 10) | wNewFreq; 
   return wFNum;
   }

// ==========================================================================
// TuneNote: Note level Coarse tuning for FM chip.

static short NEAR PASCAL TuneNote(short iNote) {
     
     if (nCoarseTune > 0) {
        iNote += nCoarseTune;
        while (iNote > 0x7F)
            iNote -= 12;
        }    
     else if (nCoarseTune <  0) {
        iNote += nCoarseTune;
        while (iNote < 0)
            iNote += 12;
        }
    return iNote;       
    }
        
//=============================================================================
// OutBoth:  Send data to Left Sound Chip, if flag is set, send to right too.

static void NEAR PASCAL OutBoth(BYTE reg, BYTE val, BOOL fBoth) {
    WORD tmp;
    
    SndOutput(reg, val);
    if (fBoth) {
        tmp = wFMChipAddr;
        wFMChipAddr = wRiteFMChip;
        SndOutput(reg, val);
        wFMChipAddr = tmp;
        }
    }

//===========================================================================
// Write Timbre: Output Timbre Parameters to FM chip.  

static void NEAR PASCAL WriteTimbre(short nVox, SBTIMBRE *pTmb) {
    BYTE *bVal;
    short ii;
    BYTE  bFB;
    short nUpr;
    BOOL  fSt;
    
    bVal  = (BYTE *)pTmb;
    SetChipAddr(nVox);
    fSt = isStereo();
    
    for (ii = 0; ii < REGSPERVOICE; ++ii) 
        OutBoth(regAddrMap[nVox][ii], bVal[ii], fSt);

    
    if (isOPL3()) {            // spkr bits only on last
        bFB = bVal[FEEDBACK]; 
        if (fSt) {                     // indiv Speaker Bits 
            nUpr = nVox+VOCPERCHIP;
            SetChipAddr(nUpr);
            SndOutput(regAddrMap[nUpr][FEEDBACK], (BYTE)(bFB|RSTBIT));
            SetChipAddr(nVox);
            SndOutput(regAddrMap[nVox][FEEDBACK], (BYTE)(bFB|LSTBIT));
            }
        else            // Write to both speakers
            SndOutput(regAddrMap[nVox][FEEDBACK], (BYTE)(bFB|MONOBIT));
        }
    }                                                                       
    

// ==========================================================================
// SetupPerc: Send Percussion Timbre parameters to FM chip.  

void NEAR SetupPerc(void) {
    int ii;
        
    for (ii = 0; ii < MAXDRUM; ++ii) {
        drmInfo[ii].dwTm   = 0L;         // Time        
        drmInfo[ii].wPitch = 0;         // Pitch Info
        drmInfo[ii].bNote  = 0;         // MIDI Note Number
        drmInfo[ii].bPatch = NOPATCH;   // Current Timbre
        drmInfo[ii].bVeloc = 0;         // Velocity
        drmInfo[ii].bVolum = 0;         // Overall Note Volume
        drmInfo[ii].bSpkrB = 0;         // Speaker Bits (OPL3)
        drmInfo[ii].bCLev  = 0;         // Chip Level
        drmInfo[ii].bMLev  = 0;         // Chip Level
        WritePercTimbre(ii, &Drm.snd[DftDrm[ii]], TRUE);
        drmInfo[ii].bPatch = DftDrm[ii];
        switch(ii) {
            case vBASSDRUM:
                drmInfo[ii].bNote  = BDPITCH; 
                drmInfo[ii].wPitch = WritePercPitch(ii, BDPITCH); 
                break;
            case vTOMTOM:
                drmInfo[ii].bNote  = TTPITCH; // Only Tom allowed to change
                drmInfo[ii].wPitch = WritePercPitch(ii, TTPITCH); 
                break;
            case vSNARE:
                drmInfo[ii].bNote  = SDPITCH;
                drmInfo[ii].wPitch = WritePercPitch(ii, SDPITCH);
                break;
            default:
                drmInfo[ii].wPitch = 0;         // Pitch Info
                drmInfo[ii].bNote  = 0;         // MIDI Note Number
                break;
            }
        }
    }
