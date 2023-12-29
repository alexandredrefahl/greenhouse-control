///////////////////////////////////////////////////////////////////////////////
#define ACK               0
// Command byte values         adr cmd  r/w
#define Reset        0x1e   // 000 1111  0
#define MeasureTemp  0x03   // 000 0001  1
#define MeasureHumi  0x05   // 000 0010  1
#define WrStatusReg  0x06   // 000 0011  0
#define RdStatusReg  0x07   // 000 0011  1
// Define Data & Clock Pins

//#define sht_data PIN_B7
//#define Sck  	   PIN_B6

short error, chkCRC, Acknowledge;
int TimeOut,err;

/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
float CalcTempValues(long Lx){
//long CalcTempValues(long Lx){
 long value;
 float Fx;

  Fx=0.01*(float)Lx;
  Fx=Fx-40;
  //Value=Fx*10;

  //return(value);
  return(Fx);
}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
float CalcHumiValues(long Lx){
//long CalcHumiValues(long Lx){
 long Value;
 float Fx, Fy;

  Fx=(float)Lx*(float)Lx;
  Fx=Fx*(-0.0000028);
  Fy=(float)Lx*0.0405;
  Fx=Fx+Fy;
  Fx=Fx-4;
  //Value=Fx*10;
  //return(value);
  return(Fx);
}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
void TransmitStart(){

  output_high(sht_data);
  output_low(Sck);  delay_us(2);
  output_high(SCK); delay_us(2);
  output_low(sht_data); delay_us(2);
  output_low(SCK);  delay_us(6);
  output_high(SCK); delay_us(2);
  output_high(sht_data);delay_us(2);
  output_low(SCK);  delay_us(2);
}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
void RST_Connection(){
 int i;

  output_high(sht_data);
  for(i=1;i<=10;++i) {
      output_high(SCK); delay_us(2);
      output_low(SCK);  delay_us(2);
  }
  TransmitStart();
}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
void RST_Software(){

}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
int ReadStatReg(int command){

}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
long ReadValue(){
 byte i, ByteHigh=0, ByteLow=0;
 long Lx;

  for(i=1;i<=8;++i){                     // read high byte VALUE from SHT11
     output_high(SCK);
     delay_us(2);
     shift_left(&ByteHigh,1,input(sht_data));
     output_low(SCK);
     delay_us(2);
  }
  output_low(sht_data);
       delay_us(2);
  output_high(SCK);
  delay_us(2);
  output_low(SCK);
  output_float(sht_data);
  delay_us(2);

  for(i=1;i<=8;++i){                     // read low byte VALUE from SHT11
     output_high(SCK);
     delay_us(2);
     shift_left(&ByteLow,1,input(sht_data));
     output_low(SCK);
     delay_us(2);
  }
  output_high(sht_data);
       delay_us(2);
  output_high(SCK);
  delay_us(2);
  output_low(SCK);
  output_float(sht_data);
  delay_us(2);
  Lx=make16(ByteHigh,ByteLow);
  return(Lx);
}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
void SendCommand(int Command){
 byte i;

  for(i=128;i>0;i/=2){
     if(i & Command) output_high(sht_data);
     else            output_low(sht_data);
     delay_us(2);
     output_high(SCK);
     delay_us(2);
     output_low(SCK);
  }
  output_float(sht_data);
  delay_us(2);
  output_high(SCK);
  delay_us(2);
  output_low(SCK);
}

/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
int WriteStatReg(int command){
   unsigned char i,error=0;
   
   TransmitStart();
   SendCommand(WrStatusReg);
   for (i=0x80;i>0;i=i/2) {
      if (i & command) {
         output_high(sht_data);
      } else {
         output_low(sht_data);
      }
      delay_us(2);
      output_high(SCK);       //Clock in SCK
      delay_us(5);            //Pulse width aprox 5 us
      output_low(SCK);
      delay_us(2);            //Wait 2 us
   }
   output_high(sht_data);     //Release Data-Line
   output_high(SCK);          //Clock number 9 for ACK
   error=input(sht_data);     //Check ACK (Data will be 0 in case of OK)
   output_low(SCK);
   return error;              //error=1 in case of NO ACK
}

/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
float ReadTemperature(){
//long ReadTemperature(){
 short Acknowledge;
 long Lx; //,Value;
 float Value;

  TransmitStart();
  SendCommand(MeasureTemp);
  while(input(sht_data));
  delay_us(2);
  Lx=ReadValue();                        // Read temperature value
  Value=CalcTempValues(Lx);
  return(Value);
}
/////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////
long ReadHumidity(){
//long ReadHumidity(){
 long Lx; //,Value;
 float Value;

  TransmitStart();
  SendCommand(MeasureHumi);
  while(input(sht_data));
  delay_us(2);
  Lx=ReadValue();                        // Read humidity value
  Value=CalcHumiValues(Lx);
  return(Value);
}
