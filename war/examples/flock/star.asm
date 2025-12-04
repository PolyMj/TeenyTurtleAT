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


!main
    lod rA, [DETECT_FORWARD]
    
    cmp rA, rZ
    je !none

    shr rA, 15
    cmp rA, rZ
    je !friend
    jne !foe

    !none
        str [MOVE_FORWARD], rZ
        jmp !main
    !friend
        dly 0xFF
        jmp !main
    !foe
        set rA, 50
        !retreat
            str [MOVE_BACKWARD], rZ
            lup rA, !retreat
        jmp !main


!stop
    dly 0xFFF
    jmp !stop