/****************************************************************************

    FMSTRING.H Strings 

 ***************************************************************************/

#ifndef FMSTRING_H_
#define FMSTRING_H_

#ifdef  DEFINE_STR
  #define EXTERN
  #define BCODE _based(_segname("_CODE"))
  #define MODIF  BCODE
#else
  #define EXTERN extern
  #define MODIF  _far
#endif

EXTERN char MODIF aszDriverName[]
#ifdef DEFINE_STR
       = "FMSYNTH.DRV"
#endif
     ;

EXTERN char MODIF aszProductName[]
#ifdef DEFINE_STR
       = "FM Synth - Jamie O'Connell"
#endif
     ;

EXTERN char MODIF aszInputName[]
#ifdef DEFINE_STR
       = "FM Synth - JWO: SysEx Input"
#endif
     ;

EXTERN char MODIF aszProfileNm[]
#ifdef DEFINE_STR
       = "FMSYNTH.INI"
#endif
     ;

EXTERN char MODIF aszSysProfileNm[]
#ifdef DEFINE_STR
       = "SYSTEM.INI"
#endif
     ;

EXTERN char MODIF aszHelpPath[]      
#ifdef DEFINE_STR
       = "FMSYNTH.HLP"
#endif
     ;

EXTERN char MODIF aszCardType[]
#ifdef DEFINE_STR
      = "CardType"
#endif
     ;

EXTERN char MODIF aszBasePort[]
#ifdef DEFINE_STR
      = "Port"
#endif
     ;

EXTERN char MODIF aszInternal[]
#ifdef DEFINE_STR
       = "<Internal>"
#endif
     ;


EXTERN char MODIF aszModeSect[]
#ifdef DEFINE_STR
       = "Mode"
#endif
     ;

EXTERN char MODIF aszBankSect[]
#ifdef DEFINE_STR
       = "Timbre"
#endif
     ;

EXTERN char MODIF aszRangeSect[]
#ifdef DEFINE_STR
       = "Range"
#endif
     ;

EXTERN char MODIF aszStereoMode[]
#ifdef DEFINE_STR
       = "Stereo"
#endif
     ;

EXTERN char MODIF aszUseOld[]
#ifdef DEFINE_STR
       = "UseOldest"
#endif
     ;

EXTERN char MODIF aszPercusMode[]
#ifdef DEFINE_STR
       = "Percussion"
#endif
     ;

EXTERN char MODIF aszPercChannel[]
#ifdef DEFINE_STR
       = "PercChannel"
#endif
     ;

EXTERN char MODIF aszPercBoost[]
#ifdef DEFINE_STR
       = "PercBoost"
#endif
     ;

EXTERN char MODIF aszBendRange[]
#ifdef DEFINE_STR
       = "Bend"
#endif
     ;

EXTERN char MODIF aszVibrato[]
#ifdef DEFINE_STR
       = "Vibrato"
#endif
     ;

EXTERN char MODIF aszTremolo[]
#ifdef DEFINE_STR
       = "Tremolo"
#endif
     ;

EXTERN char MODIF aszChanSect[]
#ifdef DEFINE_STR
       = "Channel"
#endif
     ;

EXTERN char MODIF aszChanNm[]
#ifdef DEFINE_STR
       = "Map"
#endif
     ;

EXTERN char MODIF aszAppSect[]
#ifdef DEFINE_STR
       = "Application"
#endif
     ;

EXTERN char MODIF aszInstall[]
#ifdef DEFINE_STR
       = "Installation"
#endif
     ;

EXTERN char MODIF aszControl[]
#ifdef DEFINE_STR
       = "Contents"
#endif
     ;

EXTERN char MODIF aszIconName[]
#ifdef DEFINE_STR
       = "FMSYNTH"
#endif
     ;

EXTERN char MODIF aszDFTChan[]
#ifdef DEFINE_STR
       = "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16"
#endif
     ;

EXTERN char MODIF aszDftBank[]   
#ifdef DEFINE_STR
       = "DftBank"
#endif
     ;

EXTERN char MODIF aszOldPercSw[] 
#ifdef DEFINE_STR
       = "OldPercSwitch"
#endif
     ;

EXTERN char MODIF aszPercBank[]
#ifdef DEFINE_STR
       = "PercBank"
#endif
     ;

EXTERN char MODIF aszSaveSet[]   
#ifdef DEFINE_STR
       = "SaveSettings"
#endif
     ;

EXTERN char MODIF aszCtlDlg[]   
#ifdef DEFINE_STR
       = "DoFMCtlDlg"
#endif
     ;

EXTERN char MODIF aszRegShadow[]   
#ifdef DEFINE_STR
       = "RegisterShadow"
#endif
     ;

EXTERN char MODIF aszFixPitch[]   
#ifdef DEFINE_STR
       = "PercPitchFixed"
#endif
     ;
  
EXTERN char MODIF aszLoad[]   
#ifdef DEFINE_STR
       = "LoadCPL"
#endif
     ;

/* ---------------------------------- */  
#ifdef DEBUG
  EXTERN char MODIF aszFMSynth[]
  #ifdef DEFINE_STR
     = "FMSYNTH"
  #endif
  ;

  EXTERN char MODIF aszMMDebug[]
  #ifdef DEFINE_STR
     = "mmdebug"
  #endif
  ;
#endif

#endif /* FMSTRING_H_ */
