#define F_CPU 16000000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define ROWS 4
#define COLS 4

#define NUM_MODES 4
char keyMap[ROWS][COLS] = { // key definitions
    {'3', '2', '1', '0'},
    {'7', '6', '5', '4'},
    {'A', '9', '^', '8'},
    {'S', '>', 'v', '<'}
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


void initKeypad();
char getKeyPressed();
void initTicker();
bool isNum(char c);
char* getSubstring(char * str, int start, int finish);


volatile uint32_t tick = 0;
uint32_t LCDCountTick; // time stamp


char LCDStr[2][20]; // LCD string display
char alarmTimes[5][20]; // Alarm Times String Display


// Variables for RTC
DS1302_t rtc; // DS1302 structure
DateTime dateTime; // date and time structure

int main(void)
{

    Serial.begin(9600);
    //for the sake of serial print 
    //The keypad column pins 1 to 4 are connected to PC0 to PC3 pins.
    // The keypad row pins 5 to 8 are connected to PB0 to PB3 pins.

    // Connection of LCD is as follows:
    // GND -> GND
    // VCC -> 5V
    // SDA -> A4
    // SCL -> A5
    
    // Connection of RTC is as follows:
    // CLK -> PD7
    // DAT -> PD6
    // RST -> PD5
    // VCC -> 3.3V (NOT 5V)

    
    char keyPressed = 0; // initially no keys were pressed yet
    uint8_t cursorX = 0, cursorY = 0; //for the display cursor position in LCD
    uint8_t mode = 0; //current mode to display or edit
            


    //mode 0 - view real time
    //mode 1 - set real time
    //mode 2 - show alarm times
    //mode 3 - set alarm times
    
    
    
    //initialize keypad stuff (registers)
    initKeypad();    


    //initialize ticker stuff (registers)
    initTicker(); // initialize ticker

    // Initialize the TWI module
    i2c_master_init(100000L); // set clock frequency to 100 kHz
    
    // Initialize the LCD module
    LiquidCrystalDevice_t lcd = lq_init(0x27, 16, 2, LCD_5x8DOTS);
    
    //clear/ synchronize with lcd
    lq_clear(&lcd);

    
    // Initialize DS1302 and DateTime structure. The RTC module is
    // connected as follows: SCLK -> PD7, SIO -? PD6, and CE -> PD5
    rtc = ds1302_init(&PORTD, PORTD7, &PORTD, PORTD6, &PORTD, PORTD5);
    
    // Initialize a new time to Dec 5, 2021 12:00:00
     dateTime = ds1302_date_time(2021, MONTH_DEC, 5, 12, 00, 00);
     ds1302_set_time(&rtc, &dateTime); // Sets the time. Should be commented once done

    
    lq_turnOnBacklight(&lcd); // turn on backlight
    _delay_ms(1000); //wait for 1 second

    
    while(1) {
      keyPressed = getKeyPressed();

//      Serial.print("X: ");
//      Serial.print(cursorX);
//      Serial.print(", Y: ");
//      Serial.println(cursorY);

      if (keyPressed == 'A'){
        //switch mode is pressed
        mode = (mode + 1) % NUM_MODES;
        _delay_ms(1000);
        lq_setCursor(&lcd, 0, 0);

        cursorX = 0; cursorY = 0;
      }
      
      if(mode == 0){
        //MODE 0
        //just display the time
        lq_turnOffCursor(&lcd);
        if (tick - LCDCountTick >= 250) { // update the display every 100 ticks
              dateTime = ds1302_get_time(&rtc); // get the RTC time
              if (!dateTime.halted) {
                  // Display the date in yyyy-mm-dd format
                  lq_setCursor(&lcd, 0, 0);
                  sprintf(LCDStr[0], "20%02u-%02u-%02u", dateTime.year, dateTime.month, dateTime.day);
                  lq_print(&lcd, LCDStr[0]);
                  
                  // Display the time in hh:mm:ss format (24-hour format)
                  lq_setCursor(&lcd, 1, 0);
                  sprintf(LCDStr[1], "%02u:%02u:%02u", dateTime.hour, dateTime.minute, dateTime.second);
                  lq_print(&lcd, LCDStr[1]);
              }
              LCDCountTick = tick;
        }
      }else if (mode == 1){
        //MODE 1
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
      
      }else if (mode == 2){
        //MODE 2 Display Alarm times 
        //alarmTimes

        //the <, >, and number keys; and cursor are disabled
        lq_turnOffCursor(&lcd);
    

        if(keyPressed == '^' && alarmDisplay1 + 1 <= 4 && alarmDisplay2 + 1 <= 4){
            alarmDisplay1 += 1;
            alarmDisplay2 += 1;

            //display text
        }else if (keyPressed = 'v' && alarmDisplay1 - 1 >= 0 && alarmDisplay2 - 1 >= 0){
            alarmDisplay1 -= 1;
            alarmDisplay2 -= 1;

            //display text
        }

        

        
        
      }else if (mode == 3){
        //MODE 3 Edit Alarm times
        lq_turnOffCursor(&lcd);
        

        
      }

      if(keyPressed != 0){
          _delay_ms(500); //wait for 0.5 second  
      }

    }
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
    //initialize PB0 to PB3 as inputs (by default DDRB is 0, but it is good practice to just initialize stuff)
    DDRB &= 0xF0;
    PORTB |= 0x0F; // Initialize rows pins as input with pull-up enabled
  
}

char getKeyPressed(){
  char keyPressed = 0;
  
  for (uint8_t c = 0; c < COLS; ++c) { // go through all column pins
      DDRC |= (1<<c); // we pulse each columns 
      PORTC &= ~(1<<c); // to LOW level
      for (uint8_t r = 0; r < ROWS; ++r){ // go through all row pins
          if (!(PINB & (1<<r))){
              // and check at which row was pulled LOW
              keyPressed = keyMap[r][c]; // assign the pressed key if confirmed
              _delay_ms(100); //debounce
          }

          if(keyPressed != 0){
            break;
          }
      }
      PORTC |= (1<<c); // end of column pulse
      DDRC &= ~(1<<c); // and set it back to input mode
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

ISR(TIMER0_OVF_vect) // this ISR is called approximately every 1 ms
{
    tick++; // increment tick counter
}
