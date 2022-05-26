///////////////////////////////////////////////////////////////////////
// Módulo responsável por efetuar o tristate.
// O módulo é utilizado para mudar o sentido do fluxo de dados da entra/saída (inout)
// do DHT11.
////////////////////////////////////////////////////////////////////////
module TRI_STATE (
	inout PORT,
	input DIR,
	input SEND,
	output READ
);


	assign PORT = DIR ? SEND : 1'bZ;
	assign READ = DIR ? 1'bz : PORT;
	
endmodule 