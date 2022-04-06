@
.include "gpiomem.s"
@
.global _start


_start:
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

_end: 
    mov R0, #0 @ Use 0 return code
    mov R7, #1 @ Command code 1 terms
    svc 0 @ Linux command to terminate

