@============================================================================@
@
@@ offsets of the UART registers
@
.equ UART_DR, 0x00 @ data register 0
.equ UART_FR, 0x18 @ flag register 24
.equ UART_IBRD, 0x24 @ integer baud rate divisor 36
.equ UART_FBRD, 0x28 @ fractional baud rate divisor40
.equ UART_LCRH, 0x2C @ line control register 44
.equ UART_CR, 0x30 @ control register
.equ UART_CONFIG_PARAMETERS, 0b1001110
                    @ BITS 6 5 4 3 2 1 0
                    @      1 0 0 1 1 1 0
                    @ Parity enabled
                    @ Even Parity
                    @ Two Stop Bits
                    @ 7 bits of message size
@
@@ Offsets of the Flag Register
@
.equ UART_TXFE, (1<<7) @ Transmit FIFO empty
.equ UART_RXFF, (1<<6) @ Receive FIFO full
.equ UART_TXFF, (1<<5) @ Transmit FIFO full
.equ UART_RXFE, (1<<4) @ Receive FIFO empty
.equ UART_BUSY, (1<<3) @ UART is busy xmitting
@============================================================================@
@
@@ Bits for the CR (control register)
@
.equ UART_RXE, (1<<9) @ Enable receiver
.equ UART_TXE, (1<<8) @ Enable transmitter
.equ UART_LBE, (1<<7) @ Enable loopback

.equ UART_FEN, (1<<4)

.equ UART_UARTEN_ON, (1<<0) @ Enable UART
.equ UART_UARTEN_OFF, (0<<0) @ Enable UART
@============================================================================@
@
@@ Constants for the mmap2 function
@
.equ pagelen, 4096
.equ PROT_READ, 1
.equ PROT_WRITE, 2
.equ MAP_SHARED, 1
@============================================================================@
@
@@ Constants to open the memory file
@
.EQU O_RDWR, 00000002
.EQU O_SYNC, 00010000
@============================================================================@
@
@@ Linux directives
@
.EQU sys_open, 5 @ open and possibly create a file
.EQU sys_mmap2, 192 @ function mmap2 to map the physical address
@============================================================================@
@
@@ Labels used within the program
@
.align 2
.data
devmem: .asciz "/dev/mem"
protectOptions: .word PROT_READ + PROT_WRITE
uart_address: .word 0x20201 @ 0x7E20100
@============================================================================@
.align 2

.section .text
.global _start

_start:
@============================================================================@
@
@@ Open the dev mem file to map the physical address we want
@
    LDR r0, =devmem
    LDR r1, =(O_RDWR + O_SYNC)
    MOV r7, #sys_open
    SVC 0
@
@@ End open file
@
@============================================================================@
@
@@ Setting up the parameters needed to call the mmap2 Linux service
@@ that in fact will map the physical address into virtual address
@
    MOVS r4, r0 @ fd for memmap function (fd stands for - File Descriptor)
                @ copying the content from r0 to r4
    LDR r5, =uart_address @ load the address of the label uart_address
    LDR r5, [r5] @ load the uart address we want already divided by 4096
    MOV r1, #pagelen @ size of mem we want

    @ mem protection options
    LDR r2, =protectOptions @ mov r2, #(PROT_READ + PROT_WRITE)
    MOV r3, #MAP_SHARED @ mem share options
    MOV r0, #0 @ let linux choose a virtual address

    MOV r7, #sys_mmap2 @ mmap2 service num
    SVC 0 @ call service and after that r0 will have the mapped address of uart
    MOVS r8, r0 @ keep the returned virt address
                @ R8 now contains the virtual address
@ close the devmem file
    MOV r0, r4
    MOV r7, #6
    SVC 0
@============================================================================@
@
@@ UART Configuration
@@ UARTEN bit on the Control Register must be set to 1, to activate the UART.
@@ But first we must disable the UART to change settings.
@@ Wait for the end of transmission or reception of the current character.
@@ Flush the transmit FIFO by setting the FEN bit to 0 in the Line Control Register.
@@ Reprogram the Control Register
@@ Enable the UART.
@
    MOV r0, #0 @ disable uart to begin changing settings
    STR r0, [r8, #UART_CR]    @ storing the value 0 to the first bit position
                             @ which is the bit UARTEN

@ Now we have to check if the there is a transmission or reception of data
fifo_status: @ looping until the there is no data i/o
    LDR r0, [r8, #UART_FR] @ load the flag register
    TST r0, #UART_TXFF @ r0 AND TXFF (0b10000)
    BNE fifo_status

@ Disabling FIFO on the Line Control Register
    MOV r0, #UART_FEN @ 1 << 4 which is the bit position of the FIFO enable bit
    LDR r2, [r8, #UART_LCRH] @ 11110000 AND 10000
    BIC r2, r0 @ r2 AND r0 - so all bits from r2 will get a 0, except the one in r0
    STR r2, [r8, #UART_LCRH] 

@ Configure the Line Control Register with the following options:
@ BITS 6 5 4 3 2 1 0
@      1 0 0 1 1 1 0
@ Parity enabled
@ Even Parity
@ Two Stop Bits
@ 7 bits of message size
    MOV r0, #UART_CONFIG_PARAMETERS @ #0b1001110
    LDR r1, [r8, #UART_LCRH]
    BIC r1, r0
    STR r1, [r8, #UART_LCRH]

@ Configure Baud Rate
@(3MHz / ( 115200 ∗ 16 )) = 1.62760416667
    MOV r0, #1
    STR r0, [r8, #UART_IBRD]
    MOV r0, #0x3e
    STR r0, [r8, #UART_FBRD]

@ Enable FIFO
    MOV r0, #UART_FEN @ (1 << 4) = 10000
                      @ which is the bit position of the FIFO enable bit
    LDR r1, [r8, #UART_LCRH] @     1001110
    ORR r1, r0 @ OR operation to maintain the configuration of the message
               @ now reactivating the FIFO, bit position to 1
    STR r1, [r8, #UART_LCRH]

@ Enable UART again and TX and RX
    MOV r0, #UART_UARTEN_ON @ enabling uart bit 1 on UARTEN
    MOV r2, #0b11 @ TXE and RXE bits
    LSL r2, #8 @ shifting 8 bits 1100000000
    ADD r0, r0, r2
    STR r0, [r8, #UART_CR]

@ End the execution
    MOV r0, #0
    MOV r7, #1
    SVC 0
@============================================================================@
