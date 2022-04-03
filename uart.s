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

.text
.align 2
@@ ----------------------------------------------------------
    .global UART_put_byte
UART_put_byte:
    ldr r1,=uartbase @ load base address of UART
    ldr r1,[r1] @ load base address of UART
putlp: ldr r2,[r1,#UART_FR] @ read the flag resister
    tst r2,#UART_TXFF @ check if transmit FIFO is full
    bne putlp @ loop while transmit FIFO is full
    str r0,[r1,#UART_DR] @ write the char to the FIFO
    mov pc,lr @ return
    
@@@ ---------------------------------------------------------
    .global UART_get_byte
UART_get_byte:
    ldr r1,=uartbase @ load base address of UART
    ldr r1,[r1] @ load base address of UART
getlp: ldr r2,[r1,#UART_FR] @ read the flag resister
    tst r2,#UART_RXFE @ check if receive FIFO is empty
    bne getlp @ loop while receive FIFO is empty
    ldr r0,[r1,#UART_DR] @ read the char from the FIFO
    tst r0,#UART_OE @ check for overrun error
    bne get_ok1
    @@ handle receive overrun error here - does nothing now
get_ok1:
    tst r0,#UART_BE @ check for break error
    bne get_ok2
    @@ handle receive break error here - does nothing now
get_ok2:
    tst r0,#UART_PE @ check for parity error
    bne get_ok3
    @@ handle receive parity error here - does nothing now
get_ok3:
    tst r0,#UART_FE @ check for framing error
    bne get_ok4
    @@ handle receive framing error here - does nothing now
get_ok4:
    @@ return
    mov pc,lr @ return the received character
    @@@ ---------------------------------------------------------
    @@@ UART init will set default values:
    @@@ 115200 baud, no parity, 2 stop bits, 8 data bits

    .global UART_init
UART_init:
    ldr r1,=uartbase @ load base address of UART
    ldr r1,[r1] @ load base address of UART
    @@ set baud rate divisor
    @@ (3MHz / ( 115200 ∗ 16 )) = 1.62760416667
    @@ = 1.101000 in binary
    mov r0,#1
    str r0,[r1,#UART_IBRD]
    mov r0,#0x28
    str r0,[r1,#UART_FBRD]
    @@ set parity, word length, enable FIFOS
    .equ BITS, (UART_WLEN1|UART_WLEN0|UART_FEN|UART_STP2)
    mov r0,#BITS
    str r0,[r1,#UART_LCRH]
    @@ mask all UART interrupts
    mov r0,#0
    str r0,[r1,#UART_IMSC]
    @@ enable receiver and transmitter and enable the uart
    .equ FINALBITS, (UART_RXE|UART_TXE|UART_UARTEN)
    ldr r0,=FINALBITS
    str r0,[r1,#UART_CR]
    @@ return
    mov pc,lr
    @@ ---------------------------------------------------------
    @@ UART_set_baud will change the baud rate to whatever is in r0
    @@ The baud rate divisor is calculated as follows: Baud rate
    @@ divisor BAUDDIV = (FUARTCLK/(16 Baud rate)) where FUARTCLK
    @@ is the UART reference clock frequency. The BAUDDIV
    @@ is comprised of the integer value IBRD and the
    @@ fractional value FBRD. NOTE: The contents of the
    @@ IBRD and FBRD registers are not updated until
    @@ transmission or reception of the current character
    @@ is complete.
    .global UART_set_baud
UART_set_baud:
    @@ set baud rate divisor using formula:
    @@ (3000000.0 / ( R0 ∗ 16 )) ASSUMING 3Mhz clock
    lsl r1,r0,#4 @ r1 <- desired baud ∗ 16
    ldr r0,=(3000000<<6)@ Load 3 MHz as a U(26,6) in r0
    bl divide @ divide clk freq by (baud∗16)
    asr r1,r0,#6 @ put integer divisor into r1
    and r0,r0,#0x3F @ put fractional divisor into r0
    ldr r2,=uartbase @ load base address of UART
    ldr r2,[r2] @ load base address of UART

    str r1,[r2,#UART_IBRD] @ set integer divisor
    str r0,[r2,#UART_FBRD] @ set fractional divisor
    mov pc,lr
