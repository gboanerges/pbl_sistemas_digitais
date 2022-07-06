
<h1 align="center">Problema 03 - IoT: A Internet das Coisas</h1>
<p align="center">Protótipo de um sistema IoT.
<br>Metodologia PBL - MI - Sistemas Digitais 2022.1</p>

## Tabela de conteúdos 

<p align="left"> 
• <a href="#status">Status</a> <br>
• <a href="#feat">Recursos implementados</a> <br>
• <a href="#tec">Tecnologias</a> <br>
• <a href="#problema"> Problema</a> <br>  
• <a href="#mqtt">MQTT</a> <br> 
• <a href="#codC">Programa em C</a> <br> 
• <a href="#python">Python</a> <br>
• <a href="#guia">Guia de uso</a>  <br>  
• <a href="#demo">Demonstração</a>  <br>  
• <a href="#todo">Recursos a serem implementados</a>   <br>
• <a href="#considera">Considerações finais</a>   <br>
• <a href="#equipe">Equipe</a>  <br>
• <a href="#ref">Referências externas</a>  

</p>

<h4 id="status" align="center"> 🚧  Em construção  🚧</h4>

<h2 id="feat">Recursos implementados</h2>

- [x] Codificação em C
- [x] Usar protocolo MQTT
- [x] IHM com botões e chaves (push-buttons e dip-switches)
- [x] Interface gráfica para Desktop
- [x] Informação sobre status da conexão

<h2 id="tec" >🛠 Tecnologias </h2> 

As seguintes ferramentas foram usadas na construção do projeto: 

-  C
- Python

Placas utilizadas:
-  SBC : Raspberry Pi Zero

 
<h2 id="problema">Problema</h2>

<p align="justify">
Implementar um protótipo de um sistema para monitoramento ambiental que será posteriormente integrado a um sistema para monitoramento de cidades. O protótipo deve incluir todo o tratamento e controle de sensores analógicos e digitais, bem como uma IHM (interface Homem-Máquina) para apresentação das informações,incluindo históricos dos dados.
</p>
<p align="justify">
O protótipo deve ser desenvolvido num SBC que medirá temperatura, umidade, pressão atmosférica e luminosidade. A IHM deve apresentar, em tempo real, as leituras atuais. Ela também deve permitir a visualização do histórico com as 10 últimas medições de cada sensor. O sistema deve permitir o ajuste local e remoto do intervalo de tempo que serão realizadas as medições.
</p>

<h2 id="mqtt">MQTT</h2>

<p align="justify">

MQTT

</p>
<h2 id="codC">Programa em C</h2>

<p align="justify">

No cliente em C, usou-se a biblioteca WiringPi, que possue funções prontas para utilização do display LCD e dos botões e switches do kit de extensão de GPIO.

Para a contagem do intervalo de tempo, foi necessário ter uma thread a parte da thread principal. Como a biblioteca WiringPi possue threads, instancia-se uma PiThread que contém 2 loops. No primeiro, um while true, guarda o valor do tempo inicial e dentro deste, o segundo loop, um do-while, fica salvando o tempo final e testa se a diferença entre o tempo inicial e o tempo final é menor que o intervalo atual. Quando a difença for maior que o intervalo, sai do loop interno, chama a interrupção e volta ao primeiro loop.

Interrupção

</p>

<h2 id="python">Python - Interface remota</h2>

<p align="justify">

A interface remota (cliente remoto) foi desenvolvida em Python utilizando a biblioteca Tkinter para a construção da interface e a biblioteca Paho MQTT para
implementação do protocolo MQTT.

</p>
 <h3 id="guia-interface">Guia de uso - interface remota</h3>
 <p align="justify">
A interface consiste em uma tela onde são exibidas as medidas dos sensores de acordo com o intervalo definido e um histórico das últimas 10 medidas recebidas. 

Além disso, a interface exibe o status de conexão do cliente remoto com o broker e o status de conexão com a estação de medição, indicando se está OFFLINE ou ONLINE. A verificação de status de conexão com a estação é feito por meio de um sistema de ping, ou seja, o cliente remoto envia um sinal para a estação, se esse sinal for recebido de volta pelo cliente remoto significa que está conectado com a estação, caso contrário, não.

A interface dispõe também de um campo o qual pode ser inserido o intervalo de medição (em segundos) desejado. Após inserir o intervalo desejado é só apertar no botão ao lado da caixa de diálogo que o intervalo é enviado para a estação de medição. Após o intervalo ser recebido por a estação, o valor desse intervalo
é retornado para a interface e, então, é exibido o novo intervalo de medição escolhido. Deste modo, garantimos que o intervalo que está sendo exibido no cliente remoto é igual ao configurado na estação de medição.

Para utilizar a interface remota é necessário primeiramente dispor de algumas ferramentas, vamos lá.
Ferramentas necessárias:

    - Python versão 3.8.10 (recomendado) ou superior instalado;
    - Biblioteca Tkinter do Python para a interface gráfica;
    - Gerenciador de pacotes pip do Python;
    - Biblioteca Paho MQTT.
</p>

<h4 class="instalacao">Instalação do Python</h4> 
<p align="left">
Para a instalação do Python pode ser seguido o guia disponível na própria página do Python: https://wiki.python.org/moin/BeginnersGuide/Download
</p>
<h4 class="instalacao">Biblioteca Tkinter</h4> 
<p align="justify">
A biblioteca para a construção de interfaces gráficas Tkinter do Python vem por padrão ao efetuar a instalação do Python. Contudo, em alguns sistemas operacionais como Linux e Mac, eventualmente, pode ocorrer da biblioteca não ser instalada junto ao Python, nestes casos, é ncessãrio afetuar a instalação do Python via terminal especificando que a biblioteca Tkinter também deve ser instalada. Isso pode ser feito da seguinte maneira:

```bash
# == Para Ubuntu/Debian ==

#Para a instalação da versão mais recente do Python e do Tkinter
$ sudo apt-get install python3-tk 

#Para a instalação de uma versão específica do Python
$ sudo apt-get install python3.x-tk #onde x é a versão desejada


# == Para MacOS ==
#Para a instalação de uma versão específica do Python
brew install python-tk@3.x #onde x é a versão desejada

```
</p>

<h4 class="instalacao">Instalação do pip</h4> 
<p align="justify">
O pip é um gerenciador de pacotes do Python. Normalmente ele pode ser instalado junto a instalação padrão do Python, porém isso nem sempre ocorre.
Para verificar se o pip está instalado, pode ser executado o seguinte comando:

```bash
# == No terminal do linux ou no Prompt de comando do Windows ==
pip -V  #Mostrará a versão do pip instalado no sistema

```

Caso o comando não seja reconhecido, significa que o gerenciador de pacotes não encotra-se instalado no sistema.

Para a instalação do Windows seguimos os seguintes passos:
- Baixar o arquivo "get-pip.py" (https://bootstrap.pypa.io/get-pip.py) e armazenar no diretório em que o Python está instalado.
- Abrir a pasta que o arquivo foi armazenado no Prompt de comando (CMD).
- Executar no CMD o comando:

```bash
# == No Prompt de comando do Windows ==
...> python get-pip.py  # Instalação do gerenciador de pacotes pip

```
- Aguardar o fim da instalação

Para a instalação no Linux:
```bash
# == No terminal do Ubuntu/Debian ==
$ sudo apt-get update   # 1. Atualiza os pacotes do sistema operacional 

$ sudo apt-get install python3-pip # 2. Instala o pip 
```
</p>

<h4 class="instalacao">Instalação da biblioteca Paho MQTT</h4> 
<p align="justify">
Na construção da interface remota foi utilizada a biblioteca MQTT Paho para efetuar as conexões e operações MQTT do sistema.
Essa biblioteca não é nativa do conjunto de bibliotecas no Python, sendo necessário efetuar a sua instalação por meio do gerenciador
de pacotes pip do Python.

A instalação da biblioteca pode ser feita executando o comando:
```bash
# == No terminal do linux ou no Prompt de comando do Windows ==

pip install paho-mqtt #Instalação da biblioteca MQTT Paho

```
</p>


<h2 id="guia">Guia de uso</h2>
 <p align="justify">
Para utilizar o projeto é necessário:

1) Baixar este repositório

2) Compilar o programa em C
</p>

```bash
# Acessar a pasta problema_3/

$ cd problema_3/
```
 <p align="justify">
Na pasta problema_3 há dois arquivos, um do programa principal em C e outro da interface gráfica em Python. O código em C deve estar numa placa Raspberry que possua o kit de extensão de GPIO e display LCD, e então executar o comando <b>make</b> (compilar) e o <b>sudo ./main</b>  para executar.
</p>

```bash
# Compilar

$ make

# Executar o programa em C

$ sudo ./main
```
3) Com todas as ferramentas e bibliotecas instaladas, para executar o programa basta acessar a pasta do o arquivo dashboad.py e executá-lo. No linux via terminal pode ser feito da seguinte maneira:


```bash
# Acessar a pasta problema_3/
$ cd problema_3/
```

Agora para executar o programa basta executar o comando:
```bash
# Acessar a pasta cliente_python/
$ python3 dashboard.py

```
---
<p align="justify">
Com o programa em C sendo executado, as medições atuais serão exibidas a cada contagem do intervalo de tempo configurado, no mínimo 2 segundos.

</p>

<h2 id="demo" >Demonstração</h2>

> INSTRUÇÕES DE USO DA IHM

Exibir as imagens do lcd, da interface grafica

<div id="image01" style="display: inline_block" align="center">
	
</div>
	

<h2 id="todo" >Recursos a serem implementados</h2>


<h2 id="considera" >Considerações finais</h2>

<p align="justify">

O projeto..

</p>

<h2 id="equipe" >Equipe</h2>

* Gustavo Boanerges
* Igor Soares

<h2 id="ref" >Referências externas</h2>
<p>Conteúdos que serviram de base para a construção do projeto:
 
- [HOW TO SETUP AN LCD ON THE RASPBERRY PI AND PROGRAM IT WITH C](https://www.circuitbasics.com/raspberry-pi-lcd-set-up-and-programming-in-c-with-wiringpi/)
- [HOW TO SET UP THE DHT11 HUMIDITY SENSOR ON THE RASPBERRY PI](https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-the-raspberry-pi/#:~:text=The%20DHT11%20has%20a%20surface,C)
</p>
