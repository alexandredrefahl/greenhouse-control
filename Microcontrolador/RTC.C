#include <16F877.H> 
#fuses XT, NOWDT, NOPROTECT, BROWNOUT, PUT, NOLVP 
#use delay(clock=4000000) 
#use rs232(baud=9600, xmit=PIN_C6, rcv=PIN_C7, ERRORS) 

#include <ctype.h> 

#include <PCF8583.c> 

// The default date format for this test program is MM/DD/YY. 
// If you want to use Euro format (DD/MM/YY), then un-comment 
// the following line. 
// #define USE_EURO_DATE_FORMAT  1 

//================================= 
void main() 
{ 
char c; 
char weekday[10]; 
date_time_t dt; 


PCF8583_init(); 

// Allow the user to write a preset date and time to the 
// PCF8583, if desired.  
printf("Do you want to write a sample date/time of\n\r"); 

#ifdef USE_EURO_DATE_FORMAT 
printf("23/12/06 23:59:50 (Sunday) to the PCF8583 ? (Y/N)\n\r"); 
#else  // Use U.S. date format 
printf("12/23/06 23:59:50 (Sunday) to the PCF8583 ? (Y/N)\n\r"); 
#endif 

while(1) 
  { 
   c = getc();    // Wait for user to press a key 
   c = toupper(c); 
    
   if(c == 'Y') 
     { 
      dt.month   = 12;    // December 
      dt.day     = 31;    // 31 
      dt.year    = 06;    // 2006 
      dt.hours   = 23;    // 23 hours (11pm in 24-hour time) 
      dt.minutes = 59;    // 59 minutes  
      dt.seconds = 50;    // 50 seconds 
      dt.weekday = 0;     // 0 = Sunday, 1 = Monday, etc. 

      PCF8583_set_datetime(&dt);        
      printf("\n\r"); 
      printf("New date/time written to PCF8583.\n\r"); 
      printf("Watch it rollover to 2007.\n\r"); 
      break; 
     } 

   if(c == 'N') 
      break;  
  } 

printf("\n\r"); 
printf("Reading date/time from PCF8583:\n\r"); 

// Read the date and time from the PCF8583 and display 
// it once per second. 
while(1) 
  { 
   delay_ms(1000);    

   PCF8583_read_datetime(&dt);    

   strcpy(weekday, weekday_names[dt.weekday]); 

   #ifdef USE_EURO_DATE_FORMAT 
   printf("%s, %u/%u/%02u, %u:%02u:%02u\n\r", 
           weekday, dt.day, dt.month, dt.year, 
           dt.hours, dt.minutes, dt.seconds); 
   #else  // Use U.S. date format 
   printf("%s, %u/%u/%02u, %u:%02u:%02u\n\r", 
           weekday, dt.month, dt.day, dt.year, 
           dt.hours, dt.minutes, dt.seconds); 
   #endif 
  } 

}