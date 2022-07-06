#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

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
#define SWITCH_4 3

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

void read_dht11_dat(int lcd);
void read_poten(int lcd);

// Fila declaracoes
#define SIZE 10

int RearUmidade = -1;
int FrontUmidade = -1;
float umid[SIZE];
int RearTemperatura = -1;
int FrontTemperatura = -1;
float temp[SIZE];
int RearLuminosidade = -1;
int FrontLuminosidade = -1;
float lum[SIZE];
int RearPressao = -1;
int FrontPressao = -1;
float pres[SIZE];

void dequeue(int id_array)
{
    switch (id_array)
    {
    case 1:
        // printf("Umidade DEQ\n");
        if (FrontUmidade == -1 || FrontUmidade > RearUmidade)
        {
            printf("Underflow \n");
            return;
        }
        else
        {
            // printf("Element deleted from the Queue UMID: %d\n", umid[FrontUmidade]);
            FrontUmidade = FrontUmidade + 1;
        }
        break;
    case 2:
        // printf("Temperatura DEQ\n");
        if (FrontTemperatura == -1 || FrontTemperatura > RearTemperatura)
        {
            printf("Underflow \n");
            return;
        }
        else
        {
            // printf("Element deleted from the Queue TEMP: %d\n", temp[FrontTemperatura]);
            FrontTemperatura = FrontTemperatura + 1;
        }
        break;
    case 3:
        // printf("Luminosidade DEQ\n");
        if (FrontLuminosidade == -1 || FrontLuminosidade > RearLuminosidade)
        {
            printf("Underflow \n");
            return;
        }
        else
        {
            // printf("Element deleted from the Queue TEMP: %d\n", lum[FrontLuminosidade]);
            FrontLuminosidade = FrontLuminosidade + 1;
        }
        break;
    case 4:
        // printf("Pressao DEQ\n");
        if (FrontPressao == -1 || FrontPressao > RearPressao)
        {
            printf("Underflow \n");
            return;
        }
        else
        {
            // printf("Element deleted from the Queue TEMP: %d\n", lum[FrontPressao]);
            FrontPressao = FrontPressao + 1;
        }
        break;
    default:
        break;
    }
}

void enqueue(float data, int id_array)
{
    switch (id_array)
    {
    case 1:
        // printf("Umidade ENQUEUE %.1f\n", data);
        if ((RearUmidade - FrontUmidade) == SIZE - 1)
        {

            // printf("Overflow \n");
            dequeue(id_array);
        }

        if (FrontUmidade == -1)
            FrontUmidade = 0;

        RearUmidade = RearUmidade + 1;
        umid[RearUmidade] = data;
        break;
    case 2:
        // printf("Temperatura ENQUEUE %.1f\n", data);
        if ((RearTemperatura - FrontTemperatura) == SIZE - 1)
        {

            // printf("Overflow \n");
            dequeue(id_array);
        }

        if (FrontTemperatura == -1)
            FrontTemperatura = 0;

        RearTemperatura = RearTemperatura + 1;
        temp[RearTemperatura] = data;
        break;
    case 3:
        // printf("Luminosidade ENQUEUE %.1f\n", data);
        if ((RearLuminosidade - FrontLuminosidade) == SIZE - 1)
        {

            // printf("Overflow \n");
            dequeue(id_array);
        }

        if (FrontLuminosidade == -1)
            FrontLuminosidade = 0;

        RearLuminosidade = RearLuminosidade + 1;
        lum[RearLuminosidade] = data;
        break;
    case 4:
        // printf("Pressao ENQUEUE %.1f\n", data);
        if ((RearPressao - FrontPressao) == SIZE - 1)
        {

            // printf("Overflow \n");
            dequeue(id_array);
        }

        if (FrontPressao == -1)
            FrontPressao = 0;

        RearPressao = RearPressao + 1;
        pres[RearPressao] = data;
        break;
    default:
        break;
    }
}

float umidade_atual = -1, temperatura_atual = -1, luminosidade_atual = -1, pressao_atual = -1;

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
void formata_json(float arr[], char tipo_historico[], int ultima_func)
{
    // printf("%s\n", tipo_historico);
    char aux1[1200] = "";
    int i = 0;
    char aux2[14];
    /* Utilizando os indices da umidade, pois é a primeira a ser lida/alterada */
    for (i = FrontUmidade; i <= RearUmidade; i++)
    {
        if (i < RearUmidade)
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

    pinMode(BUTTON_1, INPUT);            // configura pino como entrada
    pullUpDnControl(BUTTON_1, PUD_DOWN); // configura resistor pull-up no pino
    pinMode(BUTTON_2, INPUT);            // configura pino como entrada
    pullUpDnControl(BUTTON_2, PUD_DOWN); // configura resistor pull-up no pino
    pinMode(BUTTON_3, INPUT);            // configura pino como entrada
    pullUpDnControl(BUTTON_3, PUD_DOWN); // configura resistor pull-up no pino

    pinMode(SWITCH_1, INPUT); // configura pino como entrada
    pinMode(SWITCH_2, INPUT); // configura pino como entrada
    pinMode(SWITCH_3, INPUT); // configura pino como entrada
    pinMode(SWITCH_4, INPUT); // configura pino como entrada

    int thread = piThreadCreate(contar_interv);
    /*
        Inicializar ADS
    */
    if (openI2CBus("/dev/i2c-1") == -1)
    {
        return EXIT_FAILURE;
    }

    // setI2CSlave(0x48);

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
    /* Arrays auxiliares para salvar o conteudo atual dos arrays das medicoes e exibir o historico no lcd */
    float umidade_historico[SIZE];
    float temperatura_historico[SIZE];
    float luminosidade_historico[SIZE];
    float pressao_historico[SIZE];
    /* variaveis para salvar os indices de frente e fundo dos arrays das medicoes */
    int front_historico = -1, rear_historico = -1;

    /* variavel utilizada para escolher o intervalo na IHM  */
    int aux_intervalo = 2;
    int switch_2 = 0;
    char medicoes_atuais[111];
    char data_atual[19];

    time_t t;
    struct tm tm;
    /* Thread principal para o funcionamento do sistema, exibicao das medicoes, menus */
    while (1)
    {
        if (flag == 1)
        {
            flag = 0;
            setI2CSlave(0x48);

            read_dht11_dat(lcd);
            read_poten(lcd);
            printf("\nMEDICOES ATUAIS U:%.1f T:%.1f L:%.1f P:%.1f\n\n", umidade_atual, temperatura_atual, luminosidade_atual, pressao_atual);
            enqueue(umidade_atual, 1);
            enqueue(temperatura_atual, 2);
            enqueue(luminosidade_atual, 3);
            enqueue(pressao_atual, 4);
            /* Formatar data atual */
            t = time(NULL);
            tm = *localtime(&t);
            sprintf(data_atual, "%02d-%02d-%d %02d:%02d:%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

            /* formata a string com as medicoes atuais */
            sprintf(medicoes_atuais, "{\"umidade\":\"%.1f\",\"temperatura\":\"%.1f\",\"luminosidade\":\"%.1f\",\"pressao\":\"%.1f\",\"datahora\":\"%s\"}", umidade_atual, temperatura_atual, luminosidade_atual, pressao_atual, data_atual);
            mosquitto_publish(mosq, NULL, "teste/t2/medidas", strlen(medicoes_atuais), medicoes_atuais, 0, false);

            /*
            formata_json(umid, historico, 0);
            formata_json(temp, temperatura, 0);
            formata_json(lum, lumino, 0);
            formata_json(pres, pressao, 1);
            strcat(historico, temperatura);
            strcat(historico, lumino);
            strcat(historico, pressao);
            mosquitto_publish(mosq, NULL, "test/t1", strlen(historico), historico, 0, false);
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
                front_historico = FrontPressao;
                rear_historico = RearPressao;

                printf("f_hist %i\tr_hist %i\t%i\n", front_historico, rear_historico, (rear_historico - front_historico));
                int j = 0;
                /* percorre os arrays para salvar nos auxiliares de exibicao do historico no lcd */
                printf("umid\ttemp\tlum\tpres\n");
                for (int i = front_historico; i <= rear_historico; i++)
                {
                    umidade_historico[j] = umid[i];
                    temperatura_historico[j] = temp[i];
                    luminosidade_historico[j] = lum[i];
                    pressao_historico[j] = pres[i];
                    printf("%.1f\t", umidade_historico[i]);
                    printf("%.1f\t", temperatura_historico[i]);
                    printf("%.1f\t", luminosidade_historico[i]);
                    printf("%.1f\t\n", pressao_historico[i]);

                    j++;
                }
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
                    index_display = -1;
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
                        mosquitto_publish(mosq, NULL, "teste/t2/intervalo/new", strlen(str_intervalo), str_intervalo, 0, false);
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
                if (digitalRead(SWITCH_4) == LOW)
                {
                    delay(50);
                    while (digitalRead(BUTTON_3) == LOW)
                        ; // aguarda enquato chave ainda esta pressionada
                    delay(50);
                    /*
                        se o indice do display for igual a diferenca do fundo do array
                        e da frente, se estiver cheio (tamanho 10) index_display = 9
                        entao zera o indice
                    */

                    if (index_display == (rear_historico - front_historico))
                    {
                        index_display = 0;
                    }
                    else
                    {
                        index_display++;
                    }

                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "U:%.1f%% T:%.1fC", umidade_historico[index_display], temperatura_historico[index_display]);

                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "%i L:%.1fV P:%.1fV", (index_display + 1), luminosidade_historico[index_display], pressao_historico[index_display]);
                }
                else
                { // decrementa o indice
                    delay(50);
                    while (digitalRead(BUTTON_3) == LOW)
                        ; // aguarda enquato chave ainda esta pressionada
                    delay(50);
                    /*
                        verifica se for menor que zero, no primeiro loop o indice
                        e -1, para exibir sempre a posicao 0 do array
                    */
                    if (index_display < 0)
                    {
                        index_display = 0;
                    }
                    else if (index_display == 0)
                    {
                        // index_display = 9;
                        /*
                            indice do display sera a diferenca do fundo do array
                            e frente, se estiver cheio (tamanho 10) index_display = 9
                         */
                        index_display = (rear_historico - front_historico);
                    }
                    else
                    {
                        index_display--;
                    }

                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "U:%.1f%% T:%.1fC", umidade_historico[index_display], temperatura_historico[index_display]);

                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "%i L:%.1fV P:%.1fV", (index_display + 1), luminosidade_historico[index_display], pressao_historico[index_display]);
                }
                confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
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

void read_poten(int lcd)
{
    luminosidade_atual = readVoltage(1);
    pressao_atual = readVoltage(0);

    if (display == 0)
    {
        lcdPosition(lcd, 0, 1);
        lcdPrintf(lcd, "L:%.1fV P:%.1fV   ", luminosidade_atual, pressao_atual);
    }
}

void read_dht11_dat(int lcd)
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

        umidade_atual = dht11_dat[0] + ((float)dht11_dat[1] / 10);
        temperatura_atual = dht11_dat[2] + ((float)dht11_dat[3] / 10);
        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:%.1f%% T:%.1fC", umidade_atual, temperatura_atual);
        }
    }
    else
    {

        /* Display = 0 Exibir medicoes no lcd | Dentro das funções de leitura */

        umidade_atual = -1.1;
        temperatura_atual = -1.1;
        /*
            Add -11.1 flag to show that the sensor didnt make a correct reading
        */
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:-1.1   T:-1.1  ");
        }
    }
}
