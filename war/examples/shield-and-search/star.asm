.const MOVE_FORWARD    0xD000
.const MOVE_BACKWARD   0xD001
.const MOVE_LEFT       0xD002
.const MOVE_RIGHT      0xD003
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

set rE, !move_back

!move_back
    str [MOVE_BACKWARD], rZ
    jmp rE

!move_right
    str [MOVE_RIGHT], rZ
    jmp !move_right

!hit_edge
    set rD, !move_back
    cmp rE, rD
    jne !not_moving_back
        str [MOVE_FORWARD], rZ
        set rE, !move_right
        rti
    !not_moving_back
    set rE, !stop
    rti

!stop
    dly 0xFFF
    jmp rE