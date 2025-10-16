; TeenyAT Constants (same as demo.asm)
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

; Manually specified square corners (change these to alter the square)
.const X0 200
.const Y0 200
.const X1 440
.const Y1 200
.const X2 440
.const Y2 340
.const X3 200
.const Y3 340

; Square tracing program using external move-done interrupt

; Use external interrupt 8 (external interrupt 0 mapped to IVT index 8)
!main
    ; Install ISR into IVT (external interrupt 0 -> IVT index 8)
    set rA, !move_done_isr
    set rB, 8
    str [ INTERRUPT_VECTOR_TABLE + rB ], rA

    ; Enable external interrupt 0 (bit 8)
    set rA, 0b00000001_00000000
    str [ INTERRUPT_ENABLE_REGISTER ], rA

    ; Enable global interrupts (CSR bit)
    set rA, 0b000000000000000_1
    str [ CONTROL_STATUS_REGISTER ], rA

    ; init state counter (set to 3 so first ISR increments to 0)
    set rC, 3

    ; Issue first target (start at top-left corner)
    set rA, X0       ; x0
    set rB, Y0       ; y0
    str [ SET_X ], rA
    str [ SET_Y ], rB
    str [ FACE_XY ], rZ
    str [ GOTO_XY ], rZ

    ; Idle loop: ISR will set subsequent corners when it receives move-done interrupts
!idle
    jmp !idle

; ISR - called when host signals move complete
; increments state (rC), picks next corner, issues GOTO_XY
!move_done_isr
    psh rA           ; preserve used registers
    psh rB

    add rC, 1        ; rC = rC + 1
    mod rC, 4        ; rC = rC % 4  (0..3)

    set rA, rC       ; copy state into rA to compare
    mod rA, 4
    je !corner0      ; if rA == 0 -> corner0
    sub rA, 1
    je !corner1      ; if rA == 1 -> corner1
    sub rA, 1
    je !corner2      ; if rA == 2 -> corner2
    sub rA, 1
    je !corner3
 

!corner0
    set rA, X1       ; x1
    set rB, Y1       ; y1
    jmp !do_move

!corner1
    set rA, X2       ; x2
    set rB, Y2       ; y2
    jmp !do_move

!corner2
    set rA, X3       ; x3
    set rB, Y3       ; y3
    jmp !do_move

!corner3
    set rA, X0       ; x0 (back to start)
    set rB, Y0       ; y0
    ; fall through

!do_move
    str [ PEN_DOWN ], rZ
    str [ SET_X ], rA
    str [ SET_Y ], rB
    str [ FACE_XY ], rZ
    str [ GOTO_XY ], rZ
    

    pop rB
    pop rA
    rti
