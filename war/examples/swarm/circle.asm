.const PORT_A_DIR                0x8000
.const PORT_B_DIR                0x8001
.const PORT_A                    0x8002
.const PORT_B                    0x8003
.const RAND                      0x8010
.const RAND_BITS                 0x8011
.const INTERRUPT_VECTOR_TABLE    0x8E00
.const INTERRUPT_ENABLE_REGISTER 0x8E10
.const CONTROL_STATUS_REGISTER   0x8EFF

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

!main

!detect_loop
    lod rB, [ DETECT_BACKWARD ]
    shf rB, 15
    cmp rB, rZ
    jne !backward

    lod rB, [ DETECT_LEFT ]
    shf rB, 15
    cmp rB, rZ
    jne !left
    
    lod rB, [ DETECT_RIGHT ]
    shf rB, 15
    cmp rB, rZ
    jne !right

!foward
    str [ MOVE_FORWARD ], rZ
    jmp !detect_loop

!backward
    str [ MOVE_BACKWARD ], rZ
    jmp !detect_loop

!left
    str [ MOVE_LEFT ], rZ
    jmp !detect_loop

!right
    str [ MOVE_RIGHT ], rZ
    jmp !detect_loop

