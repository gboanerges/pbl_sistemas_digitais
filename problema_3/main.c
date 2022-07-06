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

/*
    Declaracoes das filas

    Define constante de tamanho para alocacao de memoria
    das filas das medicoes de cada sensor
 */
#define SIZE_ARRAY 1000
/* Constante de tamanhao do array para historico na raspberry */
#define SIZE 10
/*
    Variaveis das filas:
    2 Inteiros, Rear e Front, e 1 ponteiro, necessario
    para o malloc, para cada tipo de sensor
 */
int RearUmidade = -1;
int FrontUmidade = -1;
float *umid;
int RearTemperatura = -1;
int FrontTemperatura = -1;
float *temp;
int RearLuminosidade = -1;
int FrontLuminosidade = -1;
float *lum;
int RearPressao = -1;
int FrontPressao = -1;
float *pres;

/*
    Funcao para retirar um elemento da fila. Altera o indice Front para Front+1
    Parametro id_array:
        > caso 1: Umidade, altera a variavel FrontUmidade
        > caso 2: Temperatura, altera a variavel FrontTemperatura
        > caso 3: Luminosidade, altera a variavel FrontLuminosidade
        > caso 4: Pressao, altera a variavel FrontPressao

*/
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
            FrontPressao = FrontPressao + 1;
        }
        break;
    default:
        break;
    }
}
/*
    Funcao para inserir um elemento na fila. Cada insercao incrementa o indice
    Rear(fundo) da fila, ate quando a diferenca entre fundo e frente da fila for
    igual a 9 (tamanho 10), entao chama a funcao dequeue para retirar um elemento
    do inicio e insere o novo valor na fila.
    Parametros:
        => data: valor float da medicao atual do respectivo sensor
        => id_array:
        > caso 1: Umidade, altera a variavel FrontUmidade
        > caso 2: Temperatura, altera a variavel FrontTemperatura
        > caso 3: Luminosidade, altera a variavel FrontLuminosidade
        > caso 4: Pressao, altera a variavel FrontPressao

*/
void enqueue(float data, int id_array)
{
    switch (id_array)
    {
    case 1:
        // printf("Umidade ENQUEUE %.1f\n", data);
        if ((RearUmidade - FrontUmidade) == SIZE - 1)
        {
            dequeue(id_array);
        }

        if (FrontUmidade == -1)
            FrontUmidade = 0;

        RearUmidade = RearUmidade + 1;
        umid[RearUmidade] = data;
        break;

    case 2:
        if ((RearTemperatura - FrontTemperatura) == SIZE - 1)
        {
            dequeue(id_array);
        }

        if (FrontTemperatura == -1)
            FrontTemperatura = 0;

        RearTemperatura = RearTemperatura + 1;
        temp[RearTemperatura] = data;
        break;

    case 3:
        if ((RearLuminosidade - FrontLuminosidade) == SIZE - 1)
        {
            dequeue(id_array);
        }

        if (FrontLuminosidade == -1)
            FrontLuminosidade = 0;

        RearLuminosidade = RearLuminosidade + 1;
        lum[RearLuminosidade] = data;
        break;

    case 4:
        if ((RearPressao - FrontPressao) == SIZE - 1)
        {
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
/*
    Define variaveis globais para medicoes atuais de cada tipo de sensor,
    utilizadas para exibicao no display LCD e envio destas medicoes para
    o cliente remoto (interface gráfica em python) via MQTT
*/
float umidade_atual = -1, temperatura_atual = -1, luminosidade_atual = -1, pressao_atual = -1;

/*
    Variavel para verificar a conexao com o broker, envia-se uma mensagem
    num topico e espera resposta em um segundo topico. Quando ha esta res
    posta, atualiza esta variavel para o valor 1, indicando que a conexao
    existe
*/
int conexao = 0;

void send_intervalo(const char *msg_intervalo, int msg_tamanho, struct mosquitto *mosq)
{
    /* validacao do intervalo minimo (2 segundos) na interface grafica */
    intervalo = atoi(msg_intervalo);
    printf("%s\n", msg_intervalo);
    mosquitto_publish(mosq, NULL, "sd/pbl3/intervalo/new", msg_tamanho, msg_intervalo, 0, true); // Envia uma mensagem com o intervalo, isso indica que o intervalo foi recebido por  que o intervalo foi alterado
}
/* Alterar topico para testar broker raspberry */
void send_resposta(struct mosquitto *mosq)
{
    mosquitto_publish(mosq, NULL, "sd/pbl3/ping/resposta", 1, "1", 0, false); // Envia uma mensagem para informar ao cliente remoto que está conectado (sistema online)
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("ID: %d\n", *(int *)obj);
    if (rc)
    {
        printf("Error with result code: %d\n", rc);
        exit(-1);
    }
    char *subs[68] = {"sd/pbl3/ping/pedido", "sd/pbl3/ping/resposta", "sd/pbl3/intervalo/send"};
    mosquitto_subscribe_multiple(mosq, NULL, 3, subs, 0, 0, NULL);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("New message with topic %s: %s\n", msg->topic, (char *)msg->payload);
    if (strcmp((char *)msg->topic, "sd/pbl3/intervalo/send") == 0)
    {
        printf("%s\n", (char *)msg->payload);
        send_intervalo((char *)msg->payload, msg->payloadlen, mosq);
    }
    else if (strcmp((char *)msg->topic, "sd/pbl3/ping/pedido") == 0)
    {
        send_resposta(mosq);
    }
    /* Testa a conexao com o BROKER */
    if (strcmp((char *)msg->topic, "sd/pbl3/ping/resposta") == 0)
    {
        /* altera variavel para representar que a conexao com o broker esta online */
        conexao = 1;
    }
}

/*
    Instancia da thread para contar o intervalo definido, pela IHM ou inter-
    -face grafica. A forma de checagem permite que se um intervalo muito
    grande for determinado e o proximo intervalo utilizado for menor, a di-
    -ferenca entre eles e verificada.
*/
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
        /* ao passar o tempo definido pelo intervalo, chama a interrupçao */
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
    /* define os botoes e switches como entradas */
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

    /* Instancia a thread para contar o intervalo */
    int thread = piThreadCreate(contar_interv);
    /* Inicializar ADS */
    if (openI2CBus("/dev/i2c-1") == -1)
    {
        return EXIT_FAILURE;
    }
    setI2CSlave(0x48);
    /* Primeira escrita no LCD */
    lcdClear(lcd);
    lcdPosition(lcd, 0, 0);
    lcdPrintf(lcd, "Problema 03");
    lcdPosition(lcd, 0, 1);
    lcdPrintf(lcd, "IoT - MQTT");

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
    mosquitto_publish(mosq, NULL, "sd/pbl3/intervalo/new", 1, "2", 0, false);
    /* Aloca espaco na memoria para as filas das medicoes atuais */
    umid = (float *)malloc(SIZE_ARRAY * sizeof(float));
    temp = (float *)malloc(SIZE_ARRAY * sizeof(float));
    lum = (float *)malloc(SIZE_ARRAY * sizeof(float));
    pres = (float *)malloc(SIZE_ARRAY * sizeof(float));

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
        /*
            A interrupcao muda a variavel flag para 1, e entao o codigo de
            leitura dos sensores, envio das medicoes atuais e salva as medi
            coes nas filas para exibir no historico no lcd
        */
        if (flag == 1)
        {
            flag = 0;
            /* Leitura dos sensores */
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

            /* formata a string com as medicoes atuais e a data */
            sprintf(medicoes_atuais, "{\"umidade\":\"%.1f\",\"temperatura\":\"%.1f\",\"luminosidade\":\"%.1f\",\"pressao\":\"%.1f\",\"datahora\":\"%s\"}", umidade_atual, temperatura_atual, luminosidade_atual, pressao_atual, data_atual);
            /* envia as medicoes atuais e a data das medicoes */
            mosquitto_publish(mosq, NULL, "sd/pbl3/medidas", strlen(medicoes_atuais), medicoes_atuais, 0, false);
        }
        /*
            <Thread principal>

            | Display = 0 |
            Exibe medicoes atuais no lcd, que esta sendo escrito
            dentro das funções de leitura dos sensores, logo apos a medicao
        */
        if (display == 0)
        {
            /*
                Apertar um dos 3 botoes para a acessar os menus
                Fica parado nessa exibiçao ate apertar novamente
                para interagir com os menus
            */
            if ((digitalRead(BUTTON_1) == LOW) || (digitalRead(BUTTON_2) == LOW) || (digitalRead(BUTTON_3) == LOW))
            {
                /*
                    Altera a variavel display para 1, assim o menu da IHM
                    fica acessivel, utilizando os botoes e switches
                */
                display = 1;
                delay(50);
                while ((digitalRead(BUTTON_1) == LOW) || (digitalRead(BUTTON_2) == LOW) || (digitalRead(BUTTON_3) == LOW))
                    ; // aguarda enquato uma das 3 chaves chave ainda esta pressionada
                delay(50);
                /*  */
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Menu IHM 1:Intv");
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "2:ConfInt 3:Hist");

                /*
                    Salva as filas das medicoes em filas auxiliares para
                    exibir historico no lcd, utilizando os indices do
                    sensor Pressao porque e o ultimo a ser atualizado
                */
                front_historico = FrontPressao;
                rear_historico = RearPressao;

                int j = 0;
                /*
                    percorre os arrays para salvar nos auxiliares de exibicao
                    do historico no lcd
                */
                for (int i = front_historico; i <= rear_historico; i++)
                {
                    umidade_historico[j] = umid[i];
                    temperatura_historico[j] = temp[i];
                    luminosidade_historico[j] = lum[i];
                    pressao_historico[j] = pres[i];
                    j++;
                }
            }
        }
        /*
            | Display = 1 |

            Exibir menus:
                > Incrementar/Decrementar Intervalo
                > Confirmar Intervalo
                > Historico

            1º Botão
                Switch 1 == 0 -> Decrementar intervalo
                Switch 1 == 1 -> Incrementar intervalo

            2º Switch - mudar para 1 - testa e exibe conexao com o BROKER

            2º Botão
                Switch 3 = 0
                    Pressionar 1ª vez aparece mensagem com
                    o intervalo atual a ser enviado.
                    Pressionar 2ª para enviar realmente o
                    intervalo.
                Switch 3 = 1
                    retorna a exibicao das medicoes atuais

            3º Botão
                Switch 4 == 0 -> Decrementar indice do Histórico
                Switch 4 == 1 -> Incrementar indice do Histórico
        */
        else
        {
            /* Primeiro botao */
            if (digitalRead(BUTTON_1) == LOW)
            {
                /* mostrar menu para mudar intervalo */
                /*
                    | switch 1 == 1 |
                */
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
                /*
                    | switch 1 == 0 |
                */
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
                index_display = -1;
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

                /*
                    | switch 3 = 1 |
                */
                if (digitalRead(SWITCH_3) == LOW)
                {
                    /* Alternar para visualização das medicoes dos sensores */
                    display = 0;
                    /* zera variavel de controle para envio do intervalo */
                    confirmar = 0;
                    index_display = -1;
                    switch_2 = 0;
                    /*
                        Escreve no LCD as ultimas medicoes dos sensores, para
                        o caso de o intervalo estar muito grande. Caso contra-
                        -rio iria exibir novamente a medicao quando houvesse
                        novas leituras
                    */
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "U:%.1f%% T:%.1fC ", umidade_atual, temperatura_atual);
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "L:%.1fV  P:%.1fV  ", luminosidade_atual, pressao_atual);
                }
                /*
                   | switch 3 = 0 |
                */
                else
                {
                    /* Confirmar >=2 envia o intervalo */
                    if (confirmar >= 2)
                    {
                        /* Formata como string, para passar como parametro no publish */
                        char str_intervalo[2];
                        sprintf(str_intervalo, "%i", aux_intervalo);
                        /* enviar intervalo via mqtt */
                        mosquitto_publish(mosq, NULL, "sd/pbl3/intervalo/new", strlen(str_intervalo), str_intervalo, 0, false);
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
                index_display = -1;
            }
            /* Ultimo botao */
            else if (digitalRead(BUTTON_3) == LOW)
            {
                /*
                    | switch 4 == 1 |
                    incrementa o indice
                */
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
                    /* Escreve no LCD os valores do historico */
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "U:%.1f%% T:%.1fC ", umidade_historico[index_display], temperatura_historico[index_display]);
                    /*
                        Na segunda linha ha o indice para indicar qual posicao do historico esta sendo exibido
                    */
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "%i L:%.1fV P:%.1fV", (index_display + 1), luminosidade_historico[index_display], pressao_historico[index_display]);
                }
                /*
                    | switch 4 == 0 |
                    incrementa o indice
                */
                else
                {
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
                    /* Escreve no LCD os valores do historico */
                    lcdClear(lcd);
                    lcdPosition(lcd, 0, 0);
                    lcdPrintf(lcd, "U:%.1f%% T:%.1fC ", umidade_historico[index_display], temperatura_historico[index_display]);
                    /*
                        Na segunda linha ha o indice para indicar qual posicao do historico esta sendo exibido
                    */
                    lcdPosition(lcd, 0, 1);
                    lcdPrintf(lcd, "%i L:%.1fV P:%.1fV", (index_display + 1), luminosidade_historico[index_display], pressao_historico[index_display]);
                }
                confirmar = 0; /* zerar opcao de confirmar ao apertar outro botao */
            }
            /*
                | Switch 2 alternar para 1 |
            */
            else if ((digitalRead(SWITCH_2) == LOW) && switch_2 == 0)
            {
                switch_2 = 1;
                lcdClear(lcd);
                lcdPosition(lcd, 0, 0);
                lcdPrintf(lcd, "Testando a");
                lcdPosition(lcd, 0, 1);
                lcdPrintf(lcd, "Conexao BROKER");
                /*
                    envia mensagem 1 para o topico ping/pedido, depois de
                    1 segundo, se houve resposta, a variavel conexao sera
                    igual a 1 (true)
                */
                mosquitto_publish(mosq, NULL, "sd/pbl3/ping/pedido", 1, "1", 0, false);
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
                confirmar = 0;
                index_display = -1;
            }
        }
    }
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}

/*
    Funcao para agrupar a leitura dos potenciometros - sensores de
    luminosidade e potencia
*/
void read_poten(int lcd)
{
    /* Le e guarda os valores atuais nas variaveis globais */
    luminosidade_atual = readVoltage(1);
    pressao_atual = readVoltage(0);
    /*
        se a variavel display for 0, escreve na segunda linha do LCD
        os valores atuais de luminosidade e pressao
    */
    if (display == 0)
    {
        lcdPosition(lcd, 0, 1);
        lcdPrintf(lcd, "L:%.1fV  P:%.1fV  ", luminosidade_atual, pressao_atual);
    }
}

/* Funcao de leitura do sensor DHT (umidade e temperatura) */
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
        /*
            Salvando as medicoes atuais de umidade e temperatura
            Converte os inteiros do array dht11_dat para float
        */
        umidade_atual = dht11_dat[0] + ((float)dht11_dat[1] / 10);
        temperatura_atual = dht11_dat[2] + ((float)dht11_dat[3] / 10);
        /*
            se a variavel display for 0, escreve na primeira linha do LCD
            os valores atuais de umidade e temperatura
        */
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:%.1f%% T:%.1fC", umidade_atual, temperatura_atual);
        }
    }
    else
    {
        /*
            O sensor DHT pode apresentar falhas na leitura e nao retorna
            os dados corretos, assim adiciona-se o valor -1.1 (que nao
            e valor possivel de leitura) para indicar que houve erro
        */
        umidade_atual = -1.1;
        temperatura_atual = -1.1;

        /*
            se a variavel display for 0, escreve na primeira linha do LCD
            os valores atuais de umidade e temperatura
        */
        if (display == 0)
        {
            lcdPosition(lcd, 0, 0);
            lcdPrintf(lcd, "U:-1.1  T:-1.1  ");
        }
    }
}
