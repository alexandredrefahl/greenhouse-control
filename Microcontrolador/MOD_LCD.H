/*****************************************************************************
/*                   Biblioteca de funções que o LCD irá usar                *
/*****************************************************************************


/*********** Rotina que envia um NIBBLE para o LCD ******************/

void envia_nibble_lcd(int dado)
{
   //Carrega o NIBBLE baixo nas saidas dos pinos de dados
   output_bit(lcd_db4,bit_test(dado,0));
   output_bit(lcd_db5,bit_test(dado,1));
   output_bit(lcd_db6,bit_test(dado,2));
   output_bit(lcd_db7,bit_test(dado,3));

   //Gera um pulso no enable
   output_high(lcd_enable);
   delay_us(1);
   output_low(lcd_enable);

   return;
}


/*********** Rotina que envia um BYTE para o LCD ******************/

void envia_byte_lcd(boolean endereco,int dado)
{
   //Seta o bit RS para endereco 0=comandos 1=dados
   output_bit(lcd_rs,endereco);
   //Aguarda a estabilização do LCD
   delay_us(100);
   output_low(lcd_enable);
   //Envia a parte ALTA do byte primeiro
   envia_nibble_lcd(dado>>4);
   //Limpa a parte alta e envia a parte BAIXA
   envia_nibble_lcd(dado & 0x0f);
   //Abuarda 40us
   delay_us(40);
   return;
}

/*********** Rotina que envia um CARACTERE para o LCD ******************/

void escreve_lcd(char c)
{
   envia_byte_lcd(1,c);
}

/******************** Rotina LIMPA o LCD ******************************/

void limpa_lcd()
{
   //Envia o comando estabelecido no datasheet
   envia_byte_lcd(0,0x01);
   //Acuarda 2 milisegundos para a execussão da tarefa
   delay_ms(2);
   return;
}

/************** Rotina que Posiciona o cursor do LCD ******************/

void lcd_pos_xy(byte x, byte y)
{
   byte endereco;
   //Se for linha 0 é a primeira linha se for 1 é a segunda
   if (y!=1){
      //Endereco da segunda linha na RAM do Display
      endereco = 0x40;
   } else {
      endereco = 0;
   }
   //Soma o endereco do X
   endereco += x-1;
   //Envia o endereco combinado 0x80 é o da primeira posição
   envia_byte_lcd(0,0x80|endereco);
}

/****************** Rotina que INICIALIZA o LCD ***********************/

void inicializa_lcd()
{
   //Garante que todos os pinos estejam em 0 (Zero)
   output_low(lcd_db4);
   output_low(lcd_db5);
   output_low(lcd_db6);
   output_low(lcd_db7);
   output_low(lcd_rs);
   output_low(lcd_enable);
   //Aguarda a estabilização do LCD
   delay_ms(15);

   //Envia sequencia de comandos de inicialização básica
   envia_nibble_lcd(0x03);
   delay_ms(5);
   envia_nibble_lcd(0x03);
   delay_ms(5);
   envia_nibble_lcd(0x03);
   delay_ms(5);
   //Cursor HOME, comando que envia o cursor para a primeira posição
   envia_nibble_lcd(0x02);
   delay_ms(1);
   //Function Set - Configura o LCD para 4bits, 2 linhas, fonte 5x7
   envia_byte_lcd(0,0x28);
   //Display Control - Ligado sem cursor
   envia_byte_lcd(0,0x0c);
   limpa_lcd();
   //Define o Entry Mode para deslocar o cursor para direita
   envia_byte_lcd(0,0x06);
   return;
}

/************* Rotina que insere caracteres na CGRAM ***********************/
void cria_caractere(){
   //endereca à CGRAM onde se podem criar os caracteres
   //Envia o primeiro endereco da CGRAM (0x40)
   envia_byte_lcd(0,0x40);
   //Envia as 8 linhas que formarao o caractere
   envia_byte_lcd(1,0x00);     //0 0 0 0 0 0 0 0
   envia_byte_lcd(1,0x14);     //0 0 0 1 0 1 0 0
   envia_byte_lcd(1,0x0A);     //0 0 0 0 1 0 1 0
   envia_byte_lcd(1,0x05);     //0 0 0 0 0 1 0 1
   envia_byte_lcd(1,0x0A);     //0 0 0 0 1 0 1 0
   envia_byte_lcd(1,0x14);     //0 0 0 1 0 1 0 0
   envia_byte_lcd(1,0x00);     //0 0 0 0 0 0 0 0
   envia_byte_lcd(1,0x00);     //0 0 0 0 0 0 0 0  //O ultimo e vazio por causa do cursor
   //Terminada a operação manda-se o reposicionamento do cursor
   envia_byte_lcd(0,0x01);
   //Termino da criação do caractere " >> "
   
}
