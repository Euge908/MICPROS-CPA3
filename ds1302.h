#ifndef DS1302_H
#define DS1302_H

#define USE_ADELAY_LIBRARY    0           // Set to 1 to use my ADELAY library, 0 to use internal delay functions
#define USE_TIMING            5           // Can be 2 (2V slower) or 5 (5V faster) - see DS1302 datasheet
#define TCS_REGISTER_DEFAULT  0x00        // Trickle charge register default value - see DS1302 datasheet for values

#define Delay_ns(__ns) \
	if((unsigned long) (F_CPU/1000000000.0 * __ns) != F_CPU/1000000000.0 * __ns)\
	__builtin_avr_delay_cycles((unsigned long) ( F_CPU/1000000000.0 * __ns)+1);\
	else __builtin_avr_delay_cycles((unsigned long) ( F_CPU/1000000000.0 * __ns))

#define PORT(x) (*x)
#define PIN(x) (*(x - 2))           // Address of Data Direction Register of Port X
#define DDR(x) (*(x - 1))           // Address of Input Register of Port X

#if (USE_TIMING==5)
	#define DS1302_TCC 1000
	#define DS1302_CWH 1000
	#define DS1302_TDC 50
	#define DS1302_CL  250
	#define DS1302_CH  250
#elif (USE_TIMING==2)
	#define DS1302_TCC 4000
	#define DS1302_CWH 4000
	#define DS1302_TDC 200
	#define DS1302_CL  1000
	#define DS1302_CH  1000
#else
	#error USE_TIMING must be 2 or 5
#endif

#define REG_SECONDS				0x81
#define REG_MINUTES           	0x83
#define REG_HOUR              	0x85
#define REG_DATE              	0x87
#define REG_MONTH             	0x89
#define REG_DAY               	0x8B
#define REG_YEAR              	0x8D
#define REG_WP                	0x8F
#define REG_RTC_BURST         	0xBF
#define REG_RAM_BURST		  	0xFF

typedef struct DateTime {
	uint8_t year; // year is 0-99 which represents 2000-2099
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t dow;
	uint8_t halted;
} DateTime;

/**
 * Months of year
 */
enum MONTH {
	MONTH_JAN = 1,
	MONTH_FEB = 2,
	MONTH_MAR = 3,
	MONTH_APR = 4,
	MONTH_MAY = 5,
	MONTH_JUN = 6,
	MONTH_JUL = 7,
	MONTH_AUG = 8,
	MONTH_SET = 9,
	MONTH_OCT = 10,
	MONTH_NOV = 11,
	MONTH_DEC = 12
};

/**
 * Days of week
 */
enum DOW {
	DOW_MON = 1,
	DOW_TUE = 2,
	DOW_WED = 3,
	DOW_THU = 4,
	DOW_FRI = 5,
	DOW_SAT = 6,
	DOW_SUN = 7
};

typedef struct DS1302_t {
	volatile uint8_t *_sclk_port, *_sio_port, *_ce_port;
	uint8_t _sclk_pin, _sio_pin, _ce_pin;
	uint8_t device_present;
} DS1302_t;

struct DS1302_t ds1302_init(volatile uint8_t *sclk_port, uint8_t sclk_pin, 						// called before all other functions, returns a DS1302_t struct
	volatile uint8_t *sio_port, uint8_t sio_pin, volatile uint8_t *ce_port, uint8_t ce_pin);	                            

struct DateTime ds1302_get_time(struct DS1302_t *device);              						  	// gets the time
void ds1302_set_time(struct DS1302_t *device, struct DateTime *dt);                             // sets the time

unsigned char ds1302_get_ram(struct DS1302_t *device, unsigned char AAddress);                	// gets a byte from user ram
void ds1302_set_ram(struct DS1302_t *device, unsigned char AAddress, unsigned char AValue);   	// sets a byte to user ram

void ds1302_shiftout(struct DS1302_t *device, unsigned char AData);
unsigned char ds1302_shiftin(struct DS1302_t *device);

unsigned char ds1302_getbyte(struct DS1302_t *device, unsigned char AAddress);					// these low level functions are not mentioned in the documentation
void ds1302_setbyte(struct DS1302_t *device, unsigned char AAddress, unsigned char AValue);     // but allow you to read or write to the ds1302 using the datasheet
                                                                       							// addresses on the datasheet page 9, table 3
struct DateTime ds1302_date_time(uint16_t year, uint8_t month, uint8_t day, 					// constructs a new DateTime struct
	uint8_t hour, uint8_t minute, uint8_t second);

#endif

