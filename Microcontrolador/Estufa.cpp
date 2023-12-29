//---------------------------------------------------------------------------

#include <vcl.h>
//Para as operações com o Registro do Windows
#include <vcl\registry.hpp>
#include <dstring.h>
#pragma hdrstop
#include "Estufa.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#define LEN_BUFFER 8192

TForm1 *Form1;

TMultLinha *MultLinha;
//Variáveis da API
HANDLE hCom;
DCB dcb;
COMMTIMEOUTS CommTimeouts;
bool GLB_Conectado = false;     //Flag para indicar se conectado
bool GLBEnviaDados = false;     //Para habilitar/desabilitar o envio de dados pela serial.
AnsiString StrComandos;         //Armazena a string de comando lida da Serial.
char BufferRecebe[LEN_BUFFER];  //Buffer temporário para trabalhar direto com ReadFile().
char EnviaComando[LEN_BUFFER];  //Buffer temporário para trabalhar direto com WriteFile().

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner): TForm(Owner)
{
        dcb.BaudRate = CBR_9600;      //bps (Velocidade).
        dcb.ByteSize = 8;             //8 Bits de dados.
        dcb.Parity   = NOPARITY;      //Sem paridade.
        dcb.StopBits = ONESTOPBIT;    //1 stop bit.
}
//------------------------------------------------------------------------------
__fastcall TMultLinha::TMultLinha(bool CreateSuspended) : TThread(CreateSuspended)
{
}

//------------------------------------------------------------------------------
//Abre a Porta Serial COMx
bool AbrirPorta(char *NomePorta)
{
   hCom = CreateFile(
             NomePorta,
             GENERIC_READ | GENERIC_WRITE,
             0,             // dispositivos comm abertos com acesso exclusivo
             NULL,          // sem atributos de segurança
             OPEN_EXISTING, // deve usar OPEN_EXISTING
             0,             //Entrada e saída sem ovelap.
             NULL           // hTemplate deve ser NULL para comm
          );
   if(hCom == INVALID_HANDLE_VALUE)
   {
      return false;
   }
   return true;
}
//------------------------------------------------------------------------------
//CONFIGURA PORTA SERIAL.
bool ConfiguraControle(void)
{
   if(!GetCommState(hCom, &dcb))
   {
      return false;
   }
   dcb.BaudRate = CBR_9600; 
   dcb.ByteSize = 8;
   dcb.Parity   = NOPARITY;
   dcb.StopBits = ONESTOPBIT;

   if( SetCommState(hCom, &dcb) == 0 )
   {
      return false;
   }
   return true;
}
//------------------------------------------------------------------------------
//DEFINE TIMEOUTs
bool ConfiguraTimeOuts(void)
{
   if( GetCommTimeouts(hCom, &CommTimeouts) == 0 )
   {
      return false;
   }
                                
   CommTimeouts.ReadIntervalTimeout = 2;
   CommTimeouts.ReadTotalTimeoutMultiplier = 0;
   CommTimeouts.ReadTotalTimeoutConstant = 2;
   CommTimeouts.WriteTotalTimeoutMultiplier = 5;
   CommTimeouts.WriteTotalTimeoutConstant = 5;

   if( SetCommTimeouts(hCom, &CommTimeouts) == 0 )
   {
      return false;
   }
   return true;
}

//------------------------------------------------------------------------------
//Mostra string de forma sincronizada.
void __fastcall TMultLinha::MostraString(void)
{
   Form1->Memo1->Text = StrComandos;
}

//------------------------------------------------------------------------------
bool EscreveDados(char* outputData,const unsigned int sizeBuffer,unsigned long& length)
{
    if(WriteFile(hCom, outputData, sizeBuffer, &length,NULL) == 0)
    {
      return false;
    }
    return true;
}

//------------------------------------------------------------------------------
//FUNÇÃO PRINCIPAL
//------------------------------------------------------------------------------
void __fastcall TMultLinha::Execute()
{
    unsigned int cont=0;
    DWORD BytesEscritos;  //Para armazenar a quantidade de dados escritos.
    DWORD BytesLidos;     //Para armazenar a quantidade de dados lidos.
    unsigned int IntSensores;     //Para armazenar a conversão dos sensores.
    unsigned int IntTecConectado; //Para armazenar a conversão Teclados Conectados.

    FreeOnTerminate = true; //O objeto é destruído automaticamente quando a Thead terminar.

    while(!Terminated) //loop infinito. Vida do programa.
    {

      if(GLB_Conectado == true) //Se está conectado.
      {
        if(ReadFile( hCom, BufferRecebe, LEN_BUFFER, &BytesLidos, NULL) != 0 )
        {
            //Form1->Canvas->TextOutA(200,175,"ON");
            cont = 0;
            //Form1->Canvas->TextOutA(5,5,BytesLidos);
            if(BytesLidos > 0) //Se algum caracter foi lido.
            {
              while(cont < BytesLidos)
              {
                   StrComandos += BufferRecebe[cont]; //Vai guardando o que recebeu na variável StrComandos.
                   //Form1->Canvas->TextOutA(5,20,StrComandos.Length());
                   cont++;
              }
                   Synchronize(MostraString); //Mostra string de forma sincronizada.
            }
         }
        if((GLBEnviaDados == true) && (strlen(EnviaComando) > 0)) //Se há texto a ser enviado.
        {
                AnsiString StrMens = EnviaComando; //Armazena o comando digitado na variável.
                StrMens += "\r";  // \n Finaliza string e o \r envia o [Enter]

                int tama = StrMens.Length();

                EscreveDados(StrMens.SubString(0,1).c_str(), 1, BytesEscritos); //Envia string de comandos para a placa.
                Sleep(500);
                EscreveDados(StrMens.SubString(1,tama-1).c_str(), tama-1, BytesEscritos);

                //Form1->Canvas->TextOutA(5,20,BytesEscritos);
                strcpy(EnviaComando,"");
                GLBEnviaDados = false;
         }

     }else{
        Sleep(2); //Necessário para não travar processo.
     }
   }
}




//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
        TRegistry *Registro = new TRegistry;    //Cria e Aloca espeço para o objeto
        TStringList *Lista = new TStringList;   //Cria e Aloca espeço para o objeto

        AnsiString Data;

        Registro->RootKey = HKEY_LOCAL_MACHINE; //Define a Chave Raiz no Registro
        Registro->OpenKey("HARDWARE\\DEVICEMAP\\SERIALCOMM",false); //Abre a chave
        //Obtem uma string contendo todos os nomes de valores associados à chave Atual
        Registro->GetValueNames(Lista);

        //Count é a quantidade de portas no sistema
        for(int indice=0; indice<=(Lista->Count)-1 ; indice++){
                //Adiciona o nome das portas no combobox
                ComboBoxPorta->Items->Add(Registro->ReadString(Lista->Strings[indice]));
        }

        //Se o combo não for vazio...
        if(ComboBoxPorta->Items->Count > 0)
                //Posiciona no primeiro item
                ComboBoxPorta->ItemIndex = 0;

        Lista->Free();          //Libera a memória
        Registro->CloseKey();   //Fecha a chave
        Registro->Free();       //Libera a memória


        //FAZ o preenchimento do GRID

        Grid1->RowCount = 17;
        Grid1->ColCount = 4;
        Grid1->FixedCols = 1;
        Grid1->FixedRows = 1;
        //Preenche o título do grid
        //           C  L
        Grid1->Cells[0][0] = "Progs";
        Grid1->Cells[1][0] = "Hora";
        Grid1->Cells[2][0] = "Minuto";
        Grid1->Cells[3][0] = "Tempo";

        //Numera os programas possívels até 16
        for(int idx=1 ; idx <= 16 ; idx++){
                Grid1->Cells[0][idx] = idx;
        }

        //Iguala a data final do filtro com a data de hoje
        Data = TDateTime::CurrentDateTime().DateString();
        //Converte a string em data
        TDateTime Value = StrToDate(Data);
        //Formata a data para ser apresentada
        ShortDateFormat = "dd/mm/yyyy";
        //Preenche o campo com a data de hoje
        txtDataFIM->Text = Value;

        //Status->Panels->Items[1]->Text = "Aguardando";

        MultLinha = new TMultLinha(true); //Aloca memória para o objeto.
        MultLinha->Priority = tpHigher;   //Define a prioridade.

}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button1Click(TObject *Sender)
{
char *NomePorta="COM5";
bool Sucesso;

strcpy(NomePorta,ComboBoxPorta->Text.c_str());

//Canvas->TextOutA(200,55,NomePorta);

if(AbrirPorta(NomePorta) == true)
{
        GLB_Conectado = true;
        Status->Panels->Items[0]->Text ="Conectado";
        Sucesso = ConfiguraControle();
        Sucesso = ConfiguraTimeOuts();
        if(Sucesso == false)
        {
                GLB_Conectado = false;
                Status->Panels->Items[0]->Text ="Desconectado";
                CloseHandle(hCom);
                Status->Panels->Items[0]->Text ="Não foi possível abrir a porta";

        }else{
                //Não pretendo receber nada de volta...
                //Não há porque ativar essa funcao
                //MultLinha->Resume(); //Inicia processo.
        }
}else{
        GLB_Conectado = false;

}

   DWORD Bytes_Escritos;
   int TamString;
   char BufferEnvia[3];
   strcpy(BufferEnvia,"r\r");
   TamString = strlen(BufferEnvia);

   WriteFile(hCom,BufferEnvia,TamString,&Bytes_Escritos,NULL);

   CloseHandle(hCom); //Fecha porta serial
   Status->Panels->Items[0]->Text ="Desconectado";
   GLB_Conectado = false;
}
//---------------------------------------------------------------------------


void __fastcall TForm1::Button2Click(TObject *Sender)
{
char *NomePorta="COM5"; //COM1, COM2...COM9 ou portas virtuais "\\.\COMx".
bool Sucesso;

strcpy(NomePorta,ComboBoxPorta->Text.c_str());

//Canvas->TextOutA(200,55,NomePorta);

if(AbrirPorta(NomePorta) == true)
{
        GLB_Conectado = true;
        Status->Panels->Items[0]->Text ="Conectado";
        Sucesso = ConfiguraControle();
        Sucesso = ConfiguraTimeOuts();
        if(Sucesso == false)
        {
                GLB_Conectado = false;
                Status->Panels->Items[0]->Text ="Desconectado";
                CloseHandle(hCom);
        }else{
                //Agora Sim... Quero um retorno ativo a função
                GLB_Conectado = true;
                GLBEnviaDados = true;
                strcpy(EnviaComando,"l");  //Aqui a propria funcao vai por o \r\n no fim
                MultLinha->Resume(); //Inicia processo.
        }
}else{
        GLB_Conectado = false;
        Status->Panels->Items[0]->Text ="Desconectado";
}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button3Click(TObject *Sender)
{
        if (Abrir_Arquivo->Execute()){
                int tam;
                //Arquivo->Text = Abrir_Arquivo->Files->GetText();

                AnsiString nome_arquivo(Abrir_Arquivo->Files->GetText());
                AnsiString nome_corrigido;
                tam = nome_arquivo.Length();
                nome_corrigido = nome_arquivo.SubString(0,tam-2);
                /*
                while(nome_arquivo[i] != '\r'){
                        nome_corrigido += nome_arquivo[i];
                        i++;
                }*/
                //Arquivo->Text = nome_corrigido;

        /*Segunda parte */
        int Tamanho;
        char buffer[8195];
        TFileStream* Arq = new TFileStream(nome_corrigido,fmOpenRead);
        Tamanho = Arq->Size;
        Arq->Read(buffer,Tamanho);
        AnsiString Texto;
        Texto = buffer;
        Memo1->Text = Texto;
        //Linhas->Text = Memo1->Lines->Strings[2];
        delete Arq;
        }
}
//---------------------------------------------------------------------------


void __fastcall TForm1::Button4Click(TObject *Sender)
{
Memo1->Text = "";
StrComandos = "";
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ComboBoxPortaChange(TObject *Sender)
{
//Canvas->TextOutA(5,5,ComboBox1->Text.c_str());

}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button5Click(TObject *Sender)
{
        if (Salvar_Arquivo->Execute()){
                int tam;
                //Arquivo->Text = Abrir_Arquivo->Files->GetText();

                AnsiString nome_arquivo(Salvar_Arquivo->Files->GetText());
                AnsiString nome_corrigido;
                tam = nome_arquivo.Length();
                nome_corrigido = nome_arquivo.SubString(0,tam-2);
                /*
                while(nome_arquivo[i] != '\r'){
                        nome_corrigido += nome_arquivo[i];
                        i++;
                }*/
                //Arquivo->Text = nome_corrigido;

        /*Segunda parte */
        int Tamanho;
        char *BufferTexto[8200];
        TFileStream* Arq = new TFileStream(nome_corrigido,fmCreate);
        Tamanho = Memo1->GetTextLen();
        *BufferTexto = Memo1->Text.c_str();
        Arq->Write(*BufferTexto,Tamanho);
        delete Arq;
        }

}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button12Click(TObject *Sender)
{
ADOQuery1->Append();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button7Click(TObject *Sender)
{
ADOQuery1->FindFirst();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button8Click(TObject *Sender)
{
ADOQuery1->FindPrior();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button9Click(TObject *Sender)
{
ADOQuery1->FindNext();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button10Click(TObject *Sender)
{
ADOQuery1->FindLast();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button11Click(TObject *Sender)
{
ADOQuery1->Delete();
}
//---------------------------------------------------------------------------




void __fastcall TForm1::Button6Click(TObject *Sender)
{
for(int i=0 ; i <= (Memo1->Lines->Count)-1; i++){     //Looping para percorrer todo Memo1
        ADOTable1->Append();    //Cria um novo registro no Banco de Dados
        AnsiString Linha = Memo1->Lines->Strings[i];  //Pega a linha do memo

        AnsiString Data = Linha.SubString(0,5);         //Separa a data
        Data += "/08";
        int Hora = Linha.SubString(7,2).ToInt();        //Separa a Hora
        int Umi  = Linha.SubString(10,3).ToInt();       //Separa a Umidade
        Single Temp = Linha.SubString(14,3).ToDouble(); //Separa a temperatura e converte para Double
        Temp = Temp/10;                                 //Divide por 10 porque o sensor guarda inteiro

        ADOTable1->FieldByName("Data")->Value        = Data;
        ADOTable1->FieldByName("Hora")->Value        = Hora;
        ADOTable1->FieldByName("Umidade")->Value     = Umi;
        ADOTable1->FieldByName("Temperatura")->Value = Temp;
        /*
        DBEdit3->Text = Data; //"02/02/08";             Acrescenta aos campos correspondentes
        DBEdit4->Text = Hora; //"12";
        DBEdit2->Text = Umi ; //"54";
        DBEdit1->Text = Temp; //"22,1";
        */
}
ADOTable1->FindLast();            //Ao fim da operação vai ao ultimo regitro
//DataSource1->Enabled = false;   //Dá um pulso ao DataSource
//DataSource1->Enabled = true;
//ADOConnection1->Connected = false;
//ADOConnection1->Connected = true;
ADOQuery1->Refresh();
DBGrid1->Refresh();             //Atualiza o DataGrid
ADOQuery1->FindLast();

}
//---------------------------------------------------------------------------


void __fastcall TForm1::FormCloseQuery(TObject *Sender,
      TCloseAction &Action)
{
    MultLinha->Terminate();
    MultLinha = NULL;
    delete MultLinha;
    if( GLB_Conectado )
    CloseHandle(hCom);

}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void __fastcall TForm1::Button14Click(TObject *Sender)
{
   GLB_Conectado = false;
   Status->Panels->Items[0]->Text = "Desconectado";
   CloseHandle(hCom);
}
//---------------------------------------------------------------------------


void __fastcall TForm1::btnTudoClick(TObject *Sender)
{
//Define as variáveis
String SQL;

//Limpa a variável
SQL="";

//Monta a SQL
SQL = "SELECT id, Data, hora, Umidade, Temperatura FROM Temperatura ORDER BY Data";

//Fecha a consulta para poder mexer
ADOQuery1->Close();
//Limpa a SQL atual para inserir outra
ADOQuery1->SQL->Clear();
//Troca o SQL do Query
ADOQuery1->SQL->Add(SQL);
//Atualiza para mostrar os dados
ADOQuery1->Open();
        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button15Click(TObject *Sender)
{
//Define as variáveis
String SQL,DataINI,DataFIM;

//Limpa a variável
SQL="";

//Captura as datas para o filtro
//Processo para inverter a data para formato britanico
DataINI = txtDataINI->Text.SubString(4,2) + "/" + txtDataINI->Text.SubString(0,2) + "/" + txtDataINI->Text.SubString(7,4);
DataFIM = txtDataFIM->Text.SubString(4,2) + "/" + txtDataFIM->Text.SubString(0,2) + "/" + txtDataFIM->Text.SubString(7,4);

//Monta a SQL
SQL = "SELECT id, Data, hora, Umidade, Temperatura FROM Temperatura ";
SQL += "WHERE (Data>=#" + DataINI + "#) And (Data<=#" + DataFIM + "#)";
SQL += "ORDER BY Data;";

Sleep(2);

//Fecha a consulta para poder mexer
ADOQuery1->Close();
//Limpa a SQL atual para inserir outra
ADOQuery1->SQL->Clear();
//Troca o SQL do Query
ADOQuery1->SQL->Add(SQL);
//Atualiza para mostrar os dados
ADOQuery1->Open();

}
//---------------------------------------------------------------------------



void __fastcall TForm1::BitBtn2Click(TObject *Sender)
{
        //Prepara os dados para serem gravados
        AnsiString Programacao;
        int Tamanho, i, j;
        char *BufferTexto[200];

        //Faz o looping para pegar as linhas do grid
        for (i=1 ; i<=16 ; i++){
                //Faz o looping para pegar as colunas
                for (j=1 ; j<=3 ; j++){
                        if (j ==1){
                                //Se estiver na coluna 1 não tem espaco
                                //Esse if evita espacos em branco
                                if ( Grid1->Cells[j][i] == "") {
                                        Programacao += "00";
                                } else {
                                        Programacao += Grid1->Cells[j][i];
                                }
                        } else {
                                //Esse if evita espacos em branco
                                if ( Grid1->Cells[j][i] == "") {
                                        Programacao += " 00";
                                } else {
                                        //A partir da 2 tem espaço
                                        Programacao += " " + Grid1->Cells[j][i];
                                }
                        }
                }
                //Terminada a linha manda um retorno de linha
                Programacao += "\r\n";
        }
        //Ezsse é só pra teste
        //Memo1->Text = Programacao;

        if (Salvar_Prog->Execute()){
                int tam;
                //Arquivo->Text = Abrir_Arquivo->Files->GetText();

                AnsiString nome_arquivo(Salvar_Prog->Files->GetText());
                AnsiString nome_corrigido;
                tam = nome_arquivo.Length();
                nome_corrigido = nome_arquivo.SubString(0,tam-2);

        /*Segunda parte */

        TFileStream* Arq = new TFileStream(nome_corrigido,fmCreate);
        Tamanho = Programacao.Length();
        *BufferTexto = Programacao.c_str();
        Arq->Write(*BufferTexto,Tamanho);
        delete Arq;
        }
        
}
//---------------------------------------------------------------------------


void __fastcall TForm1::btnEnviarClick(TObject *Sender)
{
char *NomePorta="COM5"; //COM1, COM2...COM9 ou portas virtuais "\\.\COMx".
bool Sucesso;
int Pos_mem=0;
//Primeiro monta a string com os dados da primeira linha
AnsiString Comando,Pos;

//Define os dados da porta
strcpy(NomePorta,ComboBoxPorta->Text.c_str());

//Canvas->TextOutA(200,55,NomePorta);

if(AbrirPorta(NomePorta) == true)
{
        GLB_Conectado = true;
        Status->Panels->Items[0]->Text ="Conectado";
        Sucesso = ConfiguraControle();
        Sucesso = ConfiguraTimeOuts();
        if(Sucesso == false)
        {
                GLB_Conectado = false;
                Status->Panels->Items[0]->Text ="Desconectado";
                CloseHandle(hCom);
                Status->Panels->Items[1]->Text ="Não foi possível abrir a porta";

        }else{
                //Define as variáveis
                DWORD Bytes_Escritos;
                int TamString,i;
                char BufferEnvia[18];

                //Looping para determinar quantas linhas tem preenchido no GRID
                for(i=1; i<=10; i++){

                        //Atualiza o painel de status
                        Status->Panels->Items[0]->Text ="Conectado";
                        Status->Panels->Items[1]->Text ="Enviando Programação...";

                        //Verifica se há algo na linha específica
                        if (Grid1->Cells[1][i]!=""){
                                //Define a posicao em termos textuais para passar para o PIC
                                Pos = Pos_mem;

                                //Acerta a string para ficar com tamanho 2
                                if (Pos.Length() == 1){
                                        Pos = "0" + Pos;
                                }
                                if (Pos_mem == 0){
                                        Pos = "00";
                                }

                                //Monta a string de comando
                                //               Memoria :          Horario         :         Minuto           :    Tempo Ligado (seg.)
                                Comando = "CM:" + Pos + ":" + Grid1->Cells[1][i] + ":" + Grid1->Cells[2][i] + ":" + Grid1->Cells[3][i] + "#";
                                //Exibe o comando na caixa de texto
                                EditComando->Text = Comando;
                                Memo1->Text += Memo1->Text + "\n" + Comando;
                                //Acrescenta o [ENTER] ao fim da string
                                Comando += "\r";
                                //Coloca o comando no buffer de envio
                                strcpy(BufferEnvia,Comando.c_str());
                                //Le o tamanho da string
                                TamString = strlen(BufferEnvia);
                                //Truque por causa do tempo que o pic leva para ler o Gets()
                                //Envia primeiro o caractere "C"
                                WriteFile(hCom,"C",1,&Bytes_Escritos,NULL);
                                //Aguarda 2 segundos e meio
                                Sleep(2000);
                                //Esvazia o buffer
                                strcpy(BufferEnvia,"");
                                //Coloca no buffer só a parte que falta
                                strcpy(BufferEnvia,Comando.SubString(2,TamString-1).c_str());
                                //Mede o comprimento da string novamente
                                TamString = strlen(BufferEnvia);
                                //Manda o restante da string para a porta serial
                                WriteFile(hCom,BufferEnvia,TamString,&Bytes_Escritos,NULL);
                                //Aguarda mais 3 segundos
                                Sleep(2500);
                                //Atualiza a barra de porcentagem
                                StatusEnvio->Position++;
                                //Acrescenta Pos Mem
                                Pos_mem++;
                        } //Fecha if
                } //Fecha For

                CloseHandle(hCom); //Fecha porta serial
                //Atualiza o painel de status
                Status->Panels->Items[0]->Text ="Desconectado";
                Status->Panels->Items[1]->Text ="";
                //Informa a Thread que esta desconectado
                GLB_Conectado = false;
                //Limpa a barra de envio
                StatusEnvio->Position=0;
                
        }
}else{
        GLB_Conectado = false;

}

}
void __fastcall TForm1::btnCarregarClick(TObject *Sender)
{
        if (Abre_Prog->Execute()){
                int tam,i;
                //Arquivo->Text = Abrir_Arquivo->Files->GetText();

                AnsiString nome_arquivo(Abre_Prog->Files->GetText());
                tam = nome_arquivo.Length();
                //Tirar fora os caracteres de fim de string \o\r
                AnsiString nome_corrigido;
                nome_corrigido = nome_arquivo.SubString(0,tam-2);

                /*Segunda parte */
                int Tamanho;
                char buffer[210];
                AnsiString Texto;
                TFileStream* Arq = new TFileStream(nome_corrigido,fmOpenRead);

                Tamanho = Arq->Size;
                Arq->Read(buffer,Tamanho);

                Texto = buffer;
                Memo2->Text = Texto;
                delete Arq;

                /*Terceira Parte*/
                for (i=1 ; i<=16 ; i++){
                        AnsiString Linha;
                        //usa um memo escondido auxiliar para tratar as linhas
                        Linha = Memo2->Lines->Strings[i];
                        // Uma programacao nunca comeca com hora 00
                        if (Linha.SubString(1,2) != "00"){
                                Grid1->Cells[1][i] = Linha.SubString(1,2);
                                Grid1->Cells[2][i] = Linha.SubString(4,2);
                                Grid1->Cells[3][i] = Linha.SubString(7,2);
                        } else {
                                //Se for 00 apresenta vazio para ficar mais limpo
                                Grid1->Cells[1][i] = "";
                                Grid1->Cells[2][i] = "";
                                Grid1->Cells[3][i] = "";
                        }
                }
                //Limpar para o caso de querer abrir outra programacao
                Memo2->Text = "";
        }

}
//---------------------------------------------------------------------------


void __fastcall TForm1::btSairClick(TObject *Sender)
{
Close();        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btComandoClick(TObject *Sender)
{
char *NomePorta="COM5"; //COM1, COM2...COM9 ou portas virtuais "\\.\COMx".
bool Sucesso;

//Define os dados da porta
strcpy(NomePorta,ComboBoxPorta->Text.c_str());

//Canvas->TextOutA(200,55,NomePorta);

if(!GLB_Conectado){

        if(AbrirPorta(NomePorta) == true)
        {
                GLB_Conectado = true;
                Status->Panels->Items[0]->Text ="Conectado";
                Sucesso = ConfiguraControle();
                Sucesso = ConfiguraTimeOuts();
                if(Sucesso == false)
                {
                        GLB_Conectado = false;
                        Status->Panels->Items[0]->Text ="Desconectado";
                        CloseHandle(hCom);
                }else{
                        //Agora Sim... Quero um retorno ativo a função
                        strcpy(EnviaComando,EditComando->Text.c_str());
                        GLB_Conectado = true;
                        GLBEnviaDados = true;
                        MultLinha->Resume(); //Inicia processo.
                }
        }else{
                GLB_Conectado = false;
                Status->Panels->Items[0]->Text ="Desconectado";
        }
} else {
        // Se já tiver conectado
        strcpy(EnviaComando,EditComando->Text.c_str());
        GLBEnviaDados = true;
        MultLinha->Resume(); //Inicia processo.
}
EditComando->Text="";
       
}
//---------------------------------------------------------------------------

