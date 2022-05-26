//////////////////////////////////////////////////////////////////
// Módulo de controle do sistema.
// O módulo é responsável por verificar o dado recebido da SBC, 
// ativar o sensor e enviar as informações recebidas
// do sensor de volta para o SBC.
/////////////////////////////////////////////////////////////////

module control3(
	Clk,
	Enable_tx,
	Rx_done,
	Rx_data,
	TxData,
	Tx_done,
	Enable_dht11,
	Umi_int,
	Umi_float,
	Temp_int,
	Temp_float,
	CRC,
	Error,
	Wait_dht11,
	Rst,
);

	//inputs
	input Clk, Rx_done, Tx_done, Rst;
	input [7:0] Rx_data;  //Dado solicitado pela raspberry
	input [7:0] Umi_int;  //Parte inteira da umidade
	input [7:0] Umi_float; //Parte fracionada da umidade
	input [7:0] Temp_int;  //Parte inteira da temperatura
	input [7:0] Temp_float; //Parte fracionada da temperatura
	input [7:0] CRC;  //Somatorio - erro
	input Wait_dht11; 
	input Error;

	//output
	output reg Enable_tx = 1'b0;
	output reg Enable_dht11 = 1'b0;
	output reg [7:0] TxData;

	//Constantes
	parameter dado1 = 3'b000,  //estado inicial
				 //dado2 = 3'b001,  //estado de aquisição de dado 1
				 dht11_comu = 3'b001,  //estado de aquisição de dado 2
				 wait_1 = 3'b010, //estado de espera de resposta do dht11
				 resposta1 = 3'b011, //estado de envio de resposta1 para a raspberry
				 wait_2 = 3'b100,  //estado de espera para a transmissão do segundo dado
				 resposta2 = 3'b101; //estado de enivo de resposta2 para a raspberry

	reg [2:0] state, next;
	//Contador wait
	
	reg [26:0] counter_response = 27'b000000000000000000000000000;  //Contador para o tempo de resposta do DHT11
	reg temp_response = 1'b0;  //Sinaliza que o tempo de espera de resposta do DHT11 acabou
	//Contador envio
	
	reg [26:0] counter_dados = 27'b000000000000000000000000000;
	reg temp_dados = 1'b0;
	
	//Contador atribuição de dados
	reg [4:0] counter_atribuir = 5'b00000;
	reg atribuir = 1'b0;
	
	//recpção de dados
	reg [7:0] Rx_data1;  //Dado 1 recebido da raspberry
	//reg [7:0] Rx_data2;  //Dado 2 recebido da raspberry
	
	//Envio de dados
	reg [7:0] Tx_data1;  //Dado 1 a ser enviado para raspberry
	reg [7:0] Tx_data2;  //Dado 2 a ser enviado para raspberry
	
	//Estado inicial
	initial begin
		state = dado1;
	end
	
	//Contador para o estado de aquisição de dados do DHT11
	always @(posedge Clk or negedge Rst) begin
		if (!Rst) begin
			counter_response <= 27'b000000000000000000000000000;  //Reseta
			temp_response <= 1'b0;
		end else if(state == dht11_comu) begin //inicializa a contagem quando a máquina estiver no estado de dht11_coum
			counter_response <= counter_response + 1;  //conta mais 1 se não tiver reset
			if (counter_response == 27'b101111101011110000100000000) begin //verifica se o contador atingiu 1 segundo
				temp_response <= 1'b1;  //se atingiu sinaliza
				counter_response <= 27'b000000000000000000000000000;  //zera o contador
			end
		end else begin
			temp_response <= 1'b0;
		end
	end
	
	//Contador envio de dados
	
	always @(posedge Clk or negedge Rst)	 
	begin
		if (!Rst) begin
			counter_dados <= 27'b000000000000000000000000000;  //Reseta
			temp_dados <= 1'b0;
		end
		else if(state == wait_2) begin  //inicializa a contagem quando a máquina estiver no estado de wait_2
			counter_dados <= counter_dados + 1;  //conta mais 1 se não tiver reset
			if (counter_dados == 27'b101111101011110000100000000) begin //verifica se o contador atingiu 1s
				temp_dados <= 1'b1;  //se atingiu sinaliza
				counter_dados <= 27'b000000000000000000000000000; //59'b00000000000000000000000000000000000000000000000000000000000;  //zera o contador
			end
		end
		else temp_dados <= 1'b0;
	end
	
	//Contador atribuir dados
	always @(posedge Clk or negedge Rst) begin
		if (!Rst) begin
			counter_atribuir <= 5'b00000; //59'b00000000000000000000000000000000000000000000000000000000000;  //verifica o reset
			atribuir <= 1'b0;
		end else if(state == wait_1) begin
			counter_atribuir <= counter_atribuir + 1;  //conta mais um se n tiver reset
			if (counter_atribuir == 5'b11111) begin //27'b101111101011110000100000000 //59'b11011110000010110110101100111010011101100100000000000000000) begin //verifica se o contador atingiu 10s
				atribuir <= 1'b1;  //se atingiu sinaliza
				counter_atribuir <= 27'b000000000000000000000000000; //59'b00000000000000000000000000000000000000000000000000000000000;  //zera o contador
			end
		end else begin
			atribuir <= 1'b0;
		end
	end

	always @(posedge Clk) begin  //state
		if (!Rst) begin
			state = dado1;
			Enable_tx <= 1'b0;
			Rx_data1 = 8'b00000000;
			Tx_data1 = 8'b00000000;
			Tx_data2 = 8'b00000000;
		end
		else begin
			case (state)
				dado1: begin
					if (Rx_done == 1'b1)begin
						Rx_data1 <= Rx_data;
						Enable_dht11 <= 1'b1;
						state = dht11_comu; //muda de estado
					end
					else begin
						Enable_tx <= 1'b0;  //desabilita a transmissão
						state = state; //permanece no estado
					end
				end
				dht11_comu: begin  //Recebe os dados
					if (!Wait_dht11) begin //Aguarda o DHT11 efetuar a leitura dos dados
						state = wait_1;  //Muda para o estado de espera 1
					end
					else begin
						Enable_dht11 <= 1'b1;
						state = state;
					end
				end
				
				wait_1: begin  //Esse estado é responsável por setar para o envio a medida solicitada (temperatura, umidade e estado do sensor)
					if (Rx_data1 == 8'b000000011)begin
						if (Error)begin  //Se o sensor estiver com problema, retorna 2 bytes com o valor 31.
							TxData <= 8'b00011111; //31
							Tx_data2 <= 8'b00011111; //31
						end
						else begin  //Se o sensor estiver funcionando normalmente, retorna 102 - esse código de erro foi alterado porque zero (definido na descrição do problema) poderia ser confundido com o não recebimento de dados no SBC da FPGA
							Tx_data1 <= 8'b01100110; //102
							Tx_data2 <= 8'b01100110; //102
						end
						if (atribuir) state = resposta1;  //aguarda cerca de 1,55ms para mudar de estado - evitar problemas na atribuição de dados
						else state = state;
					end
					else if (Rx_data1 == 8'b00000100) begin
						TxData <= Temp_int;
						Tx_data2 <= Temp_float;
						if (atribuir) state = resposta1; 
						else state = state;
					end
					else if (Rx_data1 == 8'b00000101) begin
						TxData <= Umi_int;
						Tx_data2 <= Umi_float;
						if (atribuir) state = resposta1; 
						else state = state;
					end
					else begin //Se o comando recebido do SBC não corresponder a nenhum dos dados, retorna 126 (decimal) nos dois byte indicando erro
						TxData <= 8'b01111110;  //126
						Tx_data2 <= 8'b01111110; //0
						if (atribuir) state = resposta1; 
						else state = state;
					end
				end
				
				resposta1: begin  //Habilita a transmissão do primeiro byte
					if (!Tx_done) begin
						Enable_tx <= 1'b1;
						state = state;
					end
					else begin 
						Enable_tx <= 1'b0;
						state = wait_2;
					end
				end
				
				wait_2: begin
					if (!temp_dados) begin  //Espera 1 seg para enviar o segundo dado
						state = state;	
					end
					else begin
						TxData <= Tx_data2;
						state = resposta2;
					end
				end
				
				resposta2: begin //habilita a transmissão do segundo byte
					if (!Tx_done) begin
						Enable_tx <= 1'b1;
						state = state;
					end
					else begin
						Enable_tx <= 1'b0;
						state = dado1;
					end
					  //Desabilita o DHT11
					  Enable_dht11 <= 1'b0;
				end
				default:	state = dado1;
			endcase
		end
	end
endmodule
