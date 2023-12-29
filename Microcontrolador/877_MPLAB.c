//Arquivo de definição para o 16F877
#include <16f877.h>

//Ajusta os fusiveis (XT=Oscilador de Cristal) (No Watch Dog Timer) (Power Up Time)
#fuses XT,NOWDT,PUT

//Configura a velocidade para 4MHz
#use delay(clock=4000000)
//Configura o módulo interno de comunicação Serial com o PC
//#use rs232(baud=9600, xmit=pin_c6, rcv=pin_c7)
//Configura modulo interno I2C
#use i2c(master, sda=pin_d0, scl=pin_d1)

//Define os pinos do bus I2C
#define sda          pin_d0
#define scl          pin_d1

//Definição dos pinos para o display
#define  lcd_enable  pin_e1   //Pino de ENABLE do LCD
#define  lcd_rs      pin_e0   //Pino de RS do LCD
#define  lcd_db4     pin_d4   //Pino de dados d4 do LCD
#define  lcd_db5     pin_d5   //Pino de dados d5 do LCD
#define  lcd_db6     pin_d6   //Pino de dados d6 do LCD
#define  lcd_db7     pin_d7   //Pino de dados d7 do LCD

// Define o pino do Led piscante
#define led          pin_b1

//Arquivo de definição para o uso de display LCD
#include <mod_lcd.h>
//Arquivo com biblioteca de funcoes I2C
#include <i2c.h>
//Arquivo com biblioteca para o RTC I2C
#include <PCF_RTC.c>

//Arquivo de definição para o uso de funcoes matematicas especiais
#include <math.h>
//Arquivo de definição de variáveis especiais na EEPROM
#include <eeprom.h>


/*******************************************************************************
*                          FUNCAO PRINCIPAL DO PROGRAMA                        *
*******************************************************************************/

void main()
{
   //Flag de estado do LED
   boolean flag,fTemp;

   //Variavel que vai guardar o valor convertido
   long int valor;

   //Variavel que controla a aquisição de temperatura 1 a cada 3 segundos
   int8 conta;

   //Variavel tipo estrutura que recebera os dados do RTC
   date_time_t dt;

   //Variavel de ponto flutuante para armazenar as temperaturas
   float Temp,T_Max,T_Min;

   //Define entradas e saídas em cada Port
   // bit        76543210
   SET_TRIS_B( 0b00000000 );
   SET_TRIS_D( 0b00000000 );  //Port onde está os dados do LCD
   //SET_TRIS_E( 0b00000000 );  //Port onde está os comandos do LCD
   //SET_TRIS_C( 0b00000000 );  //Port onde está o tacometro

   //Faz as definicoes do conversor A/D
   setup_ADC_ports (AN0);
   setup_adc(ADC_CLOCK_INTERNAL);
   set_adc_channel(0);

   //define o valor do flag do led
   flag=1;
   //Usada para quando ligar igualar T_Max com Temp e T_Min com Temp
   fTemp=1;

   //Leva todas as saídas de todos os ports a 0 por causa do LCD
   portB=0;
   portD=0;
   output_low(pin_e0);
   output_low(pin_e1);

   //Inicializa o LCD
   inicializa_lcd();

   //Escreve na tela
   printf(escreve_lcd," Prototipo 2.10 ");
   //Aguarda 2 segundos
   delay_ms(2000);
   //Limpa a tela e aguarda a digitação
   limpa_lcd();

   //Inicializa o RTC
   //PCF8583_init();

   //Inicializa a variavel conta com 2 para atuar já na 1a passagem
   conta=2;

   //Entra no loop contínuo
   while (true)
   {
      //Incrementa a variavel conta (2+1=3)
      conta++;

      //Realiza a operação com o led
      output_bit(led,flag);
      //Inverte o estado do Led
      flag = !flag;

      if (!(conta % 3)) {
         //Define o primeiro canal analógico para uso
         set_adc_channel(0);

         //Realiza a conversão do ADC
         valor = read_adc();
         //Calcula a temperatura pela equação de regressão estimada
         Temp = (float) ((103.77 * log(valor)) - 670.81);

         //Se for a primeira vez que passa aqui
         if (fTemp){
            T_Max=Temp;
            T_Min=Temp;
            write_float_eeprom(1,T_Max);
            write_float_eeprom(5,T_Min);
            fTemp=0;
         }

         //Verifica se a temperatura é maxima ou minima e grava na EEPROM
         if (Temp > T_Max) {
            write_float_eeprom(1,Temp);
         }
         if (Temp < T_Min) {
            write_float_eeprom(5,Temp);
         }

         //Le os valores de max e min na eeprom
         T_Max=read_float_eeprom(1);
         T_Min=read_float_eeprom(5);
      }
      //Posiciona o cursor do LCD (Col,Lin)
      lcd_pos_xy(1,1);
      //Escreve o valor da temperatura no LCD
      printf(escreve_lcd,"%2.1f",Temp);
      //Escreve o simbolo de graus Celcius
      escreve_lcd(0b1101111);
      escreve_lcd("C");

      //Posiciona o cursor no fim da primeira linha para escrever a Max
      lcd_pos_xy(12,1);
      printf(escreve_lcd,"%2.1f",T_Max);

      //Faz a leitura do RTC para pegar a hora,minuto,segundo
      PCF8583_read_datetime(&dt);

      //Posiciona o cursor na segunda linha
      lcd_pos_xy(1,2);
      //Escreve no LCD a hora,minuto,segundo
      printf(escreve_lcd,"%02u:%02u:%02u",dt.hours, dt.minutes, dt.seconds);

      //Posiciona o cursor no fim da primeira linha para escrever a Min
      lcd_pos_xy(12,2);
      printf(escreve_lcd,"%2.1f",T_Min);

/*
      //Se veio alguma tecla da serial
      if (kbhit())
      {
         if (getc()==13)
         {
            //Imprime a mensagem na serial
            printf("%02u/%02u/%02u - %02u:%02u - T=%2.1f Tmax=%2.1f Tmin=%2.1f\n\r",
                  dt.day,dt.month,dt.year,dt.hours,dt.minutes,Temp,T_Max,T_Min);
                  escreve_eeprom(0xa0,1,5);
            printf("EEPROM - %u\n\r",le_eeprom(0xa0,1));
         }
      }

      //Aguarda 1 segundo para o próximo looping

*/
   delay_ms(1000);
   }

}
