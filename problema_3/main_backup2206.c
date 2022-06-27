#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "ads1115_rpi.h"

// USE WIRINGPI PIN NUMBERS
#define LCD_RS 6  // Register select pin
#define LCD_E 31  // Enable Pin
#define LCD_D4 26 // Data pin 4
#define LCD_D5 27 // Data pin 5
#define LCD_D6 28 // Data pin 6
#define LCD_D7 29 // Data pin 7
#define MAXTIMINGS 85
#define DHTPIN 1 // gpio 18

void read_dht11_dat(float arrUmid[], int *FrontUmid, int *RearUmid, int FrUmid, int ReUmid, float arrTemp[], int *FrontTemp, int *RearTemp, int FrTemp, int ReTemp);
void read_poten(float arrLum[], int *FrontLum, int *RearLum, int FrLum, int ReLum, float arrPres[], int *FrontPres, int *RearPres, int FrPres, int RePres);

void display_hist(int lcd);
void display_interv(int lcd);
void display_interv_conf(int lcd);
int dht11_dat[5] = {0, 0, 0, 0, 0};

int index_display = 0, confirmar = 0, intervalo = 2;

// Q declarations
#define SIZE 10
void enqueue(float arr[], float data, int *Front, int *Rear, int Fr, int Re);
void dequeue(float arr[], int *Front, int *Rear, int Fr, int Re);
void show(float arr[], int Front, int Rear);
float temp[SIZE];
float umid[SIZE];
float lum[SIZE];
float pres[SIZE];

// End Q
/* Variaveis para os vetores de medicoes */
int RearUmid = -1, FrontUmid = -1;
int RearTemp = -1, FrontTemp = -1;
int RearLum = -1, FrontLum = -1;
int RearPres = -1, FrontPres = -1;

int main()
{

    /* Inicializar LCD */
    int lcd;
    wiringPiSetup();
    lcd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);

    pinMode(21, INPUT);            // configura pino como entrada
    pullUpDnControl(21, PUD_DOWN); // configura resistor pull-up no pino
    pinMode(24, INPUT);            // configura pino como entrada
    pullUpDnControl(24, PUD_DOWN); // configura resistor pull-up no pino
    pinMode(25, INPUT);            // configura pino como entrada
    pullUpDnControl(25, PUD_DOWN); // configura resistor pull-up no pino

    /*
        Inicializar ADS
    */
    if (openI2CBus("/dev/i2c-1") == -1)
    {
        return EXIT_FAILURE;
    }

    setI2CSlave(0x48);

    lcdClear(lcd);
    lcdPosition(lcd, 0, 0);
    lcdPrintf(lcd, "Problema 03");
    lcdPosition(lcd, 0, 1);
    lcdPrintf(lcd, "IoT - MQTT");
    /* Leitura dos sensores */

    // read_dht11_dat(umid, &FrontUmid, &RearUmid, FrontUmid, RearUmid, temp, &FrontTemp, &RearTemp, FrontTemp, RearTemp);
    // read_poten(lum, &FrontLum, &RearLum, FrontLum, RearLum, pres, &FrontPres, &RearPres, FrontPres, RearPres);

    while (1)
    {
        /* Primeiro botao */
        if (digitalRead(21) == LOW)
        {
            /* mostrar menu para mudar intervalo */
            /* switch 1 */
            if (digitalRead(7) == LOW)
            {
                /* incrementar intervalo */
                delay(50);
                while (digitalRead(21) == LOW)
                    ; // aguarda enquato chave ainda esta pressionada
                delay(50);
                intervalo++;
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Intervalo: %i", intervalo);
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "Incrementar");
            }
            else
            {
                delay(50);
                while (digitalRead(21) == LOW)
                    ; // aguarda enquato chave ainda esta pressionada
                delay(50);
                /* decrementar intervalo */
                if (intervalo == 2)
                {
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "Intervalo: %i", intervalo);
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "< Interv Possiv");
                }
                else
                {
                    intervalo--;
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "Intervalo: %i", intervalo);
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "Decrementar");
                }
            }
            confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
        }
        /* Segundo botao */
        else if (digitalRead(24) == LOW)
        {
            /* Confirmar e enviar intervalo de medicao */
            confirmar++;
            delay(50);
            while (digitalRead(24) == LOW)
                ; // aguarda enquato chave ainda esta pressionada
            delay(50);

            if (confirmar >= 2)
            {
                /* enviar intervalo via mqtt */
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Intervalo %i s", intervalo);
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "Enviado");
                confirmar = 0;
            }
            else
            {
                confirmar++;
                /* Confirmar envio de intervalo */
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Intervalo: %i s", intervalo);
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "Confirmar?");
            }
        }
        /* Ultimo botao */
        else if (digitalRead(25) == LOW)
        {
            /* switch for 1, decrementa o indice */
            if (digitalRead(3) == LOW)
            {
                delay(50);
                while (digitalRead(25) == LOW)
                    ; // aguarda enquato chave ainda esta pressionada
                delay(50);
                if (index_display == 9)
                {
                    index_display = 0;
                }
                else
                {
                    index_display++;
                }

                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "%i Temp:%.1f", (index_display + 1), temp[index_display]);

                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "  Umid:%.1f", umid[index_display]);

                delay(1000);
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "  Luminos:%.1f", lum[index_display]);

                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "  Pressao:%.1f", pres[index_display]);
            }
            else
            { // incrementa o indice
                delay(50);
                while (digitalRead(25) == LOW)
                    ; // aguarda enquato chave ainda esta pressionada
                delay(50);
                if (index_display == 0)
                {
                    index_display = 9;
                }
                else
                {
                    index_display--;
                }

                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "%i Temp:%.1f", (index_display + 1), temp[index_display]);

                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "  Umid:%.1f", umid[index_display]);

                delay(1000);
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "  Luminos:%.1f", lum[index_display]);

                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "  Pressao:%.1f", pres[index_display]);
            }
            confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
        }
    }
    return 0;
}

void read_poten(float arrLum[], int *FrontLum, int *RearLum, int FrLum, int ReLum, float arrPres[], int *FrontPres, int *RearPres, int FrPres, int RePres)
{
    enqueue(arrLum, readVoltage(1), FrontLum, RearLum, FrLum, ReLum);
    enqueue(arrPres, readVoltage(0), FrontPres, RearPres, FrPres, RePres);
}

void read_dht11_dat(float arrUmid[], int *FrontUmid, int *RearUmid, int FrUmid, int ReUmid, float arrTemp[], int *FrontTemp, int *RearTemp, int FrTemp, int ReTemp)
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    float f;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = digitalRead(DHTPIN);

        if (counter == 255)
            break;

        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_dat[j / 8] <<= 1;
            if (counter > 16)
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) &&
        (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        printf("\nHumidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n\n",
               dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f);

        float umidade = dht11_dat[0] + ((float)dht11_dat[1] / 10);
        float temperatura = dht11_dat[2] + ((float)dht11_dat[3] / 10);

        enqueue(arrUmid, umidade, FrontUmid, RearUmid, FrUmid, ReUmid);
        enqueue(arrTemp, temperatura, FrontTemp, RearTemp, FrTemp, ReTemp);
    }
    else
    {
        printf("Data not good, skip\n");
        // Add -1.0 flag to show that the sensor didnt make a correct reading
        enqueue(arrUmid, -1.0, FrontUmid, RearUmid, FrUmid, ReUmid);
        enqueue(arrTemp, -1.0, FrontTemp, RearTemp, FrTemp, ReTemp);
    }
}

void dequeue(float arr[], int *Front, int *Rear, int Fr, int Re)
{
    *Front = *Front + 1;
    return;
}

void enqueue(float arr[], float data, int *Front, int *Rear, int Fr, int Re)
{
    if ((Re - Fr) == SIZE - 1)
    {
        dequeue(temp, Front, Rear, Fr, Re);
        // return;
    }

    if (Fr == -1)
        *Front = 0;

    *Rear = *Rear + 1;
    arr[Re + 1] = data;
}

void show(float arr[], int Front, int Rear)
{
    if (Front == -1)
        printf("Empty Queue \n");
    else
    {
        printf("Queue: \n");
        for (int i = Front; i <= Rear; i++)
            printf("%.1f ", arr[i]);
        printf("\n");
    }
}
