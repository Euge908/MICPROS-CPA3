#define F_CPU 16000000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define ROWS 4
#define COLS 4
#define NUM_MODES 3

char keyMap[ROWS][COLS] = { // key definitions
    {'0', '1', '2', '3'},
    {'4', '5', '6', '7'},
    {'8', '^', '9', 'A'},
    {'<', 'v', '>', 'S'}
};

//<, >, ^, v are arrow keys
//A is used to cycle through the mode(s) 
//Display num would be changed depending on the curor similar to the 'insert' key
//S is the button to save changes
//B is the mode to view the time
//this module should be able to change BOTH the date and time
//note that the lcd is a 16x2 lcd

#include "i2c_master.h"
#include "liquid_crystal_i2c.h"
#include "ds1302.h"

//FUNCTION HEADERS:

void initKeypad();
char getKeyPressed();
void initTicker();
bool isNum(char c);
char* getSubstring(char * str, int start, int finish);


// RELEVANT CONNECTIONS:

// Connection of LCD is as follows:
// GND -> GND
// VCC -> 5V
// SDA -> A4
// SCL -> A5


// The DS1302 RTC doesn't necessarily use the SPI registers of the arduino so it's ok to use DS1302 pins on MOSI, MISO, SS, and SCK (D10 to D13)
// Connection of RTC is as follows:
// CLK -> A3
// DAT -> A2
// RST -> A1
// VCC -> 3.3V (NOT 5V)


// Connections of Keypad:
// COLS 0 to 3 -> PD4 to PD7
// ROWS 0 to 3 -> PB0 to PB3

// Connections to Soil Sensor:
// Vcc -> 5V
// GND -> GND
// A0 -> A0

// Connection to Transistor Base -> PD3

// For the sake of Serial, D0 and D1 is unconnected to anything



// GLOBAL VARIABLE DECLARATIONS:

volatile uint32_t tick = 0;
uint32_t LCDCountTick; // time stamp



//NOTE: NULL FRICKING TERMINAL STILL COUNTS IN THE STRING LENGTH
char LCDStr[2][17]; // LCD string display
char alarmTimeStr[2][17]; // Alarm time string



char keyPressed = 0; // initially no keys were pressed yet


// Variables for RTC
DS1302_t rtc; // DS1302 structure
DateTime dateTime; // date and time structure



//MAIN FUNCTION:

int main(void)
{

    Serial.begin(9600);


    // LOCAL VARIABLE DECLARATIONS: 
    uint8_t cursorX = 0, cursorY = 0; //for the display cursor position in LCD
    uint8_t mode = 0; //current mode to display or edit

    //Initialize Array Values


    for(int i = 0, n = sizeof(alarmTimeStr)/sizeof(alarmTimeStr[0]); i < n; i++){
      memset(alarmTimeStr[i], ' ',sizeof(alarmTimeStr[i])/ sizeof(char));
      strncpy(alarmTimeStr[i], "__:__:__", 8 );
    }


    //MODES:
    //mode 0 - view time
    //mode 1 - set time
    //mode 2 - set alarm time
    //mode 4 - set moisture requirement??? (I think it would be better if hard coded)

    initTicker();
    

    
    // Initialize the TWI module
    i2c_master_init(100000L); // set clock frequency to 100 kHz
    // Initialize the LCD module
    LiquidCrystalDevice_t lcd = lq_init(0x27, 16, 2, LCD_5x8DOTS);
    
    //clear/ synchronize with lcd 2x, because of no capacitor
    lq_clear(&lcd);
    _delay_ms(500);
    
    // Initialize DS1302 and DateTime structure. The RTC module is
    // connected as follows: SCLK -> A3, SIO -> A2, and CE -> A1
    rtc = ds1302_init(&PORTC, PORTC3, &PORTC, PORTC2, &PORTC, PORTC1);
    
    // Initialize a new time to Dec 5, 2021 12:00:00
     dateTime = ds1302_date_time(2021, MONTH_DEC, 5, 12, 00, 00);
     ds1302_set_time(&rtc, &dateTime); // Sets the time. Should be commented once done

    
    lq_turnOnBacklight(&lcd); // turn on backlight
    _delay_ms(1000); //wait for 1 second

    lq_clear(&lcd);
    _delay_ms(500);

    
    //initialize keypad stuff (registers)
    initKeypad();  
    
    while(1) {
      //get keypress
      keyPressed = getKeyPressed();



//      Serial.print("X: ");
//      Serial.print(cursorX);
//      Serial.print(", Y: ");
//      Serial.println(cursorY);

     
  
      if (keyPressed == 'A'){
        //switch mode is pressed
        mode = (mode + 1) % NUM_MODES;        


        if(mode == 0){
          
        }else if (mode == 1){
        
        }else if(mode == 2){
          strncpy(LCDStr[0], alarmTimeStr[0], 12 );
          strncpy(LCDStr[1], alarmTimeStr[1], 12 );

          
        }

        LCDStr[0][13] = 'M';
        LCDStr[0][14] = ':';
        LCDStr[0][15] = mode + '0'; 
        //convert uint to char
        //subtract by '0' to convert char to uint



        
        lq_setCursor(&lcd, 0, 0);
        lq_print(&lcd, LCDStr[0]);
        
        lq_setCursor(&lcd, 1, 0);
        lq_print(&lcd, LCDStr[1]);
        
      }
    
      if(mode == 0){
        //just display the time
        lq_turnOffCursor(&lcd);
        if (tick - LCDCountTick >= 250) { // update the display every 100 ticks
              dateTime = ds1302_get_time(&rtc); // get the RTC time
              if (!dateTime.halted) {
                  // Display the date in yyyy-mm-dd format
                  lq_setCursor(&lcd, 0, 0);
                  sprintf(LCDStr[0], "20%02u-%02u-%02u   M:%01u\0", dateTime.year, dateTime.month, dateTime.day, mode);
                  lq_print(&lcd, LCDStr[0]);
                  
                  // Display the time in hh:mm:ss format (24-hour format)
                  lq_setCursor(&lcd, 1, 0);
                  sprintf(LCDStr[1], "%02u:%02u:%02u", dateTime.hour, dateTime.minute, dateTime.second);
                  lq_print(&lcd, LCDStr[1]);
              }
              LCDCountTick = tick;
        }

      }else if (mode == 1){
        lq_turnOnCursor(&lcd);
        lq_setCursor(&lcd, cursorY, cursorX);


        


        
        if(isNum(keyPressed)){
          //number is pressed
          if(isNum(LCDStr[cursorY][cursorX])){
             LCDStr[cursorY][cursorX] = keyPressed;

             lq_clear(&lcd);

             lq_setCursor(&lcd, 0, 0);
             lq_print(&lcd, LCDStr[0]);

             lq_setCursor(&lcd, 1, 0);
             lq_print(&lcd, LCDStr[1]);

             lq_setCursor(&lcd, cursorY, cursorX);

          }
          
        }
        else if (keyPressed == 'S'){
          //save is pressed
          
          //FORMAT:
          //LCDStr[0] = yyyy-mm-dd format 
          //LCDSTR[1] = hh:mm:ss
          
          //MONTH_DEC IS A NUM (ie 12)
          char *month, *day, *year, *hour, *minute, *second;
          year = getSubstring(LCDStr[0], 2, 4);
          month = getSubstring(LCDStr[0], 5, 7);
          day = getSubstring(LCDStr[0], 8, 10);
          hour  = getSubstring(LCDStr[1], 0, 2);
          minute = getSubstring(LCDStr[1], 3, 5);
          second = getSubstring(LCDStr[1], 6, 8);


//          char* x = (char*) malloc(100 * sizeof(char));
//          x[100] = 0;
          sprintf(LCDStr[0], "20%s-%s-%s", year, month, day);
          sprintf(LCDStr[1], "%s:%s:%s", hour, minute, second);


          //holy shit this works. special thanks to git --reset hard 
//          Serial.println(LCDStr[0]);
//          Serial.println(LCDStr[1]);
//          Serial.println(x);
//
//          sprintf(x, "%u-%u-%u || %u:%u:%u", atoi(year), atoi(month), atoi(day), atoi(hour), atoi(minute), atoi(second));
//          Serial.println(x);

          //dateTime = ds1302_get_time(&rtc); // get the RTC time
          
          dateTime = ds1302_date_time( atoi(year), atoi(month), atoi(day), atoi(hour), atoi(minute), atoi(second) );
          ds1302_set_time(&rtc, &dateTime); // Sets the time. Should be commented once done
          free(month);
          free(year);
          free(day);
          free(hour);
          free(minute);
          free(second);
//          free(x);


          
        }else if (keyPressed == '<' | keyPressed == '>' | keyPressed == '^' | keyPressed == 'v' ){
          //shift cursor buttons are pressed
  
          if(keyPressed == '<'){
            if( (cursorX - 1 >= 2 && cursorY == 0) || (cursorX - 1 >= 0 && cursorY == 1) ){
              cursorX -= 1;            
            }
          }else if(keyPressed == '>' && cursorX + 1 <=15){
            cursorX += 1;
          }else if(keyPressed == '^' && cursorY - 1 >= 0){
            if(cursorX >=2){
              cursorY -= 1;            
            }
            
          }else if(keyPressed == 'v' && cursorY + 1 <= 1){
            cursorY += 1;
          }

          
          lq_setCursor(&lcd, cursorY, cursorX);

        }
      
      } else if (mode == 2){
        lq_turnOnCursor(&lcd);
        lq_setCursor(&lcd, cursorY, cursorX);
        
        //allow input and keypad functionality
        if(isNum(keyPressed)){
          if(isNum(LCDStr[cursorY][cursorX]) || LCDStr[cursorY][cursorX] == '_'){
             LCDStr[cursorY][cursorX] = keyPressed;

             lq_clear(&lcd);

             lq_setCursor(&lcd, 0, 0);
             lq_print(&lcd, LCDStr[0]);

             lq_setCursor(&lcd, 1, 0);
             lq_print(&lcd, LCDStr[1]);

             lq_setCursor(&lcd, cursorY, cursorX);

          }
          
        }else if (keyPressed == '<' | keyPressed == '>' | keyPressed == '^' | keyPressed == 'v' ){
          //set boundary

          //shift cursor buttons are pressed
  
          if(keyPressed == '<' && cursorX - 1 >= 0){
              cursorX -= 1;             
          }else if(keyPressed == '>' && cursorX + 1 <=15){
            cursorX += 1;
          }else if(keyPressed == '^' && cursorY - 1 >= 0){
            cursorY -= 1;
          }else if(keyPressed == 'v' && cursorY + 1 <= 1){
            cursorY += 1;
          }
          
          lq_setCursor(&lcd, cursorY, cursorX);

        }else if (keyPressed == 'S'){}
        //Save button is pressed
                
      }



    }

}

ISR(TIMER0_OVF_vect) // this ISR is called approximately every 1 ms
{
    tick++; // increment tick counter
}

char* getSubstring(char * str, int start, int finish){
  int len = finish - start;
  char* buff = (char*) malloc((len + 1) * sizeof(char));
  
  for (int i = start; i < finish && (*(str + i) != 0); i++){
      *buff = *(str+ i);
      buff ++;
  }

  *buff = 0;
  
  return buff - len;
}



bool isNum(char keyPressed){
  if(keyPressed == '0' | keyPressed == '1' | keyPressed == '2' | keyPressed == '3' | keyPressed == '4' | keyPressed == '5' 
        | keyPressed == '6' | keyPressed == '7' | keyPressed == '7' | keyPressed == '8' | keyPressed == '9')
  {
    return true;          
  }

  return false;
  
}

void initKeypad(){
  //ROW PINS: PB0 TO PB3
  DDRB &= 0xF0;
  PORTB |= 0x0F; // Initialize rows pins as input with pull-up enabled
  
}

char getKeyPressed(){
  char keyPressed = 0;
  //COL PINS: PD4 TO PD7 
  
  for (uint8_t c = 0; c < COLS; ++c) { // go through all column pins
      DDRD |= (1 << c << 4); // we pulse each columns 
      PORTD &= ~(1 << c << 4); // to LOW level
      
      for (uint8_t r = 0; r < ROWS; ++r){ // go through all row pins
          if (!(PINB & (1<<r))){
              // and check at which row was pulled LOW
              keyPressed = keyMap[r][c]; // assign the pressed key if confirmed
          }
      }
      PORTD |= (1 << c << 4); // end of column pulse
      DDRD &= ~(1 << c << 4); // and set it back to input mode
  }

  if(keyPressed != 0){
    _delay_ms(1000);  
    Serial.print("KeyPressed: ");
    Serial.println(keyPressed);
  }

  return keyPressed;
}




void initTicker() {
    // We will set Timer0 to normal mode of operation but it will
    // generate an interrupt every time the counter overflows. The
    // interrupt will increment the tick counter every 1.024 ms
    
    TCNT0 = 0; // reset counter initially to 0
    TIMSK0 |= (1<<TOIE0); // enable overflow interrupt
    TCCR0B |= (1<<CS01) | (1<<CS00); // set prescaler to 64 and start timer
    sei(); // enable global interrupts
}
