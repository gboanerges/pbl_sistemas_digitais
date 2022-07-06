all: main
	echo "Build finish!"

main: main.c
	gcc -o main main.c ads1115_rpi.c -lwiringPi -lwiringPiDev -lmosquitto

clean:
	rm main