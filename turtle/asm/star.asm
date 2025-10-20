; TeenyAT Turtle - Infinite Star/Gear

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

; Variables
.const angle          0x0100
.const increment      0x0200

; Set pen properties
set rA, 0x001F        ; Blue Color
str [ PEN_COLOR ], rA

set rA, 2             ; Pen size
str [ PEN_SIZE ], rA

; Put pen down
str [ PEN_DOWN ], rZ

; Initialize angle to 0
str [ angle ], rZ

set rA, 1
str [increment], rA

!spiral_loop
    lod rC, [increment]
    add rC, 90000
    str [increment], rC

    ; Load and set current angle
    lod rA, [ angle ]
    str [ TURTLE_ANGLE ], rA
    
    ; Move forward
    set rB, 1
    str [ MOVE ], rB
    
    ; Small delay
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    dly 50000
    
    ; Increment angle by 5 degrees
    lod rA, [ angle ]
    add rA, increment
    str [ angle ], rA


    ; Loop forever
    jmp !spiral_loop