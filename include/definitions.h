/**
 * All IO pin definitions
 */

// Lora module
#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_SCK 9
#define LoRa_nss 8
#define LoRa_dio1 14
#define LoRa_nrst 12
#define LoRa_busy 13


// Passive buzzer
#define PIN_BUZZER_PLUS 48
#define PIN_BUZZER_GND 33

/**
 * Rotary encoder (GIAK) KY-040 
 * https://www.amazon.de/GIAK-Drehwinkelgeber-Druckknopf-Automobilelektronik-Multimedia-Audio/dp/B09726Y8RB/ref=sr_1_1_sspa?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=1LTME6CGAIHYZ&keywords=rotary+encoder&qid=1688818356&sprefix=rotary+encoder%2Caps%2C77&sr=8-1-spons&sp_csd=d2lkZ2V0TmFtZT1zcF9hdGY&psc=1
 * Connect all pins directly
 */
#define PIN_ROTARY_DT 19  //  Blue
#define PIN_ROTARY_CLK 20 //  Yellow
#define PIN_ROTARY_SW 26  //  Green
#define PIN_ROTARY_GND 34 //  Black
// 3.3V to +pin           Red

/**
 * Voltage divider for battery measurements
 * R1 = 4.7MOhm
 * R2 = 4.7MOhm
 */
#define PIN_VBAT 5 // Yellow
#define DIODE_VOLTAGE_DROP 0.777

// laser
#define PIN_LASER 4 // 22k Ohm pullup resistor to 3.3v

// LEDs
#define PIN_LED_WHITE 35 // white led ledbuiltin
#define PIN_WS2812b 47

/**
 *  Other definitions
 */
#define RADIO_STANDBY_TIME_US 5000

#define NUM_LEDS_DISPLAY 8 * 32
#define NUM_LEDS_LASER 4
#define MAX_AMPS_PER_PIXEL 0.05
#define MAX_CONTINUOUS_AMPS 0.5 // should give approx 20 - 24h of battery life on a 12Wh battery
