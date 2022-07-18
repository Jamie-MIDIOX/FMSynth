        page    60, 132
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   FMSyntha.asm
;
;   Copyright (c) 1991-1992 Microsoft Corporation.  All Rights Reserved.
;
;   Adapted for FMSynth 930404 JWO
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        PMODE = 1

        .xlist
        include cmacros.inc                   
        .list

        ?PLM=1                          ; Pascal calling convention
        ?WIN=0                          ; NO! Windows prolog/epilog code

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   debug support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifdef DEBUG
    extrn OutputDebugStr:far          ; mmsystem
    extrn _wDebugLevel:word           ; initc.c
endif

D1 macro text
    DOUT 1, < ",13,10,"FMSYNTH: &text&>
    endm
D2 macro text
    DOUT 2, < &text&>
    endm
D3 macro text
    DOUT 3, < &text&>
    endm
D4 macro text
    DOUT 4, < &text&>
    endm

DOUT macro level, text
    local   string_buffer
    local   wrong_level

ifdef DEBUG

_DATA segment
string_buffer label byte
    db      "&text&", 0
_DATA ends

    cmp     [_wDebugLevel], level
    jl      wrong_level
    push    ds
    push    DataOFFSET string_buffer
    call    OutputDebugStr
wrong_level:
endif
    endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   assert macros
;
;   AssertF byte        -- fail iff byte==0
;   AssertT byte        -- fail iff byte!=0
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

AssertF     macro exp
    local   assert_ok
ifdef DEBUG
    push    ax
    
    mov     al, exp
    or      al, al
    jnz     assert_ok

    D1      <AssertF fail (&exp&)>
    int     3

assert_ok:
    pop     ax
endif
    endm

AssertT     macro exp
    local   assert_ok
ifdef DEBUG
    push    ax
    
    mov     al, exp
    or      al, al
    jz      assert_ok

    D1      <AssertT fail (&exp&)>
    int     3

assert_ok:
    pop     ax
endif
    endm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   data segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sBegin Data
        externW <_wPort>                 ; base address of sound chip
        externW <_wFMChipAddr>           ; current address of chip
        externW <_wRiteFMChip>           ; if high chip, address
        externW <_wWriteDelay1>          ; delay time for write
        externW <_wWriteDelay2>          ; delay time for write
        externB <_LRegMap>               ; Register Map
        externB <_RRegMap>               ; Right Register Map  
        externW <_fShadow>               ; delay time for write      
sEnd Data

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef SEGNAME
        SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg

        assumes cs, CodeSeg
        assumes ds, Data

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; @doc INTERNAL 
;
; @api BYTE | inport | Read a port.
;
; @parm WORD | port | The port to read.
;
; @rdesc Returns the port value read.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        assumes ds, Data
        assumes es, nothing

cProc inport <FAR, PUBLIC> <>
cBegin nogen

        mov     dx, [_wFMChipAddr]
        in      al, dx

        D4 <I:#AL>

        retf

cEnd nogen


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; @doc INTERNAL 
;
; @api void | BlindOutput | This function writes data to the chip registers.
;
; @parm BYTE | bRegister | The address of the register to write.
;
; @parm BYTE | bData | The data value to write.
;
; @rdesc There is no return value.
;
; @comm Delay is included after each write.  Current implementation is less
;     than ideal - I'm currently investigating methods of calculating the
;     real number of cycles needed (which requires determining machine speed)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        assumes ds, Data
        assumes es, nothing

cProc BlindOutput <FAR, PUBLIC> <>
        ParmB   bRegister               ; register address
        ParmB   bData                   ; value to write
cBegin

ifdef DEBUG
        mov     al, bRegister
        mov     ah, bData
        D4 <O:#AL,#AH>
endif
        push    si
        mov     dx, [_wFMChipAddr]      ; select address of sound chip

        mov     al, bRegister           ; register number to write to
        mov     ah, bData               ; value to write (5/4/1 cycles)
        mov     bl, al
        xor     bh, bh

        cmp     dx, [_wRiteFMChip]      ; is it the top one?
        je      UseRite

        lea     si, _LRegMap     ; Use left shadow regs
        jmp     RiteValue

UseRite:
        lea     si, _RRegMap     ; use right shadow regs

RiteValue:
        out     dx, al

        mov     [si+bx],ah              ; 9/3/2 Assign value to map

;   A a minimum of 3.3 us delay is needed after selecting register:

        mov     cx, [_wWriteDelay1]     ; 5/4/1 cycles
@@:     dec     cx                      ; 2/2/1 cycles
        jnz     @B                      ; 9/9/3 cycles (3/3/1 last time)

        inc     dx                 ; data write address of chip (2/2/1 cycles)
        mov     al, ah             ; value to write (5/4/1 cycles)
        out     dx, al

;   A a minimum of 23 us delay is needed after writing a note:
;   To OPL2 -- OPL3 requires 4 master clock cycles
        mov     cx, [_wWriteDelay2]     ; 5/4/1 cycles
@@:     dec     cx                      ; 2/2/1 cycles
        jnz     @B                      ; 9/9/3 cycles (3/3/1 last time)
        pop     si
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; @doc INTERNAL 
;
; @api void | SndOutput | This function writes data to the chip registers.
;
; @parm BYTE | bRegister | The address of the register to write.
;
; @parm BYTE | bData | The data value to write.
;
; @rdesc There is no return value.
;
; @comm Delay is included after each write.  Current implementation is less
;     than ideal - I'm currently investigating methods of calculating the
;     real number of cycles needed (which requires determining machine speed)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; This now uses shadow registers to avoid repeat writes

        assumes ds, Data
        assumes es, nothing

cProc SndOutput <FAR, PUBLIC> <>
        ParmB   bRegister               ; register address
        ParmB   bData                   ; value to write
cBegin

        push    si
        
ifdef DEBUG
        mov     al, bRegister
        mov     ah, bData
        D4 <O:#AL,#AH>
endif

        mov     dx, [_wFMChipAddr]      ; select address of sound chip
        mov     al, bRegister           ; register number to write to
        mov     ah, bData               ; value to write (5/4/1 cycles)
        mov     bl, al
        xor     bh, bh

        cmp     dx, [_wRiteFMChip]      ; is it the top one?
        je      ChkRite

        lea     si, _LRegMap     ; Use left shadow regs
        jmp     ChkValue

ChkRite:
        lea     si, _RRegMap     ; use right shadow regs

ChkValue:
        cmp     [_fShadow],0            ; shadowing?
        je      ForceIT                 ; No

        cmp     [si+bx],ah              ; reg already set?
        je      HadValue                ; yes...leave

ForceIT:
        mov     al, bl
        out     dx, al

;   A a minimum of 3.3 us delay is needed after selecting register:

        mov     cx, [_wWriteDelay1]     ; 5/4/1 cycles
        mov     [si+bx],ah              ; 9/3/2 Assign value to map

@@:     dec     cx                      ; 2/2/1 cycles
        jnz     @B                      ; 9/9/3 cycles (3/3/1 last time)
        
        inc     dx                 ; data write address of chip (2/2/1 cycles)

        mov     al, ah
        out     dx, al

;   A a minimum of 23 us delay is needed after writing a note:
;   To OPL2 -- OPL3 requires 4 master clock cycles
        mov     cx, [_wWriteDelay2]     ; 5/4/1 cycles
@@:     dec     cx                      ; 2/2/1 cycles
        jnz     @B                      ; 9/9/3 cycles (3/3/1 last time)

HadValue:
        pop     si
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; CritEnter
;
;   Saves the current state of the interrupt flag on the stack and
;   then disables interrupts.
;
; Registers Destroyed:
;       BX, FLAGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        assumes ds, nothing
        assumes es, nothing

cProc CritEnter <FAR, PASCAL, PUBLIC>, <>
cBegin nogen
        pop     dx                      ; get return address
        pop     ax
        pushf
        pushf
        pop     bx
        test    bh, 2                   ; if interrupts are already off, don't
        jz      no_cli                  ; ... blow ~300 clocks doing the cli
        cli
no_cli:
        push    ax
        push    dx
        ret
cEnd nogen

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; CritLeave
;
;   Restores the interrupt state saved by CritEnter
;
; Registers Destroyed:
;       BX, FLAGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        assumes ds, nothing
        assumes es, nothing

cProc CritLeave <FAR, PASCAL, PUBLIC>, <>
cBegin nogen
        pop     dx                      ; get return address
        pop     ax

        pop     bx
        test    bh, 2
        jz      no_sti
        sti
no_sti:
        push    ax
        push    dx
        ret
cEnd nogen

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; @doc INTERNAL
;
; @asm WEP | This function is called when the DLL is unloaded.
;
; @parm WORD | UselessParm | This parameter has no meaning.
;
; @comm WARNING: This function is basically useless since you can't call any
;     kernel function that may cause the LoadModule() code to be reentered.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        assumes ds, nothing
        assumes es, nothing

cProc WEP <FAR, PUBLIC, PASCAL>, <>
;       ParmW   wUselessParm
cBegin nogen
        mov     ax, 1
        retf    2
cEnd nogen

sEnd CodeSeg

        end
