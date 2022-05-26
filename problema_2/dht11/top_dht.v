////////////////////////////////////////////////////////////////////
// Módulo principal que instância todos os outros módulos e conecta
// uns aos outros.
////////////////////////////////////////////////////////////////////
module top_dht(
   Clk                     ,
   Rst_n                   ,   
   Rx                      ,    
   Tx                      ,
	RxData		        		,
	col							,
	dht11							,
	);

/////////////////////////////////////////////////////////////////////////////////////////
input           Clk             ; // Clock
input           Rst_n           ; // Reset
input           Rx              ; // RX
output          Tx              ; // TX
output [7:0]    RxData          ; // Dado recebido
/////////////////////////////////////////////////////////////////////////////////////////
wire          	 RxDone          ; // Indicador de recepção de dados finalizada. Dado recebido
wire          	 TxDone          ; // Indicador de transmissão de dados finalizadas. Dado enviado
wire 				 RxEn				  ; // Fio para ativação da transmissão de dados
wire [3:0]      NBits    	     ; // Fio para o número de bits da mensagem a ser enviada/recebida
/////////////////////////////////////////////////////////////////////////////////////////
assign 			 RxEn = 1'b1	;
assign 			 NBits = 4'b1000	;	//Envio/recebimento de 8 bits
/////////////////////////////////////////////////////////////////////////////////////////
//Ativa a primeira coluna da matriz de led;
output reg col = 1'b0;

//ativadores
wire TxEn; 

//inout
inout dht11; //Entrada/saída DHT11

//wires dht11
wire [7:0] TEMP_INT, TEMP_FLOAT, HUM, HUM_FLOAT, CRC, TxData, RxD;
wire DhtEn, DataDht11, CrcDht11, Error, Wait_dht11; 

reg dh = 1'b0;

assign RxD = RxData;
assign Error = ErrorDHT11;
assign Wait_dht11 = WAIT_DHT11;

//Cria a conexão entre o módulo Rx, os inputs e outputs do módulo principal e outros módulos
UART_rs232_rx I_RS232RX(
    	.Clk(Clk)             	,
   	.Rst_n(Rst_n)         	,
    	.RxEn(RxEn)           	,
    	.RxData(RxData)       	,
    	.RxDone(RxDone)       	,
    	.Rx(Rx)               	,
    	.NBits(NBits)
    );

//Cria a conexão entre o módulo Tx, os inputs e outputs do módulo principal e outros módulos
UART_rs232_tx I_RS232TX(
   	.Clk(Clk)            	,
    	.Rst_n(Rst_n)         	,
    	.TxEn(TxEn)           	,
    	.TxData(TxData)      	,			
   	.TxDone(TxDone)      	,
   	.Tx(Tx)               	,
   	.NBits(NBits)
    );
	 
//Cria a conexão entre o módulo do DHT11, os inputs e outputs do módulo principal e outros módulos
DHT11 DHT11(
		 .CLK(Clk),  //50 MHz
		 .EN(DhtEn),
		 .RST(Rst_n),
		 .DHT_DATA(dht11),
		 .HUM_INT(HUM),  //HUM_INT
		 .HUM_FLOAT(HUM_FLOAT),
		 .TEMP_INT(TEMP_INT),
		 .TEMP_FLOAT(TEMP_FLOAT),
		 .CRC(CRC),
		 .WAIT(WAIT_DHT11),
		 .error_out(ErrorDHT11),
		 .DEBUG(PMOD2_P3)
	);

//Cria a conexão entre o módulo Tx, os inputs e outputs do módulo principal e outros módulos
control3 control3(
	.Clk(Clk),
	.Enable_tx(TxEn),
	.Rx_done(RxDone),
	.Rx_data(RxD),
	.TxData(TxData),
	.Tx_done(TxDone),
	.Enable_dht11(DhtEn),
	.Umi_int(HUM),
	.Umi_float(HUM_FLOAT),
	.Temp_int(TEMP_INT),
	.Temp_float(TEMP_FLOAT),
	.CRC(CRC),
	.Error(Error),
	.Wait_dht11(Wait_dht11),
	.Rst(Rst_n),
);

endmodule
