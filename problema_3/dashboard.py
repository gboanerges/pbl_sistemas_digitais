# -*- coding: utf-8 -*-
#------ Imports ---------
from tkinter import *
from tkinter import messagebox
import json
import random
import time
from paho.mqtt import client as mqtt_client
import threading
import time
#-------- Fim imports -------



#----- LISTA E VARIÁVEIS DE DADOS ------
DESCONECTADO = 1 #Constante
sinalizaFecharPrograma = False
statusEstacaoFlag = "0"  #Variável para indicar se o cliente encontra-se conectado com a estação ou não
broker = '10.0.0.101' #broker
port = 1883  #Porta do broker
topics = [("sd/pbl3/medidas", 0), ("sd/pbl3/ping/resposta", 0), ("sd/pbl3/intervalo/new", 0)] #Tópicos que o cliente está inscrito
# generate client ID
client_id = f'python-mqtt-{random.randint(0, 1000)}'
username = 'aluno'
password = 'aluno*123'


tempH = [] #Lista para o histórico de medidas da temperatura
umidH = [] #Lista para o histórico de medidas da umidade
presH = [] #Lista para o histórico de medidas da pressão
lumiH = [] #Lista para o histórico de medidas da luminosidade
dataHora = [] #Lista para data e hora das medidas

intervalo = 2  #O intervalo inicial é dois por padrão

temp = ""  # Variável para guardar a medida atual de temperatura
umid = ""  # Variável para guardar a medida atual de umidade
pres = ""  # Variável para guardar a medida atual de pressão
lumi = ""  # Variável para guardar a medida atual de luminosidade
#--------- FIM MQTT DADOS ---------


#---------- Funções ----------
def connect_mqtt():
    try:
        def on_connect(client, userdata, flags, rc):
            setStausBroker(rc)
            if rc == 0:
                print("Connected to MQTT Broker!")
            else:
                print("Failed to connect, return code %d\n", rc)
                statusEstacao.config(text="OFFLINE", fg="red")

        def on_disconnect(client, userdata, rc):
            setStausBroker(DESCONECTADO)

        
        client = mqtt_client.Client(client_id, reconnect_on_failure=False)
        client.username_pw_set(username, password)
        client.on_connect = on_connect
        client.on_disconnect = on_disconnect
        client.connect(broker, port, keepalive=5)

        return client
    except:
        setStausBroker(DESCONECTADO)  #Se não conseguir acessar o servidor, informa que não está conectado ao Broker
        statusEstacao.config(text="OFFLINE", fg="red") 

def publish(client, intervaloSend, topic):
    msg = f"{str(intervaloSend)}"
    result = client.publish(topic, msg)
    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{topic}`")
    else:
        print(f"Failed to send message to topic {topic}")

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
        try:
            if (msg.topic == "sd/pbl3/medidas"):
                #Refencia as variáveis globais das medidas atuais
                global temp, umid, pres, lumi

                #carrega os dados recebido do broker
                m = json.loads(msg.payload.decode())
                temp = m["temperatura"]
                umid = m["umidade"]
                pres = m["pressao"]
                lumi = m["luminosidade"]
                horaData = m["datahora"]
                carregarDataHora(horaData) # Aramzena a data e hora na lista de data e hora das medidas recebidas
                
                #carregar a tela com as medidas
                carregarMedidasAtuais() # Exibe as medidas atuais na interface
                carregarHistoricoTemp() # Armazena a temperatura recebida na lista de histórico de temperaturas e carrega o histórico na tela
                carregarHistoricoUmid() # Armazena a umidade recebida na lista de histórico de umidade e carrega o histórico na tela
                carregarHistoricoPres() # Armazena a pressão recebida na lista de histórico de pressão e carrega o histórico na tela
                carregarHistoricoLumi() # Armazena a luminosidade recebida na lista de histórico de luminosidade e carrega o histórico na tela                
            elif (msg.topic == "sd/pbl3/intervalo/new"):
                inter = msg.payload.decode() #Recebe o novo intervalo da estação de medição
                carregarIntervalo(inter) #Atualiza o valor do intervalo na interface
            elif (msg.topic == "sd/pbl3/ping/resposta"):
                global statusEstacaoFlag #Flag para indicar se o cliente está conectado com a estação de medição
                statusEstacaoFlag = msg.payload.decode() #Insere o valor correspondete a está conectado com a estação recebido
        except:
            pass
    client.subscribe(topics)
    client.on_message = on_message

def setIntervalo():
    try:
        global intervalo
        intervaloTemp = int(intervaloBox.get())
        if (intervaloTemp < 2):
            messagebox.showerror("Intervalo inválido", "O intervalo selecionado é menor que 2 segundos")
        else:
            global client
            publish(client, intervaloTemp, "sd/pbl3/intervalo/send")
    except ValueError:
        messagebox.showerror("Intervalo inválido", "O intervalo selecionado não é um número inteiro")

def carregarHistoricoTemp():
    count = 1 # Variável índice da exibição dos dados do histórico
    if (len(tempH) < 10): #Verifica se o histórico já tem 10 medidas
        tempH.insert(0, temp) #Se não tiver insere a medida atual no histórico
    else:  # Se tiver, remove a medida mais antiga do histórico e adiciona a nova
        tempH.pop() # Remove a medida mais antiga
        tempH.insert(0, temp) # Insere a medida atual no histórico
    
    # Carrega o histórico na tela
    if (listBoxTemp.size() > 0):  #Se tiver algum elemento dentro do listBox remove
        listBoxTemp.delete(0,END)
    for temperatura in tempH: # Carrega os dados na listBox
        listBoxTemp.insert(END, f"{count} : " + temperatura + "°C" + f" | {dataHora[count-1]}")
        count += 1

def carregarHistoricoUmid():
    count = 1 # Variável índice da exibição dos dados do histórico
    if (len(umidH) < 10):
        umidH.insert(0, umid) # Insere a medida atual no histórico 
    else:
        umidH.pop() # Remove a medida mais antiga do histórico
        umidH.insert(0, umid) # Insere a medida atual no histórico 
    
    if (listBoxUmid.size() > 0): #Se tiver algum elemento dentro do listBox remove
        listBoxUmid.delete(0,END)
    for umidade in umidH: # Carrega os dados na listBox
        listBoxUmid.insert(END, f"{count} : " + umidade + " %" + f" | {dataHora[count-1]}")
        count += 1

def carregarHistoricoPres():
    count = 1 # Variável índice da exibição dos dados do histórico
    if (len(presH) < 10):
        presH.insert(0, pres) # Insere a medida atual no histórico 
    else:
        presH.pop() # Remove a medida mais antiga do histórico
        presH.insert(0, pres) # Insere a medida atual no histórico 
    
    if (listBoxPres.size() > 0): #Se tiver algum elemento dentro do listBox remove
        listBoxPres.delete(0,END)
    for pressao in presH: # Carrega os dados na listBox
        listBoxPres.insert(END, f"{count} : " + pressao + " V" + f" | {dataHora[count-1]}")
        count += 1

def carregarHistoricoLumi():
    count = 1
    if (len(lumiH) < 10):
        lumiH.insert(0, lumi) # Insere a medida atual no histórico 
    else:
        lumiH.pop() # Remove a medida mais antiga do histórico
        lumiH.insert(0, lumi) # Insere a medida atual no histórico

    if (listBoxLumi.size() > 0): #Se tiver algum elemento dentro do listBox remove
        listBoxLumi.delete(0,END)
    for luminosidade in lumiH: # Carrega os dados na listBox
        listBoxLumi.insert(END, f"{count} : " + luminosidade + " V" + f" | {dataHora[count-1]}")
        count += 1
        
def carregarDataHora(horaData):
    if (len(dataHora) < 10): # Verifica a se a lista tem menos de 10 datas
        dataHora.insert(0, horaData) # Se tiver menos de 10 insere na lista
    else: # Se não tiver, remove uma data do final da lista e adiciona a nova data no inicio da lista
        dataHora.pop()
        dataHora.insert(0, horaData)

def carregarIntervalo(intervaloNovo):
    global intervalo
    intervalo = intervaloNovo
    intervaloAtual.config(text=f"Intervalo de medição atual: {str(intervalo)} segundos")

def carregarMedidasAtuais():
    temperaturaValor.config(text=f"{temp} °C") # A medida atual é a última medida da lista de temperatura
    umidadeValor.config(text=f"{umid} %") # A medida atual é a última medida da lista de umidade
    pressaoValor.config(text=f"{pres} V") # A medida atual é a última medida da lista de pressão
    luminosidadeValor.config(text=f"{lumi} V") # A medida atual é a medida última da lista de luminosidade

def setStausBroker(statusB):
    if (statusB == 0): #CONECTADO
        statusBroker.configure(fg="green", text="CONECTADO COM O BROKER!")
        reconectar.place_forget()
        reconectar.config(state=DISABLED, text="", bg="#191a19", highlightbackground="#191a19")
        #Desativar o botão de reconectar
    else: #NÃO CONECTADO
        textoStatusBroker = "Não conectado ao Broker"
        statusBroker.config(fg="red", text="NÃO CONECTADO AO BROKER!")
        reconectar.config(state=NORMAL, text="RECONECTAR", bg="#ba440d", activebackground="#d44908") #ativar o botão de conectar

def send_verificacao_estacao():
    while True:
        try:
            global client
            client.publish("sd/pbl3/ping/pedido", "1")
            global statusEstacaoFlag
            statusEstacaoFlag = "0"
            time.sleep(2) #Em segundos
            if ("1" in statusEstacaoFlag):  #Verifica se recebeu 1 de volta
                statusEstacao.config(text="ONLINE", fg="green")
            else:
                statusEstacao.config(text="OFFLINE", fg="red")
        except:
            global sinalizaFecharPrograma
            if (sinalizaFecharPrograma):
                print("Exit...")
                exit()

def conectar_ao_broker():
    try:
        global client
        client = connect_mqtt()
        subscribe(client)
        client.loop_start()
    except:
        setStausBroker(DESCONECTADO) #Erro ao conectar no Broker

def run():
    global client, sinalizaFecharPrograma
    conectar_ao_broker()
    try:
        thread = threading.Thread(target=send_verificacao_estacao)
        thread.start()
        window.mainloop()

        #Tenta finalizar o client 
        client.loop_stop()
        sinalizaFecharPrograma = True
    except:  #Se o cliente não tiver conseguido acessar um broker, captura a execeção
        pass  #Não faz nada

# ----------- Fim funções ------------



#--------- Dados iniciais da tela -------------
window = Tk()
window.title("Dashboard")
window.geometry("1400x600")
window.resizable(True, False)
window.configure(bg="#191a19")
#---------- Fim dados tela ----------


#------- Medidas atuais ---------------
temperaturaAtual = Label(window, text="Temperatura atual", bg="#191a19", fg="white")
umidadeAtual = Label(window, text="Umidade atual", bg="#191a19", fg="white")
pressaoAtual = Label(window, text="Pressão atual", bg="#191a19", fg="white")
luminosidadeturaAtual = Label(window, text="Luminosidade atual", bg="#191a19", fg="white")

temperaturaAtual.grid(column=0, row=0, padx=35, pady=20)
umidadeAtual.grid(column=1, row=0, padx=35, pady=20)
pressaoAtual.grid(column=2, row=0, padx=35, pady=20)
luminosidadeturaAtual.grid(column=3, row=0, padx=35, pady=20)

#label temperatura
temperaturaValor = Label(window, text=" °C", font=("Helvetica", 16), bg="#191a19", fg="white")
temperaturaValor.grid(column=0, row=1)
#label umidade
umidadeValor = Label(window, text=" %", font=("Helvetica", 16), bg="#191a19", fg="white")
umidadeValor.grid(column=1, row=1)
#label pressão
pressaoValor = Label(window, text=" V", font=("Helvetica", 16), bg="#191a19", fg="white")
pressaoValor.grid(column=2, row=1)
#label luminosidade
luminosidadeValor = Label(window, text=" V", font=("Helvetica", 16), bg="#191a19", fg="white")
luminosidadeValor.grid(column=3, row=1)
# -------- fim medidas atuais ------------------


#---------- histórico --------------------------
temperaturaH = Label(window, text="Histórico de Temperatura (°C)", bg="#191a19", fg="white")
umidadeH = Label(window, text="Histórico de Umidade (%)", bg="#191a19", fg="white")
pressaoH = Label(window, text="Histórico de Pressão (V)", bg="#191a19", fg="white")
luminosidadeturaH = Label(window, text="Histórico de Luminosidade (V)", bg="#191a19", fg="white")

temperaturaH.grid(column=0, row=2, padx=35, pady=20)
umidadeH.grid(column=1, row=2, padx=35, pady=20)
pressaoH.grid(column=2, row=2, padx=35, pady=20)
luminosidadeturaH.grid(column=3, row=2, padx=35, pady=20)

#listbox temperatura
listBoxTemp = Listbox(window, bg="#191a19", fg="white", highlightbackground="#ba440d", width=35)
listBoxTemp.grid(column=0, row=3, padx=10)
#listbox umidade
listBoxUmid = Listbox(window, bg="#191a19", fg="white", highlightbackground="#ba440d", width=35)
listBoxUmid.grid(column=1, row=3, padx=10)
#listbox pressão
listBoxPres = Listbox(window, bg="#191a19", fg="white", highlightbackground="#ba440d", width=35)
listBoxPres.grid(column=2, row=3, padx=10)
#listbox luminosidade
listBoxLumi = Listbox(window, bg="#191a19", fg="white", highlightbackground="#ba440d", width=35)
listBoxLumi.grid(column=3, row=3, padx=10)
#------------- Fim histórico ------------------------


# ------------ Intervalo de medição ----------------
intervaloAtual = Label(window, text= f"Intervalo de medição atual: {str(intervalo)} segundos", bg="#191a19", fg="white")
intervaloAtual.grid(row=4, padx=35, pady=35)

textIntervalo = Label(window, text="Insira o intervalo de medição (em segundos)", bg="#191a19", fg="white")
textIntervalo.grid(column=0, row=5, padx=35, pady=35)

intervaloBox = Entry(window, width=30, bg="#191a19", highlightbackground="#ba440d", fg="white")
intervaloBox.grid(column=1, row=5, padx=35, pady=35)

botaoIntervalo = Button(window, text="Mudar intervalo", command=setIntervalo, bg="#ba440d", highlightbackground="#ba440d", relief=FLAT, activebackground="#d44908")
botaoIntervalo.grid(column=2, row=5, padx=35, pady=35)
# ------------ Fim intervalo medição ----------------


# ------------ Status conexão com a estação de medição ------------------
textoStatusEStacao = Label(window, text="Status de conexão com a estação:", bg="#191a19", fg="white")
textoStatusEStacao.grid(column=0, row=6)

statusEstacao = Label(window, text="", bg="#191a19", font="-weight bold -size 11")
statusEstacao.grid(column=1, row=6)
# ------------ Fim status conexão com a estação de medição -----------------


# ------------ Status conexão broker ----------
statusBroker = Label(window, bg="#191a19", font="-weight bold -size 11")
statusBroker.grid(column=3, row=5)

#Botão para reconexão
reconectar = Button(window, text="Reconectar", command=conectar_ao_broker, bg="#ba440d", highlightbackground="#ba440d", relief=FLAT, activebackground="#d44908")
reconectar.grid(column=3, row=6)
#------------ Fim Status conexão Broker -------------


#---- run ------
run()