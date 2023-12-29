/*****************************************************************/
/*  I2C.C                                                        */
/*  Biblioteca I2C - Comunicação I2C por software com suporte    */
/*  a memórias EEPROM (modo mestre)                              */
/*                                                               */
/*  Autor: Fábio Pereira                                         */
/*                                                               */
/*****************************************************************/

#ifndef scl
	// Definições dos pinos de comunicação
	#define scl  pin_b1		     // pino de clock
	#define sda  pin_b0		     // pino de dados
#endif

#ifndef EEPROM_SIZE
   #define EEPROM_SIZE 8192    // tamanho em bytes da memória EEPROM (8192 x 8bits)
#endif

#define seta_scl   output_float(scl)		// seta o pino scl
#define apaga_scl  output_low(scl)		   // apaga o pino scl
#define seta_sda   output_float(sda)		// seta o pino sda
#define apaga_sda  output_low(sda)	    	// apaga o pino sda

void I2C_start(void)
// coloca o barramento na condição de start
{
	apaga_scl;  // coloca a linha de clock em nível 0
	seta_sda;	// coloca a linha de dados em alta impedância (1)
	seta_scl;	// coloca a linha de clock em alta impedância (1)
	apaga_sda;	// coloca a linha de dados em nível 0
	apaga_scl;	// coloca a linha de clock em nível 0
}
void I2C_stop(void)
// coloca o barramento na condição de stop
{
	apaga_scl;	// coloca a linha de clock em nível 0
	apaga_sda;	// coloca a linha de dados em nível 0
	seta_scl;	// coloca a linha de clock em alta impedância (1)
	seta_sda;	// coloca a linha de dados em alta impedância (1)
}
void i2c_ack()
// coloca sinal de reconhecimento (ack) no barramento
{
	apaga_sda;	// coloca a linha de dados em nível 0
	seta_scl;	// coloca a linha de clock em alta impedância (1)
	apaga_scl;	// coloca a linha de clock em nível 0
	seta_sda;	// coloca a linha de dados em alta impedância (1)
}
void i2c_nack()
// coloca sinal de não reconhecimento (nack) no barramento
{
	seta_sda;	// coloca a linha de dados em alta impedância (1)
	seta_scl;	// coloca a linha de clock em alta impedância (1)
	apaga_scl;	// coloca a linha de clock em nível 0
}

// Verifica se a memoria esta OK
BOOLEAN ext_eeprom_ready() {
   int1 ack;
   i2c_start();            // Se o comando de escrita for reconhecido,
   ack = i2c_write(0xa0);  // entao o dispositivo esta pronto.
   i2c_stop();
   return !ack;
}

boolean i2c_le_ack()
// efetua a leitura do sinal de ack/nack
{
	boolean estado;
	seta_sda; 	// coloca a linha de dados em alta impedância (1)
	seta_scl;	// coloca a linha de clock em alta impedância (1)
	estado = input(sda);	// lê o bit (ack/nack)
	apaga_scl;	// coloca a linha de clock em nível 0
	return estado;
}
void I2C_escreve_byte(unsigned char dado)
{
// envia um byte pelo barramento I2C
	int conta=8;
	apaga_scl;		// coloca SCL em 0
	while (conta)
	{
		// envia primeiro o MSB
		if (shift_left(&dado,1,0)) seta_sda; else apaga_sda;
		// dá um pulso em scl
		seta_scl;
		conta--;
		apaga_scl;
	}
	// ativa sda
	seta_sda;
}
unsigned char I2C_le_byte()
// recebe um byte pelo barramento I2C
{
	unsigned char bytelido, conta = 8;
	bytelido = 0;
	apaga_scl;
	seta_sda;
	while (conta)
	{
		// ativa scl
		seta_scl;
		// lê o bit em sda, deslocando em bytelido
		shift_left(&bytelido,1,input(sda));
		conta--;
		// desativa scl
		apaga_scl;
	}
	return bytelido;
}

void escreve_eeprom(byte dispositivo, long endereco, byte dado)
// Escreve um dado em um endereço do dispositivo
// dispositivo - é o endereço do dispositivo escravo (0 - 7)
// endereco - é o endereço da memória a ser escrito
// dado - é a informação a ser armazenada
{
/* Funcao antiga retirada do livro
	if (dispositivo>7) dispositivo = 7;
   i2c_start();
   i2c_escreve_byte(0xa0 | (dispositivo << 1)); // endereça o dispositivo
   i2c_le_ack();
   i2c_escreve_byte(endereco >> 8);	// parte alta do endereço
   i2c_le_ack();
   i2c_escreve_byte(endereco);	// parte baixa do endereço
   i2c_le_ack();
   i2c_escreve_byte(dado);			// dado a ser escrito
   i2c_le_ack();
   i2c_stop();
   delay_ms(10); // aguarda a programação da memória
*/
   //Funcao atual retirada dos manuais do C CSS
   while(!ext_eeprom_ready());
   i2c_start();
   i2c_write(0xa0 | (dispositivo << 1));
   i2c_write(endereco >> 8);
   i2c_write(endereco);
   i2c_write(dado);
   i2c_stop();
   delay_ms(10);
}

byte le_eeprom(byte dispositivo, long int endereco)
// Lê um dado de um endereço especificado no dispositivo
// dispositivo - é o endereço do dispositivo escravo (0 - 7)
// endereco - é o endereço da memória a ser escrito
{
   byte dado;
	if (dispositivo>7) dispositivo = 7;
   i2c_start();
   i2c_escreve_byte(0xa0 | (dispositivo << 1)); // endereça o dispositivo
   i2c_le_ack();
   i2c_escreve_byte((endereco >> 8));	// envia a parte alta do endereço
   i2c_le_ack();
   i2c_escreve_byte(endereco);	// envia a parte baixa do endereço
   i2c_le_ack();
   i2c_start();
	//envia comando de leitura
   i2c_escreve_byte(0xa1 | (dispositivo << 1));
   i2c_le_ack();
   dado = i2c_le_byte();	// lê o dado
	i2c_nack();
   i2c_stop();
   return dado;
}

// Lê um dado de 16 bits um endereço especificado no dispositivo
// dispositivo - é o endereço do dispositivo escravo (0 - 7)
// endereco - é o endereço da memória a ser escrito

long int le_eeprom_16(byte dispositivo,long endereco){
  int8 pLow, pHigh;
  long resultado;
  pHigh = le_eeprom(dispositivo,endereco);
  endereco++;
  pLow =  le_eeprom(dispositivo,endereco);
  resultado=(pHigh<<8);
  resultado+=pLow;
  return resultado;
}

// Escreve um dado em um endereço do dispositivo
// dispositivo - é o endereço do dispositivo escravo (0 - 7)
// endereco - é o endereço da memória a ser escrito
// dado - é a informação a ser armazenada

void escreve_eeprom_16(byte dispositivo,long endereco, long dado){
  int8 pLow, pHigh;
  pLow = dado;
  pHigh = (dado >> 8);
  escreve_eeprom(dispositivo,endereco,pHigh);
  delay_ms(12);
  endereco++;
  escreve_eeprom(dispositivo,endereco,pLow);
  delay_ms(12);
}

