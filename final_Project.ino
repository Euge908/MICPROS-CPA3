//ITEM 1: NOT THE SPI SHIFT REGISTER 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/delay.h>


// This variable keeps track of number of ticks since MCU reset.
// Each tick corresponds to approximately 1 ms of time (1.024 ms
// to be exact).
volatile uint32_t tick = 0;
void initTicker(void);
void setSegVal(float val);


//adc stuff
void initADC();
uint16_t readADC(uint8_t channel);



// These variables are used to contain current count information
// and states to be used by SPI data transfer complete interrupt
volatile float seg7val = 1234; // set the initial value to be displayed
volatile uint8_t digitNum = 0;

uint32_t counTick = 0; // tick stamp for updating the count
inline uint8_t digit_to_7seg(uint8_t);


volatile char stringValue[5]; //can only story "X.XX" kind of num
void readInternalVoltage() ;

float Vcc;

int main(void)
{
    // In this example, the digit control lines are connected to
    // PC0 to PC3. The SN74HC595N IC lines are connected to the
    // SPI module as follows:
    // SER -> PB3 (MOSI)
    // RCLK -> PB2 (SS)
    // SRCLK -> PB5 (SCK)
    
    DDRB |= (1<<PORTB5) | (1<<PORTB3) | (1<<PORTB2); // set SCK, MOSI, and SS pins to output mode
    PORTC |= 0x0F; DDRC |= 0x0F; // setup digit control lines


    //initialize adc
    initADC();

  
    // Enable SPI in master mode with interrupts. Set SCK frequency to 125 kHz
    SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0); 
    SPDR = 0xFF; // initiate data transfer
    initTicker(); // initialize ticker
    Serial.begin(9600);

    setSegVal(1.23);

    
    while(1)
    {

      readInternalVoltage();
      float x = ( ((float)readADC(5) )/ 1023) * Vcc;
      
      setSegVal(x);
      Serial.println(Vcc);
      _delay_ms(100); //displays the value every 50ms

      
        // Since digit switching is taken care by the SPI interrupt,
        // we just increment the display value here every approximately
        // 1 second.
//    char x[5] = {stringValue};
//    Serial.println(x);        
    }
}



void readInternalVoltage(){
  ADMUX = (1<<REFS0) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1);
  //ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0) | (1<<ADEN); // set prescaler to 128 and enable ADC
  _delay_ms(200);


  //burn the first 10 adc values
  for(int i = 0; i< 11; i++){
    ADCSRA |= (1<<ADSC); // start single conversion
    while(ADCSRA & (1<<ADSC)); // wait until conversion is complete
  }
  

  Vcc = (1023/ ADC) * 1.1;//prone to some errors because of some math stuff but ok
  ADMUX = (1<<REFS0);
  
}





void initADC() {
    ADMUX |= (1<<REFS0); // selects internal AVCC as voltage reference
    ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0) | (1<<ADEN); // set prescaler to 128 and enable ADC
}

uint16_t readADC(uint8_t channel) {
    ADMUX |= (channel & 0x0F); // select the input channel
    _delay_ms(200);
    ADCSRA |= (1<<ADSC); // start single conversion
    while(ADCSRA & (1<<ADSC)); // wait until conversion is complete
    return ADC;
}



void setSegVal(float val){
    seg7val = val;
    float _seg7val = seg7val;

    uint8_t rem = fabs( ( (_seg7val -  floor(_seg7val)) * 100) );
    sprintf(stringValue,"%d.%02d", (int) _seg7val , rem); 
    
//  Serial.print("VAL: ");        
//  Serial.println((int) seg7val );
//  Serial.print("REM: ");        
//  Serial.println( rem);
//     Serial.print("STR: ");
    char x[5] = {stringValue};

//     Serial.println(x);
            

}




ISR(SPI_STC_vect) // this interrupt is executed after the SPI data transfer is done
{
    // copy these to local variables so they can be stored in registers
    // (volatile variables must be read from memory on every access)
    uint8_t _digitNum = digitNum;
    
    PORTB |= (1<<PORTB2); // set SS to HIGH
    PORTC |= 0x0F; // disable all digits

    SPDR = digit_to_7seg(stringValue[_digitNum]); // and start new data transfer
    
    PORTC &= ~(1<<_digitNum); // enable the digit
    _digitNum = (_digitNum + 1) % 4; // set next digit
    digitNum = _digitNum; // save the new states
    PORTB &= ~(1<<PORTB2); // set SS to LOW
}

inline uint8_t digit_to_7seg(uint8_t digit) { // converts a number to 7-segment display format
    if (digit == '0') return 0b00111111;
    else if (digit == '1') return 0b00000110;
    else if (digit == '2') return 0b01011011;
    else if (digit == '3') return 0b01001111;
    else if (digit == '4') return 0b01100110;
    else if (digit == '5') return 0b01101101;
    else if (digit == '6') return 0b01111101;
    else if (digit == '7') return 0b00000111;
    else if (digit == '8') return 0b01111111;
    else if (digit == '9') return 0b01101111;
    else if (digit == '.') return 0b10000000;
    else return 0;
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
