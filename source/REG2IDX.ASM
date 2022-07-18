        lea     bx, Reg2Idx            ; use xlation table
        xlat    bx                      ; Look up index

        ; Register To Index Map

        Reg2Idx db      00h, 00h, 01h, 02h, 03h, 04h, 00h, 00h,  ; 00-07
                        05h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 08-0F
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 10-17
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 18-1F
                        06h, 07h, 08h  09h, 0Ah, 0Bh, 00h, 00h,  ; 20-27
                        0Ch, 0Dh, 0Eh, 0Fh, 10h, 11h, 00h, 00h,  ; 28-2F
                        12h, 13h, 14h, 15h, 16h, 17h, 00h, 00h,  ; 30-37
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 38-3F
                        18h, 19h, 1Ah, 1Bh, 1Ch, 1Dh, 00h, 00h,  ; 40-47
                        1Eh, 1Fh, 20h, 21h, 22h, 23h, 00h, 00h,  ; 48-4F
                        24h, 25h, 26h, 27h, 28h, 29h, 00h, 00h,  ; 50-57
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 58-5F
                        2Ah, 2Bh, 2Ch, 2Dh, 2Eh, 2Fh, 00h, 00h,  ; 60-67
                        30h, 31h, 32h, 33h, 34h, 35h, 00h, 00h,  ; 68-6F
                        36h, 37h, 38h, 39h, 3Ah, 3Bh, 00h, 00h,  ; 70-77
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 78-7F
                        3Ch, 3Dh, 3Eh, 3Fh, 40h, 41h, 00h, 00h,  ; 80-87
                        42h, 43h, 44h, 45h, 46h, 47h, 00h, 00h,  ; 88-8F
                        48h, 49h, 4Ah, 4Bh, 4Ch, 4Dh, 00h, 00h,  ; 90-97
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; 98-9F
                        4Eh, 4Fh, 50h, 51h, 52h, 53h, 54h, 55h,  ; A0-A7
                        56h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; A8-AF
                        57h, 58h, 59h, 5Ah, 5Bh, 5Ch, 5Dh, 5Eh,  ; B0-B7
                        5Fh, 00h, 00h, 00h, 00h, 60h, 00h, 00h,  ; B8-BF
                        61h, 62h, 63h, 64h, 65h, 66h, 67h, 68h,  ; C0-C7
                        69h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; C8-CF
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; D0-D7
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h,  ; D8-DF
                        6Ah, 6Bh, 6Ch, 6Dh, 6Eh, 6Fh, 00h, 00h,  ; E0-E7
                        70h, 71h, 72h, 73h, 74h, 75h, 00h, 00h,  ; E8-EF
                        76h, 77h, 78h, 79h, 7Ah, 7Bh, 00h, 00h,  ; F0-F7
                        00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h   ; F8-FF
