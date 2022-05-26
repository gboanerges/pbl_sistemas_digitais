main: uart.c uartLib.a
	gcc -o main uart.c uartLib.a
uartLib.a: uartConfig.o uartSend.o uartGet.o
	ar -cvq uartLib.a uartConfig.o uartSend.o uartGet.o
uartConfig.o: uartConfig.s
	as -o uartConfig.o uartConfig.s
uartSend.o: uartSend.s
	as -o uartSend.o uartSend.s
uartGet.o: uartGet.s
	as -o uartGet.o uartGet.s
clean:
	rm *.o *.a main
