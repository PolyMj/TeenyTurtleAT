; TeenyAT Constants
.const PORT_A_DIR                0x8000
.const PORT_B_DIR                0x8001
.const PORT_A                    0x8002
.const PORT_B                    0x8003
.const RAND                      0x8010
.const RAND_BITS                 0x8011
.const INTERRUPT_VECTOR_TABLE    0x8E00
.const INTERRUPT_ENABLE_REGISTER 0x8E10
.const CONTROL_STATUS_REGISTER   0x8EFF

; Turtle Peripherals
.const TURTLE_X       0xD000
.const TURTLE_Y       0xD001
.const TURTLE_ANGLE   0xD002
.const SET_X          0xD003
.const SET_Y          0xD004
.const PEN_DOWN       0xD010
.const PEN_UP         0xD011
.const PEN_COLOR      0xD012
.const PEN_SIZE       0xD013
.const GOTO_XY        0xE000
.const FACE_XY        0xE001
.const MOVE           0xE002
.const DETECT         0xE003
.const SET_ERASER     0xE004
.const DETECT_AHEAD   0xE005
.const GET_COLOR_CHANGE  0xE010
.const NEXT_COLOR_CHANGE 0xE011
.const STOP_MOVE         0xE012
.const TERM           0xE0F0

; Setup interrupt callback by mapping it to external interrupt 10
set rA, !color_change_interrupt
set rB, 10
str [ INTERRUPT_VECTOR_TABLE + rB ], rA

; Enable external interrupt 8
set rA, 0b00000101_00000000
str [ INTERRUPT_ENABLE_REGISTER ], rA

; Enable interrupts globally
set rA, 0b000000000000000_1
str [ CONTROL_STATUS_REGISTER ], rA

set rA, 90
str [TURTLE_ANGLE], rA
set rA, 1
str [ MOVE ], rA

!main
    jmp !main

!color_change_interrupt

    ; stop movement while we decide
    str [ MOVE ], rZ

    ; Read current color-change value; if non-zero -> no wall -> resume
    lod rA, [ GET_COLOR_CHANGE ]
    cmp rA, rZ
    jne !resume_move

    ; --- wall detected: try rotate +90 and test ---
    lod rB, [ TURTLE_ANGLE ]
    set rC, 90
    add rB, rC
    str [ TURTLE_ANGLE ], rB

    ; check again
    lod rA, [ DETECT_AHEAD ]
    cmp rA, rZ
    jne !start_move  ; open after +90

    ; --- still blocked: rotate +180 from current (+90 -> +270 relative orig) ---
    lod rB, [ TURTLE_ANGLE ]
    set rC, 180
    add rB, rC
    str [ TURTLE_ANGLE ], rB

    ; check again
    lod rA, [ DETECT_AHEAD ]
    cmp rA, rZ
    jne !start_move  ; open after +180

    ; --- still blocked: rotate -90 (from current) and then move ---
    lod rB, [ TURTLE_ANGLE ]
    set rC, 90
    sub rB, rC
    str [ TURTLE_ANGLE ], rB

    ; fall through to start move
!start_move
    set rA, 1
    str [ MOVE ], rA
    rti

!resume_move
    ; no wall originally — resume movement
    set rA, 1
    str [ MOVE ], rA
    rti