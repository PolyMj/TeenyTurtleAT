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

.const INTERRUPT_VECTOR_TABLE    0x8E00
.const INTERRUPT_ENABLE_REGISTER 0x8E10
.const CONTROL_STATUS_REGISTER   0x8EFF


; Setup hit-edge interrupt
    set rA, !hit_edge
    set rB, 12
    str [ INTERRUPT_VECTOR_TABLE + rB ], rA

; Enable external interrupt 12
    set rA, 0b00010000_00000000
    str [ INTERRUPT_ENABLE_REGISTER ], rA

; Enable interrupts globally
    set rA, 0b000000000000000_1
    str [ CONTROL_STATUS_REGISTER ], rA

set rE, MOVE_LEFT
set rD, 0b1000_0000_0000_0100 ; Enemy star

!main
    lod rA, [DETECT_FORWARD]
    cmp rA, rD ; Check if enemy star
    je !found_star

    str [rE], rZ
    jmp !main


!found_star
    set rC, 45
    !star_push
        str [MOVE_FORWARD], rZ
        lup rC, !star_push
    jmp !main

!hit_edge
    set rB, MOVE_LEFT
    cmp rE, rB
    je !need_move_right
    set rE, rB
    !need_move_right
    set rE, MOVE_RIGHT

    ; Move off of wall
    str [rE], rZ

    set rB, 30
    !next_row
        str [MOVE_FORWARD], rZ
        lup rB, !next_row
    
    rti