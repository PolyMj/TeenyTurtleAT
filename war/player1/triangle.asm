.const MOVE_FORWARD    0xD000
.const MOVE_BACKWARD   0xD001
.const MOVE_LEFT       0xD002
.const MOVE_RIGHT      0xD003
.const DETECT_FORWARD  0xE000
.const DETECT_BACKWARD 0xE001
.const DETECT_LEFT     0xE002
.const DETECT_RIGHT    0xE003
.const CURRENT_HEALTH  0xF000
.const ALVIE_UNITS     0xF001
.const DEAD_UNITS      0xF002

set rB, MOVE_FORWARD

!main
    set rA, 30
    !loop
        dly 0xF
        str [rB], rZ
        lup rA, !loop

    inc rB
    neg rB
    dec rB
    or  rB, 4
    inc rB
    neg rB

    jmp !main