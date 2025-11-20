
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

      
!loop

set rC, 120


!delay_loop
dec rC
cmp rC, 0
jne !delay_loop

; After delay, move forward
set rA, MOVE_FORWARD
set rB, 1
str [ rA ], rB

; Jump back to loop
jmp !loop