#include <avr/io.h>


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

float readVcc(){
  ADMUX = (1<<REFS0) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1);

  //burn the first 10 adc values
  for(int i = 0; i< 10; i++){
    ADCSRA |= (1<<ADSC); // start single conversion
    while(ADCSRA & (1<<ADSC)); // wait until conversion is complete
  }
  

  float Vcc = (1023/ ADC) * 1.1;//prone to some errors because of some math stuff but ok
  ADMUX = (1<<REFS0);
  
  return Vcc;
  
}


//function to set up soil sensor pins in analog mode
void setUpSoilSensorInAnalog(uint8_t analogPin, uint8_t powerPin){
	DDRD |= (1 << powerPin); //set port d as output
	PORTD |= (1 << powerPin); //set port d as high
	
	DDRC &= !(1 << analogPin);
}

//function to set up soil sensor in digital mode
//not gonna do it since analog is all I need

//function to read from soil sensor
uint16_t readSoilSensor(){
	
}
