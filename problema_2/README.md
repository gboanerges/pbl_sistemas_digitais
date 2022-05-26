
<h1 align="center">Problema 02 - Interfaces de E/S</h1>
<p align="center">Projeto de Sensor Digital em FPGA utilizando Comunicação Serial<br>Metodologia PBL - MI - Sistemas Digitais 2022.1</p>

## Tabela de conteúdos 

<p align="left"> 
• <a href="#status">Status</a> <br>
• <a href="#feat">Recursos implementados</a> <br>
• <a href="#tec">Tecnologias</a> <br>
• <a href="#problema"> Problema</a> <br>  
• <a href="#uart">UART</a> <br> 
• <a href="#fpga">Entradas e saídas da FPGA</a> <br> 
• <a href="#instalacao">Guia de uso</a>  <br>  
• <a href="#demo">Demonstração</a>  <br>  
• <a href="#todo">Recursos a serem implementados</a>   <br>
• <a href="#considera">Considerações finais</a>   <br>
• <a href="#ref">Referências externas</a>   <br>
• <a href="#equipe">Equipe</a> 
</p>

<h4 id="status" align="center"> ✅ Finalizado ✅ </h4>

<h2 id="feat">Recursos implementados</h2>

- [x] Integração do Assembly com o C
- [x] Programa em C  para a comunicação com a FPGA
- [x] Comunicação entre o SBC e a FPGA via UART
- [x] Obtenção dos dados do sensor DHT11 (visualizado por meio do osciloscópio)
- [ ] Visualização dos dados retornados pelo sensor na SBC
- [ ] Comunicação com 32 sensores
- [ ]  Modularização para utilização de outros tipos de sensores

<h2 id="tec" >🛠 Tecnologias </h2> 
As seguintes ferramentas foram usadas na construção do projeto: 

-  Assembly 
-  C
-  Verilog

Placas utilizadas:
-  SBC : Raspberry Pi Zero
-  FPGA : Kit Mercurio IV Ciclone IV EP4CE30F23C7
 
## Problema
<p id="problema" align="justify">
Implementar um sistema que será comandado por um Single Board Computer (SBC), e deve ser capaz de controlar o acionamento de um conjunto variável de sensores, assim como monitorar o seu funcionamento, de forma automatizada. Cada operação de leitura ou monitoramento deve ser representada por um código. Dessa forma, o sistema embarcado em FPGA deve ser capaz de interpretá-los e realizá-los de maneira adequada, por meio do driver da UART implementado anteriormente.

<b>Apenas o SBC pode começar a comunicação com o sensor DHT11</b>
 </p>
 
 <h2 id="uart" >UART</h2>

<p id="uart" align="justify">
Um dos requisitos foi utilizar a já implementada UART em linguagem  Assembly dentro do código em C. Para isso, foi necessário criar 3 arquivos da UART em Assembly e compilar como biblioteca. 

 1) Configuração da UART com os parâmetros: 8 bits de tamanho da mensagem e 1 stop bit.
 2) Mapeando o endereço base da UART da Raspberry Pi Zero  e enviando um dado - recebido como parâmetro do C, e 
 3) Mapeando o endereço base da UART da Raspberry Pi Zero e esperar o recebimento de 1 byte. 

Afim de facilitar a compilação destes códigos, utilizou-se um arquivo ***makefile***, com os comandos: 

```bash
make
make clean
```
Com o **make** os arquivos são compilados e o comando **make clean** remove os arquivos binários, a biblioteca e o arquivo executável, de modo a evitar algum erro de arquivo não atualizado ( se for necessário alterar algum dos arquivos).

```bash
sudo ./main
```
O comando *sudo ./main* executa o programa em C.

 </p>

 <h2 id="fpga">Módulos implementados na FPGA</h2> 
<p align="justify">
Para a implementação dos requisitos do sistema por parte da FPGA utilizamos a Linguagem de Descrição de Hardware Verilog.
</p>
<p>O código desenvolvido para a FPGA apresenta-se divido em 6 módulos, sendo eles:

 - top_dht
 - UART_rs232_tx
 - UART_rs232_rx
 - DHT11
 - control3
 - TRI_STATE
</p>
<p>*top_dht*:  o módulo top-level do projeto, sendo responsável por instanciar todos os outros módulos e efetuar as interconexões entre módulos, entradas e saídas da FPGA. </p>
<p>*UART_rs232_tx*: módulo da UART responsável por transmitir os dados.</p>
<p>*UART_rs232_rx*: módulo da UART responsável por receber os dados.</p>
<p>*DHT11*: módulo responsável por controlar o sensor DHT11. O módulo efetua a comunicação com o sensor, recebe os dados e os transmite para suas saídas.</p>
<p>*control3*: módulo responsável por verificar qual dados foi recebido do SBC por meio da UART, habilitar a comunicação do módulo DHT11 com o sensor, selecionar os dados recebidos do módulo do DHT11 e habilitar a transmissão dos dados de volta para a SBC.</p>
<p>*TRI_STATE*: módulo responsável por alterar o estado da entrada/saída do sensor DHT11. Como o sensor utiliza apenas um único pino tanto para entrada quanto para saída de dados, é necessário utilizar o módulo TRI_STATE para efetuar a seleção, se me determinado momento o pino atuará como entrada ou como saída de dados.</p>

## Entradas e saídas da FPGA
<p>O módulo *top_dht* é responsável pelas saídas e entradas da FPGA.</p>
<p>As entradas são:

 - `Clk`: entrada do clock.
 - `Rst_n`: entrada de reset do sistema. (lógica invertida, reseta em nível lógico baixo)
 - `Rx`: entrada (recebimento) de dados da UART.
 - `dht11`: entrada de dados do sensor DHT11 (essa entrada é do tipo inout, ou seja, também é uma saída).
</p>
<p>As saídas são:

 - `Tx`: saída (envio) de dados da UART.
 - `RxData`: saída de 8 bits com os dados recebido pela UART. Neste projeto, utilizamos essa saída para debug, exibindo o dado recebido na matriz de LEDs presente na placa.
 - `col`: saída para ativar uma das colunas da matriz de LEDs.
 - `dht11`: saída de dados para a comunicação com o DHT11 (essa entrada também é uma saída).

</p>

<h2 id="instalacao"> Guia de uso</h2>

Para utilizar o projeto é necessário:

1) Baixar este repositório

2) Compilar o programa em C
```bash
# Acessar a pasta problema_2/uart_assembly

$ cd problema_2/uart_assembly
```
 <p align="justify">
Na pasta uart_assembly há 5 arquivos relativos ao programa em C. Sendo necessário passar para uma Raspberry Pi 0 (por conta do mapeamento da memória), executar o comando **make** (compilar) e o **sudo ./main**  para executar.
</p>

```bash
$ make

# Executar o programa em C

$ sudo ./main
```
 <p align="justify">
3) Possuindo o Quartus instalado, basta abrir o projeto e compilar.

 Atribuir pinos da GPIO da FPGA para o Tx, Rx e DHT_DATA. Após a compilação e pinagem, é possível realizar a <b>descarga</b> do projeto para a FPGA.

</p>

---
 <p align="justify">
Agora com o programa em C rodando, solicite uma das 3 opções do menu. O comando enviado para a FPGA é fixo, e os valores possíveis são apresentados na tabela  abaixo:
</p>

<div align="center">

Tabela 1 – Comandos de requisição.
| Código | Descrição do comando                |
|:------:|:-----------------------------------:|
| 0x03   | Solicita a situação atual do sensor.|
| 0x04   | Solicita a medida de temperatura.   |
| 0x05   | Solicita a medida de umidade.       |
</div>

<p align="justify">
As possíveis respostas são descritas abaixo, na tabela 2. Se não apresentar código de resposta, significa que devolveu o valor da medição pedida
</p>

<div align="center">

Tabela 2– Comandos de resposta
| Código | Descrição |
|:------:|:-----------------------------------:|
| 0x78   | Sensor com problema.|
| 0x66   | Sensor funcionando normalmente.   |
| 0x7E  | Comando desconhecido.      |
 </div>

<h2 id="demo" >Demonstração</h2>

O GIF a seguir demonstra a comunicação com o sensor DHT11, os pulsos de sincronismo e dos 40 bits de dados.

![Alt Text](https://media1.giphy.com/media/vv2izDpU3MLKUibhe6/giphy.gif?cid=790b7611bfa5451fbf94fb2ad7bb4d961fa0b4d329e902a8&rid=giphy.gif&ct=g)

<p align="justify">
Nas imagens abaixo podemos distinguir os dados que o DHT11 devolve. Primeiro começa com pulsos de sincronismo, 18 mS em 1 e 80 uS em 0 e 1 novamente. A partir do bit 3 são os 40 bits de resposta do sensor.
</p>

![Pulsos de sincronismo](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_2/assets/sincronismo.png  "L")


<p align="center">
Imagem 1 - Pulsos de sincronismo
<p/>
<p align="justify">
Na segunda imagem, o valor recebido da umidade foi, da esquerda para a direita, em binário, 01000011 parte inteira e 00000000 parte fracionária. Correspondendo a 67,0 % de umidade.
<p/>

![Umidade](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_2/assets/umidade.png  "L")

<p align="center">
Imagem 2 - Medição Umidade
<p/>
<p align="justify">
Imagem 3, a medição de temperatura, em laranja, em binário 00011001 parte inteira e 00000000 parte fracionária. Correspondendo a 25,0 º.
<p/>

![Temperatura](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_2/assets/temperatura.png  "L")

<p align="center">
Imagem 3 - Medição Temperatura
<p/>
<p align="justify">
Os últimos 8 bits da resposta do sensor DHT11 são para checar a paridade dos dados que o mesmo enviou, que foi o binário 01011100 que é 92 no sistema decimal. Somando os bits 8 a 8: parte inteira + parte fracionária da Umidade, este resultado + parte inteira da Temperatura e o resultado + parte fracionária da Temperatura. No final ao comparar precisa ser o mesmo número binário resultante, pois senão for houve erro na medição do sensor.  Fazendo as contas, 67 + 0 + 25 + 0 = 92. Então o sensor respondeu corretamente 92 = 92 (Paridade)
<p/>

![Paridade](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_2/assets/paridade.png  "L")

<p align="center">
Imagem 4 - Paridade
<p/>

<h2 id="todo" > Recursos a serem implementados</h2>

* Visualização dos dados do sensor no SBC;
* Permitir que o programa em C receba dois bytes de resultado, um para a parte inteira e outro para a parte fracionária, em caso de requisição de Temperatura ou Umidade;
* Suporte para até 32 sensores, sendo possível requisitar a Temperatura, Umidade ou estado de qualquer um deles.
* Modularização para utilização de outros tipos de sensores

<h2 id="considera" > Considerações finais</h2>
	<p>O projeto, até então, ainda não cumpre todas as funcionalidades desejadas. Porém, a atual implementação permite suprimir parte dos requisitos. </p>

<p>Conseguimos realizar a implementação da UART para o envio e o recebimento de dados entre o SBC e a FPGA e ativação do sensor DHT11. Porém, não possível receber o dado retornado pelo sensor no SBC. </p>

<p>Foi possível constatar o funcionamento desses recursos por meio de testes de Loopback entre a própria Raspberry Pi Zero, entre a Raspberry Pi Zero e a FPGA e também por meio do uso da matriz de LEDs para exibir os dados recebidos na FPGA . Já para a constatação da ativação e aquisição de dados do sensor, efetuamos o uso do osciloscópio o qual nos permitiu visualizar os dados fornecidos pelo sensor DHT11.</p>

<h2 id="equipe" >Equipe</h2>

* Gustavo Boanerges
* Igor Soares

<h2 id="ref" >Referências externas</h2>
<p>Conteúdos que serviram de base para a construção do projeto:

 - [UART & FPGA Bluetooth connection [Vídeo]](https://www.youtube.com/watch?v=QlscDcbKUV4)
 - [TERMÔMETRO EM FPGA COM DHT11 [Vídeo]](https://www.youtube.com/watch?v=BkTYD7kujTk)
 - [TERMÔMETRO EM FPGA ALTERA [Vídeo]](https://www.youtube.com/watch?v=D67z1-vPH7U)
 - [Nandland Go Board Project 7 - UART Receiver [Vídeo]](https://www.youtube.com/watch?v=Vh0KdoXaVgU)
 - [Nandland Go Board Project 8 - UART Transmitter [ Vídeo]](https://www.youtube.com/watch?v=Jy5jRhDqNss)
</p>


