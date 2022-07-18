/*
 * Version 1.0 JWO 1993
 *
 */

#define MSM_NOTIFY            (WM_USER+13)

//Control Identifiers.
#define ID_NULL             -1
#define ID_VERTEDIT         300
#define ID_HORZEDIT         301
#define ID_HORZSCROLL       302
#define ID_VERTSCROLL       303


//Special style used for MicroScroll on a 'spin' button.
#define MSS_SPIN (MSS_VERTICAL | MSS_TEXTHASRANGE | MSS_INVERTRANGE)


//Function prototypes.

long FAR PASCAL SpinEditWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL SetupSpinEdit(HWND hParent, HINSTANCE hInst, WORD idCtl, 
                   WORD idSpin, WORD msMin, WORD msMax16,  
                   WORD msDft, BOOL fNotify);      
