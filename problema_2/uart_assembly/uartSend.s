@@ offsets to the UART registers
.equ UART_DR, 0x00 @ data register
.equ UART_RSRECR, 0x04 @ Receive Status/Error clear
.equ UART_FR, 0x18 @ flag register
.equ UART_ILPR, 0x20 @ not used
.equ UART_IBRD, 0x24 @ integer baud rate divisor
.equ UART_FBRD, 0x28 @ fractional baud rate divisor
.equ UART_LCRH, 0x2C @ line control register
.equ UART_CR, 0x30 @ control register
.equ UART_IFLS, 0x34 @ interrupt FIFO level select
.equ UART_IMSC, 0x38 @ Interrupt mask set clear
.equ UART_RIS, 0x3C @ raw interrupt status
.equ UART_MIS, 0x40 @ masked interrupt status
.equ UART_ICR, 0x44 @ interrupt clear register
.equ UART_DMACR, 0x48 @ DMA control register
.equ UART_ITCR, 0x80 @ test control register
.equ UART_ITIP, 0x84 @ integration test input

.equ UART_ITOP, 0x88 @ integration test output
.equ UART_TDR, 0x8C @ test data register

@@ error condition bits when reading the DR (data register)
.equ UART_OE, (1<<11) @ overrun error bit
.equ UART_BE, (1<<10) @ break error bit
.equ UART_PE, (1<<9) @ parity error bit
.equ UART_FE, (1<<8 ) @ framing error bit

@@ Bits for the FR (flags register)
.equ UART_RI, (1<<8) @ Unsupported
.equ UART_TXFE, (1<<7) @ Transmit FIFO empty
.equ UART_RXFF, (1<<6) @ Receive FIFO full
.equ UART_TXFF, (1<<5) @ Transmit FIFO full
.equ UART_RXFE, (1<<4) @ Receive FIFO empty
.equ UART_BUSY, (1<<3) @ UART is busy xmitting
.equ UART_DCD, (1<<2) @ Unsupported
.equ UART_DSR, (1<<1) @ Unsupported
.equ UART_CTS, (1<<0) @ Clear to send

@@ Bits for the LCRH (line control register)
.equ UART_SPS, (1<<7) @ enable stick parity
.equ UART_WLEN1, (1<<6) @ MSB of word length
.equ UART_WLEN0, (1<<5) @ LSB of word length
.equ UART_FEN, (1<<4) @ Enable FIFOs
.equ UART_STP2, (1<<3) @ Use 2 stop bits
.equ UART_EPS, (1<<2) @ Even parity select
.equ UART_PEN, (1<<1) @ Enable parity
.equ UART_BRK, (1<<0) @ Send break

@@ Bits for the CR (control register)
.equ UART_CTSEN, (1<<15) @ Enable CTS
.equ UART_RTSEN, (1<<14) @ Enable RTS
.equ UART_OUT2, (1<<13) @ Unsupported
.equ UART_OUT1, (1<<12) @ Unsupported
.equ UART_RTS, (1<<11) @ Request to send
.equ UART_DTR, (1<<10) @ Unsupported
.equ UART_RXE, (1<<9) @ Enable receiver
.equ UART_TXE, (1<<8) @ Enable transmitter
.equ UART_LBE, (1<<7) @ Enable loopback
.equ UART_SIRLP, (1<<2) @ Unsupported
.equ UART_SIREN, (1<<1) @ Unsupported
.equ UART_UARTEN, (1<<0) @ Enable UART
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
@ linux directives
.EQU sys_read, 3 @ read from a file descriptor
.EQU sys_write, 4 @ write to a file descriptor
.EQU sys_open, 5 @ open and possibly create a file
.EQU sys_close, 6 @ close a file descriptor
.EQU sys_mmap2, 192 @ map files or devices
.align 2

.data
flags: .word O_RDWR + O_SYNC
openMode: .word 0666
devmem: .asciz "/dev/mem"
uartaddr: .word 0x20201 @ 0x7E20100
@ mem address of gpio register / 4096
.align 2

.section .text
.global uartSend

uartSend:

    MOV r9, r0 @ save the command parameter to use later to send the message

@ Open the dev mem file to map the physical address we want
    ldr r0, =devmem
    ldr r1, =(O_RDWR + O_SYNC)
    mov r7, #sys_open
    svc 0
    @ end open file
    MOVS r4, r0 @ fd for memmap (fd stands for - File Descriptor)
    BPL 1f @ pos number file opened ok

@ Sets up the parameters needed to can call the mmap2 Linux service
@ that in fact will map the physical address into virtual address
1:
    LDR r5, =uartaddr @ address we want / 4096
    LDR r5, [r5] @ load the address
    MOV r1, #pagelen @ size of mem we want

@ mem protection options
    MOV r2, #(PROT_READ + PROT_WRITE)
    MOV r3, #MAP_SHARED @ mem share options
    MOV r0, #0 @ let linux choose a virtual address

    MOV r7, #sys_mmap2 @ mmap2 service num
    SVC 0 @ call service
    MOVS r8, r0 @ keep the returned virt addr
    @ R8 now contains the virtual address
    
    @esperar a uart acabar a transmissão se houver alguma
wait_fifo:	LDR r2, [r8, #UART_FR]
	tst r2, #UART_TXFF @ trasmit fifo has at least room for 1 byte
	bne wait_fifo

    @ send the byte passed as a parameter from the C call
	str r9, [r8, #UART_DR]
@    mov r0, #0b01001001
@	str r0, [r8, #UART_DR]


wait_busy: ldr 	r1, [r8, #UART_FR]
	and 	r1, #0b1000
	cmp	r1, #0
	bne	wait_busy

    mov r0, r9
    BX LR
