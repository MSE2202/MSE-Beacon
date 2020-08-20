/*
  MSE 2202 Beacon
  2020 2021
  By EJ Porter


  beacon wil transmit at 2400 baud the ASCII character "U" (This can be changed at #define")
 
  output at IR LED  of ASCII byte is:
  if byte bit is a 1 ( high) LED is off
  if byte bit is a 0 (low) LED is pulsed at ~38kHz

  The low bits pulsing at ~38kHz will be deteced by the TSOP IR receiver and drive its output low
  if the output of the TSOP IR Receiver is connected to the Rx pin on the MSE-Duino and the MSE-Duino i sset up to read this serial port at 2400 baud.
  The MSE_Duino will read a "U" (0x55, or what ASCII character is defined)

  At 2400 baud , each bit is ~417uS
  output flow: 7 stop bits, 1 start bit(low) , 8 character bits. Repeat 
  There will a ~50mS delay between transmissions of the ASCII character to give the TSOP IR receiver time to reset bewteen receptions

  Beacon output will transmit #define NUMBER_OF_CHARACTERS_TRANSMITTED  then delay #define BLINK_OUTPUT_TIME_BETWEEN_CHARACTER_GROUP milliseconds before tranmitting chatater group again
  if continuous mode is needed  #defined CONTINUOUS un-commented

  TSOP 32338 IR Receiver pin 1 is output, 2 is Vcc, 3 is Gnd; pin on is on left side when looking at front of part 
 
*/

#include <Tone.h>

Tone IR_Output;


#define TX_CHAR 0x65 //"U"
#define BLANK_TIME_BETWEEN_CHARACTERS 50000  //50000 = 50mS
#define BIT_TIME 417
#define IR_TSOP_FREQUENCY 38000
//#define CONTINUOUS 1


#ifndef CONTINUOUS
 #define NUMBER_OF_CHARACTERS_TRANSMITTED 10
 #define BLINK_OUTPUT_TIME_BETWEEN_CHARACTER_GROUP 500  //in milliseconds
#endif

#define IRLED 12

boolean b_Bit;
unsigned char uc_Bit_Index;
unsigned char uc_Bit_Time_Counter;

unsigned char uc_Number_of_Characters;

unsigned int ui_Blink_Character_Counter;
unsigned int ui_Tranmit_Datum;

unsigned long ul_NowTime;
unsigned long ul_PreviousTime;



void setup() 
{
  // initialize digital pin LED_BUILTIN as an output.
  IR_Output.begin(IRLED);

  ui_Tranmit_Datum = (TX_CHAR << 8) | 0x007F;
 Serial.begin(9600);
 Serial.println(ui_Tranmit_Datum,BIN);
 Serial.println(ui_Tranmit_Datum,HEX);
  uc_Bit_Index = 0;
  uc_Number_of_Characters = NUMBER_OF_CHARACTERS_TRANSMITTED;
  b_Bit = true;
  uc_Bit_Time_Counter = 0;
  ui_Blink_Character_Counter = BLINK_OUTPUT_TIME_BETWEEN_CHARACTER_GROUP * 25;
   IR_Output.stop();  
}


void loop() 
{


 ul_NowTime = micros();
 if(ul_NowTime - ul_PreviousTime >= 40)
 {
  
  ul_PreviousTime = micros();
  uc_Bit_Time_Counter = uc_Bit_Time_Counter + 1;
  if(uc_Bit_Time_Counter >= 10)
  {
    uc_Bit_Time_Counter = 0;
    b_Bit = (boolean)((ui_Tranmit_Datum >> uc_Bit_Index) & 0x0001);
    uc_Bit_Index = uc_Bit_Index + 1;
    if(uc_Bit_Index >= 16)
    {
      uc_Bit_Index = 0;
      
    }
  }
 }
 if(b_Bit)
 {
  IR_Output.stop();  
 }
 else
 {  
  IR_Output.play(IR_TSOP_FREQUENCY, 10);
 }
              
                      
}
