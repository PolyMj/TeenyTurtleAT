; This assembly makes the turtle goto random target positions, with the pen either up or down
    ; When the pen is down, it just draws to the target unimpeded
    ; When the pen is up, it sees all non-white colors as walls
        ; When it hits a wall, it selects a new target position

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
.const GET_COLOR_CHANGE  0xE010
.const NEXT_COLOR_CHANGE 0xE011
.const STOP_MOVE         0xE012
.const TERM           0xE0F0

; Setup interrupt callback by mapping it to external interrupt 8
set rA, !set_new_target_XY
set rB, 8
str [ INTERRUPT_VECTOR_TABLE + rB ], rA

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

str [ GOTO_XY ], rZ
str [ IS_PEN_DOWN ], rZ
set rA, 1
str [ PEN_SIZE ], rA
!main
    set rA, rZ
    jmp !main


.var IS_PEN_DOWN 0
;-------------------------------------------------
!set_new_target_XY
    psh rA
    lod rA, [ RAND ]
    mod rA, 640
    str [ SET_X ], rA
    lod rA, [ RAND ]
    mod rA, 500
    str [ SET_Y ], rA
    ; jmp !return ; Skip penup/pendown (no drawing, just navigation)
    mod rA, 2
    je !pen_goes_up
    lod rA, [ RAND_BITS ]
    str [ PEN_COLOR ], rA
    str [ PEN_DOWN ], rZ
    set rA, 1
    str [ IS_PEN_DOWN ], rA
    jmp !return
!pen_goes_up
    str [ PEN_UP ], rZ
    str [ IS_PEN_DOWN ], rZ
!return
    str [ GOTO_XY ], rZ
    str [ FACE_XY ], rZ
    pop rA
    rti

!color_change_interrupt
    psh rA
    lod rA, [ IS_PEN_DOWN ]
    cmp rA, rZ
    je !cc_loop
    rti
!cc_loop
    lod rA, [ GET_COLOR_CHANGE ]
    inc rA
    cmp rA, rZ
    jne !wall ; If not white -> is a wall -> jmp !wall
    ; Check if there is another color change
    lod rA, [ NEXT_COLOR_CHANGE ]
    cmp rA, rZ
    jne !cc_loop
    jmp !no_wall
!wall
    dec rA
    lod rA, [ RAND ]
    mod rA, 640
    str [ SET_X ], rA
    lod rA, [ RAND ]
    mod rA, 500
    str [ SET_Y ], rA
    str [ GOTO_XY ], rZ
    str [ FACE_XY ], rZ
    str [ STOP_MOVE ], rZ
!no_wall
    pop rA
    rti