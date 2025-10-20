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

set rA, 2
str [ PEN_DOWN ], rZ
!main
    str [ MOVE ], rA
    str [ TURTLE_ANGLE ], rB
    lod rC, [ RAND_BITS ]
    str [ PEN_COLOR ], rC
    lod rC, [ RAND ]
    mod rC, 0x622
    lod rD, [ RAND ]
    mod rD, 0x10
    dly rD, rC
    add rB, 5
    jmp !main
