#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
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

#define BUTTON_1 21
#define BUTTON_2 24
#define BUTTON_3 25
#define SWITCH_1 7
#define SWITCH_2 0
#define SWITCH_3 2
//#define SWITCH_4 3

int dht11_dat[5] = {0, 0, 0, 0, 0};

int index_display = -1, confirmar = 0, display = 0, intervalo = 2;

/* Interruption declarations */
unsigned int flag;
#pragma interrupt_handler ISR
void ISR(void)
{
    flag = 1;
}
/* END Interrption declarations */

// A linked list (LL) node to store a queue entry
struct QNode
{
    float key;
    struct QNode *next;
};

// The queue, front stores the front node of LL and rear stores the
// last node of LL
struct Queue
{
    struct QNode *front, *rear;
};

struct QNode *newNode(float k);
struct Queue *createQueue();
void deQueue(struct Queue *q);
void enQueue(struct Queue *q, int k, int size);
void printQueue(struct Queue *q);
void read_dht11_dat(int lcd, struct Queue *umid, int sizeUmid, struct Queue *temp, int sizeTemp);
void read_poten(int lcd, struct Queue *lumi, int sizeLumi, struct Queue *pressao, int sizePressao);

/* Variaveis para formatar em JSON os vetores de medicoes */
char historico[420] = "{\"historico_umidade\":[";
char temperatura[110] = "\"historico_temperatura\":[";
char lumino[110] = "\"historico_luminosidade\":[";
char pressao[110] = "\"historico_pressao\":[";

int conexao = 0;

/*
    Percorrer e formatar os arrays das medicoes, em float, para
    string no formato JSON
*/
void formata_json(struct Queue *q, char tipo_historico[], int sizeQueue, int ultima_func)
{
    char aux1[90] = "";
    struct Queue *aux = createQueue();
    aux->front = q->front;
    int i = 0;
    char aux2[14];
    while (aux->front->next != NULL)
    {
        aux->front = aux->front->next;
        if (i < sizeQueue)
        {
            sprintf(aux2, "\"%.1f\",", aux->front->key);
            strcat(aux1, aux2);
            i++;
        }
        else
        {
            /* Se for a ultima funcao de historico, nao coloca virgula no final*/
            if (ultima_func)
            {
                sprintf(aux2, "\"%.1f\"]}", aux->front->key);
                strcat(aux1, aux2);
            }
            else
            {
                sprintf(aux2, "\"%.1f\"],", aux->front->key);
                strcat(aux1, aux2);
            }
        }
    }
    /* Concatena a string formatada dos valores com o inicio da msg json */
    strcat(tipo_historico, aux1);
}

void send_intervalo(const char *msg_intervalo, int msg_tamanho, struct mosquitto *mosq)
{
    /* validacao do intervalo minimo na interface grafica */
    intervalo = atoi(msg_intervalo);
    printf("%s\n", msg_intervalo);
    mosquitto_publish(mosq, NULL, "teste/t2/intervalo/new", msg_tamanho, msg_intervalo, 0, true); // Envia uma mensagem com o intervalo, isso indica que o intervalo foi recebido por  que o intervalo foi alterado
}
/* Alterar topico para testar broker raspberry */
void send_resposta(struct mosquitto *mosq)
{
    mosquitto_publish(mosq, NULL, "teste/t2/ping/resposta", 1, "1", 0, false); // Envia uma mensagem para informar ao cliente remoto que está conectado (sistema online)
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("ID: %d\n", *(int *)obj);
    if (rc)
    {
        printf("Error with result code: %d\n", rc);
        exit(-1);
    }
    char *subs[68] = {"teste/t2/ping/pedido", "teste/t2/ping/resposta", "teste/t2/intervalo/send"};
    mosquitto_subscribe_multiple(mosq, NULL, 3, subs, 0, 0, NULL);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("New message with topic %s: %s\n", msg->topic, (char *)msg->payload);
    if (strcmp((char *)msg->topic, "teste/t2/intervalo/send") == 0)
    {
        printf("%s\n", (char *)msg->payload);
        send_intervalo((char *)msg->payload, msg->payloadlen, mosq);
    }
    else if (strcmp((char *)msg->topic, "teste/t2/ping/pedido") == 0)
    {
        send_resposta(mosq);
    }
    /* Testa a conexao com o BROKER */
    if (strcmp((char *)msg->topic, "teste/t2/ping/resposta") == 0)
    {
        /* altera variavel para representar que a conexao com o broker esta online */
        conexao = 1;
    }
}

PI_THREAD(contar_interv)
{
    printf("Thread Contar Intervalo.\n");
    time_t inicio, fim;
    double tempo_decorrido;

    while (1)
    {
        /* inicia a contagem */
        time(&inicio);

        printf("intervalo atual: %i\n", intervalo);
        do
        {
            /* para contagem */
            time(&fim);
            /* salva a diferença entre o tempo inicial e o de parada */
            tempo_decorrido = difftime(fim, inicio);
            /* verifica se o tempo passado é menor que o intervalo */
        } while (tempo_decorrido < intervalo);
        /* ao passar o tempo definido pelo intervalo, chama a interrupçao  */
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

    struct Queue *umid = createQueue();
    struct Queue *temp = createQueue();
    struct Queue *lumi = createQueue();
    struct Queue *pressao = createQueue();
    int sizeUmid = -1, sizeTemp = -1, sizeLum = -1, sizePres = -1;
    /* listas auxiliares para exibir historico na rasp */
    struct Queue *auxUmi = createQueue();
    struct Queue *auxTemp = createQueue();
    struct Queue *auxLum = createQueue();
    struct Queue *auxPres = createQueue();
    int sizeAux = -1;
    struct Queue *auxUmi2 = createQueue();
    struct Queue *auxTemp2 = createQueue();
    struct Queue *auxLum2 = createQueue();
    struct Queue *auxPres2 = createQueue();
    lcdClear(lcd);
    lcdPosition(lcd, 0, 0);
    lcdPrintf(lcd, "Problema 03");
    lcdPosition(lcd, 0, 1);
    lcdPrintf(lcd, "IoT - MQTT");
    /* Leitura dos sensores */

    int rc;

    struct mosquitto *mosq;

    mosquitto_lib_init();

    mosq = mosquitto_new("G03-IOT", true, NULL);
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

    /* Inicia loop para o subscriber do mosquitto ficar ativo */
    mosquitto_loop_start(mosq);
    /* Enviar o intervalo base logo a iniciar a execucao */
    mosquitto_publish(mosq, NULL, "teste/t2/intervalo/new", 1, "2", 0, false);

    int aux_intervalo = 2;
    int switch_2 = 0;
    /* Thread principal para o funcionamento do sistema, exibicao das medicoes, menus */
    while (1)
    {
        if (flag == 1)
        {
            flag = 0;
            read_dht11_dat(lcd, umid, sizeUmid, temp, sizeTemp);
            sizeUmid++;
            sizeTemp++;
            read_poten(lcd, lumi, sizeLum, pressao, sizePres);
            sizeLum++;
            sizePres++;
            /*
            formata_json(umid, historico, 0);
            formata_json(temp, temperatura, 0);
            formata_json(lum, lumino, 0);
            formata_json(pres, pressao, 1);
            strcat(historico, temperatura);
            strcat(historico, lumino);
            strcat(historico, pressao);
            mosquitto_publish(mosq, NULL, "test/t1", 80, historico, 0, false);
            */
        }
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */
        if (display == 0)
        {
            /*
                Apertar um dos 3 botoes para a acessar os menus
                Fica parado nessa exibiçao ate apertar novamente
                para interagir com os menus
            */
            if ((digitalRead(BUTTON_1) == LOW) || (digitalRead(BUTTON_2) == LOW) || (digitalRead(BUTTON_3) == LOW))
            {
                display = 1;
                delay(50);
                while ((digitalRead(BUTTON_1) == LOW) || (digitalRead(BUTTON_2) == LOW) || (digitalRead(BUTTON_3) == LOW))
                    ; // aguarda enquato uma das 3 chaves chave ainda esta pressionada
                delay(50);
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Menu IHM: 1:Intv");
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "2:ConfInt 3:Hist");

                /*
                    Salva as listas em listas auxiliares para exibir historico
                */
                auxUmi->front = umid->front;
                auxTemp->front = temp->front;
                auxLum->front = lumi->front;
                auxPres->front = pressao->front;
                /* listas para guardar a primeira posicao */
                auxUmi2->front = umid->front;
                auxTemp2->front = temp->front;
                auxLum2->front = lumi->front;
                auxPres2->front = pressao->front;
                if (sizeUmid >= 10)
                {
                    sizeAux = 10;
                }
                else
                {
                    sizeAux = sizeUmid;
                }
                printQueue(temp);
                printf("size aux %i\n", sizeAux);
            }
        }
        else /* Display = 1 Exibir menus, INC/DEC Intervalo, CONF Intervalo e HISTORICO */
        {
            /* Primeiro botao */
            if (digitalRead(BUTTON_1) == LOW)
            {
                /* mostrar menu para mudar intervalo */
                /* switch 1 */
                if (digitalRead(SWITCH_1) == LOW)
                {
                    /* incrementar intervalo */
                    delay(50);
                    while (digitalRead(BUTTON_1) == LOW)
                        ; // aguarda enquato chave ainda esta pressionada
                    delay(50);
                    aux_intervalo++;
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "Intervalo: %i", aux_intervalo);
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "Incrementar");
                }
                else
                {
                    delay(50);
                    while (digitalRead(BUTTON_1) == LOW)
                        ; // aguarda enquato chave ainda esta pressionada
                    delay(50);
                    /* decrementar intervalo */
                    if (aux_intervalo == 2)
                    {
                        lcdClear(lcd);
                        lcdPosition(lcd, 0, 0);
                        lcdPrintf(lcd, "Intervalo: %i", aux_intervalo);
                        lcdPosition(lcd, 0, 1);
                        lcdPrintf(lcd, "< Interv Possiv");
                    }
                    else
                    {
                        aux_intervalo--;
                        lcdClear(lcd);
                        lcdPosition(lcd, 0, 0);
                        lcdPrintf(lcd, "Intervalo: %i", aux_intervalo);
                        lcdPosition(lcd, 0, 1);
                        lcdPrintf(lcd, "Decrementar");
                    }
                }
                confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
            }
            /* Segundo botao */
            else if (digitalRead(BUTTON_2) == LOW)
            {
                /* Confirmar e enviar intervalo de medicao */
                confirmar++;
                delay(50);
                while (digitalRead(BUTTON_2) == LOW)
                    ; // aguarda enquato chave ainda esta pressionada
                delay(50);

                /* switch 3 = 1 */
                /* Alternar para visualização das medicoes dos sensores */
                if (digitalRead(SWITCH_3) == LOW)
                {
                    display = 0;
                    /* zera variavel de controle para envio do intervalo */
                    confirmar = 0;
                    switch_2 = 0;
                }
                else
                {
                    /* switch 3 = 0 */
                    /* Confirmar >=2 envia o intervalo */
                    if (confirmar >= 2)
                    {
                        /* Formata como string, para passar como parametro no publish */
                        char str_intervalo[2];
                        sprintf(str_intervalo, "%i", aux_intervalo);
                        /* enviar intervalo via mqtt */
                        mosquitto_publish(mosq, NULL, "teste/t2/intervalo/new", 2, str_intervalo, 0, false);
                        /* altera o intervalo */
                        intervalo = aux_intervalo;
                        lcdClear(lcd);
                        lcdPosition(lcd, 0, 0);
                        lcdPrintf(lcd, "Intervalo %i s", aux_intervalo);
                        lcdPosition(lcd, 0, 1);
                        lcdPrintf(lcd, "Enviado");
                        confirmar = 0;
                    }
                    else
                    {
                        confirmar++;
                        /* mostrar menu para confirmar intervalo */
                        lcdClear(lcd);
                        lcdPosition(lcd, 0, 0);
                        lcdPrintf(lcd, "Intervalo: %i s", aux_intervalo);
                        lcdPosition(lcd, 0, 1);
                        lcdPrintf(lcd, "Confirmar?");
                    }
                }
            }
            /* Ultimo botao */
            else if (digitalRead(BUTTON_3) == LOW)
            {
                /* switch 4 for 1, incrementa o indice */
                // if (digitalRead(SWITCH_4) == LOW)
                // {
                delay(50);
                while (digitalRead(BUTTON_3) == LOW)
                    ; // aguarda enquato chave ainda esta pressionada
                delay(50);
                /*
                    Verifica o auxiliar de Umidade porque
                    e o primeiro a ser inserido
                */
                printf("size %i\n", sizeAux);

                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "%i U:%.1f T:%.1f", (index_display + 1), auxUmi->front->key, auxTemp->front->key);

                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "  L:%.1f P:%.1f", auxLum->front->key, auxPres->front->key);
                confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
                if ((index_display < sizeAux) && auxUmi->front != NULL)
                {
                    auxUmi->front = auxUmi->front->next;
                    auxTemp->front = auxTemp->front->next;
                    auxLum->front = auxLum->front->next;
                    auxPres->front = auxPres->front->next;
                    index_display++;

                    // }
                }
                /* chegou no fim da lista, mudar ponteiro para o inicio */
                else
                {
                    index_display = -1;
                    auxUmi->front = auxUmi2->front;
                    auxTemp->front = auxTemp2->front;
                    auxLum->front = auxLum2->front;
                    auxPres->front = auxPres2->front;
                }
            }
            /* Switch 2 */
            else if ((digitalRead(SWITCH_2) == LOW) && switch_2 == 0)
            {
                switch_2 = 1;
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Testando a");
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "Conexao BROKER");
                mosquitto_publish(mosq, NULL, "teste/t2/ping/pedido", 1, "1", 0, false);
                /* Contar 1 segundo */
                time_t inicio, fim;
                double tempo_total;
                /* inicia a contagem */
                time(&inicio);
                do
                {
                    /* para contagem */
                    time(&fim);
                    /* salva a diferença entre o tempo inicial e o de parada */
                    tempo_total = difftime(fim, inicio);
                    /* verifica se o tempo passado é menor que o intervalo */
                } while (tempo_total < 1);
                /* Se a conexao for 1 exibe msg que esta online */
                if (conexao == 1)
                {
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "Status do Broker");
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "     ONLINE     ");
                    /* atribuir valor falso para evitar que o boolean fique sempre TRUE */
                    conexao = 0;
                }
                /* Senao exibe msg que esta offline */
                else
                {
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "Status do Broker");
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "     OFFLINE    ");
                }
            }
        }
    }
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}

void read_poten(int lcd, struct Queue *lumi, int sizeLumi, struct Queue *pressao, int sizePressao)
{
    float read_pot_luminosidade = readVoltage(1);
    float read_pot_pressao = readVoltage(0);
    enQueue(lumi, read_pot_luminosidade, sizeLumi);
    enQueue(pressao, read_pot_pressao, sizePressao);
    if (display == 0)
    {
        lcdPosition(lcd, 0, 1);
        lcdPrintf(lcd, "L:%.1fV P:%.1fV", read_pot_luminosidade, read_pot_pressao);
    }
}

void read_dht11_dat(int lcd, struct Queue *umid, int sizeUmid, struct Queue *temp, int sizeTemp)
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

        // printf("\nHumidity = %d.%d %% Temperature = %d.%d C\n\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);

        float umidade = dht11_dat[0] + ((float)dht11_dat[1] / 10);
        float temperatura = dht11_dat[2] + ((float)dht11_dat[3] / 10);
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:%.1f%% T:%.1fC", umidade, temperatura);
        }

        enQueue(umid, umidade, sizeUmid);
        enQueue(temp, temperatura, sizeTemp);
    }
    else
    {
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */

        // printf("\nData not good, skip\n\n");
        //  Add -11.1 flag to show that the sensor didnt make a correct reading
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:-11,1 T:-11,1 ");
        }
        enQueue(umid, -11.1, sizeUmid);
        enQueue(temp, -11.1, sizeTemp);
    }
}

// A utility function to create a new linked list node.
struct QNode *newNode(float k)
{
    struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
    temp->key = k;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty queue
struct Queue *createQueue()
{
    struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

// Function to remove a key from given queue q
void deQueue(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;

    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    free(temp);
}

// The function to add a key k to q
void enQueue(struct Queue *q, int k, int size)
{
    // Create a new LL node
    struct QNode *temp = newNode(k);

    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL)
    {
        q->front = q->rear = temp;
        return;
    }

    if (size >= 10)
    {
        deQueue(q);
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}

void printQueue(struct Queue *q)
{
    printf("print\n");
    struct Queue *aux = createQueue();
    aux->front = q->front;
    // printf("%d \n", q->front->key);
    while (aux->front->next != NULL)
    {
        aux->front = aux->front->next;
        printf("%.1f \n", aux->front->key);
    }
    // printf("%d \n", aux->front->key);
}