<h1  align="center">Problema 03 - IoT: A Internet das Coisas</h1>

<p  align="center">
Protótipo de um sistema IoT.
<br>
Metodologia PBL - MI - Sistemas Digitais 2022.1
</p>


## Tabela de conteúdos

<p  align="left">

• <a  href="#status">Status</a> <br>
• <a  href="#feat">Recursos implementados</a> <br>
• <a  href="#tec">Tecnologias</a> <br>
• <a  href="#problema"> Problema</a> <br>
• <a  href="#mqtt">MQTT</a> <br>
• <a  href="#codC">Programa em C</a> <br>
• <a  href="#python">Python</a> <br>
• <a  href="#guia">Guia de uso</a> <br>
• <a  href="#demo">Demonstração</a> <br>
• <a  href="#todo">Recursos a serem implementados</a> <br>
• <a  href="#considera">Considerações finais</a> <br>
• <a  href="#equipe">Equipe</a> <br>
• <a  href="#ref">Referências externas</a>

</p>

<h4  id="status"  align="center"> ✅ Finalizado ✅ </h4>

<h2  id="feat">Recursos implementados</h2>

- [x] Codificação em C
- [x] Usar protocolo MQTT
- [x] IHM com botões e chaves (push-buttons e dip-switches)
- [x] Interface gráfica para Desktop
- [x] Informação sobre status da conexão

  
<h2  id="tec" >🛠 Tecnologias </h2>

As seguintes ferramentas foram usadas na construção do projeto:

- C

- Python

Placas utilizadas:

- SBC : Raspberry Pi Zero

<h2  id="problema">Problema</h2>

<p  align="justify">
Implementar um protótipo de um sistema para monitoramento ambiental que será posteriormente integrado a um sistema para monitoramento de cidades. O protótipo deve incluir todo o tratamento e controle de sensores analógicos e digitais, bem como uma IHM (interface Homem-Máquina) para apresentação das informações,incluindo históricos dos dados.
<br><br>
O protótipo deve ser desenvolvido num SBC que medirá temperatura, umidade, pressão atmosférica e luminosidade. A IHM deve apresentar, em tempo real, as leituras atuais. Ela também deve permitir a visualização do histórico com as 10 últimas medições de cada sensor. O sistema deve permitir o ajuste local e remoto do intervalo de tempo que serão realizadas as medições.
<br><br>
O sensor utilizado para umidade e temperatura foi o DHT11. Para a luminosidade e pressão atmosférica recorreu-se ao uso de 2 potenciometros e um conversor analógico-digital para simular os sensores destas duas grandezas.
</p>  

<h2  id="mqtt">MQTT</h2>

<p  align="justify">
Protocolo MQTT (Message Queuing Telemetry Transport) é um protocolo de transporte de mensagens de formato Cliente/Servidor, que possibilita a comunicação entre máquinas (Machine to Machine – M2M) e é amplamente usado para conectividade de IoT (Internet of Things). É aberto, leve e tem fácil implementação, sendo executado em TCP/IP ou em outros protocolos de rede.
<br><br>
Entre suas características principais, destacam-se a sua qualidade de serviço, maior nível de segurança, facilidade de aplicação, baixa alocação de banda e compatibilidade com linguagens de programação.
<br><br>

Os tópicos que foram utilizados são: 

- `sd/pbl3/ping/pedido`: tópico para o envio do ping da interface remota para a estação de medição
- `sd/pbl3/ping/resposta`: tópico responsável pelo envio do ping da estação de medição para a interface remota
- `sd/pbl3/intervalo/send`: recebe o intervalo enviado pela interface gráfica
- `sd/pbl3/intervalo/new`: envio de intervalo da raspberry para interface gráfica
- `sd/pbl3/medidas`: por meio deste tópico as medições atuais dos sensores e a data da leitura são enviados
</p>

<h2  id="codC">Programa em C</h2>

<p  align="justify">
O cliente em C é o responsável pela conexão entre sensores, envio das medições e interface homem-máquina.
<br><br>
Usou-se a biblioteca WiringPi, que possue funções prontas para utilização do display LCD e dos botões e switches do kit de extensão de GPIO. A interface homem-máquina (IHM) utiliza uma estrutura de if-else, checando se o botão X foi pressionado, e se o switch Y está em valor 1 ou 0. O guia de como utilizar o menu IHM está na seção Demonstração. A lógica dos botões e switches verifica se a leitura destes é igual a LOW, nível lógico baixo, mas significa que o botão foi pressionado e que o switch está com valor 1.
<br><br>
Para a contagem do intervalo de tempo, foi necessário ter uma thread a parte da thread principal. Aproveitando a biblioteca WiringPi , instancia-se uma PiThread que contém 2 loops. No primeiro, um while true, guarda o valor do tempo inicial e dentro deste, o segundo loop, um do-while, fica salvando o tempo final e testa se a diferença entre o tempo inicial e o tempo final é menor que o intervalo atual. Quando a diferença for maior que o intervalo, sai do loop interno, chama a interrupção e volta ao primeiro loop, testando novamente o intervalo. <br><br>
A forma de teste permite que se um <strong> intervalo muito grande for determinado </strong> e o <strong>próximo intervalo utilizado for menor</strong>, a diferença entre eles é verificada, assim não é necessário esperar o intervalo maior empregado anteriormente. A cada passagem de intervalo, uma função de interrupção que altera uma variável que indica se houve a interrupção  e assim ocorre a leitura dos sensores, envio das medições atuais e escrita no lcd.
<br><br>
A função de interrupção altera o valor da variável 'flag' para 1 e dentro da thread principal há um 'if (flag==1)'. Assim ao ocorrer desta variável o código dentro deste if irá ser executado, sendo a primeira linha alterar 'flag' novamente para o valor 0.
<br><br>
<strong>Vale ressaltar que é necessário instalar na placa Raspberry a biblioteca Mosquitto MQTT e a WiringPi. </strong>
</p>
<h4  id="wiring">Instalar WiringPi e Mosquitto</h4>

<p  align="justify">
Para instalar a biblioteca WiringPi, numa raspberry com acesso a internet:
</p>

```bash
# Clonar o repositório git do WiringPi
git clone git://git.drogon.net/wiringPi
# Acessar a pasta do repositório
cd ~/wiringPi
# Executar o script para instalar e aguardar finalizar
./build
```

<p  align="justify">
Para a biblioteca Mosquitto:
</p>

```
sudo apt-get install -y libmosquitto-dev
```
<h2  id="python">Python - Interface remota</h2>

<p  align="justify">
A interface remota (cliente remoto) foi desenvolvida em Python utilizando a biblioteca Tkinter para a construção da interface e a biblioteca Paho MQTT para implementação do protocolo MQTT.

</p>

<h3  id="guia-interface">Interface remota</h3>

<p  align="justify">
A aplicação remota foi desenvolvida em Python, utilizando a biblioteca Tkinter para a construção da interface e a biblioteca Paho MQTT para implementação do protocolo MQTT.
<br><br>
Além disso, a interface exibe o status de conexão do cliente remoto com o broker e o status de conexão com a estação de medição, indicando se está OFFLINE ou ONLINE. A verificação de status de conexão com a estação é feito por meio de um sistema de ping, ou seja, o cliente remoto envia um sinal para a estação, se esse sinal for recebido de volta pelo cliente remoto significa que está conectado com a estação, caso contrário, não.
<br><br>
A interface dispõe também de um campo o qual pode ser inserido o intervalo de medição (em segundos) desejado. Após inserir o intervalo desejado é só apertar no botão ao lado da caixa de diálogo que o intervalo é enviado para a estação de medição. Após o intervalo ser recebido por a estação, o valor desse intervalo é retornado para a interface e, então, é exibido o novo intervalo de medição escolhido. Deste modo, garantimos que o intervalo que está sendo exibido no cliente remoto é igual ao configurado na estação de medição.
<br><br>
Para utilizar a interface remota é necessário primeiramente dispor de algumas ferramentas, vamos lá.

Ferramentas necessárias:

- Python versão 3.8.10 (recomendado) ou superior instalado;

- Biblioteca Tkinter do Python para a interface gráfica;

- Gerenciador de pacotes pip do Python;

- Biblioteca Paho MQTT.

</p>

<h4  class="instalacao">Instalação do Python</h4>

<p  align="justify">

Para a instalação do Python pode ser seguido o guia disponível na própria página do Python: https://wiki.python.org/moin/BeginnersGuide/Download

</p>

<h4  class="instalacao">Biblioteca Tkinter</h4>

<p  align="justify">
A biblioteca para a construção de interfaces gráficas Tkinter do Python vem por padrão ao efetuar a instalação do Python. Contudo, em alguns sistemas operacionais como Linux e Mac, eventualmente, pode ocorrer da biblioteca não ser instalada junto ao Python, nestes casos, é ncessãrio afetuar a instalação do Python via terminal especificando que a biblioteca Tkinter também deve ser instalada. Isso pode ser feito da seguinte maneira:
<br>

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

  

<h4  class="instalacao">Instalação do pip</h4>

<p  align="justify">
O pip é um gerenciador de pacotes do Python. Normalmente ele pode ser instalado junto a instalação padrão do Python, porém isso nem sempre ocorre.
<br><br>
Para verificar se o pip está instalado, pode ser executado o seguinte comando:

  

```bash
# == No terminal do linux ou no Prompt de comando do Windows ==

pip -V #Mostrará a versão do pip instalado no sistema 

```

  

Caso o comando não seja reconhecido, significa que o gerenciador de pacotes não encotra-se instalado no sistema.<br>
Para a instalação do Windows seguimos os seguintes passos:

- Baixar o arquivo "get-pip.py" (https://bootstrap.pypa.io/get-pip.py) e armazenar no diretório em que o Python está instalado.

- Abrir a pasta que o arquivo foi armazenado no Prompt de comando (CMD).

- Executar no CMD o comando:

  

```bash
# == No Prompt de comando do Windows ==

...> python get-pip.py # Instalação do gerenciador de pacotes pip
```

- Aguardar o fim da instalação

  

Para a instalação no Linux:

```bash
# == No terminal do Ubuntu/Debian ==

$ sudo apt-get update # 1. Atualiza os pacotes do sistema operacional

$ sudo apt-get install python3-pip # 2. Instala o pip

```

</p>

  

<h3  class="instalacao">Instalação da biblioteca Paho MQTT</h3>
<br>

<p  align="justify">
Na construção da interface remota foi utilizada a biblioteca MQTT Paho para efetuar as conexões e operações MQTT do sistema.
<br><br>
Essa biblioteca não é nativa do conjunto de bibliotecas no Python, sendo necessário efetuar a sua instalação por meio do gerenciador de pacotes pip do Python.
<br><br>
A instalação da biblioteca pode ser feita executando o comando:
<br><br>

```bash
# == No terminal do linux ou no Prompt de comando do Windows ==

pip install paho-mqtt #Instalação da biblioteca MQTT Paho
```

</p>

  
  

<h2  id="guia">Guia de uso</h2>

<p  align="justify">

Para utilizar o projeto é necessário:


1) Baixar este repositório

2) Compilar o programa em C

</p>

  

```bash
# Acessar a pasta problema_3/
$ cd problema_3/
```

<p  align="justify">
Na pasta problema_3 há dois arquivos, um do programa principal em C e outro da interface gráfica em Python. O código em C deve estar numa placa Raspberry que possua o kit de extensão de GPIO e display LCD, e então executar o comando <b>make</b> (compilar) e o <b>sudo ./main</b> para executar.<br>

</p>

```bash
# Compilar
$ make
# Executar o programa em C
$ sudo ./main
```
<br>

3) Para executar a aplicação remota em Python, basta acessar a pasta com o arquivo dashboad.py e executá-lo. Certifique-se de ter instalado no dispositivo todas as ferramentas necessárias.  <strong>No linux via terminal, a execução da aplicação pode ser feita da seguinte maneira:</strong>
<br><br>
  
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

<p  align="justify">
Com o programa em C sendo executado, as medições atuais serão exibidas a cada contagem do intervalo de tempo configurado, no mínimo 2 segundos.
</p>

 
<h2  id="demo" >Demonstração</h2>

  

Ao executar o programa em C será escrito no LCD a seguinte mensagem:

<div  id="image01" style="display: inline_block" align="center">

![LCD_INICIO](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/startLCD.gif  "LCD_INICIO")

GIF - Início do programa, LCD
</div>

<p  align="justify">
Primeiro é necessário apertar 1 dos 3 botões, para desativar a exibição das medições atuais e poder utilizar os menus da interface homem-máquina (IHM)
</p>


```
1º Botão

> Switch 1 = 0 -> Decrementar intervalo
> Switch 1 = 1 -> Incrementar intervalo

2º Botão

> Switch 3 = 0

Pressionar 1ª vez aparece mensagem como intervalo atual a ser enviado.
Pressionar 2ª para enviar realmente o intervalo.

> Switch 3 = 1
Retorna a exibição das medições atuais

3º Botão

> Switch 4 = 0 -> Decrementar indice do Histórico
> Switch 4 = 1 -> Incrementar indice do Histórico

Switch 2 - mudar para 1 - Testa e exibe conexão
```
<br>
As imagens a seguir mostram o que aparece no display LCD ao interagir com a interface IHM.
<br>
<br>

<div  id="image01"  style="display: inline_block"  align="center">

  ![MENU_IHM](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/menuIHM.jpeg  "MENU_IHM")

Imagem 1 : Ao pressionar um dos três botões para acessar o menu IHM
</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![INCREMENTAR_INTERVALO](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/intervaloINC.jpeg  "INCREMENTAR_INTERVALO")

Imagem 2 : 1º Botão - Incrementar intervalo

</div>
<br>

<div  id="image01"  style="display: inline_block"  align="center">

![DECREMENTAR_INTERVALO](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/intervaloDEC.jpeg  "DECREMENTAR_INTERVALO")

Imagem 3 : 1º Botão - Decrementar intervalo

</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![ENVIAR_INTERVALO1](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/enviarIntervalo1.jpeg  "ENVIAR_INTERVALO1")

Imagem 4 : 2º Botão - Confirmar enviar intervalo
</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![ENVIAR_INTERVALO2](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/enviarIntervalo2.jpeg  "ENVIAR_INTERVALO2")

Imagem 5 : 2º Botão -Intervalo enviado

</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![HISTORICO_1](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/historicoRasp1.jpeg  "HISTORICO_1")

Imagem 6 : 3º Botão - Histórico na Raspberry, "primeira página" de medições
</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![HISTORICO_2](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/historicoRasp2.jpeg  "HISTORICO_2")

Imagem 7 : 3º Botão - Histórico na Raspberry, "segunda página" de medições
</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![CONEXAO_BROKER](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/statusBROKER.jpeg  "CONEXAO_BROKER")

Imagem 8 : 2º Switch - Teste de conexão com o Broker
</div>
<br>

Na interface remota todas as informações do sistemas já estão disponíveis na página inicial, como mostram as imagens 9 e 10.

<div  id="image01" style="display: inline_block" align="center">

![INT_GRAFICA1](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/intGrafica9.jpeg  "INT_GRAFICA1")

Imagem 9 : Interface gráfica pt.1
</div>
<br>

<div  id="image01" style="display: inline_block" align="center">

![INT_GRAFICA2](https://raw.githubusercontent.com/gboanerges/pbl_sistemas_digitais/main/problema_3/assets/intGrafica10.jpeg  "INT_GRAFICA2")

Imagem 10 : Interface gráfica pt.2
</div>
<br>

<p  align="justify">
Identificação dos itens das imagens 9 e 10:

1. Mostra as medições atuais recebidas da estação de medição.
2. Mostra o histórico das 10 últimas medições recebidas na interface.
3. Mostra o intervalo de medição atual configurado na estação de medição.
4. Caixa de diálogo que permite enviar um novo intervalo de medição para estação.
5. Botão para o envio do intervalo inserido na caixa de diálogo
6. Status de conexão com o broker
7. Status de conexão da interface remota com a estação de medição.
8. Botão para reconexão com o broker, caso a conexão tenha sido perdida.
</p>

<h2  id="todo" >Recursos a serem implementados</h2>
 
- Exibir histórico na Raspberry de forma dinâmica.

- Garantir que os clientes remotos possuam o mesmo histórico de medições

- Utilizar timer da placa Raspberry para contagem do intervalo de tempo

<h2  id="considera" >Considerações finais</h2>

<p  align="justify">
O projeto cumpre os requisitos solicitados, embora haja espaço para melhorias, tanto na IHM, quanto na interface remota, de forma a melhorar a operação do sistema como um todo.
</p>


<h2  id="equipe" >Equipe</h2>
  
* Gustavo Boanerges

* Igor Soares

<h2  id="ref" >Referências externas</h2>

<p>Conteúdos que serviram de base para a construção do projeto:

- [HOW TO SETUP AN LCD ON THE RASPBERRY PI AND PROGRAM IT WITH C](https://www.circuitbasics.com/raspberry-pi-lcd-set-up-and-programming-in-c-with-wiringpi/)

- [HOW TO SET UP THE DHT11 HUMIDITY SENSOR ON THE RASPBERRY PI](https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-the-raspberry-pi/#:~:text=The%20DHT11%20has%20a%20surface,C)

- [Interrupts short and simple: Part 1 – Good programming practices](https://www.embedded.com/interrupts-short-and-simple-part-1-good-programming-practices/)
- [MQTT in Python | building MQTT dashboard](https://www.youtube.com/watch?v=IQBWMHMTTO8)
</p>
