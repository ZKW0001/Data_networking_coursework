# Data_networking_project

## Project overview
This project focuses on developing an embedded system application on an STM32 microcontroller integrated with an EEPROM breakout board featuring two EEPROMs, utilizing low-level programming. The system reads temperature data from a sensor on the microcontroller, stores it in the payload field of a custom packet structure, and writes the complete packet to the primary EEPROM. The packet follows an experimental protocol, comprising fields for destination, source, length, payload, and a checksum. A 1’s complement checksum ensures signal integrity. The system also supports reading data from the EEPROM and includes a backup mechanism to write the packet to a secondary EEPROM, improving data reliability.  

The structure of the packet to be stored in the EEPROM is given as below:  
| **Field**      | **Size (bytes)** | **Description**                                                                       |  
|----------------|------------------|---------------------------------------------------------------------------------------|  
| **dest**       | 4                | Destination address of the packet, stored in hexadecimal. This is set to 0xcafebabe.  |  
| **src**        | 4                | Source address of the packet, stored in hexadecimal. This is set to 0xc0ffeec0.       |  
| **length**     | 2                | Length of the payload, fixed at 70 bytes.                                             |  
| **payload**    | 70               | Contains data, including the right-aligned 11-bit temperature value. Remaining bytes are zero-padded. |  
| **checksum**   | 2                | 1's complement checksum (IPv4 style) calculated over the `dest`, `src`, `length`, and `payload` fields. |  
<br/>

## Microcontroller overview

https://github.com/user-attachments/assets/20254749-130f-4a1a-a7d0-a68d8d5f2f42

The STM32 microcontroller (UP) is integrated with an EEPROM breakout board featuring two EEPROMs (BOTTOM). Two protocols manage the read and write processes: the I2C protocol, used for communication with the LM75BD temperature sensor and the 24LC32AF EEPROM, and the SPI protocol, used to interface with the C12832 LCD screen on the microcontroller for displaying results.

## Example output

- **Read and write to the EEPROM**

https://github.com/user-attachments/assets/a4a187ec-c064-4522-b218-08a3be2f1886

When press the "center" button of the joystick, read the temperature register value from the sensor over I2C and display it together with its hexadecimal representation on the LCD screen (in degree Celsius), known as the "Sampled data". Further ‘centre’ button presses result in the temperature register data being read again and stored in the packet, meaning that the temperature will not update in real time.
<br/>
When press the "left" button of the joystick, the LCD screen will display the temperature value read from the EEPROM. At first, it should be 0 as nothing is being stored in the EEPROM. It is known as the "Memory data". It is worthnotcing that both reading and writing of the EEPROM uses EEPROM's page mode due to limited amount of data are designed to be accessed and modified at in a single transaction.
<br/>
When press the "right" button on the joystickm, the entire 82-byte packet will be saved to the EEPROM. This includes the destination (4 bytes), source (4 bytes), length (2 bytes), payload (70 bytes with temperature data in the last 2 bytes and zero-padding), and a 2-byte 1’s complement checksum for integrity. The packet is stored sequentially at predefined EEPROM addresses. After the data is stored in the EEPROM, a further "left" press of the joystick will result in the stored termperature value being displayed as "Memory data".
<br/>
<br/>  
- **Packet dispaly, checksum and backup storage**
  
https://github.com/user-attachments/assets/21e8e4ec-aa25-4d59-a627-2ac478a45fee

When press the "up" button at any time, the LCD screen will display the packet data respectively, from desitination to checksum. Once the checksum field is displayed, pressing "up" again triggers checksum recalculation. If the recalculated checksum matches the stored checksum from the EEPROM, "Checksum OK" with the calculated value is shown; otherwise, "Checksum ERROR" is displayed, including the stored checksum value. Therefore, if the sampled data hasn't been saved to the EEPROM, "Checksum ERROR" is displayed, as demonstrated in the video. 

A further "up" press after the checksum validation reads the entire packet from the primary EEPROM and writes it to the secondary EEPROM as a backup. Upon successful completion, an indication such as an LED flash or a message on the LCD confirms the backup operation.
