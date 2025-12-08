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
        jmp !go_left

    !right_edge        
        set rE, DETECT_LEFT
        jmp !go_right

    !center_alone        
        set rE, 0
        jmp !leader_loop


!go_left
    ; Move left first
    set rC, 5
    !move_left
        str [MOVE_LEFT], rZ
        lup rC, !move_left
    
    ; Then move forward
    !left_forward_loop
        lod rA, [rE]
        cmp rA, rZ
        jne !left_forward_loop

        str [MOVE_FORWARD], rZ
        jmp !left_forward_loop


!go_right
    ; Move right first
    set rC, 5
    !move_right
        str [MOVE_RIGHT], rZ
        lup rC, !move_right
    
    ; Then move forward
    !right_forward_loop
        lod rA, [rE]
        cmp rA, rZ
        jne !right_forward_loop

        str [MOVE_FORWARD], rZ
        jmp !right_forward_loop


!leader_loop
    dly 0x7
    str [MOVE_FORWARD], rZ
    jmp !leader_loop


!stop
    dly 0xFFF
    jmp !stop