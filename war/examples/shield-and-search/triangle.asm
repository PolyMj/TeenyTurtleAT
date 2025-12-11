.const MOVE_FORWARD    0xD000
.const MOVE_RIGHT      0xD003

!main
    str [MOVE_FORWARD], rZ
    str [MOVE_FORWARD], rZ
    str [MOVE_FORWARD], rZ
    str [MOVE_RIGHT], rZ
    dly 0xFF
    jmp !main