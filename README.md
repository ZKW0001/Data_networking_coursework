# Data_networking_project

## Project overview
This project aims on developing an embedded system application on an STM32 microcontroller integrated with an EEPROM breakout board containing two EEPROMs. The system reads temperature data from a sensor on the microcontroller, stores it in the payload field of a custom packet structure, and writes the packet to the primary EEPROM. The packet adheres to an experimental protocol and includes fields for destination, source, length, payload, and checksum. A 1’s complement checksum is implemented to ensure signal integrity. Additionally, the system supports reading back data from EEPROM and features a mechanism to back up the packet to a secondary EEPROM, enhancing data reliability.

The structure of the pocket to be stored in the EEPROM is given as below:
| **Field**      | **Size (bytes)** | **Description**                                                                       |  
|----------------|------------------|---------------------------------------------------------------------------------------|  
| **dest**       | 4                | Destination address of the packet, stored in hexadecimal. This is set to 0xcafebabe.  |  
| **src**        | 4                | Source address of the packet, stored in hexadecimal. This is set to 0xc0ffeec0.       |  
| **length**     | 2                | Length of the payload, fixed at 70 bytes.                                             |  
| **payload**    | 70               | Contains data, including the right-aligned 11-bit temperature value. Remaining bytes are zero-padded. |  
| **checksum**   | 2                | 1's complement checksum (IPv4 style) calculated over the `dest`, `src`, `length`, and `payload` fields. |  

## Example output
