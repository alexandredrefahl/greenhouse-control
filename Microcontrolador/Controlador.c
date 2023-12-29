//Arquivo de defini��o para o 16F877
#include <16f877.h>

//Ajusta os fusiveis (XT=Oscilador de Cristal) (No Watch Dog Timer) (Power Up Time)
#fuses XT,NOWDT,PUT

//Configura a velocidade para 4MHz
#use delay(clock=4000000)
//Configura o m�dulo interno de I2C
#use i2c(master, sda=pin_d0, scl=pin_d1)
//Configura o m�dulo interno de comunica��o Serial com o PC
#use rs232(baud=9600, parity=N, bits=8, xmit=pin_c6, rcv=pin_c7)

//Define os pinos do bus I2C
#define sda          pin_d0
#define scl          pin_d1

//Defini��o dos pinos para o display
#define  lcd_enable  pin_b0   //Pino de ENABLE do LCD
#define  lcd_rs      pin_b1   //Pino de RS do LCD
#define  lcd_db4     pin_d4   //Pino de dados d4 do LCD
#define  lcd_db5     pin_d5   //Pino de dados d5 do LCD
#define  lcd_db6     pin_d6   //Pino de dados d6 do LCD
#define  lcd_db7     pin_d7   //Pino de dados d7 do LCD

// Define o pino do Led piscante
#define led          pin_b7

//Define o pino do Rele da Bomba
#define rele         pin_c1
//Define os pinos das linhas de irriga��o
#define linha1       pin_b5
#define linha2       pin_b4

// Define onde est�o as duas teclas
#define tecla1       !input(pin_c2)
#define tecla2       !input(pin_c3)

//Arquivo com biblioteca de funcoes I2C
#include <i2c.h>
//Arquivo com biblioteca de funcoes do RTC
#include <PCF_RTC.c>
//Arquivo com biblioteca de funcoes matematicas especiais
#include <math_mod.h>
//Arquivo de defini��o para o uso de display LCD
#include <mod_lcd.h>
//Arquivo de defini��o para uso de funcoes de entrada da RS-232
#include <input.c>
//Arquivo de defini��o para escrita de vari�veis longas na eeprom
#include <eeprom.h>
//Arquivo de defini��o das funcoes auxiliares do programa do controlador
#include <funcoes.h>


/*******************************************************************************
*                          FUNCAO PRINCIPAL DO PROGRAMA                        *
*******************************************************************************/

void main()
{
   //Flag de estado do LED
   int1 flag=1,Ftemp=1,Fregistro=1,fBomba=0,fBomba_old=0;
   //Variavel de 8 bits para teste
   int8 conta,a,pos;
   //Variavel de 16 bits para teste
   long mem_pos=0,i=0,Temp_16 /*,T1,T2 */;
   //Variavel de ponto flutuante para armazenar as temperaturas
   float Temp,T_Max,T_Min,T_Temp,RH;
   //Variavel que armazena a tecla vinda da porta serial
   char c;
   //Variavel tipo estrutura que recebera os dados do RTC
   date_time_t dt;

   //Define entradas e sa�das em cada Port
   // bit        76543210
   SET_TRIS_B( 0b00000000 );
   SET_TRIS_D( 0b00000000 );  //Port onde est� os dados do LCD

   disable_interrupts ( GLOBAL );

   //Faz as definicoes do conversor A/D
   setup_ADC_ports (AN0_AN1_AN3);   //Pino AN0, AN1, AN3 ref=Vdd
   setup_adc(ADC_CLOCK_INTERNAL);

   //Leva todas as sa�das de todos os ports a 0 por causa do LCD
   //portB=0;
   //portD=0;

   //Inicia com o rele desligado
   output_low(rele);

   //Inicializa o LCD
   inicializa_lcd();

   //Cria os caracteres especiaias na CGRAM
   cria_caractere();

   //Limpa a tela ap�s a cria��o dos caracteres na CGRAM
   limpa_lcd();

   //Rotina para conseguir um RESET da Max e Min
   //Quando ligar o circuito com a tecla 1 pressionada
   if (tecla1){
      //Escreve pedindo a confirma��o
      printf(escreve_lcd,"RESET Max Min?");
      delay_ms(1000);
      lcd_pos_xy(1,2);
      printf(escreve_lcd,"1=Sim 2=Nao");
      delay_ms(2000);
      while ((!tecla1) || (!tecla2)){
         if (tecla1){
            //For�a o sistema a passar pela condicional fazendo Ftemp ser 1
            write_eeprom(1,0xFF);
            //Apaga LCD
            limpa_lcd();
            printf(escreve_lcd,"Max/Min Limpos");
            //Aguarda 2 segundos
            delay_ms(2000);
         } else {
            //Apenas limpa o LCD
            limpa_lcd();
            //Sai do loop
            break;
         }
      }
   }


   //Escreve na tela   1234567890123456
   printf(escreve_lcd,"    Beta 1.0");
   //Aguarda 2 segundos
   delay_ms(2000);
   //Limpa a tela e aguarda a digita��o
   limpa_lcd();

   //Recupera o endereco da EEPROM na Memoria interna
   mem_pos=read_eeprom_16(17);

   //Compensar o erro de grava��o do PIC que seta a EEPROM para 0xFF toda grava��o
   if (mem_pos==0xFFFF){
      mem_pos=0;
      write_eeprom_16(17,0x00);
   }

   //Se j� tiver algo gravado na EEPROM n�o altera nada
   if (!(read_eeprom(1)==0xFF)){
      T_Max=read_float_eeprom(1);
      T_Min=read_float_eeprom(9);
      Ftemp=0;
   } else { //Se n�o tiver for�a a passagem pelo condicional
      Ftemp=1;
   }

   //Entra no loop cont�nuo
   while (true)
   {

      //Realiza a opera��o com o led
      output_bit(led,flag);
      //Inverte o estado do Led
      flag = !flag;

      //Variavel auxiliar para testar eeprom
      conta++;

      //Define a convers�o para ocorrer apenas a cada 5 segundos
      if (!(conta % 5)) {

         //Configura o ADC para ler o canal 0 (NTC-10K da Temperatura)
         set_adc_channel(0);

         //Calcula a temperatura pela equa��o de regress�o estimada
         Temp = (float) (-0.0983*read_adc()) + 75.63; //76,63
         //T1=read_adc();

         //Configura o ADC para ler o canal 1 (HIH-4000 sensor de umidade)
         set_adc_channel(1);

         //Cria um atrazo para dar tempo do ADC se reestabelecer
         delay_ms(100);

         //Calcula a umidade pela equa��o de regress�o estimada
         RH = (float) (0.159*read_adc()) - 31.205;
         //T2=read_adc();

         //Primeira vez que passa aqui, zera as temperaturas para evitar erro
         if (Ftemp)
         {

               T_Max=Temp;
               T_Min=Temp;
               //Escreve na EEPROM interna a Maxima e a hora que ocorreu
               write_float_eeprom(1,T_Max);
               write_eeprom(5,dt.hours);
               write_eeprom(6,dt.minutes);
               write_eeprom(7,dt.day);
               write_eeprom(8,dt.month);
               //Escreve na EEPROM interna a Minima e a hora que ocorreu
               write_float_eeprom(9,T_Min);
               write_eeprom(13,dt.hours);
               write_eeprom(14,dt.minutes);
               write_eeprom(15,dt.day);
               write_eeprom(16,dt.month);
               Ftemp=0;

         }

         //Mostra a temperatura e umidade no LCD
         lcd_pos_xy(12,1);
         //printf(escreve_lcd,"%4lu",T1);
         printf(escreve_lcd,"%2.1fC",Temp);
         lcd_pos_xy(12,2);
         printf(escreve_lcd,"%3.1f%%",RH);
         //printf(escreve_lcd,"%4lu",T2);

         //Verifica se a temperatura � maxima ou minima e grava na EEPROM Interna
         if (Temp > T_Max) {
            write_float_eeprom(1,Temp);
            write_eeprom(5,dt.hours);
            write_eeprom(6,dt.minutes);
            write_eeprom(7,dt.day);
            write_eeprom(8,dt.month);
         }
         if (Temp < T_Min) {
            write_float_eeprom(9,Temp);
            write_eeprom(13,dt.hours);
            write_eeprom(14,dt.minutes);
            write_eeprom(15,dt.day);
            write_eeprom(16,dt.month);
         }


         //Le os valores de max e min na eeprom
         //T_Max=read_float_eeprom(1);
         //T_Min=read_float_eeprom(9);

      }

      //Faz a leitura do RTC para pegar a hora,minuto,segundo
      PCF8583_read_datetime(&dt);

      //Escreve a hora do RTC na tela de LCD
      lcd_pos_xy(1,1);
      printf(escreve_lcd,"%02u/%02u",dt.day,dt.month);
      lcd_pos_xy(1,2);
      printf(escreve_lcd,"%02u:%02u",dt.hours,dt.minutes);
      // 99/99
      // 00:00

      //Se veio alguma tecla da serial
      if (kbhit())
      {
         c=getc();
         //putc(c);
         //Se for pressionado o [Enter]
         if (c==13)
         {
            i=0;
            while(i<mem_pos)
            {
               //Dia
               printf("%02u/",le_eeprom(0,i));
               i++;
               //Mes
               printf("%02u ",le_eeprom(0,i));
               i++;
               //Hora
               printf("%02u ",le_eeprom(0,i));
               i++;
               //Umidade
               printf("%03u ",le_eeprom(0,i));
               i++;
               //Temperatura e fim de linha
               printf("%lu\n\r",le_eeprom_16(0,i));
               i++;
               i++;
               //Tempo de espera s� para evitar erros de envio
               delay_ms(2);
            }
         }

         if (c=='r'){
            pos=1;
            limpa_lcd();
            printf(escreve_lcd,"    Aguarde");
            for(i=0;i<8191;i++){
               escreve_eeprom(0,i,0x00);
               if (!(i % 512)){
                  lcd_pos_xy(pos,2);
                  escreve_lcd(0x00);
                  pos++;
               }
            }
            limpa_lcd();
            printf("OK\n\r");
            write_eeprom_16(17,0x00);
            mem_pos=0;
         }
/*         if (c=='f'){
            limpa_lcd();
            printf(escreve_lcd," Formatando...");
            for (i=0;i<=8191;i++){
               escreve_eeprom(0,i,0x00);
            }
            write_eeprom_16(17,0x00);
            mem_pos=0;
            printf("EEPROM Externa Formatada 8192 registros livres!\n\r");
         }
*/
      }

      /**************************************************************
      *   Aqui � feito todo o controle de irriga��o                 *
      ***************************************************************/
      //tempo++;


      /*******  Esse Bloco Controla a irriga��o tradicional *********/

      T_Temp  = Temp * 10;
      Temp_16 = (long) T_Temp;
      //tempo=0;

      // Irriga��o de maior volume (3x ao dia)
      if ((Temp_16>310)) /*&& ((int)RH<70))*/ {    //Se a Temp>31 e humid<70
         if ((dt.hours==9) || (dt.hours==12) || (dt.hours==15)){ // Nesses horarios
            if (dt.minutes<2){               // Irriga por 2 minutos
               output_high(linha1);          // Ativa linha 1
               output_high(linha2);          // Ativa linha2
               fBomba=1;                     // Liga a bomba
            } else {                         // Passados 3 minutos
               fBomba=0;                     // Desliga a bomba
               output_low(linha1);           // Fecha linha 1
               output_low(linha2);           // Fecha linha 2
            }
         }
      
      // Irriga��es intermitentes (sprais)
      if ((dt.hours>=8) && (dt.hours<18)){   //Se for entre as 8:00 e as 17:00
         if ((dt.hours>=12) && (dt.hours<=14)){ //Se estiver entre as 12 e 14
            if ((dt.minutes==15) || (dt.minutes==30) || (dt.minutes==45)) {
               if (dt.seconds<20){           //Irriga 15 segundos
                  output_high(linha1);
                  output_high(linha2);
                  fBomba=1;
               } else {
                  output_low(linha1);
                  output_low(linha2);
                  fBomba=0;
               }
            }
         } else {
            if ((dt.minutes==15) || (dt.minutes==45)){
               if (dt.seconds<20){
                  output_high(linha1);
                  output_high(linha2);
                  fBomba=1;
               } else {
                  output_low(linha1);
                  output_low(linha2);
                  fBomba=0;
               }
            }
         }
      }
      }
   

//*********************************************

      // Irriga��o de maior volume (3x ao dia)
      if ((Temp_16<310)) /*&& ((int)RH>70))*/{       //Se a Temp<31 e humid>70
         if ((dt.hours==10) || (dt.hours==15)){ // Nesses horarios
            if (dt.minutes<2){                  // Irriga por 2 minutos
               output_high(linha1);             // Ativa linha 1
               fBomba=1;                        // Liga a bomba
            } else {                            // Passados 3 minutos
               fBomba=0;                        // Desliga a bomba
               output_low(linha1);              // Fecha linha 1
            }
         }

      // Irriga��es intermitentes (sprais)
      if ((dt.hours>=9) && (dt.hours<17)){   //Se for entre as 9:00 e as 16:00
            if (dt.minutes==10) {            //Uma vez por hora quando o minuto for 10
               if (dt.seconds<15){           //Irriga 20 segundos
                  output_high(linha2);
                  fBomba=1;
               } else {
                  output_low(linha2);
                  fBomba=0;
               }
            }
      }
      }

/*
      if ((dt.hours==9) || (dt.hours==14)) { // Se for 9 ou 14 horas
         if (RH<70){                         // e a Umidade estiver abaixo dos 70%
            if (dt.minutes<2){               // E for antes dos 3 minutos
               fBomba=1;                     // Liga a bomba
            } else {                         // Se passaram dos 3 minutos
               fBomba=0;                     // Desliga a bomba
            }
         } else {                            // Se a Umidade estiver acima de 70%
         if (dt.minutes<2){                  // E for antes dos 2 minutos
               fBomba=1;                     // Liga a bomba
            } else {                         // Se passaram dos 2 minutos
               fBomba=0;                     // Desliga a bomba
            }
         }
      }

*/
      /*******  Esse Bloco Controla a irriga��o Especial *********/
/*
      //Se for entre 11 e 15 horas
      if (dt.hours>=11 && dt.hours<=15){
         //De meia em meia hora
         if ((dt.minutes==15) || (dt.minutes==45)){
            //Irriga somente 30 segundos (um fog de complemento)
            if (dt.seconds<31){
               //Se a temperatura foi maior que 32�C
               if ((Temp_16>320) && ((int8) RH < 71)){
                  fBomba=1;            //Sinaliza para ligar a bomba
               }
            } else {
               fBomba=0;               //Sinaliza para desligar a bomba
            }
         }
       }
*/

      /*****  OPERA A BOMBA *****/

      //Se o estado l�gico da bomba mudou... ent�o atualiza
      if (fBomba!=fBomba_old){
         if (fBomba) {           //Se a bomba foi ligada!
            output_high(rele);
         } else {                //Se a bomba foi desligada!
            output_low(rele);
         }
         fBomba_old=fBomba;      //Iguala para que s� mude na pr�xima vez!
      }

      /**************************************************************
      *   Aqui � feito todo o Registro de temperatura               *
      ***************************************************************/

      //Primeiro verifica se a mem�ria n�o est� cheia
      if (mem_pos<8191){

         //A cada 4 horas
         if (!(dt.hours % 4)) //Horas
         {
            if (Fregistro){
               //Registra o Dia
               escreve_eeprom(0,mem_pos,dt.day);
               mem_pos++;
               //Registra o Mes
               escreve_eeprom(0,mem_pos,dt.month);
               mem_pos++;
               //Registra a hora
               escreve_eeprom(0,mem_pos,dt.hours); //N�o esquecer de mudar para horas
               mem_pos++;
               //Registra a humidade
               escreve_eeprom(0,mem_pos,(int8) RH);
               mem_pos++;
               //Registra a temperatura
               //Funcao especial para escrever 16 bits na eeprom externa
               escreve_eeprom_16(0,mem_pos,Temp_16);
               //uma vari�vel long ocupa 16 bits ou seja 2 bytes
               mem_pos++;
               mem_pos++;
               //Guarda a posicao atual da EEPROM externa, na memoria interna
               write_eeprom_16(17,mem_pos);
               //Sinaliza que j� foi armazenada a temperatura
               Fregistro=0;
               //Posiciona o cursor e escreve o registro de controle
               lcd_pos_xy(7,2);
               printf(escreve_lcd,"%04lu",mem_pos);
            }
         } else {
            Fregistro=1;
         }
      } else { //Se a memoria estiver cheia avisa no LCD
         //Posiciona o cursor e escreve o registro de controle
         lcd_pos_xy(7,2);
         printf(escreve_lcd,"FULL");
      }

      // Se a tecla 1 estiver pressionada desvia para rotina de arrumar o relogio
      if (tecla1)
      {
         arruma_hora();
      }
      if (tecla2)
      {
         max_min();
      }

      //Aguarda um segundo
      delay_ms(1000);
   }

}

