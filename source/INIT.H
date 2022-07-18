/****************************************************************************

    init.c

 ***************************************************************************/

extern void FAR  PASCAL setRunTime(BOOL fStereo);
extern void FAR  InitChannelInfo(void);
extern void FAR  SoundWarmInit(void);
extern BOOL FAR  OnlyOneInstance(void);
extern WORD FAR  GetChipSet(void); 
extern WORD FAR  GetBaseAddr(void);
extern WORD FAR  PASCAL StrToHex(LPSTR asz);
extern BOOL FAR  BoardInstalled(void);
extern BOOL FAR  PASCAL Enable(void);
extern void FAR  PASCAL Disable(void);
extern BOOL FAR  PASCAL LoadBank(LPCSTR lpBankPath, short idx);
extern int  FAR  PASCAL LoadPatches(short idx);
extern BOOL FAR  PASCAL LoadDrumBank(LPCSTR lpBankPath);
extern int  FAR  PASCAL LoadDrumPatches(void);
extern void FAR  LoadExtraBanks(void);
