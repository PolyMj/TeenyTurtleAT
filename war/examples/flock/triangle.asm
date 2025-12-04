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
    lod rA, [DETECT_LEFT]
    lod rB, [DETECT_RIGHT]
    set rC, rA
    or rC, rB

    ; Units start in a line, check to see if you're alone, or on an edge
    cmp rC, rZ
    je !center_alone
    cmp rA, rZ
    je !left_edge
    cmp rB, rZ
    je !right_edge

    str [MOVE_FORWARD], rZ
    jmp !main

    !left_edge
        set rE, DETECT_RIGHT
        set rD, MOVE_LEFT
        jmp !flock

    !right_edge        
        set rE, DETECT_LEFT
        set rD, MOVE_RIGHT
        jmp !flock

    !center_alone        
        set rE, 0
        jmp !leader_loop


!flock
    ; Move flock members back and out
    set rC, 5
    !expand
        str [rD], rZ
        str [MOVE_BACKWARD], rZ
        lup rC, !expand
    
    ; Move flock members without passing
    !flock_loop
        lod rA, [rE]
        cmp rA, rZ
        jne !flock_loop

        str [MOVE_FORWARD], rZ
        jmp !flock_loop


!leader_loop
    dly 0x7 ; Leader should move slow enough for flock to catch up
    str [MOVE_FORWARD], rZ
    jmp !leader_loop


!stop
    dly 0xFFF
    jmp !stop