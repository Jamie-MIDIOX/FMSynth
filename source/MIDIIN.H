
/****************************************************************************

    midiin.c
        
 ***************************************************************************/

extern void  NEAR PASCAL midSendPartBuffer(WORD id);
extern DWORD FAR  PASCAL _loadds midMessage(WORD id, UINT msg, DWORD dwUser, 
                                      DWORD dwParam1, DWORD dwParam2);

extern void FAR PASCAL midDumpSysEx(WORD id, WORD wBank, WORD wTmb, 
                                       WORD wCmd, WORD wSize);
extern short FAR PASCAL GetParms(LPBYTE SXBuf, WORD wloc, WORD wOfs);

