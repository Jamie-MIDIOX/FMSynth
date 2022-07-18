/****************************************************************************

    fmsyntha.asm

 ***************************************************************************/

extern BYTE FAR PASCAL inport(void);
extern void FAR PASCAL SndOutput(BYTE addr, BYTE dataVal);
extern void FAR PASCAL BlindOutput(BYTE addr, BYTE dataVal);
extern void FAR PASCAL CritEnter(void);
extern void FAR PASCAL CritLeave(void);
