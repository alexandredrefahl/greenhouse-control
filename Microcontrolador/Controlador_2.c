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
#define EEPROM_SDA   pin_d0
#define EEPROM_SCL   pin_d1

//Define os pinos de comunicacao com o sensor de temperatura/umidade
#define sht_data     pin_a0
#define sck          pin_a1

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
//Arquivo de definicao do sensor de umidade/temperatura SHT11
#include <sht11_drv.c>
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
   long mem_pos=0,i=0,Temp_16;//,Humi_16 /*,T1,T2 */;
   //Variavel de ponto flutuante para armazenar as temperaturas/pt. de orvalho
   float Temp,T_Max,T_Min,T_Temp,RH /*,dew_point*/ ;
   //Variavel tipo estrutura que recebera os dados do RTC
   date_time_t dt;
   //Variavel que recebe o caractere via serial
   char c;
   
   //Define entradas e sa�das em cada Port
   // bit        76543210
   SET_TRIS_B( 0b00000000 );
   SET_TRIS_D( 0b00000000 );  //Port onde est� os dados do LCD

   disable_interrupts ( GLOBAL );

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

   // Inicializa o sensor de temperatura
   RST_Connection();
   printf(escreve_lcd," Inicializando");
   lcd_pos_xy(2,2);
   printf(escreve_lcd,"Sensor SHT11");
   //Escreve o valor 0 no Registrador de Status do sensor para zeras as funcoes
   error=WriteStatReg(0x00);
   // Caso retorne um valor=1 deu erro
   if (error==1){
      limpa_lcd();
      printf(escreve_lcd,"Erro no Sensor");
      //Aguarda 2 segundos
      delay_ms(2000);
   }
   //Aguarda 1 segundo
   delay_ms(1000);
   limpa_lcd();

   //Rotina para conseguir um RESET da Max e Min
   //Quando ligar o circuito com a tecla 1 pressionada
   if (tecla1){
      //Escreve pedindo a confirma��o
      printf(escreve_lcd,"RESET Max Min?");
      lcd_pos_xy(1,2);
      printf(escreve_lcd,"1=Sim 2=Nao");
      delay_ms(3000);
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
   printf(escreve_lcd,"   Final 1.2");
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
      
      //Zera a variavel que armazena o erro do sensor
      error=0;
      
      //Variavel auxiliar para testar eeprom
      conta++;
      
/******************************************************************************
*  Comunicacao serial com o computador                                        *
*  [ENTER]  - Devolve os dados de Temperatura e Umidade para os dias          *
*             No formato dd/mm ttt uuu                                        *
*  R        - Reseta a EEPROM externa zerando seus dados                      *
******************************************************************************/
      //Colocado fora do Loop para verificar toda vez
      //Se veio alguma tecla da serial 
      if (kbhit())
      {
         c=getc();
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
         // Rotina que "formata" a EEPROM externa zerando todos valores
         if (c=='r'){
            pos=1;
            limpa_lcd();
            printf(escreve_lcd,"    Aguarde");
            for(i=0;i<8191;i++){
               // Escreve o valor 0 na EEPROM no endereco i
               escreve_eeprom(0,i,0x00);
               // A cada 512 valores Atualiza um grafico de progresso
               if (!(i % 512)){
                  lcd_pos_xy(pos,2);
                  //Escreve o caractere ">" para indicar o progresso
                  escreve_lcd(0x00);
                  pos++;
               }
            }
            limpa_lcd();
            // Retorna um OK via serial para indicar o encerramento da tranmisao
            printf("OK\n\r");
            //Atualiza o endereco na EEPROM interna
            write_eeprom_16(17,0x00);
            //Zera a variavel de posicao
            mem_pos=0;
         }
      }
      
      //Define a convers�o para ocorrer apenas a cada 5 segundos
      if (!(conta % 5)) {
         
/******************************************************************************
*  Leitura do Sensor SHT11 e do RTC                                           *
******************************************************************************/
         
         // Mede a temperatura no sensor combinado SHT11
         Temp = ReadTemperature();
         // Mede a umidade relativa do ar no sensor combinado SHT11
         RH =   ReadHumidity();
         //Faz a leitura do RTC para pegar a hora,minuto,segundo
         PCF8583_read_datetime(&dt);
         
/******************************************************************************
*  Faz todo o registro de temperatura maxima e minima registrada              *
******************************************************************************/
         
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
         printf(escreve_lcd,"%2.1fC",Temp);
         lcd_pos_xy(12,2);
         printf(escreve_lcd,"%3.1f%%",RH);
                 
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
      }
      
      //Escreve a hora do RTC na tela de LCD
      lcd_pos_xy(1,1);
      printf(escreve_lcd,"%02u/%02u",dt.day,dt.month);
      lcd_pos_xy(1,2);
      printf(escreve_lcd,"%02u:%02u",dt.hours,dt.minutes);
      // 99/99
      // 00:00
     
/******************************************************************************
*   Controle de irriga��o                                *
******************************************************************************/
      
      /*******  Esse Bloco Controla a irriga��o tradicional *********/
      // Transforma a TEmperatura de Float para Long
      Temp_16 = (long)(Temp*10);
      
      //*********************************************
      
      // Irriga��o nebulizacao para estaquia
      if (((int)RH<70)){                        //Se humid<70
         if ((dt.hours==8) || (dt.hours==9)  || (dt.hours==10)
            || (dt.hours==11  || (dt.hours==12) || (dt.hours==13)
               || (dt.hours==14) || (dt.hours==15) || (dt.hours==16)
                  || (dt.hours==17) )){                  // Nesses horarios
         if (dt.minutes==0) {
            if (dt.seconds<8){                  // Irriga por 7 segundos
               output_high(linha1);             // Ativa linha 1
               fBomba=1;                        // Liga a bomba
            } else {                            // Passados 7 segundos
               fBomba=0;                        // Desliga a bomba
               output_low(linha1);              // Fecha linha 1
            }
         }
      }
   }
   
   
   // Irriga��o de maior volume (3x ao dia)
   if (((Temp_16>300)) && ((int)RH<60)) {    //Se a Temp>30 e humid<60
      if ((dt.hours==9) || (dt.hours==12) || (dt.hours==15)){ // Nesses horarios
         if (dt.minutes<2){               // Irriga por 2 minutos
            //output_high(linha1);          // Ativa linha 1
            output_high(linha2);          // Ativa linha2
            fBomba=1;                     // Liga a bomba
         } else {                         // Passados 2 minutos
            fBomba=0;                     // Desliga a bomba
            //output_low(linha1);           // Fecha linha 1
            output_low(linha2);           // Fecha linha 2
         }
      }
   }
   
   
   //*********************************************
   
   // Irriga��o de maior volume (2x ao dia nos dias frios)
   if (((Temp_16<300)) && ((int)RH>60)){     //Se a Temp<30 e humid>60
      if ((dt.hours==10) || (dt.hours==15)){ // Nesses horarios
         if (dt.minutes<1){                  // Irriga por 1 minuto
            //output_high(linha1);             // Ativa linha 1
            output_high(linha2);             // Ativa linha 2
            fBomba=1;                        // Liga a bomba
         } else {                            // Passados 3 minutos
            fBomba=0;                        // Desliga a bomba
            //output_low(linha1);              // Fecha linha 1
            output_low(linha2);              // Fecha linha 2
         }
      }
   }
   
   
/**************************************************************
*       OPERA A BOMBA                                         *
***************************************************************/
   
   //Se o estado l�gico da bomba mudou... ent�o atualiza
   if (fBomba!=fBomba_old){
      if (fBomba) {           //Se a bomba foi ligada!
         output_high(rele);
      } else {                //Se a bomba foi desligada!
         output_low(rele);
         //Por seguran�a desliga as linhas tambem
         output_low(linha1);
         output_low(linha2);
      }
      fBomba_old=fBomba;      //Iguala para que s� mude na pr�xima vez!
   }
   
/**************************************************************
*   Registro de temperatura                                   *
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
            escreve_eeprom(0,mem_pos,dt.hours); 
            mem_pos++;
            //Registra a humidade
            escreve_eeprom(0,mem_pos,(int) RH);
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
            printf(escreve_lcd,"%04lu",(int16)(mem_pos/6));
            //Divide por 6 porque cada registro ocupa 6 bytes na EEPROM
         }
      } else {
         Fregistro=1;
      }
   } else {
      //Se a memoria estiver cheia avisa no LCD
      //Posiciona o cursor e escreve o registro de controle
      lcd_pos_xy(7,2);
      printf(escreve_lcd,"FULL");
   }
   
/******************************************************************************
*  Funcoes especiais das teclas                                               *
*     Tecla 1 - Ajustar a hora do relogio                                     *
*     Tecla 2 - Visualizar a maxima e minima registrada                       *
******************************************************************************/
   
   // Se a tecla 1 estiver pressionada desvia para rotina de arrumar o relogio
   if (tecla1)
   {
      arruma_hora();
   }
   if (tecla2)
   {
      max_min();
   }
   
   //Aguarda dois segundos
   delay_ms(2000);
   } // Fecha o bloco de 5 segundos
   } // Fecha o While Principal
