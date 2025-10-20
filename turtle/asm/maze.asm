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

; Setup interrupt callback by mapping it to external interrupt 8
set rA, !move_done_isr
set rB, 8
str [ INTERRUPT_VECTOR_TABLE + rB ], rA

; Enable external interrupt 8
set rA, 0b00000001_00000000
str [ INTERRUPT_ENABLE_REGISTER ], rA

; Enable interrupts globally
set rA, 0b000000000000000_1
str [ CONTROL_STATUS_REGISTER ], rA

; default heading
set rA, 90
str [TURTLE_ANGLE], rA

jmp !main


!loop
    ; forever loop
    jmp !loop
    
!main
    ; Read color ahead into rA
    lod rA, [ DETECT_AHEAD ]
    ; If red, stop
    set rB, 0xf800
    cmp rA, rB
    je !end

    ; If blocked (black == 0), perform rotation checks
    cmp rA, rZ
    je  !do_rotate   ; if black, rotate logic

    ; Not black -> move one pixel forward according to heading
    ; Read heading into rA
    lod rA, [ TURTLE_ANGLE ]

    ; Check heading == 0
    set rB, 0
    set rC, rA
    sub rC, rB
    je  !fwd_0

    ; Check heading == 90
    set rB, 90
    set rC, rA
    sub rC, rB
    je  !fwd_90

    ; Check heading == 180
    set rB, 180
    set rC, rA
    sub rC, rB
    je  !fwd_180

    ; else treat as 270
    jmp !fwd_270

!fwd_0
    ; -Y direction
    lod rD, [ TURTLE_Y ]
    dec rD
    str [ SET_Y ], rD
    lod rE, [ TURTLE_X ]
    str [ SET_X ], rE
    str [ FACE_XY ], rZ
    str [ GOTO_XY ], rZ
    rti

!fwd_90
    ; +X direction
    lod rD, [ TURTLE_X ]
    inc rD
    str [ SET_X ], rD
    lod rE, [ TURTLE_Y ]
    str [ SET_Y ], rE
    str [ FACE_XY ], rZ
    str [ GOTO_XY ], rZ
    rti

!fwd_180
    ; +Y direction
    lod rD, [ TURTLE_Y ]
    inc rD
    str [ SET_Y ], rD
    lod rE, [ TURTLE_X ]
    str [ SET_X ], rE
    str [ FACE_XY ], rZ
    str [ GOTO_XY ], rZ
    rti

!fwd_270
    ; -X direction
    lod rD, [ TURTLE_X ]
    dec rD
    str [ SET_X ], rD
    lod rE, [ TURTLE_Y ]
    str [ SET_Y ], rE
    str [ FACE_XY ], rZ
    str [ GOTO_XY ], rZ
    rti

; Rotate handling: try +90, if still blocked try +180, if still blocked -90
!do_rotate

    ; --- wall detected: try rotate +90 and test ---
    lod rB, [ TURTLE_ANGLE ]
    set rC, 90
    add rB, rC
    str [ TURTLE_ANGLE ], rB

    ; check again
    lod rA, [ DETECT_AHEAD ]
    cmp rA, rZ
    jne !main

    ; --- still blocked: rotate +180 from current (+90 -> +270 relative orig) ---
    lod rB, [ TURTLE_ANGLE ]
    set rC, 180
    add rB, rC
    str [ TURTLE_ANGLE ], rB

    ; check again
    lod rA, [ DETECT_AHEAD ]
    cmp rA, rZ
    jne !main

    ; --- still blocked: rotate -90 (from current) and then continue (+180 relative orig)---
    lod rB, [ TURTLE_ANGLE ]
    set rC, 90
    sub rB, rC
    str [ TURTLE_ANGLE ], rB
    jmp !main

;ISR
!move_done_isr    
    jmp !main

!end
    jmp !end
