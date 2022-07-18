# FMSynth
A Windows 3.1 Adlib/Sound Blaster FM Synthesis MIDI Driver

This is an ancient mess of code used to build an FM Synth driver circa 1994.  You would need to have the MS DDK and the VS C++ compiler from that era if you were to attempt to build it.

The driver operated by analyzing an incoming MIDI stream and converting it to hardware register writes on the Adlib and Sound Blaster sound cards to make audible sound (music).
 
