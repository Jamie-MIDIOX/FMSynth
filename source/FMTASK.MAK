############################################################################
#
#   makefile
#
#   Copyright (c) 1991-1992 Microsoft Corporation.  All Rights Reserved.
#
#   constructs fmtaskth device driver
#
#   to build a debug version:
#       NMAKE
#   to build a non debug version:
#       NMAKE DEBUG=NO
#
############################################################################

NAME    = fmtask
OBJ1    = fmtask.obj 
LIBS    = libw mlibcew fmsynth

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

CC      = cl -c -nologo -W3 -Zp -G2sw -AM $(DEF) $(CLOPT)
LINK    = link /NOPACKCODE/NOD/NOE/MAP/ALIGN:16 $(LINKOPT)

.c.obj:
    @$(CC) $*.c

###################### goal ######################

goal: $(NAME).exe
    @echo ***** finished making $(NAME) *****


# ..\bin\$(NAME).sym: $$(@F)
#    @copy $(@F) $@

##################################################

$(NAME).exe $(NAME).sym: $(OBJ1) $(NAME).def $(NAME).res
    @$(LINK) @<<
    $(OBJ1),
    $(NAME).exe,
    $(NAME).map,
    $(LIBS),
    $(NAME).def
<<
    @rc -t $(DEF) $(NAME).res $(NAME).exe
    @mapsym /n $(NAME).map

$(NAME).res: $(NAME).rc fmsynth.h fmvers.h
    @rc $(DEF) -r -z $(NAME).rc

##################### clean ######################

clean:
    -del $(NAME).drv
    -del $(NAME).res
    -del *.sym
    -del *.map
    -del *.obj
    -del *.cod

##################### depend #####################

##################################################
# START Dependencies 

fmtask.obj: fmtask.c fmsynth.h fmvers.h

# END Dependencies 
