/****************************************************************************

    midimain.c

 ***************************************************************************/

extern void FAR  synthInit(void);
extern void NEAR PASCAL synthMidiData(WORD id, HPBYTE lpBuf, DWORD dwLength);
extern void NEAR PASCAL synthAllNotesOff(void);
extern void FAR  PercMap(void);
extern void FAR  MeloMap(void);
extern void FAR  PASCAL UpdVoxTmb(LPTIMBRE ltmb, WORD wBnk, WORD wTmb);
