
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
• <a href="#python">Python</a> <br>
• <a href="#instalacao">Guia de uso</a>  <br>  
• <a href="#demo">Demonstração</a>  <br>  
• <a href="#todo">Recursos a serem implementados</a>   <br>
• <a href="#considera">Considerações finais</a>   <br>
• <a href="#equipe">Equipe</a>  <br>
• <a href="#ref">Referências externas</a>  

</p>

<h4 align="center"> 🚧  Em construção  🚧</h4>

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

 
<h2  id="problema">Problema</h2>

<p align="justify">
Implementar um protótipo de um sistema para monitoramento ambiental que será posteriormente integrado a um sistema para monitoramento de cidades. O protótipo deve incluir todo o tratamento e controle de sensores analógicos e digitais, bem como uma IHM (interface Homem-Máquina) para apresentação das informações,incluindo históricos dos dados. O protótipo deve ser desenvolvido num SBC que medirá temperatura, umidade, pressão atmosférica e luminosidade. A IHM deve apresentar, em tempo real, as leituras atuais. Ela também deve permitir a visualização do histórico com as 10 últimas medições de cada sensor. O sistema deve permitir o ajuste local e remoto do intervalo de tempo que serão realizadas as medições.
 </p>

<h2 id="mqtt">MQTT</h2>

<p align="justify">

MQTT

</p>

<h2 id="python">Python</h2>

<p align="justify">

PYTHON

</p>
 
<h2 id="instalacao"> Guia de uso</h2>

Para utilizar o projeto é necessário:

1) Baixar este repositório

2) Compilar o programa em C
```bash
# Acessar a pasta problema_3/

$ cd problema_3/
```
 <p align="justify">
Na pasta problema_3 há dois arquivos, um do programa principal em C e outro da interface gráfica em Python. O código em C deve estar numa placa Raspberry que possua o KIT COM LCD (QUAL O NOME DISSO), e então executar o comando <b>make</b> (compilar) e o <b>sudo ./main</b>  para executar.
</p>

```bash
# Compilar

$ make

# Executar o programa em C

$ sudo ./main
```
<p align="justify">
3) Em um computador, executar o arquivo Python para abrir a interface gráfica.
</p>

---
<p align="justify">
Com o programa em C sendo executado, as medições atuais serão exibidas a cada contagem do intervalo de tempo configurado, no mínimo 2 segundos.

</p>

<h2 id="demo" >Demonstração</h2>


<div id="image01" style="display: inline_block" align="center">
	
</div>
	

<h2 id="todo" >Recursos a serem implementados</h2>




<h2 id="considera" >Considerações finais</h2>

<p align="justify">
O projeto, até então, ainda não cumpre todas as funcionalidades desejadas. Porém, a atual implementação permite suprimir parte dos requisitos.

</p>

<h2 id="equipe" >Equipe</h2>

* Gustavo Boanerges
* Igor Soares

<h2 id="ref" >Referências externas</h2>
<p>Conteúdos que serviram de base para a construção do projeto:
 

</p>
