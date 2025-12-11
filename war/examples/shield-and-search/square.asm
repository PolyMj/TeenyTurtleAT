.const MOVE_FORWARD    0xD000
.const MOVE_BACKWARD   0xD001
.const MOVE_LEFT       0xD002
.const MOVE_RIGHT      0xD003
.const DETECT_FORWARD  0xE000
.const DETECT_BACKWARD 0xE001
.const DETECT_LEFT     0xE002
.const DETECT_RIGHT    0xE003
.const FRIEND_SQUARE   0b0000_0000_0000_0001
.const FRIEND_STAR     0b0000_0000_0000_00100

set rE, 40
!backward
    str [MOVE_BACKWARD], rZ
    dly 0x3F
    lup rE, !backward

set rE, FRIEND_SQUARE
set rD, FRIEND_STAR

!main
    lod rA, [DETECT_RIGHT]
    cmp rA, rE ; rE = FRIEND_SQUARE
    je !main

    cmp rA, rZ
    je !check_star

    !is_enemy
        str [MOVE_RIGHT], rZ
        jmp !main

    !check_star
        lod rB, [DETECT_BACKWARD]
        cmp rB, rD ; rD = FRIEND_STAR
        jne !main
        str [MOVE_RIGHT], rZ ; Move to the right of the star
        jmp !main


!forward_loop
    str [MOVE_FORWARD], rZ
    jmp !forward_loop

!stop
    dly 0xFFF
    jmp !stop