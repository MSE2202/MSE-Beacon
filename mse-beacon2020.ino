/*
  MSE 2202 Beacon
  2020 2021
  By EJ Porter


  beacon will transmit at 2400 baud the ASCII character "U" (This can be changed at #define")
 
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

  While running red led blinks every 0/5 s
 
  If the limit switch is pressed the beacon will stop tramsitting for ~ 30 s  ; red led will stop blinking 
*/

#include <Tone.h>

Tone IR_Output;


#define TX_CHAR 0x55 //"U"
#define BLANK_TIME_BETWEEN_CHARACTERS 50000  //50000 = 50mS
#define BIT_TIME 417
#define IR_TSOP_FREQUENCY 38000

//#define CONTINUOUS 1


#define NUMBER_OF_CHARACTERS_TRANSMITTED 10
#define BLINK_OUTPUT_TIME_BETWEEN_CHARACTER_GROUP 500  //in milliseconds


#define IRLED 12   
#define IR_Resistor 11 
#define LIMITSWITCH_INPUT 6
#define LIMITSWITCH_OUTPUT 5  

boolean b_Bit;
boolean b_Freeze_Transmission;
boolean b_LimitSwitchHit;
boolean b_ToggleRedLED;

unsigned char uc_Bit_Index;
unsigned char uc_Bit_Time_Counter;

unsigned char uc_Number_of_Characters;

unsigned int ui_Blink_Character_Counter;
unsigned int ui_Tranmit_Datum;

unsigned int ui_ToggleRedLEDCounter;

unsigned long ul_NowTime;
unsigned long ul_PreviousTime;

unsigned long ul_LimitSwitchTimeOut;


void setup() 
{
  // initialize digital pin LED_BUILTIN as an output.
  IR_Output.begin(IRLED);

  ui_Tranmit_Datum = (TX_CHAR << 8) | 0x007F;
 Serial.begin(9600);
 Serial.println(ui_Tranmit_Datum,BIN);
 Serial.println(ui_Tranmit_Datum,HEX);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LIMITSWITCH_INPUT, INPUT_PULLUP);
  pinMode(LIMITSWITCH_OUTPUT, OUTPUT);
  pinMode(LIMITSWITCH_INPUT, INPUT_PULLUP);
  pinMode(11, OUTPUT);
  digitalWrite(IR_Resistor, LOW); //if you soldered the LED in backwards - change this to HIGH
  
  b_LimitSwitchHit = false;
  
  uc_Bit_Index = 0;
  uc_Number_of_Characters = NUMBER_OF_CHARACTERS_TRANSMITTED;
  b_Bit = true;
  uc_Bit_Time_Counter = 0;
  ui_Blink_Character_Counter = 0;

   b_Freeze_Transmission = false;

   IR_Output.stop();  
}


void loop() 
{

 
 ul_NowTime = micros();
 if(ul_NowTime - ul_PreviousTime >= 40)
 {
  ul_PreviousTime = micros();
  if(digitalRead(LIMITSWITCH_INPUT) == 0)
  {
    b_LimitSwitchHit = true;
    b_Freeze_Transmission = true;
     
  }
  
  if(b_LimitSwitchHit)
  {
    b_ToggleRedLED = true;
    ui_ToggleRedLEDCounter = 0;
    ul_LimitSwitchTimeOut = ul_LimitSwitchTimeOut + 1;
    if(ul_LimitSwitchTimeOut >= 750000) //~30s
    {
      ul_LimitSwitchTimeOut = 0;
      b_LimitSwitchHit = false;
      b_Freeze_Transmission = false;
    }
  }
  ui_ToggleRedLEDCounter = ui_ToggleRedLEDCounter + 1;
  if(ui_ToggleRedLEDCounter >= 6250) //~1/4 sec blink
  {
    ui_ToggleRedLEDCounter = 0;
    b_ToggleRedLED ^= 1;
    digitalWrite(LED_BUILTIN, b_ToggleRedLED);
  }
  
  
  if(b_LimitSwitchHit == false)
  {
    ui_Blink_Character_Counter = ui_Blink_Character_Counter + 1;
  }
  uc_Bit_Time_Counter = uc_Bit_Time_Counter + 1;
  if(uc_Bit_Time_Counter >= 10)
  {
    uc_Bit_Time_Counter = 0;
    b_Bit = (boolean)((ui_Tranmit_Datum >> uc_Bit_Index) & 0x0001);
    uc_Bit_Index = uc_Bit_Index + 1;
    if(uc_Bit_Index >= 16)
    {
      uc_Bit_Index = 0;
#ifndef CONTINUOUS    
      if(!b_Freeze_Transmission)
      {
        uc_Number_of_Characters = uc_Number_of_Characters - 1;
        if(uc_Number_of_Characters == 0)
        {
         uc_Number_of_Characters = NUMBER_OF_CHARACTERS_TRANSMITTED;
         b_Freeze_Transmission = true;
        }
      }
#endif      
    }
  }
 }
 if(b_Freeze_Transmission)
 {
   if(ui_Blink_Character_Counter >= BLINK_OUTPUT_TIME_BETWEEN_CHARACTER_GROUP * 25)
   {
    ui_Blink_Character_Counter = 0;
    b_Freeze_Transmission = false;
   }
 }
 else
 {
  if(b_Bit)
  {
  IR_Output.stop();  
  }
  else
  {  
  IR_Output.play(IR_TSOP_FREQUENCY, 10); //sends 10 cycles of 38Khz
  }
 }            
                      
}
