#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "ads1115_rpi.h"
#include <mosquitto.h>
// USE WIRINGPI PIN NUMBERS
#define LCD_RS 6  // Register select pin
#define LCD_E 31  // Enable Pin
#define LCD_D4 26 // Data pin 4
#define LCD_D5 27 // Data pin 5
#define LCD_D6 28 // Data pin 6
#define LCD_D7 29 // Data pin 7
#define MAXTIMINGS 85
#define DHTPIN 1 // gpio 18

void read_dht11_dat(int lcd, float arrUmid[], int *FrontUmid, int *RearUmid, int FrUmid, int ReUmid, float arrTemp[], int *FrontTemp, int *RearTemp, int FrTemp, int ReTemp);
void read_poten(int lcd, float arrLum[], int *FrontLum, int *RearLum, int FrLum, int ReLum, float arrPres[], int *FrontPres, int *RearPres, int FrPres, int RePres);

int dht11_dat[5] = {0, 0, 0, 0, 0};

int index_display = 0, confirmar = 0, display = 0, intervalo = 2;

/* Interruption declarations */
unsigned int flag;
#pragma interrupt_handler ISR
void ISR(void)
{
    flag = 1;
}
/* END Interrption declarations */

// Fila declaracoes
#define SIZE 10
void enqueue(float arr[], float data, int *Front, int *Rear, int Fr, int Re);
void dequeue(float arr[], int *Front, int *Rear, int Fr, int Re);
void show(float arr[], int Front, int Rear);
float temp[SIZE];
float umid[SIZE];
float lum[SIZE];
float pres[SIZE];
// End

/* Variaveis para os vetores de medicoes */
int RearUmid = -1, FrontUmid = -1;
int RearTemp = -1, FrontTemp = -1;
int RearLum = -1, FrontLum = -1;
int RearPres = -1, FrontPres = -1;

/* Variaveis para formatar em JSON os vetores de medicoes */
char historico[400] = "{\"historico_umidade\":[";
char temperatura[128] = "\"historico_temperatura\":[";
char lumino[128] = "\"historico_luminosidade\":[";
char pressao[128] = "\"historico_pressao\":[";

/*
    Percorrer e formatar os arrays das medicoes, em float, para
    string no formato JSON
*/
void formata_json(float arr[], char tipo_historico[], int ultima_func)
{
    // printf("%s\n", tipo_historico);
    char aux1[1200] = "";
    int i = 0;
    char aux2[14];
    /* Utilizando os indices da umidade, pois é a primeira a ser lida/alterada */
    for (i = FrontUmid; i <= RearUmid; i++)
    {

        if (i < 9)
        {
            sprintf(aux2, "\"%.1f\",", arr[i]);
            strcat(aux1, aux2);
        }
        else
        {
            /* Se for a ultima funcao de historico, nao coloca virgula no final*/
            if (ultima_func)
            {
                sprintf(aux2, "\"%.1f\"]}", arr[i]);
                strcat(aux1, aux2);
            }
            else
            {
                sprintf(aux2, "\"%.1f\"],", arr[i]);
                strcat(aux1, aux2);
            }
        }
    }
    /* Concatena a string formatada dos valores com o inicio da msg json */
    strcat(tipo_historico, aux1);
}

// void mudar_intervalo(char *msg)
// {
//     int aux = 0;
//     aux = atoi(msg);
//     printf("Float value %i\n", aux);
// }

// void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
// {
//     printf("New message with topic %s: %s\n", msg->topic, (char *)msg->payload);
//     if (msg->topic == "intervalo")
//     {
//         /* checar formato da msg? */
//         mudar_intervalo((char *)msg->payload);
//     }
// }

void send_intervalo(const char *msg_intervalo, int msg_tamanho, struct mosquitto *mosq)
{
    intervalo = atoi(msg_intervalo);
    printf("%s", msg_intervalo);
    mosquitto_publish(mosq, NULL, "teste/t2/intervalo/new", msg_tamanho, msg_intervalo, 0, true); // Envia uma mensagem com o intervalo, isso indica que o intervalo foi recebido por  que o intervalo foi alterado
}

void send_resposta(struct mosquitto *mosq)
{
    mosquitto_publish(mosq, NULL, "teste/t2/ping/resposta", 2, "1", 0, false); // Envia uma mensagem para informar ao cliente remoto que está conectado (sistema online)
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("ID: %d\n", *(int *)obj);
    if (rc)
    {
        printf("Error with result code: %d\n", rc);
        exit(-1);
    }
    char *subs[45] = {"teste/t2/ping/pedido", "teste/t2/intervalo/send"};
    mosquitto_subscribe_multiple(mosq, NULL, 2, subs, 0, 0, NULL);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("New message with topic %s: %s\n", msg->topic, (char *)msg->payload);
    if (strcmp((char *)msg->topic, "teste/t2/intervalo/send") == 0)
    {
        printf("%s", (char *)msg->payload);
        send_intervalo((char *)msg->payload, msg->payloadlen, mosq);
    }
    else if (strcmp((char *)msg->topic, "teste/t2/ping/pedido") == 0)
    {
        send_resposta(mosq);
    }
}

PI_THREAD(contar_interv)
{
    printf("Thread Contar Intervalo.\n");

    while (1)
    {
        delay(intervalo * 1000);
        ISR();
    }
    return (0);
}

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

    int t = piThreadCreate(contar_interv);
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

    int rc;

    struct mosquitto *mosq;

    mosquitto_lib_init();

    mosq = mosquitto_new("publisher-test", true, NULL);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    rc = mosquitto_username_pw_set(mosq, "aluno", "aluno*123");
    rc = mosquitto_connect(mosq, "10.0.0.101", 1883, 5);
    if (rc != 0)
    {
        printf("Error pass");
        mosquitto_destroy(mosq);
        return -1;
    }

    /* Enviar o intervalo base logo a iniciar a execucao */
    mosquitto_publish(mosq, NULL, "test/intervalo", 400, intervalo, 0, false);
    /* Inicia loop para o subscriber do mosquitto ficar ativo */
    mosquitto_loop_start(mosq);
    /* Thread principal para exibicao das medicoes, menus */
    while (1)
    {
        if (flag == 1)
        {
            flag = 0;
            read_dht11_dat(lcd, umid, &FrontUmid, &RearUmid, FrontUmid, RearUmid, temp, &FrontTemp, &RearTemp, FrontTemp, RearTemp);
            read_poten(lcd, lum, &FrontLum, &RearLum, FrontLum, RearLum, pres, &FrontPres, &RearPres, FrontPres, RearPres);

            formata_json(umid, historico, 0);
            formata_json(temp, temperatura, 0);
            formata_json(lum, lumino, 0);
            formata_json(pres, pressao, 1);
            strcat(historico, temperatura);
            strcat(historico, lumino);
            strcat(historico, pressao);
            mosquitto_publish(mosq, NULL, "test/t1", 80, historico, 0, false);
        }
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */
        if (display == 0)
        {
            /*
                Apertar um dos 3 botoes para a acessar os menus

                Fica parado nessa exibiçao ate apertar novamente
                para interagir com os menus
            */
            if ((digitalRead(21) == LOW) || (digitalRead(24) == LOW) || (digitalRead(25) == LOW))
            {
                display = 1;
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Menu IHM: 1:Intv");
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "2:ConfInt 3:Hist");
            }
        }
        else /* Display = 1 Exibir menus, INC/DEC Intervalo, CONF Intervalo e HISTORICO */
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

                /* switch 3 = 1 */
                /* Alternar para visualização das medicoes dos sensores */
                if (digitalRead(2) == LOW)
                {
                    display = 0;
                }
                else
                {
                    /* switch 3 = 0 */
                    /* mostrar menu para confirmar intervalo */
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
            }
            /* Ultimo botao */
            else if (digitalRead(25) == LOW)
            {
                /* switch 4 for 1, incrementa o indice */
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
                    lcdPrintf(lcd, "%i U:%.1f T:%.1f", (index_display + 1), temp[index_display]);

                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "  L:%.1f P:%.1f", pres[index_display]);
                }
                else
                { // decrementa o indice
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
                    lcdPrintf(lcd, "%i U:%.1f T:%.1f", (index_display + 1), temp[index_display]);

                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "  L:%.1f P:%.1f", pres[index_display]);
                }
                confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
            }
        }
    }
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}

void read_poten(int lcd, float arrLum[], int *FrontLum, int *RearLum, int FrLum, int ReLum, float arrPres[], int *FrontPres, int *RearPres, int FrPres, int RePres)
{
    enqueue(arrLum, readVoltage(1), FrontLum, RearLum, FrLum, ReLum);
    enqueue(arrPres, readVoltage(0), FrontPres, RearPres, FrPres, RePres);
    if (display == 0)
    {

        lcdPosition(lcd, 0, 1);
        lcdPrintf(lcd, "L:%.1f%% P:%.1f", readVoltage(1), readVoltage(0));
    }
}

void read_dht11_dat(int lcd, float arrUmid[], int *FrontUmid, int *RearUmid, int FrUmid, int ReUmid, float arrTemp[], int *FrontTemp, int *RearTemp, int FrTemp, int ReTemp)
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
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:%.1f%% T:%.1f", umidade, temperatura);
        }

        enqueue(arrUmid, umidade, FrontUmid, RearUmid, FrUmid, ReUmid);
        enqueue(arrTemp, temperatura, FrontTemp, RearTemp, FrTemp, ReTemp);
    }
    else
    {
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */

        printf("Data not good, skip\n");
        // Add -1.0 flag to show that the sensor didnt make a correct reading
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:-1 T:-1", umidade, temperatura);
        }
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
