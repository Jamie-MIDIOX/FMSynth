############################################################################
#
#   makefile
#
#   Copyright (c) 1991-1992 Microsoft Corporation.  All Rights Reserved.
#
#   constructs fmcfg dll
#
#   to build a debug version:
#       NMAKE
#   to build a non debug version:
#       NMAKE DEBUG=NO
#
############################################################################

NAME    = fmcfg
OBJ1    = fmcfg.obj cplapp.obj mscroll.obj msinit.obj ctldlg.obj paint.obj msapi.obj spinedit.obj
# sysparm.obj
LIBS    = libw mdllcew mmsystem commdlg fmsynth ctl3dd

# !!! ctl3dv2
# !!! ctl3dd.obj

!if "$(DEBUG)" == "YES"
DEF     = -DDEBUG
CLOPT   = -Oxws -Zid
MASMOPT = -Zi
LINKOPT = /CO/LI
!else
DEF     =
CLOPT   = -Oxws
MASMOPT =
LINKOPT =
!endif

CC      = cl -c -nologo -W3 -Zp -G2sw -AMw $(DEF) $(CLOPT)
LINK    = link /NOPACKCODE/NOD/NOE/MAP/ALIGN:16 $(LINKOPT)

.c.obj:
    @$(CC) $*.c

###################### goal ######################

goal: $(NAME).dll
    @echo ***** finished making $(NAME) *****


..\bin\$(NAME).sym: $$(@F)
   @copy $(@F) $@

##################################################

$(NAME).dll $(NAME).sym: $(OBJ1) $(NAME).def $(NAME).res
    @$(LINK) @<<
    $(OBJ1),
    $(NAME).dll,
    $(NAME).map,
    $(LIBS),
    $(NAME).def
<<
    @rc -t $(DEF) $(NAME).res $(NAME).dll
    @mapsym /n $(NAME).map

$(NAME).res: $(NAME).rc fmsynth.h fmvers.h 
    @rc $(DEF) -r -z $(NAME).rc

$(NAME).lib: $(NAME).dll
    implib $(NAME).lib $(NAME).def


##################### clean ######################

clean:
    -del $(NAME).drv
    -del $(NAME).res
    -del *.sym
    -del *.map
    -del *.obj
    -del *.cod

##################### depend #####################

depend:
    mv fmcfg.mak makefile.old
    sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > fmcfg.mak
    -del makefile.old
    echo # START Dependencies >> fmcfg.mak
    includes -l *.c *.asm >> fmcfg.mak
    echo # END Dependencies >> fmcfg.mak

##################################################
# START Dependencies 

fmcfg.obj: fmcfg.c fmsynth.h fmcfg.h

cplapp.obj: cplapp.c fmsynth.h fmcfg.h fmvers.h

mscroll.obj: mscroll.c mscroll.h mscrdll.h

msinit.obj: msinit.c mscroll.h mscrdll.h

ctldlg.obj: ctldlg.c mscroll.h mscrdll.h

paint.obj: paint.c mscroll.h mscrdll.h

msapi.obj: msapi.c mscroll.h mscrdll.h

spinedit.obj: spinedit.c mscroll.h mscrdll.h

# END Dependencies 
