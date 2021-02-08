/*
  MSE 2202 Beacon
  2020 2021
  By EJ Porter

  This program is used by the beacon to transmit the ASCII character "U" (This can be changed at #define") at 2400 baud
 
  Output at IR LED of ASCII byte is:
  if byte bit is a 1 (high) LED is off
  if byte bit is a 0 (low) LED is pulsed at ~38 kHz

  The low bits pulsing at ~38 kHz will be deteced by the TSOP IR receiver and drive its output low
  If the output of the TSOP IR Receiver is connected to the Rx pin on the MSEduino and the MSEduino is set up to read this serial port at 2400 baud,
  the MSEduino will read a "U" (0x55, or what ASCII character is defined)

  At 2400 baud , each bit is ~417 usec
  output flow: 7 stop bits, 1 start bit (low), 8 character bits. Repeat 

  Beacon output will transmit NUMBER_OF_CHARACTERS_TRANSMITTED then delay FREEZE_COUNT cycles before transmitting the character group again
  If continuous mode is needed, uncomment #define CONTINUOUS 1

  TSOP 32338 IR Receiver pin 1 is output, 2 is Vcc, 3 is Gnd; pin one is on left side when looking at front of part 

  While running, a red "heartbeat" LED blinks every 0.25 sec
 
  If the limit switch is pressed the beacon will stop tramsitting for ~30 s and the red LED will stop blinking 
*/

//#define CONTINUOUS 1  // uncomment for continuous transmission of character

#define NUMBER_OF_CHARACTERS_TRANSMITTED 10
#define FREEZE_COUNT 3188 // ~150 ms; 2125 ~= 100 ms

#define IRLED 12 //if you soldered the IR LED in backwards, change this to 11
#define IR_RESISTOR 11 //if you soldered the IR LED in backwards, change this to 12
#define LIMITSWITCH_INPUT 6
#define LIMITSWITCH_OUTPUT 5  

#define DATUM_BLANK 0x7F
#define DATUM_TO_TX1 0x55  // Tx an "U" (0x55) and then blank for one character 
#define DATUM_TO_TX2 0x41  // Tx an "A" (0x41) and then blank for one character 

boolean b_LimitSwitchHit;
boolean b_ToggleRedLED;

volatile int8_t vi8TXdata;

volatile int8_t v8_TooogleBit;
 
volatile unsigned char vuc_Freeze_Transmission;
volatile unsigned char vuc_Number_of_Characters;
volatile unsigned char vuc_ACKTx;
volatile unsigned char vuc_CarryBit;
volatile unsigned int vui_Tranmit_Datum;
volatile unsigned char vuc_Datum_High;

unsigned int ui_UnFreezeTimer;
unsigned int ui_ToggleRedLEDCounter;
unsigned long ul_NowTime;
unsigned long ul_PreviousTime;
unsigned long ul_LimitSwitchTimeOut;

void setup() 
{
  vi8TXdata = DATUM_TO_TX1;
  vui_Tranmit_Datum = DATUM_TO_TX1;   //set up data to transmit
  vui_Tranmit_Datum = (vui_Tranmit_Datum << 8) + DATUM_BLANK;
  vuc_CarryBit = 0x01;
  
  Serial.begin(115200);

  Serial.println(vui_Tranmit_Datum,BIN);
  Serial.println(" ");
  Serial.println(vui_Tranmit_Datum,HEX);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LIMITSWITCH_INPUT, INPUT_PULLUP);
  pinMode(LIMITSWITCH_OUTPUT, OUTPUT);
  pinMode(IR_RESISTOR, OUTPUT);
  pinMode(IRLED, OUTPUT);
  
  digitalWrite(IR_RESISTOR, LOW);

  //debug point 
  pinMode(8, OUTPUT);
  
  b_LimitSwitchHit = false;

  vuc_Freeze_Transmission = 0x00;
  vuc_ACKTx = 0;
  
  cli();//stop interrupts

  //set Timer1 interrupt at 2400 Hz
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 2400 Hz increments
  OCR1A = 103; // = (16*10^6) / (2400*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS01 and CS00 bits for 64 prescaler // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS01) | (1 << CS00); //(1 << CS12) | (1 << CS10);//
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  //set Timer2 interrupt at 76 kHz
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; //initialize counter value to 0
  // set compare match register for ~76 kHz increments
  OCR2A = 25; // = (16*10^6) / (2*38000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  
  sei();//allow interrupts
}

ISR(TIMER1_COMPA_vect,ISR_NOBLOCK)
{ //Timer1 interrupt 2400 Hz
  // generates pulse wave of frequency 2400 Hz - each bit is 1/2400 baud
  // rotate the vuc_Tx_Datum vaiable left and checks the carry flag
  // set vuc_CarryBit to whatever carry bit is 
  asm volatile(
     "cbi %6, %7         \n" //D8
     "sbrc %3, 0x00      \n" //ack need to be taken low to transmit
     "jmp 110f           \n" //goto end 
     "sec                \n" //set carry to hi
     "sbrs %1, 0x00      \n" //was last carry bit hi or low
     "clc                \n" //set carry to Low
     "ROR %B0            \n" //rotate Tx data lower byte to the left through carry
     "ROR %A0            \n" //rotate Tx data upper byte to the left through carry
     "BRCC 1f            \n" //check carry to see if low and if it is jump to 1
     "ldi %1, 0x01       \n" //store carry and carry is high so turn off 38Khz to LED
     "jmp 100f           \n" //goto end
     "1:                 \n" 
     "ldi %1, 0x00       \n" //store carry and carry is low so turn on 38Khz to LED
     "100:               \n" 
     "cp %B0, %4         \n" //check  vui_Tranmit_Datum high byte is charater to transmit ( might need to change it if you change Tx character
     "brne 110f          \n"
     "cpi %A0, %5        \n" //check  vui_Tranmit_Datum low byte is 0x7f startbit in UART communications
     "brne 110f          \n"
     "inc %2             \n"  //count the number of times the character was Transmitted
     "ldi %3, 0xFF       \n" //ack a charater has gone out
     "sbi %6, %7         \n" //D8
     "110:               \n" 
     :"+r" (vui_Tranmit_Datum),"+r" (vuc_CarryBit),"+r" (vuc_Number_of_Characters),"+r" (vuc_ACKTx)
     :"r" (vi8TXdata),"M" (DATUM_BLANK),"I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB0)
  );
}

 
ISR(TIMER2_COMPA_vect,ISR_BLOCK)
{ //timer2 interrupt 38kHz
  //generates pulse wave of frequency 38 kHz - set at 76 kHz/2 (takes two cycles for full wave)
  //toggle the D11 pin if the carry bit is clear
  //if vuc_CarryBit is high then don't toggle D11
  //if vuc_CarryBit is low then toggle D11 at 38 kHz to LED 
  
  //arduino:
  //Port Registers
  //    B (digital pin 8 to 13)
  //    C (analog input pins)
  //    D (digital pins 0 to 7) 
  //
  //    DDRD - The Port D Data Direction Register - read/write
  //    PORTD - The Port D Data Register - read/write
  //    PIND - The Port D Input Pins Register - read only 
  asm volatile(
     "sbrc %1, 0x00      \n" //if last carry bit was high then don't transmit
     "jmp 100f           \n" //goto end 
     "inc %0             \n"// toggle v8_TooogleBit
     "sbrc %0, 0x00      \n" //is v8_TooogleBit hi
     "cbi %2, %3         \n" //Toggle D11 low
     "sbrs %0, 0x00      \n" //was last v8_TooogleBit hi
     "100:               \n"
     "sbi %2, %3         \n" //Toggle D11 high
     : "+r" (v8_TooogleBit)
     : "r" (vuc_CarryBit),"I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB4)
  ); 
}

void loop() 
{
  ul_NowTime = micros();
  if(ul_NowTime - ul_PreviousTime >= 40)
  {
    ul_PreviousTime = micros();
    
    //Limit Switch code
    //**********************************************************************************************
    if(digitalRead(LIMITSWITCH_INPUT) == 0)
    {
      b_LimitSwitchHit = true; 
    }
    if(b_LimitSwitchHit)
    { 
      ul_LimitSwitchTimeOut = ul_LimitSwitchTimeOut + 1;
      if(ul_LimitSwitchTimeOut >= 250000) // ~10 sec
      {
        ul_LimitSwitchTimeOut = 0;
        b_LimitSwitchHit = false;
      }
    }
    ui_ToggleRedLEDCounter = ui_ToggleRedLEDCounter + 1;
    if((ui_ToggleRedLEDCounter >= 6250) || (ui_ToggleRedLEDCounter >= 1560 && b_LimitSwitchHit)) // ~1/4 sec blink; ~1/16 sec when hit
    {
      ui_ToggleRedLEDCounter = 0;
      b_ToggleRedLED ^= 1;
      digitalWrite(LED_BUILTIN, b_ToggleRedLED);
    }
      
    //end Limit Switch code
    //***********************************************************************
  
    //unfreeze after FREEZE_COUNT counts
    if(vuc_Freeze_Transmission > 0)
    {
      ui_UnFreezeTimer = ui_UnFreezeTimer + 1;
      if(ui_UnFreezeTimer  > FREEZE_COUNT)
      {
        ui_UnFreezeTimer = 0;
        vuc_Freeze_Transmission = 0;
      }
    }
    //Tx NUMBER_OF_CHARACTERS_TRANSMITTED stop Txing the same number of characters, rinse repeat
  
#ifndef CONTINUOUS    
    if((vuc_ACKTx > 0) && (vuc_Freeze_Transmission == 0))
    {
      if(b_LimitSwitchHit)
      {
        vi8TXdata = DATUM_TO_TX2;
        vui_Tranmit_Datum = DATUM_TO_TX2;   //set up data to transmit
        vui_Tranmit_Datum = (vui_Tranmit_Datum << 8) + DATUM_BLANK;
      }
      else
      {
        vi8TXdata = DATUM_TO_TX1;
        vui_Tranmit_Datum = DATUM_TO_TX1;   //set up data to transmit
        vui_Tranmit_Datum = (vui_Tranmit_Datum << 8) + DATUM_BLANK;
      }
      if(vuc_Number_of_Characters >= NUMBER_OF_CHARACTERS_TRANSMITTED)
      {
        vuc_Number_of_Characters = 0;
        vuc_Freeze_Transmission = 0xFF;
      }
      else
      {
         vuc_ACKTx = 0;
      }
    }
#else   
    if((vuc_ACKTx >= 1))
    {
      if(b_LimitSwitchHit)
      {
        vi8TXdata = DATUM_TO_TX2;
        vui_Tranmit_Datum = DATUM_TO_TX2;   //set up data to transmit
        vui_Tranmit_Datum = (vui_Tranmit_Datum << 8) + DATUM_BLANK;
      }
      else
      {
        vi8TXdata = DATUM_TO_TX1;
        vui_Tranmit_Datum = DATUM_TO_TX1;   //set up data to transmit
        vui_Tranmit_Datum = (vui_Tranmit_Datum << 8) + DATUM_BLANK;
      }
      vuc_Freeze_Transmission = 0x0;
      vuc_ACKTx = 0;
      vuc_Number_of_Characters = 0;
    }
#endif
  } 
}
