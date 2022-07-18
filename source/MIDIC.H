/****************************************************************************

    midic.c
        
 ***************************************************************************/

extern void  FAR PASCAL midiCallback(NPSYNTHALLOC pClient, WORD msg, 
                                      DWORD dw1, DWORD dw2);

extern DWORD FAR PASCAL _loadds modMessage(WORD id, WORD msg, DWORD dwUser, 
                                       DWORD dwParam1, DWORD dwParam2);

extern BYTE bCurrentLen;
extern BYTE status[];
extern BOOL fhaveMix;
extern HINSTANCE hInstMix;
