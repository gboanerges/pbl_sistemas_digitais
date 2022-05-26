/*
    C program to call uart function
*/

#include <stdio.h>
#include <unistd.h>

/* function declaration */
extern int uartConfig();
extern int uartSend(int command);
extern int uartGet();

int main()
{
    int choice, endereco, comando, resposta;
    printf("\e[1;1H\e[2J"); // limpar o console

    printf("\nProblema 02 - Interfaces de E/S\n");
    printf("\nProjeto de Sensor Digital em FPGA utilizando Comunicação Serial\n\n");

    uartConfig();

    do {

        printf("Comandos de requisicao\n");
        printf("1 - Situacao atual do sensor\n");
        printf("2 - Temperatura\n");
        printf("3 - Umidade\n");
        printf("4 - Sair\n\n");
        scanf("%d", &choice);

        switch(choice) {

            case 1: //Situacao atual do sensor

                printf("\e[1;1H\e[2J"); // limpar o console

                comando = 3; // comando para pedir situacao do sensor

                printf("\nEnviando o comando: %i \n", comando);

                uartSend(comando); // testar retorno

                sleep(2);
		        resposta = -1;
                resposta = uartGet();

                   // 102 = sensor OK
                   // 126 = comando desconhecido
                   // 120 = sensor com erro

                if (resposta == 102){

                    printf("\nSensor funcionando normalmente! %d\n\n", resposta);

                } else if (resposta == 126) {

                    printf("\nComando desconhecido: %d \n\n", resposta);

                } else if (resposta == 120) {

                    printf("\nSensor com erro: %d \n\n", resposta);

                } else {

                    printf("\nAlgo deu errado. :( %d  \n\n", resposta);

                }

                break;
            //Temperatura
            case 2:
                printf("\e[1;1H\e[2J");
                comando = 4;
                printf("\nEnviando o comando: %i \n", comando);

                uartSend(comando);

                sleep(2);
		        resposta = -1;

                resposta = uartGet();
                /*
                    126 = comando desconhecido
                    120 = sensor com erro
                */

                if (resposta == 126) {

                    printf("\nComando desconhecido: %d \n\n", resposta);
                } else if (resposta == 120) {

                    printf("\nSensor com erro: %d \n\n", resposta);
                } else { // nao foram codigos de erro, printa oq recebeu

                    printf("\nTemperatura: %dº\n\n", resposta);
                }

                break;


            //Umidade
            case 3:
                printf("\e[1;1H\e[2J");
                comando = 5;
                printf("\nEnviando o comando: %i\n\n", comando);

                uartSend(comando);
                sleep(2);
                resposta = -1;
                resposta = uartGet();

                /*
                    126 = comando desconhecido
                    120 = sensor com erro
                */

                if (resposta == 126) {

                    printf("\nComando desconhecido: %d\n\n", resposta);
                } else if (resposta == 120) {

                    printf("\nSensor com erro: %d \n\n", resposta);
                } else { // nao foram codigos de erro, printa oq recebeu

                    printf("\nUmidade: %d %% \n\n", resposta);
                }

                break;

            case 4:

                printf("\nEncerrando programa.\n");
                return (0);

            default:

                printf("\e[1;1H\e[2J"); // limpar o console
                printf("\nOpcao invalida! Tente novamente.\n");
                break;
        }

    } while (choice != 4);
}
