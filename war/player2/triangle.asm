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
    
!loop
    dly 0xF
    ;set rA, 0x8000
    lod rB, [ DETECT_FORWARD ]
    ;and rB, rA
    shf rB, 15
    cmp rB, rZ
    jne !stop
    str [ MOVE_FORWARD ], rZ
    jmp !loop

!stop
    jmp !stop