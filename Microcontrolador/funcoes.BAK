/*******************************************************************************
*                         FUNCOES AUXILIARES DO PROGRAMA                       *
*******************************************************************************/

/***** Funcao de ajuste da hora *******/

void arruma_hora()
{
   boolean flag=0;
   int8 num=0, time[5],limit[5],pos[5];
   date_time_t dat;

   limpa_lcd();
   printf(escreve_lcd,"Ajuste da Hora");

   //Tira o repique da tecla de funcao
   delay_ms(1000);

   //Faz a leitura do RTC para pegar a hora,minuto,segundo
   PCF8583_read_datetime(&dat);

   //Escreve a hora do RTC na tela de LCD
   lcd_pos_xy(2,2);
   printf(escreve_lcd,"%02u:%02u %02u/%02u/%02u",dat.hours,dat.minutes,dat.day,dat.month,dat.year);
   // 00:00 99/99

   //Iguala o valor, com o valor lido
   time[0]=dat.hours;
   time[1]=dat.minutes;
   time[2]=dat.day;
   time[3]=dat.month;
   time[4]=dat.year;

   //limita cada posição ao seu valor maximo
   limit[0]=23;   //Horas
   limit[1]=59;   //Minutos
   limit[2]=31;   //Dias
   limit[3]=12;   //Meses
   limit[4]=15;   //Ano

   //Define a posicão no LCD
   pos[0]=2;   //Hora
   pos[1]=5;   //Minuto
   pos[2]=8;   //Dia
   pos[3]=11;  //Mes
   pos[4]=14;  //Ano

   //Até que todos os digitos estejam certos ( hh:mm dd/mm )
   while (num<=4)
   {
       //para indicar quem está sendo ajustado no momento
       lcd_pos_xy(pos[num]-1,2);
       printf(escreve_lcd,"%c",0x7E);  //Setinha

       if (tecla2) {
            //Soma um ao dígito atual
            time[num]++;
            if (time[num]>limit[num])
            {
               time[num]=01;
            }
            //Mostra no LCD
            lcd_pos_xy(pos[num],2);
            printf(escreve_lcd,"%02u",time[num]);

            //Tira o repique e evita erros
            delay_ms(500);
       }
       if (tecla1) {
            //Passa ao proximo numero a ser acertado
            num++;
            //Tira o repique
            delay_ms(500);
       }
   }

   dat.month   = time[3];   // Mês
   dat.day     = time[2];   // Dia
   dat.year    = time[4];   // Ano
   dat.hours   = time[0];   // Horas
   dat.minutes = time[1];   // Minutos
   dat.seconds = 00;        // Segundos
   dat.weekday = 3;         // 0 = Sunday, 1 = Monday, etc.

   //Ajusta o relogio definitivamente
   PCF8583_set_datetime(&dat);

   limpa_lcd();

}

/***** Funcao que mostra a máxima e mínima *******/

void Max_Min()
{
   //Le os valores de max e min na eeprom
   limpa_lcd();
   // Escreve a Temperatura Máxima
   //                  + 30.1 22/11 14h
   printf(escreve_lcd,"+%2.1f %02u/%02u %02uh",read_float_eeprom(1),
                                  read_eeprom(7),read_eeprom(8),read_eeprom(5));
   // Escreve a Temperatura Mínima
   lcd_pos_xy(1,2);
   printf(escreve_lcd,"-%2.1f %02u/%02u %02uh",read_float_eeprom(9),
                                  read_eeprom(15),read_eeprom(16),read_eeprom(13));
   //Aguarda 5 Segundos
   delay_ms(5000);
   //Limpa o LCD
   limpa_lcd();
   //Volta
   return;
}

/***** Funcao de tratamento serial *******/

/******************************************************************************
*  Comunicacao serial com o computador                                        *
*  L        - Devolve os dados de Temperatura e Umidade para os dias          *
*             No formato dd/mm ttt uuu                                        *
*  R        - Reseta a EEPROM externa zerando seus dados                      *
*  C        - Recebe uma String Assim CM:00:12:01:15#                         *
*              Significa que Grava na Posição 01                              *
*                                    Ligar as 12 Horas                        *
*                                             01 Minutos                      *
*                             Fica ligado por 15 Segundos                     *
******************************************************************************/

void trata_serial(){
   signed long i=0,EEPos,Hora,Minuto,Tempo;
   int pos;
   char *ptr,tmp[2];

   //Se for pressionado o "L" - Le as temperaturas e devolve
   if (Buffer[0] =='l')
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
         //Tempo de espera só para evitar erros de envio
         delay_ms(2);
      }
   }
   // Rotina que "formata" a EEPROM externa zerando todos valores
   if (Buffer[0] == 'r'){
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
   if (Buffer[0] == 'C'){

      //Recebe uma String Assim CM:00:12:01:15#
      // Significa que Grava na Posição 01
      //                       Ligar as 12 Horas
      //                                01 Minutos
      //                Fica ligado por 15 Segundos

      //Separa os tempos
      //Separa os dois primeiros caracteres
      tmp[0]=Buffer[3];
      tmp[1]=Buffer[4];
      //Converte o valor em Long
      EEPos  = strtol(tmp,&ptr,10);

      //Continua separando
      tmp[0]=Buffer[6];
      tmp[1]=Buffer[7];
      //Converte o valor em Long
      Hora   = strtol(tmp,&ptr,10);

      tmp[0]=Buffer[9];
      tmp[1]=Buffer[10];
      //Converte o valor em Long
      Minuto = strtol(tmp,&ptr,10);

      tmp[0]=Buffer[12];
      tmp[1]=Buffer[13];
      //Converte o valor em Long
      Tempo  = strtol(tmp,&ptr,10);

      //Calcula a posição na EEprom interna
      i = 20+((int)EEPos*3);

      //Escreve na EEprom Interna a Hora
      write_eeprom(i,(int)Hora);
      i++;
      //Escreve na EEprom interna o Minuto
      write_eeprom(i,(int)Minuto);
      i++;
      //Escreve na EEprom interna o Tempo Ligado
      write_eeprom(i,(int)Tempo);

      //Limpa o Buffer para próxima recepção
      printf(escreve_lcd,"%s",Buffer);
      lcd_pos_xy(1,2);
      printf(escreve_lcd,"%2ld%2ld%2ld%2ld",EEpos,Hora,Minuto,Tempo);
      strcpy(Buffer,"");
      delay_ms(3000);
   }
}
