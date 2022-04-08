@
@.include "gpiomem.s"
@
.include "fileio.s"
@
.equ pagelen, 4096
.equ setregoffset, 28
.equ clrregoffset, 40
.equ PROT_READ, 1
.equ PROT_WRITE, 2
.equ MAP_SHARED, 1
@
.EQU O_RDONLY, 0
.EQU O_WRONLY, 1
.EQU O_CREAT, 0100
.EQU O_RDWR, 00000002
.EQU O_SYNC, 00010000
.EQU S_RDWR, 0666
# linux directives
.EQU sys_read, 3 @ read from a file descriptor
.EQU sys_write, 4 @ write to a file descriptor
.EQU sys_open, 5 @ open and possibly create a file
.EQU sys_close, 6 @ close a file descriptor
.EQU sys_mmap2, 192 @ map files or devices
@
.align 2
.section .data
timespecsec: .word 0
timespecnano: .word 100000000
devmem: .asciz "/dev/mem"
memOpnErr: .asciz "Failed to open /dev/mem\n"
memOpnsz: .word .-memOpnErr
memMapErr: .asciz "Failed to map memory\n"
memMapsz: .word .-memMapErr
accessFlags: .word O_RDWR + O_SYNC @ 00000002 + 00010000
accessMode: .word S_RDWR @0666
protectOptions: .word PROT_READ + PROT_WRITE 

@ mem address of gpio register / 4096
gpioaddr: .word 0x20200 @ 0x7E20100
pin17: .word 4 @ offset to select register
 .word 21 @ bit offset in select register
 .word 17 @ bit offset in set & clr register
pin22: .word 8 @ offset to select register
 .word 6 @ bit offset in select register
 .word 22 @ bit offset in set & clr register
pin27: .word 8 @ offset to select register
 .word 21 @ bit offset in select register
 .word 27 @ bit offset in set & clr register

.section .text
	 .align 2
.global _start

_start:
    openFile devmem @ open /dev/mem
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
    ldr r2, =protectOptions @(PROT_READ + PROT_WRITE)
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

_end: 
    mov R0, #0 @ Use 0 return code
    mov R7, #1 @ Command code 1 terms
    svc 0 @ Linux command to terminate

