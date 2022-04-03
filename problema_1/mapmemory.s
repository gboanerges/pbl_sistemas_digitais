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
.macro mapMem
    openFile devmem, S_RDWR @ open /dev/mem
    movs r4, r0 @ fd for memmap
    @ check for error and print error msg if necessary
    BPL 1f @ pos number file opened ok
    MOV R1, #1 @ stdout
    LDR R2, =memOpnsz @ Error msg
    LDR R2, [R2]
    writeFile R1, memOpnErr, R2 @ print the error
    B _end
@ Set up can call the mmap2 Linux service
1: ldr r5, =gpioaddr @ address we want / 4096
    ldr r5, [r5] @ load the address
    mov r1, #pagelen @ size of mem we want

@ mem protection options
    mov r2, #(PROT_READ + PROT_WRITE)
    mov r3, #MAP_SHARED @ mem share options
    mov r0, #0 @ let linux choose a virtual address
    
    mov r7, #sys_mmap2 @ mmap2 service num
    svc 0 @ call service
    movs r8, r0 @ keep the returned virt addr
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

.data
timespecsec: .word 0
timespecnano: .word 100000000
devmem: .asciz "/dev/mem"
memOpnErr: .asciz "Failed to open /dev/mem\n"
memOpnsz: .word .-memOpnErr
memMapErr: .asciz "Failed to map memory\n"
memMapsz: .word .-memMapErr
 .align 4 @ realign after strings
@ mem address of gpio register / 4096
gpioaddr: .word 0x3F200
pin17: .word 4 @ offset to select register
 .word 21 @ bit offset in select register
 .word 17 @ bit offset in set & clr register
pin22: .word 8 @ offset to select register
 .word 6 @ bit offset in select register
 .word 22 @ bit offset in set & clr register
pin27: .word 8 @ offset to select register
 .word 21 @ bit offset in select register
 .word 27 @ bit offset in set & clr register
.text