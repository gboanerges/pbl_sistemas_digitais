/////////////////////////////////////////////////////////////////////
// Módulo controlador do envio de dados da UART.
// O módulo é reponsável por efetuar o envio de dados da UART.
/////////////////////////////////////////////////////////////////////
module UART_rs232_tx 
#(parameter CLKS_PER_BIT = 434) // 434 para 115200 baud rate
(Clk,Rst_n,TxEn,TxData,TxDone,Tx,NBits);	

input Clk, Rst_n, TxEn;	//Define inputs de 1 bit 
input [3:0]NBits;		//Define input de 4 bits para tamanho da mensagem
input [7:0]TxData;		//Define input de 8 bit que sera o dado a ser enviado

output Tx = 1'b1;
output TxDone;



// Variaveis da maquina de estados
parameter  IDLE = 1'b0, WRITE = 1'b1;	// 2 estados para a maquina WRITE e IDLE
reg  State, Next;			// Criando registradores para os estados
reg  TxDone = 1'b0;			// Variavel para notificar que acabou uma transmissao 
reg  Tx;				// Registrador de input para enviar o dado
reg write_enable = 1'b0;		//Variável utilizada para ativar ou desativar o processo de transmissão		
reg start_bit = 1'b1;			//Variavel utilizada para notificar que detectou ou nao START bit
reg stop_bit = 1'b0;			//Variavel utilizada para notificar que detectou ou nao STOP bit
reg [4:0] Bit = 5'b00000;		//Variável usada para o loop de escrita bit a bit (neste caso 8 bits entao 8)

reg [7:0] in_data=8'b00000000;		//Registre onde armazenamos os dados que chegaram com a entrada TxData e devem ser enviados
reg [1:0] R_edge;			// Variável para evitar debounce do pino de habilitar escrita
wire D_edge;				//Wire used to connect the D_edge

reg [8:0] counter = 9'b000000000;


///////////////////////////////STATE MACHINE////////////////////////////////
////////////////////////////////////////////////////////////////////////////
///////////////////////////////////Reset////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
always @ (posedge Clk or negedge Rst_n)			
begin
if (!Rst_n)	State <= IDLE;				// Se o pino de reset estiver baixo, chegamos ao estado inicial que e IDLE
else 		State <= Next;				// senao pula para o proximo estado Next
end




////////////////////////////////////////////////////////////////////////////
////////////////////////////Next step decision//////////////////////////////
////////////////////////////////////////////////////////////////////////////
/*  Cada vez que  "State or D_edge or TxData or TxDone"  mudarem seus valores
decide qual estado ir.
 - Se D_edge foi detectado, TxEn foi habilitado, comeca o processo de envio
 - Se TxDone esta 1, voltar para o IDLE esperando o proximo TxEn*/
always @ (State or D_edge or TxData or TxDone) 
begin
    case(State)	
	IDLE:	if(D_edge)		Next = WRITE;		// Se o estado for IDLE e D_edge se ativar, comeca o processo de escrita
		else			Next = IDLE;
	WRITE:	if(TxDone)		Next = IDLE;  		//Se esta no estado WRITE e TxDONE ficar 1, voltar para o IDLE e espera
		else			Next = WRITE;
	default 			Next = IDLE;
    endcase
end



////////////////////////////////////////////////////////////////////////////
///////////////////////////ENABLE WRITE OR NOT//////////////////////////////
////////////////////////////////////////////////////////////////////////////
always @ (State)
begin
    case (State)
	WRITE: begin
		write_enable <= 1'b1;	// se esta no estado WRITE, habilita a escrita
	end
	
	IDLE: begin
		write_enable <= 1'b0;	//se esta no estado IDLE , deshabilita a escrita
	end
    endcase
end





////////////////////////////////////////////////////////////////////////////
///////////////////////Write the data out on Tx pin/////////////////////////
////////////////////////////////////////////////////////////////////////////
/*Cada vez que contar a quantidade de pulsos definida no inicio do modulo, 434 para 115200 baudrate, 
e se o write_enable esta habilitado, conta novamente um pulso da baud. Primeiro seta o Tx para 0, que 
significa o START bit. Entao a cada pulso da baud rate muda o Tx para o valor de acordo com o valor "in_data" 
que e o dado a ser enviado. Fazendo isso deslocando o "in_data" com as linhas:
	in_data <= {1'b0,in_data[7:1]};
	Tx <= in_data[0]; 
*/


always @ (posedge Clk)
begin

	if (!write_enable)				// Se write_enable nao esta ativado, reseta todas as variaveis para o proximo TxEn loop
		begin
		TxDone = 1'b0;
		start_bit <=1'b1;
		stop_bit <= 1'b0;
		end

	if (write_enable)				//Se write_enable esta ativado, comeca a contar e mudar a saida Tx
	begin
	counter <= counter+1;				//Incrementando o contador 
	
	
	if(start_bit & !stop_bit)			//Setar o Tx para 0 (start bit) e passa a entrada TxData para o registrador in:data
	begin
	Tx <=1'b0;					//start bit  
	in_data <= TxData;				//Passe os dados a serem enviados para o registrador in_data para que possamos usá-lo
	end		

	if ((counter == CLKS_PER_BIT) & (start_bit) )	//se o contador chegar a qtd de pulsos definida, cria o primeiro bit e seta o start_bit para 0
	begin		
	start_bit <= 1'b0;
	in_data <= {1'b0,in_data[7:1]};
	Tx <= in_data[0];
	//counter <= 4'b0000;
	end


	if ((counter == CLKS_PER_BIT) & (!start_bit) &  (Bit < NBits-1))	//se o contador chegar a qtd de pulsos definida novamente, fica em loop para os proximos 7 bits
	begin		
	in_data <= {1'b0,in_data[7:1]};
	Bit<=Bit+1;
	Tx <= in_data[0];
	start_bit <= 1'b0;
	counter <= 4'b0000;
	end	



	
	if ((counter == CLKS_PER_BIT) & (Bit == NBits-1) & (!stop_bit))	//Terminando de enviar seta Tx para 1 (Stop bit)
	begin
	Tx <= 1'b1;	
	counter <= 4'b0000;	
	stop_bit<=1'b1;
	end

	if ((counter == CLKS_PER_BIT) & (Bit == NBits-1) & (stop_bit) )	//Se o stop bit estiver ativado, redefinimos os valores e aguardamos o processo de escrita TxEn
	begin
	Bit <= 4'b0000;
	TxDone <= 1'b1;
	counter <= 4'b0000;
	//start_bit <=1'b1;
	end
	
	end
		

end



////////////////////////////////////////////////////////////////////////////
////////////////////////////Input enable detect/////////////////////////////
////////////////////////////////////////////////////////////////////////////
/*Aqui detectamos se houve reset ou se o TxEn foi ativado.
Se "TxEn" foi ativado, iniciamos o processo de escrita e enviaremos
os dados que estão na entrada "TxData", portanto, certifique-se de que no
momento em que você ativa "TxEn" o TxData" tem os valores que deseja enviar. */
always @ (posedge Clk or negedge Rst_n)
begin

	if(!Rst_n)
	begin
	R_edge <= 2'b00;
	end
	
	else
	begin
	R_edge <={R_edge[0], TxEn};
	end
end
assign D_edge = !R_edge[1] & R_edge[0];





endmodule
