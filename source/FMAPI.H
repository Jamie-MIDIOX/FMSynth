/* FMAPI.H */

// Exported

#define TMB_WORKING_STORAGE      0
#define TMB_BANK_STORAGE         1
#define TMB_BANK_PERCUSSION_OLD  2
#define TMB_BANK_PERCUSSION_NEW  3

typedef DRUMPATCH FAR * LPPERCMAP;

extern void FAR PASCAL _loadds GetPercMap(LPPERCMAP lpPercMap);
extern void FAR PASCAL _loadds SetPercMap(LPPERCMAP lpPercMap);

extern WORD FAR PASCAL _loadds GetTimbre(WORD wLoc, LPTIMBRE lpTmb, 
                                                        WORD wSrc);
extern WORD FAR PASCAL _loadds SetTimbre(WORD wLoc, LPTIMBRE lpTmb, 
                                                        WORD wDest);
    
            
extern WORD FAR  PASCAL _loadds GetParm(LPSYSPARM lpSysParm, WORD StructLen);
extern WORD FAR  PASCAL _loadds SetParm(LPSYSPARM lpSysParm, WORD StructLen);

/* Undoc ****************************************************************/

extern void FAR PASCAL _loadds UpdBank(short ii, LPSTR lpszName, BOOL fSave);
extern void FAR PASCAL _loadds GetBankPath(short idx, 
                                           LPSTR aszPath, WORD len);
extern void FAR PASCAL _loadds SoundBoardInit(void);
extern BOOL FAR PASCAL _loadds SoundFullInit(void);

extern void FAR PASCAL _loadds ResetTimbres(void);
extern void FAR PASCAL _loadds WriteParms(void);
extern void FAR PASCAL _loadds StoreTask(HWND hwnd);

extern void FAR PASCAL _loadds MIDIOff(void);
extern void FAR PASCAL _loadds MIDIOn(void);
