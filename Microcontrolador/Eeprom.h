/*******************************************************************************
*             FUNCOES ESPECIFICAS PARA GRAVAR FLOAT NA EEPROM                  *
*******************************************************************************/

// Used to adjust the address range
#ifndef INT_EEPROM_ADDRESS
   #define INT_EEPROM_ADDRESS int8
#endif

// Escreve um número de ponto flutuante na EEPROM interna
// Parametros 1) Um endereço da EEPROM (4 Endereços são usados para Float)
//            2) O número flutuante a ser escrito na eeprom

void write_float_eeprom(INT_EEPROM_ADDRESS address, float data){

  int8 i;

  for(i = 0; i < 4; ++i){
    write_eeprom(address + i,*(&data + i));
  }
}

// Lê um número de ponto flutuante da eeprom
// Parametros: Um endereço da EEPROM (endereço inicial)
// Retorna um ponto flutuante que foi lido no endereço da EEPROM

float read_float_eeprom(INT_EEPROM_ADDRESS address){

  int8 i;
  float data;

  for(i = 0; i < 4; ++i){
    *(&data + i) = read_eeprom(address + i);
  }
  return data;
}

// Lê um número de 16 bits da eeprom interna
// Parametros: Um endereço da EEPROM (endereço inicial)
// Retorna um inteiro long que foi lido no endereço da EEPROM

long int read_eeprom_16(long int endereco){
  int8 pLow, pHigh;
  long resultado;
  pHigh = read_eeprom(endereco);
  endereco++;
  pLow =  read_eeprom(endereco);
  resultado=(pHigh<<8);
  resultado+=pLow;
  return resultado;
}

// Escreve um dado de 16 bits na eeprom interna
// endereco - é o endereço da memória a ser escrito
// dado - é a informação a ser armazenada

void write_eeprom_16(int endereco, long dado){
  int8 pLow, pHigh;
  pLow = dado;
  pHigh = (dado >> 8);
  write_eeprom(endereco,pHigh);
  delay_ms(12);
  endereco++;
  write_eeprom(endereco,pLow);
  delay_ms(12);
}

