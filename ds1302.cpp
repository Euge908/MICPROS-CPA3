#ifndef  F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include "ds1302.h"

struct DS1302_t ds1302_init(volatile uint8_t *sclk_port, uint8_t sclk_pin, 
  volatile uint8_t *sio_port, uint8_t sio_pin, volatile uint8_t *ce_port, uint8_t ce_pin)
{
  unsigned char c1,c2;
  DS1302_t device;

  device._sclk_port = sclk_port;
  device._sclk_pin = sclk_pin;
  device._sio_port = sio_port;
  device._sio_pin = sio_pin;
  device._ce_port = ce_port;
  device._ce_pin = ce_pin;

  *(device._ce_port) &= ~_BV(device._ce_pin);         // Default port configuration
  DDR(device._ce_port) |= _BV(device._ce_pin);        // CE output, low
  *(device._sclk_port) &= ~_BV(device._sclk_pin);       
  DDR(device._sclk_port) |= _BV(device._sclk_pin);    // SCLK output, low
  *(device._sio_port) &= ~_BV(device._sio_pin);       // SIO input, no pullup
  DDR(device._sio_port) &= ~_BV(device._sio_pin);

  c1 = ds1302_getbyte(&device, REG_WP);               // Test device presence (read WP byte)
  if (c1 != 0x00 && c1 != 0x80) {                     // Result should always be 0x00 or 0x80
    device.device_present = 0;
    return device;
  }
  c1 ^=_BV(7);                                        // Flip bit and see if we can write/reread it
  ds1302_setbyte(&device, REG_WP, c1);
  c2 = ds1302_getbyte(&device, REG_WP);
  if (c1 != c2) {                                     // New result should match written flipped bit
    device.device_present = 0;
    return device;
  }
  if (!c2)
    ds1302_setbyte(&device, REG_WP, 0x80);            // Leave WP set

  device.device_present = 1;
  return device;
}

struct DateTime ds1302_get_time(struct DS1302_t *device)
{
  unsigned char c1, s1[8];
  DateTime dt;
  dt.halted = 1;
                           
  *(device->_ce_port) |= _BV(device->_ce_pin);        // Set CE high
  Delay_ns(DS1302_TCC);
  ds1302_shiftout(device, REG_RTC_BURST);             // Burst read of time gets all registers in sync with each other
  for (c1 = 0; c1 < 8; c1++)
    s1[c1] = ds1302_shiftin(device);
  *(device->_ce_port) &= ~_BV(device->_ce_pin);       // Take CE back low
  Delay_ns(DS1302_CWH);
  if (s1[0] & _BV(7))                                 // If CH is set then clock is not set, return 0
    return dt;
  
  // Convert from BCD
  dt.year  =((s1[6] & (_BV(4)|_BV(5)|_BV(6)|_BV(7)))>>4)*10  +(s1[6] & (_BV(0)|_BV(1)|_BV(2)|_BV(3)));
  dt.month =((s1[4] & (_BV(4)))>>4)*10                       +(s1[4] & (_BV(0)|_BV(1)|_BV(2)|_BV(3)));
  dt.day   =((s1[3] & (_BV(4)|_BV(5)))>>4)*10                +(s1[3] & (_BV(0)|_BV(1)|_BV(2)|_BV(3)));
  dt.hour  =((s1[2] & (_BV(4)|_BV(5)))>>4)*10                +(s1[2] & (_BV(0)|_BV(1)|_BV(2)|_BV(3)));
  dt.minute=((s1[1] & (_BV(4)|_BV(5)|_BV(6)))>>4)*10         +(s1[1] & (_BV(0)|_BV(1)|_BV(2)|_BV(3)));
  dt.second=((s1[0] & (_BV(4)|_BV(5)|_BV(6)))>>4)*10         +(s1[0] & (_BV(0)|_BV(1)|_BV(2)|_BV(3)));
  dt.dow   =                                                  (s1[5] & (_BV(0)|_BV(1)|_BV(2)));
  dt.halted = 0;

  return dt;
}

void ds1302_set_time(struct DS1302_t *device, struct DateTime *dt)
{
  unsigned char c1, s1[8];
  
  // Convert to BCD
  s1[0]=((dt->second/10)<<4)+dt->second%10;
  s1[1]=((dt->minute/10)<<4)+dt->minute%10;
  s1[2]=((dt->hour/10)<<4)  +dt->hour%10;
  s1[3]=((dt->day/10)<<4)   +dt->day%10;
  s1[4]=((dt->month/10)<<4) +dt->month%10;
  s1[6]=((dt->year/10)<<4)  +dt->year%10;
  s1[7]=0;

  ds1302_setbyte(device, REG_WP, 0x00);               // Disable WP
  *(device->_ce_port) |= _BV(device->_ce_pin);        // Set CE high
  Delay_ns(DS1302_TCC);
  ds1302_shiftout(device, REG_RTC_BURST & ~_BV(0));   // Burst write of time sets all registers in sync with each other
  for (c1 = 0; c1 < 8; c1++)
    ds1302_shiftout(device, s1[c1]);
  *(device->_ce_port) &= ~_BV(device->_ce_pin);       // Take CE back low
  Delay_ns(DS1302_CWH);
  ds1302_setbyte(device, REG_WP, 0x80);               // Reenable WP
}

unsigned char ds1302_get_ram(struct DS1302_t *device, unsigned char AAddress)
{
  if (AAddress<31)
    return ds1302_getbyte(device, 0xc1+2*AAddress);
  else return 0;
}

void ds1302_set_ram(struct DS1302_t *device, unsigned char AAddress, unsigned char AValue)
{
  if (AAddress<31)
  {
    ds1302_setbyte(device, REG_WP, 0x00);             // Disable WP
    ds1302_setbyte(device, 0xc0+2*AAddress, AValue);  // Write
    ds1302_setbyte(device, REG_WP, 0x80);             // Reenable WP
  }
}

void ds1302_shiftout(struct DS1302_t *device, unsigned char AData)
{
  unsigned char Bits=8;

  DDR(device->_sio_port) |= _BV(device->_sio_pin);    // Set SIO to output
  while (Bits) {
    if (AData & _BV(0)) 
      *(device->_sio_port) |= _BV(device->_sio_pin);  // LSB sent first
    else *(device->_sio_port) &= ~_BV(device->_sio_pin);
    Delay_ns(DS1302_TDC);
    *(device->_sclk_port) |= _BV(device->_sclk_pin);  // Cycle SCLK
    Delay_ns(DS1302_CH);
    *(device->_sclk_port) &= ~_BV(device->_sclk_pin);
    Delay_ns(DS1302_CL);
    AData>>=1;                                        // Prepare to send next bit
    Bits--;
  }
  DDR(device->_sio_port) &= ~_BV(device->_sio_pin);   // Set SIO back to input
}

unsigned char ds1302_shiftin(struct DS1302_t *device)
{
  unsigned char Bits=8;
  unsigned int ui1=0;

  while (Bits) {
    ui1 >>= 1;
    ui1 |= (PIN(device->_sio_port) & _BV(device->_sio_pin)) ? 128 : 0;  // LSB received first
    *(device->_sclk_port) |= _BV(device->_sclk_pin);  // Cycle SCLK
    Delay_ns(DS1302_CH);
    *(device->_sclk_port) &= ~_BV(device->_sclk_pin);
    Delay_ns(DS1302_CL);
    Bits--;
  }
  return ui1;
}

unsigned char ds1302_getbyte(struct DS1302_t *device, unsigned char AAddress)
{
  unsigned char c1;

  *(device->_ce_port) |= _BV(device->_ce_pin);        // Set CE high
  Delay_ns(DS1302_TCC);
  ds1302_shiftout(device, AAddress | _BV(0));         // Send command byte
  c1 = ds1302_shiftin(device);                        // Get result
  *(device->_ce_port) &= ~_BV(device->_ce_pin);       // Take CE back low
  Delay_ns(DS1302_CWH);
  return c1;
}

void ds1302_setbyte(struct DS1302_t *device, unsigned char AAddress, unsigned char AValue)
{
  *(device->_ce_port) |= _BV(device->_ce_pin);        // Set CE high
  Delay_ns(DS1302_TCC);
  ds1302_shiftout(device, AAddress & ~_BV(0));        // Send command byte
  ds1302_shiftout(device, AValue);                    // Send value
  *(device->_ce_port) &= ~_BV(device->_ce_pin);       // Take CE back low
  Delay_ns(DS1302_CWH);
}
  
struct DateTime ds1302_date_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
  DateTime dt;
  dt.year = year % 100;
  dt.month = month;
  dt.day = day;
  dt.hour = hour;
  dt.minute = minute;
  dt.second = second;
  return dt;
}
