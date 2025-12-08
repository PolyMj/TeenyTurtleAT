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

set rE, 0

!main
    ; Move right first
    set rC, 100
    !move_right
        str [MOVE_RIGHT], rZ
        lup rC, !move_right
    
    ; Move forward
    set rC, 200
    !move_forward
        str [MOVE_FORWARD], rZ
        lup rC, !move_forward
    
    ; Move left
    set rC, 100
    !move_left
        str [MOVE_LEFT], rZ
        lup rC, !move_left
    
    ; Move forward again continuously
    !forward_loop
        str [MOVE_FORWARD], rZ
        jmp !forward_loop


!stop
    dly 0xFFF
    jmp !stop