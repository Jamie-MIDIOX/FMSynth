/****************************************************************************

    fmsynth.c

 ***************************************************************************/

extern short nPercBoost;
extern BOOL  fPercWriteBoth;

extern void FAR PASCAL  sndNote(short nVox, WORD wKey, WORD wVel, WORD wChan);
extern void FAR PASCAL  sndDrumMode(BOOL fOn);
extern void FAR PASCAL  sndPatch(short nVox,  WORD wChan, 
                                 WORD wBank, WORD wPatch);
extern void FAR PASCAL  sndVolume(short nVox, WORD wAmt, WORD wChan);
extern void FAR PASCAL  sndBend(short nVox, WORD wChan);
extern void FAR PASCAL  sndParm(WORD wParm, WORD wPval, WORD wChan);
extern void FAR PASCAL  sndPan(short nVox,  WORD wChan); 
extern void FAR PASCAL  sndModn(short nVox, WORD wChan);
extern void FAR PASCAL  sndPressure(short nVox, WORD wAmt, WORD wChan);
extern void FAR PASCAL  sndModDepth(WORD wDepth);
extern void FAR PASCAL  setPercBits(WORD thebits);
extern void FAR PASCAL  updDftBank(WORD theBank);
extern void FAR PASCAL  CopyFreqs(void);

extern BYTE gbMidiLengths[];
extern BYTE gbSysLengths[];

#define MIDILENGTH(bStatus)     (gbMidiLengths[((bStatus) & 0x70) >> 4])
#define SYSLENGTH(bStatus)      (gbSysLengths[(bStatus) & 0x07])
#define PercChan(bChan)         (chanInfo[(bChan)].bType == PERCTYPE)

