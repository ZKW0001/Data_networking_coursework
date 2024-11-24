/*******************************************************************************
  * main.c
  ******************************************************************************
  * EEEN30111 Lab 1 
	* Kaiwen Zhao 03/11/2024
  *****************************************************************************/
	
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"
#include "i2c.h"

/* Private includes ----------------------------------------------------------*/
#include "lcd.h"
#include "fonts/Fonts.h"
#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define LM75BD_ADDRESS 0x48
#define EEPROM_ADDRESS 0X50 //IF ALL SWITCHES ARE SET TO ZERO
#define BACKUP_EEPROM_ADDRESS 0x51 //A0 TO 1

/* Global variables ----------------------------------------------------------*/
Display mbed_lcd;
//packet data
uint8_t packet[80];

uint32_t dest = 0xcafebabe;
uint32_t src = 0xc0ffeec0;
uint16_t length = 0x41;
uint8_t payload[70] = {0};
uint16_t checksum = 0;
//eeprom data
uint32_t dest_from_eeprom;
uint32_t src_from_eeprom;
uint16_t length_from_eeprom;
uint8_t payload_from_eeprom[70];
uint16_t checksum_from_eeprom = 0;

//eeprom backup data
uint32_t dest_from_backup_eeprom;
uint32_t src_from_backup_eeprom;
uint16_t length_from_backup_eeprom;
uint8_t payload_from_backup_eeprom[70];
uint16_t checksum_from_backup_eeprom = 0;

//state
uint8_t state = 0;
/* Function prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
void rgb_led_set(rgb_led_t colour);
//joystick functions
uint32_t joystick_centre(void);
uint32_t joystick_left(void);
uint32_t joystick_right(void);
uint32_t joystick_up(void);
uint32_t joystick_down(void);
//i2c functions
void configure_i2c(void);
//display functions
void display_temperature_in_eeprom(void);
void display_temperature_in_memory(void);
void display_written(void);
void display_field_up(uint8_t value);
void display_field_down(uint8_t value);
//eeprom functions and temperature functions
uint16_t get_temperature(void);
void write_eeprom(uint8_t lowbyte, uint8_t highbyte);
void read_eeprom(uint8_t lowbyte, uint8_t highbyte);
void write_backup_eeprom(uint8_t lowbyte, uint8_t highbyte);
void read_backup_eeprom(uint8_t lowbyte, uint8_t highbyte);
//packet functions and checksum
uint16_t Checksum(uint8_t const data[80]);
void CreatePacket(uint8_t packet[80], uint32_t dest, uint32_t src, uint16_t length, uint8_t payload[70]);
/* Program start--------------------------------------------------------------*/
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initialises the Flash interface and  Systick.  */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0); /* System interrupt init*/

  /* Configure the system clock to 84 MHz */
  SystemClock_Config();

  /* Initialise peripherals */
  MX_GPIO_Init(); // Initialises the GPIO for RGB LED and LCD
  MX_SPI1_Init(); // Enables SPI for LCD

  /* Setup LCD software structure */
  mbed_lcd.height = 32;
  mbed_lcd.width = 128;
  mbed_lcd.font = (unsigned char*)ArialR20x20; // If you want to use a different font, change this (see fonts folder for available fonts)

  LL_SPI_Enable(SPI1);
  lcd_chip_select_pin(1);
  LL_mDelay(5);
  lcd_chip_select_pin(0);
  lcd_display_init(&mbed_lcd);
  lcd_clear_screen(&mbed_lcd);

  /* Initialise the RGB LED */
  rgb_led_t colour = RGB_LED_RED;

	/* Cycle the RGB LED a few times as a welcome message */
  for(int i = 0; i < 20; i++){
    rgb_led_set(colour);
    colour++;
    if(colour == RGB_LED_OFF){
      colour = RGB_LED_RED; // Reset colour
    }
    LL_mDelay(50);
  }
  rgb_led_set(RGB_LED_OFF);
	
	configure_i2c();

  /* Infinite loop */
	while (1)
  {
    if (joystick_centre()){
      state = 0;
      rgb_led_set(RGB_LED_RED);
      display_temperature_in_memory(); // Display temperature in memory
      rgb_led_set(RGB_LED_OFF);
      LL_mDelay(200);
    }
    else if(joystick_left()){
      state = 0;
      read_eeprom(0, 0);
      rgb_led_set(RGB_LED_GREEN);
      display_temperature_in_eeprom(); // Display temperature in eeprom
      rgb_led_set(RGB_LED_OFF);
      LL_mDelay(200);
    }
    else if(joystick_right()){
      state = 0;
      write_eeprom(0, 0);
      rgb_led_set(RGB_LED_BLUE); // Write to eeprom
      display_written();
      rgb_led_set(RGB_LED_OFF);
      LL_mDelay(200);
    }
    else if(joystick_up()){
      rgb_led_set(RGB_LED_YELLOW);
      read_eeprom(0, 0);
      display_field_up(state); // Display field up
      rgb_led_set(RGB_LED_OFF);
      LL_mDelay(200);
	  }
    else if(joystick_down()){
      rgb_led_set(RGB_LED_CYAN);
      read_eeprom(0, 0);
      display_field_down(state); // Display field down
      rgb_led_set(RGB_LED_OFF);
      LL_mDelay(200);
    }
  }
}


void CreatePacket(uint8_t packet[80], uint32_t dest, uint32_t src, uint16_t length, uint8_t payload[70]) {
    // Combine dest into the packet
    packet[0] = (dest >> 24) & 0xFF;  // Most significant byte
    packet[1] = (dest >> 16) & 0xFF;
    packet[2] = (dest >> 8) & 0xFF;
    packet[3] = dest & 0xFF;          // Least significant byte

    // Combine src into the packet
    packet[4] = (src >> 24) & 0xFF;   // Most significant byte
    packet[5] = (src >> 16) & 0xFF;
    packet[6] = (src >> 8) & 0xFF;
    packet[7] = src & 0xFF;           // Least significant byte

    // Combine length into the packet
    packet[8] = (length >> 8) & 0xFF; // Most significant byte
    packet[9] = length & 0xFF;        // Least significant byte

    // Copy the payload into the packet
    for (int i = 0; i < 70; i++) {
        packet[10 + i] = payload[i];  // Manually copy each byte of the payload
    }
}


void display_temperature_in_memory(void){
  uint32_t temperature_data = get_temperature() * 0.125f;
  uint16_t temperature_from_memory = ((payload[68] << 8) | payload[69]); // Combine the two bytes into one value
  CreatePacket(packet, dest, src, length, payload);
  checksum = Checksum(packet); // Calculate the checksum
	char temp_string[20];
	lcd_clear_screen(&mbed_lcd);
  mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
  sprintf(temp_string, "EEPROM Temp: %d C", (uint32_t)temperature_data); // Display the temperature in memory
  lcd_put_string(&mbed_lcd, 0, 0, temp_string);
  mbed_lcd.font = ((unsigned char*)Arial11x11);
  sprintf(temp_string, "Sampled: %.4X", (uint16_t)temperature_from_memory); // Display the temperature in memory
  lcd_put_string(&mbed_lcd, 0, 20, temp_string); 
  LL_mDelay(100);
}

void display_temperature_in_eeprom(void){
  uint16_t temperature_from_eeprom = ((payload_from_eeprom[68] << 8) | payload_from_eeprom[69]);
  uint32_t temperature_data = temperature_from_eeprom * 0.125f; // Convert the temperature to degree Celsius
	char temp_string[20];
	lcd_clear_screen(&mbed_lcd);
  mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
  sprintf(temp_string, "EEPROM Temp: %d C", (uint32_t)temperature_data); // Display the temperature in eeprom
  lcd_put_string(&mbed_lcd, 0, 0, temp_string);
  mbed_lcd.font = ((unsigned char*)Arial11x11);
  sprintf(temp_string, "Memory: %.4X", (uint16_t)temperature_from_eeprom); // Display the temperature in eeprom
  lcd_put_string(&mbed_lcd, 0, 20, temp_string); 
  LL_mDelay(100);
}

void display_written(void){
	char temp_string[20];
	lcd_clear_screen(&mbed_lcd);
  mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
  sprintf(temp_string, "Written packet");  // Display written packet
  lcd_put_string(&mbed_lcd, 0, 0, temp_string);
  LL_mDelay(100);
}

void display_field_up(uint8_t value){
  if (state <= 7){
    state++;
  }
  else{
    state = 7;
  } //state change from 1 to 7

  if (state == 1){  // Display MAC Destination
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "MAC Destination:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", dest_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 2){   // Display MAC Source
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "MAC Source:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", src_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 3){   // Display Length
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Length:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", length_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 4){   // Display Payload
    uint16_t payload_combined = (payload_from_eeprom[68] << 8) | payload_from_eeprom[69];
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Payload:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", payload_combined);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 5){   // Display Checksum
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Checksum:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", checksum_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
    }

  else if (state == 6){   // Checksum: OK or ERROR
    if (checksum == checksum_from_eeprom){
      char temp_string[20];
      lcd_clear_screen(&mbed_lcd);
      mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
      sprintf(temp_string, "Checksum: OK");
      lcd_put_string(&mbed_lcd, 0, 0, temp_string);
      mbed_lcd.font = ((unsigned char*)Arial11x11);
      sprintf(temp_string, "0x%.4x", checksum_from_eeprom);
      lcd_put_string(&mbed_lcd, 0, 20, temp_string);
      LL_mDelay(100);
    }
    else{
      char temp_string[20];
      lcd_clear_screen(&mbed_lcd);
      mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
      sprintf(temp_string, "Checksum: ERROR");
      lcd_put_string(&mbed_lcd, 0, 0, temp_string);
      mbed_lcd.font = ((unsigned char*)Arial11x11);
      sprintf(temp_string, "0x%.4x", checksum_from_eeprom);
      lcd_put_string(&mbed_lcd, 0, 20, temp_string);
      LL_mDelay(100);
    }
    }
  else if (state == 7){   // Backup EEPROM
    write_backup_eeprom(0, 0);
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Backed up 82-byte");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "packet to EEPROM 2");
    lcd_put_string(&mbed_lcd, 0, 15, temp_string);
    rgb_led_set(RGB_LED_PURPLE);  
    LL_mDelay(100);
    rgb_led_set(RGB_LED_OFF);
    LL_mDelay(100);
    rgb_led_set(RGB_LED_PURPLE);
    LL_mDelay(100);
    }
  }


void display_field_down(uint8_t value){
  if (state >= 1){
    state--;
    }
  else{
    state = 1;
  }   //state change from 7 to 1

  if (state == 1){  // Display MAC Destination
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "MAC Destination:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", dest_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 2){   // Display MAC Source
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "MAC Source:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", src_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }

  else if (state == 3){   // Display Length
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Length:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", length_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 4){   // Display Payload
    uint16_t payload_combined = (payload_from_eeprom[68] << 8) | payload_from_eeprom[69];
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Payload:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", payload_combined);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
  }
  else if (state == 5){   // Display Checksum
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Checksum:");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Arial11x11);
    sprintf(temp_string, "0x%.4x", checksum_from_eeprom);
    lcd_put_string(&mbed_lcd, 0, 20, temp_string);
    LL_mDelay(100);
    }

  else if (state == 6){   // Checksum: OK or ERROR
    if (checksum == checksum_from_eeprom){
      char temp_string[20];
      lcd_clear_screen(&mbed_lcd);
      mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
      sprintf(temp_string, "Checksum: OK");
      lcd_put_string(&mbed_lcd, 0, 0, temp_string);
      mbed_lcd.font = ((unsigned char*)Arial11x11);
      sprintf(temp_string, "0x%.4x", checksum_from_eeprom);
      lcd_put_string(&mbed_lcd, 0, 20, temp_string);
      LL_mDelay(100);
    }
    else{
      char temp_string[20];
      lcd_clear_screen(&mbed_lcd);
      mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
      sprintf(temp_string, "Checksum: ERROR");
      lcd_put_string(&mbed_lcd, 0, 0, temp_string);
      mbed_lcd.font = ((unsigned char*)Arial11x11);
      sprintf(temp_string, "0x%.4x", checksum_from_eeprom);
      lcd_put_string(&mbed_lcd, 0, 20, temp_string);
      LL_mDelay(100);
    }
  }
  else if (state == 7){   // Backup EEPROM
    write_backup_eeprom(0, 0);
    char temp_string[20];
    lcd_clear_screen(&mbed_lcd);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "Backed up 82-byte");
    lcd_put_string(&mbed_lcd, 0, 0, temp_string);
    mbed_lcd.font = ((unsigned char*)Fixedsys8x14);
    sprintf(temp_string, "packet to EEPROM 2");
    lcd_put_string(&mbed_lcd, 0, 15, temp_string);
    rgb_led_set(RGB_LED_PURPLE);
    LL_mDelay(100);
    rgb_led_set(RGB_LED_OFF);
    LL_mDelay(100);
    rgb_led_set(RGB_LED_PURPLE);
    LL_mDelay(100);
    }

}

uint16_t Checksum(uint8_t const data[80]) {
    uint32_t sum = 0;

    /*
     * Process each 16-bit word in the 80-byte data.
     */
    for (int i = 0; i < 80; i += 2) {
        uint16_t word = (data[i] << 8) | data[i + 1];  // Combine two bytes into one 16-bit word
        sum += word;

        // If overflow occurs, wrap it around
        if (sum & 0x10000) {  // Check if the sum exceeds 16 bits
            sum = (sum & 0xFFFF) + 1;
        }
    }

    /*
     * Use carries to compute the 1's complement sum.
     */
    sum = (sum >> 16) + (sum & 0xFFFF);  // Add the upper 16 bits to the lower 16 bits
    sum += (sum >> 16);                  // Add any remaining carry

    /*
     * Return the inverted 16-bit result.
     */
    return (uint16_t)(~sum);  // One's complement and truncate to 16 bits
}

void configure_i2c(void){
	// Setup I2C GPIO pins (PB8 and PB9)
	// Configure SCL(PB8) as Alternate function, Open Drain, Pull Up:
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF_4);
	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);

	// Configure SDA(PB9) as: Alternate, Open Drain, Pull Up:
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_4);
	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);
	
	// Enable the I2C1 Peripheral:
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
	LL_I2C_Disable(I2C1);  // disable I2C1 prior to configuration
	LL_I2C_SetMode(I2C1, LL_I2C_MODE_I2C);
	LL_I2C_ConfigSpeed(I2C1, 84000000, 100000, LL_I2C_DUTYCYCLE_2);  // set speed to 100 kHz
	LL_I2C_Enable(I2C1);  // re-enable I2C1
}

uint16_t get_temperature(void){
	// LM75BD_ADDRESS is a macro that equates to 0x48, the 7-bit address of the sensor

	// Select the temperature register of the LM75BD
	LL_I2C_GenerateStartCondition(I2C1);  // START
	while(!LL_I2C_IsActiveFlag_SB(I2C1));

	LL_I2C_TransmitData8(I2C1, (LM75BD_ADDRESS<<1));  // ADDRESS + WRITE
	while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
	LL_I2C_ClearFlag_ADDR(I2C1);

	LL_I2C_TransmitData8(I2C1, 0x00);  // Select temperature register
	while(!LL_I2C_IsActiveFlag_TXE(I2C1));

	LL_I2C_GenerateStopCondition(I2C1);  // STOP
	
	while(LL_I2C_IsActiveFlag_BUSY(I2C1));         /* Wait till line not busy */
	
	uint8_t received_data[2]; // to hold our received data
	I2C_ReadBuffer(LM75BD_ADDRESS, received_data, 2);
	
	// Combine the bytes into a temperature
	uint16_t temperature;
	temperature = received_data[0] << 8 | received_data[1];   // Combine the two bytes into one value
  temperature = temperature >> 5;                           // Shift the bits to the right to get the actual temperature value                                             
  
  payload[68] = (temperature >> 8) & 0xFF;  // Most significant byte
  payload[69] = temperature & 0xFF;         // Least significant byte
  
  return temperature;
}


//write to eeprom
void write_eeprom(uint8_t lowbyte, uint8_t highbyte){
  //first page
  uint8_t packet[34];
	packet[0] = highbyte;
	packet[1] = lowbyte;
	
  //set up dest
  packet[2] = (dest >> 24) & 0xFF;  // Extract the most significant byte
  packet[3] = (dest >> 16) & 0xFF;  // Extract the second byte
  packet[4] = (dest >> 8) & 0xFF;   // Extract the third byte
  packet[5] = dest & 0xFF;          // Extract the least significant byte

  //set up src
  packet[6] = (src >> 24) & 0xFF;  // Extract the most significant byte
  packet[7] = (src >> 16) & 0xFF;  // Extract the second byte
  packet[8] = (src >> 8) & 0xFF;   // Extract the third byte
  packet[9] = src & 0xFF;          // Extract the least significant byte

  //set up length
  packet[10] = (length >> 8) & 0xFF;  // Extract the most significant byte
  packet[11] = length & 0xFF;         // Extract the least significant byte

  //set up payload in the first
  for (int i = 0; i < 22; i++) {
    packet[12 + i] = payload[i]; // Copy part of the payload, src, dest, length into the first page
  }

  I2C_WriteBuffer(EEPROM_ADDRESS, packet, 34);

  LL_mDelay(5); // Small delay to allow EEPROM to write

  //second page
  highbyte = 0;
  lowbyte += 32;
  packet[0] = highbyte;
  packet[1] = lowbyte;

  for (int i = 0; i < 32; i++) {  
    packet[2 + i] = payload[22 + i]; // Copy the next 32 bytes of the payload into the second page
  }

  I2C_WriteBuffer(EEPROM_ADDRESS, packet, 34);

  LL_mDelay(5); // Small delay to allow EEPROM to write

  //third page
  highbyte = 0;
  lowbyte += 32;
  packet[0] = highbyte;
  packet[1] = lowbyte;

  for (int i = 0; i < 16; i++) {  
    packet[2 + i] = payload[54 + i]; // Copy the last 16 bytes of the payload into the third page
  }

  packet[18] = (checksum >> 8) & 0xFF;  // Extract the most significant byte
  packet[19] = checksum & 0xFF;         // Extract the least significant byte

  I2C_WriteBuffer(EEPROM_ADDRESS, packet, 20); // Write the last 18 bytes of the payload and the checksum

  LL_mDelay(5); // Small delay to allow EEPROM to write

}

//write to backup eeprom
void write_backup_eeprom(uint8_t lowbyte, uint8_t highbyte){
  //first page
  uint8_t packet[34];
	packet[0] = highbyte;
	packet[1] = lowbyte;
	
  //set up dest
  packet[2] = (dest_from_eeprom >> 24) & 0xFF;  // Extract the most significant byte
  packet[3] = (dest_from_eeprom >> 16) & 0xFF;  // Extract the second byte
  packet[4] = (dest_from_eeprom >> 8) & 0xFF;   // Extract the third byte
  packet[5] = dest_from_eeprom & 0xFF;          // Extract the least significant byte

  //set up src
  packet[6] = (src_from_eeprom >> 24) & 0xFF;  // Extract the most significant byte
  packet[7] = (src_from_eeprom >> 16) & 0xFF;  // Extract the second byte
  packet[8] = (src_from_eeprom >> 8) & 0xFF;   // Extract the third byte
  packet[9] = src_from_eeprom & 0xFF;          // Extract the least significant byte

  //set up length
  packet[10] = (length_from_eeprom >> 8) & 0xFF;  // Extract the most significant byte
  packet[11] = length_from_eeprom & 0xFF;         // Extract the least significant byte

  //set up payload in the first
  for (int i = 0; i < 22; i++) {
    packet[12 + i] = payload_from_eeprom[i]; // Copy part of the payload, src, dest, length into the first page
  }

  I2C_WriteBuffer(BACKUP_EEPROM_ADDRESS, packet, 34);

  LL_mDelay(5); // Small delay to allow EEPROM to write

  //second page
  highbyte = 0;
  lowbyte += 32;
  packet[0] = highbyte;
  packet[1] = lowbyte;

  for (int i = 0; i < 32; i++) {  
    packet[2 + i] = payload_from_eeprom[22 + i]; // Copy the next 32 bytes of the payload into the second page
  }

  I2C_WriteBuffer(BACKUP_EEPROM_ADDRESS, packet, 34);

  LL_mDelay(5); // Small delay to allow EEPROM to write

  //third page
  highbyte = 0;
  lowbyte += 32;
  packet[0] = highbyte;
  packet[1] = lowbyte;

  for (int i = 0; i < 16; i++) {  
    packet[2 + i] = payload_from_eeprom[54 + i]; // Copy the last 16 bytes of the payload into the third page
  }

  packet[18] = (checksum_from_eeprom >> 8) & 0xFF;  // Extract the most significant byte
  packet[19] = checksum_from_eeprom & 0xFF;         // Extract the least significant byte

  I2C_WriteBuffer(BACKUP_EEPROM_ADDRESS, packet, 20); // Write the last 18 bytes of the payload and the checksum

  LL_mDelay(5); // Small delay to allow EEPROM to write

}

//read from eeprom
void read_eeprom(uint8_t lowbyte, uint8_t highbyte) {
    uint8_t address_data[2];
    address_data[0] = highbyte; // High byte
    address_data[1] = lowbyte;  // Low byte

    // Set the EEPROM's address for reading
    I2C_WriteBuffer(EEPROM_ADDRESS, address_data, 2); // Write address to EEPROM

    LL_mDelay(5);// Small delay to allow EEPROM to write

    uint8_t data[10]; // Buffer to store read data
    I2C_ReadBuffer(EEPROM_ADDRESS, data, 10); // Read 10 bytes into `data`
	
		lowbyte += 64;
		address_data[0] = highbyte; // High byte
    address_data[1] = lowbyte;  // Low byte

		I2C_WriteBuffer(EEPROM_ADDRESS, address_data, 2); // Write address to EEPROM
    
    LL_mDelay(5); // Small delay to write in the address

    uint8_t value[18]; // Buffer to store read data
    I2C_ReadBuffer(EEPROM_ADDRESS, value, 18); // Read 30 bytes into `data`
    
    uint8_t dest[4];
    uint8_t src[4];
    uint8_t length[2];

    for (int i = 0; i < 4; i++) {
      dest[i] = data[i];       // First 4 bytes go to 'dest'
      src[i] = data[i + 4];    // Next 4 bytes go to 'src'
    }

    dest_from_eeprom = dest[0] << 24 | dest[1] << 16 | dest[2] << 8 | dest[3]; // Combine the four bytes into one value
    src_from_eeprom = src[0] << 24 | src[1] << 16 | src[2] << 8 | src[3]; // Combine the four bytes into one value
    length_from_eeprom = data[8] << 8 | data[9]; // Combine the two bytes into one value

    payload_from_eeprom[68] = value[14];
    payload_from_eeprom[69] = value[15]; // Copy the last 2 bytes of the payload into the last 2 bytes of the payload_from_eeprom

    uint8_t checksum_data[2];
    checksum_data[0] = value[16];
    checksum_data[1] = value[17]; // Copy the checksum into the checksum_data array

    checksum_from_eeprom = checksum_data[0] << 8 | checksum_data[1]; // Combine the two bytes into one value


}

//read from backup eeprom
void read_backup_eeprom(uint8_t lowbyte, uint8_t highbyte) {
    uint8_t address_data[2];
    address_data[0] = highbyte; // High byte
    address_data[1] = lowbyte;  // Low byte

    // Set the EEPROM's address for reading
    I2C_WriteBuffer(BACKUP_EEPROM_ADDRESS, address_data, 2); // Write address to EEPROM

    LL_mDelay(5);// Small delay to allow EEPROM to write

    uint8_t data[10]; // Buffer to store read data
    I2C_ReadBuffer(BACKUP_EEPROM_ADDRESS, data, 10); // Read 10 bytes into `data`
	
		lowbyte += 64;
		address_data[0] = highbyte; // High byte
    address_data[1] = lowbyte;  // Low byte

		I2C_WriteBuffer(BACKUP_EEPROM_ADDRESS, address_data, 2); // Write address to EEPROM
    
    LL_mDelay(5); // Small delay to write in the address

    uint8_t value[18]; // Buffer to store read data
    I2C_ReadBuffer(BACKUP_EEPROM_ADDRESS, value, 18); // Read 30 bytes into `data`
    
    uint8_t dest[4];
    uint8_t src[4];
    uint8_t length[2];

    for (int i = 0; i < 4; i++) {
      dest[i] = data[i];       // First 4 bytes go to 'dest'
      src[i] = data[i + 4];    // Next 4 bytes go to 'src'
    }

    dest_from_backup_eeprom = dest[0] << 24 | dest[1] << 16 | dest[2] << 8 | dest[3]; // Combine the four bytes into one value
    src_from_backup_eeprom = src[0] << 24 | src[1] << 16 | src[2] << 8 | src[3]; // Combine the four bytes into one value
    length_from_backup_eeprom = data[8] << 8 | data[9]; // Combine the two bytes into one value

    payload_from_backup_eeprom[68] = value[14];
    payload_from_backup_eeprom[69] = value[15]; // Copy the last 2 bytes of the payload into the last 2 bytes of the payload_from_eeprom

    uint8_t checksum_data[2];
    checksum_data[0] = value[16];
    checksum_data[1] = value[17]; // Copy the checksum into the checksum_data array

    checksum_from_backup_eeprom = checksum_data[0] << 8 | checksum_data[1]; // Combine the two bytes into one value


}

//joystick functions
uint32_t joystick_centre(void) {
	//Returns 1 if the joystick is pressed in the centre, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_5));
}

uint32_t joystick_left(void) {
	//Returns 1 if the joystick is pressed left, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_1));
}

uint32_t joystick_right(void) {
	//Returns 1 if the joystick is pressed right, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_0));
}

uint32_t joystick_up(void) {
	//Returns 1 if the joystick is pressed right, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_4));
}

uint32_t joystick_down(void) {
	//Returns 1 if the joystick is pressed right, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_0));
}


/*----------------------------------------------------------------------------
 * rgb_led_set: Sets the RGB LED colour on the MBED shield
	 (rgb_led_t is defined in main.h)
 ----------------------------------------------------------------------------*/
void rgb_led_set(rgb_led_t colour){
    /*  The RGB LED on the MBED shield is connected to the following pins:
        Red   - PB_4
        Green - PB_5
        Blue  - PB_10
    */
    if(colour == RGB_LED_RED){
      LL_GPIO_ResetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
    else if(colour == RGB_LED_GREEN){
      LL_GPIO_ResetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
    else if(colour == RGB_LED_BLUE){
      LL_GPIO_ResetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
    }
    else if(colour == RGB_LED_WHITE){
      LL_GPIO_ResetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_ResetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_ResetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
    else if(colour == RGB_LED_YELLOW){
      LL_GPIO_ResetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_ResetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
    else if(colour == RGB_LED_CYAN){
      LL_GPIO_SetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_ResetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_ResetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
    else if(colour == RGB_LED_PURPLE){
      LL_GPIO_ResetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_ResetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
    else if(colour == RGB_LED_OFF){
      LL_GPIO_SetOutputPin(RGB_LED_RED_N_GPIO_Port, RGB_LED_RED_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_GREEN_N_GPIO_Port, RGB_LED_GREEN_N_Pin);
      LL_GPIO_SetOutputPin(RGB_LED_BLUE_N_GPIO_Port, RGB_LED_BLUE_N_Pin);
    }
}



/*----------------------------------------------------------------------------
  * System Clock Configuration 
	* Sets up Nucleo board for 84 MHz from 8 MHz crystal)
 ----------------------------------------------------------------------------*/
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2);
	
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_HSE_EnableBypass();
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1);
	
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_4, 84, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1);
	
  while (LL_PWR_IsActiveFlag_VOS() == 0);
	
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);
  LL_Init1msTick(84000000);
  LL_SetSystemCoreClock(84000000);
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}