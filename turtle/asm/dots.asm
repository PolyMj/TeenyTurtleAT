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

; Setup interrupt callback by mapping it to external interrupt 8
set rA, !set_new_target_XY
set rB, 8
str [ INTERRUPT_VECTOR_TABLE + rB ], rA

; Enable external interrupt 8
set rA, 0b00000001_00000000
str [ INTERRUPT_ENABLE_REGISTER ], rA

; Enable interrupts globally
set rA, 0b000000000000000_1
str [ CONTROL_STATUS_REGISTER ], rA

set rA, 8
str [ PEN_SIZE ], rA
set rA, rZ

!main
    str [ GOTO_XY ], rZ
    str [ FACE_XY ], rZ
    jmp !main

;-------------------------------------------------
!set_new_target_XY
    psh rA
    ;; Set Target X/Y
    lod rA, [ RAND ]
    mod rA, 60
    add rA, 290
    str [ SET_X ], rA
    lod rA, [RAND]
    mod rA, 60
    add rA, 220
    str [ SET_Y ], rA
    ;; Set pen color
    lod rA, [ RAND_BITS ]
    str [ PEN_COLOR ], rA
    ;; Draw dot by placing pen down then bringing it back up
    str [ PEN_DOWN ], rZ
    str [ PEN_UP ], rZ
    pop rA
    rti
