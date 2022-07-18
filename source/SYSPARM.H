/****************************************************************************

   sysparm.c 
       
 ***************************************************************************/

typedef struct _PARMSTORE {
   WORD  Vers;
   BOOL  fSaveSet;
   BOOL  fStereo;
   BOOL  fPercussion;
   BOOL  fPercSw;
   BOOL  fRegShadow;
   BOOL  fPitchFix;
   short nPercBoost;
   WORD  wModnDepth; 
   WORD  wDftBank;
   WORD  wPercChan;
   WORD  wBendRange;             
   BOOL  ChanMap[MAXCHAN];
   } PARMSTORE, FAR * LPARMSTORE;

extern BOOL  FAR GetSaveSet(void);
extern BOOL  FAR GetStereo(void);
extern BOOL  FAR GetPercussion(void);
extern WORD  FAR GetPercChannel(void);
extern WORD  FAR GetPercSwitch(void);
extern WORD  FAR GetBendRange(void);
extern WORD  FAR GetModnDepth(void);
extern void  FAR GetChanMap(void);
extern WORD  FAR GetCurrentBank(void);
extern short FAR GetPercBoost(void);
extern HWND  FAR GethwndTask(void);
extern WORD  FAR GetRegShadow(void);
extern WORD  FAR GetFixedPitch(void);
extern BOOL  FAR GetUseOldest(void);

extern BOOL  FAR useOldOff(void);
extern BOOL  FAR isOPL3(void);
extern BOOL  FAR isPro(void);
extern BOOL  FAR isSaveSet(void);
extern BOOL  FAR isStereo(void);
extern BOOL  FAR isPercussion(void);
extern BOOL  FAR isPercSwitch(void);
extern BOOL  FAR isPercBoth(void);
extern BOOL  FAR isPitchFixed(void);
extern BOOL  FAR isRegShadow(void);

extern BOOL  FAR isChanOn(WORD wChan);
extern WORD  FAR theBendRange(WORD wChan);
extern WORD  FAR theModnDepth(void);
extern WORD  FAR thePercChannel(void);
extern WORD  FAR theCurrentBank(void);
extern short FAR thePercBoost(void);

extern void FAR PASCAL SetUseOldest(BOOL flag, BOOL bWrite);
extern void FAR PASCAL SetOPL3(BOOL flag);
extern void FAR PASCAL SetPro(BOOL flag);
extern void FAR PASCAL SetPercBoth(BOOL fVal);
extern void FAR PASCAL SetSaveSet(BOOL flag, BOOL bWrite);
extern BOOL FAR PASCAL SetStereo(BOOL flag, BOOL bWrite);
extern void FAR PASCAL SetPercussion(BOOL flag, BOOL bWrite);
extern void FAR PASCAL SetFixedPitch(BOOL flag, BOOL bWrite);
extern void FAR PASCAL SetRegShadow(BOOL flag, BOOL bWrite);
extern void FAR PASCAL SetPercChannel(WORD wChan, BOOL bWrite);
extern void FAR PASCAL SetModnDepth(WORD wDepth, BOOL bWrite);
extern void FAR PASCAL SetBendRange(WORD wChan, WORD wRange, BOOL bWrite);
extern void FAR PASCAL SetCurrentBank(WORD wBank, BOOL bWrite);
extern void FAR PASCAL SetBankPath(short idx, LPSTR aszPath);
extern void FAR PASCAL SetChanMap(LPINT ChanAry, BOOL bWrite);
extern void FAR PASCAL SetPercBoost(short nBoost, BOOL bWrite);
extern void FAR PASCAL SetParms(LPARMSTORE lpPS);
