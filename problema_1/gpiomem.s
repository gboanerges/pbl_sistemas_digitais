@ Various macros to access the GPIO pins
@ on the Raspberry Pi.
@
@ R8 - memory map address.
@
.include "fileio.s"

.equ pagelen, 4096
.equ setregoffset, 28
.equ clrregoffset, 40
.equ PROT_READ, 1
.equ PROT_WRITE, 2
.equ MAP_SHARED, 1

@ Macro to map memory for GPIO Registers
.macro mapMem phys_address
    openFile devmem @ open /dev/mem
    MOVS r4, r0 @ fd for memmap
    @ check for error and print error msg if necessary
    BPL 1f @ pos number file opened ok
    MOV R1, #1 @ stdout
    LDR R2, =memOpnsz @ Error msg
    LDR R2, [R2]
    writeFile R1, memOpnErr, R2 @ print the error
    B _end
@ Set up can call the mmap2 Linux service
1:
    LDR r5, =\phys_address @ address we want / 4096
    LDR r5, [r5] @ load the address
    MOV r1, #pagelen @ size of mem we want

@ mem protection options
    MOV r2, #(PROT_READ + PROT_WRITE) @ protectOptions
    MOV r3, #MAP_SHARED @ mem share options
    MOV r0, #0 @ let linux choose a virtual address
    
    MOV r7, #sys_mmap2 @ mmap2 service num
    SVC 0 @ call service
    MOVS r8, r0 @ keep the returned virt addr
    @ check for error and print error msg
    @ if necessary.
    BPL 2f @ pos number file opened ok
    MOV R1, #1 @ stdout
    LDR R2, =memMapsz @ Error msg
    LDR R2, [R2]
    writeFile R1, memMapErr, R2 @ print the error
    B _end
2:
.endm

@ Macro nanoSleep to sleep .1 second
@ Calls Linux nanosleep entry point which is function 162.
@ Pass a reference to a timespec in both r0 and r1
@ First is input time to sleep in seconds and nanoseconds.
@ Second is time left to sleep if interrupted (which we ignore)
.macro nanoSleep
    LDR r0, =timespecsec
    LDR r1, =timespecsec
    MOV r7, #sys_nanosleep
    SVC 0
.endm

.macro GPIODirectionOut pin
    LDR r2, =\pin @ offset of select register
    LDR r2, [r2] @ load the value
    LDR r1, [r8, r2] @ address of register
    LDR r3, =\pin @ address of pin table
    ADD r3, #4 @ load amount to shift from table
    LDR r3, [r3] @ load value of shift amt
    MOV r0, #0b111 @ mask to clear 3 bits
    LSL r0, r3 @ shift into position
    BIC r1, r0 @ clear the three bits
    MOV r0, #1 @ 1 bit to shift into pos
    LSL r0, r3 @ shift by amount from table
    ORR r1, r0 @ set the bit
    STR r1, [r8, r2] @ save it to reg to do work
.endm

.macro GPIOTurnOn pin, value
    MOV r2, r8 @ address of gpio regs
    ADD r2, #setregoffset @ off to set reg
    MOV r0, #1 @ 1 bit to shift into pos
    LDR r3, =\pin @ base of pin info table
    ADD r3, #8 @ ADD offset for shift amt
    LDR r3, [r3] @ load shift from table
    LSL r0, r3 @ do the shift
    STR r0, [r2] @ write to the register
.endm

.macro GPIOTurnOff pin, value
    MOV r2, r8 @ address of gpio regs
    ADD r2, #clrregoffset @ off set of clr reg
    MOV r0, #1 @ 1 bit to shift into pos
    LDR r3, =\pin @ base of pin info table
    ADD r3, #8 @ ADD offset for shift amt
    LDR r3, [r3] @ load shift from table
    LSL r0, r3 @ do the shift
    STR r0, [r2] @ write to the register
.endm