# scooter-repair
Collected information on electric scooter repair.

## Project structure
+ logs: Captured niu scooter RS485 bus logs
+ logs_processed: Processed logs by utils/preprocess.py
+ niu-ecu: CH32V003J4M6 risc-v MCU project source code. Acts like a simplified niu ECU
+ pcb: PCB pdf and LCEDA/EasyEDA project file
+ utils: Scripts analyzing data
+ serial_logger.py: Log all RS485 bus data using pyserial
+ utils/niu_u_dash.py: Use python to control niu dash board
+ utils/serial_niu.py: Interactively send common commands to niu scooter

## Important bits
RS485 bus UART configuration is: 9600 8E1. Or baud rate 9600, 8 bit data, even parity, 1 stop bit. The "even parity" is the most important. In MCU you may need to configure as "9 bit mode" to include the parity bit.

ECU sends FOC query packet, FOC will respond with serial number. This serial number is crucial for unlocking the FOC. If not, the FOC will be stuck at gear 1, very slow, render the scooter useless.
```
60.46832990646362   : ->FOC     : 20 01 01 | 03
60.49982523918152   : <-FOC     : 20 81 10 | 4d 55 31 33 33 37 33 42 33 33 32 30 30 32 31 36

>>> bytes.fromhex('4d 55 31 33 33 37 33 42 33 33 32 30 30 32 31 36')
b'MU13373B33200216'
```

FOC unlock packets. After receiving serial number, ECU must calculates a CRC16 like modbus, but using polynomial 0x7085 (appearing in code as bit wise flipped 0xA10E), not the common modbus 0x8005 (flipped as 0xA001). See function `calc_serial_number_crc16` in file `niu_controller.c`:
```C
uint16_t calc_serial_number_crc16(uint8_t *data, uint8_t length) {
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < length; ++i) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                // 0x7085 bit wise flip is 0xA10E: reflect_bits_16(0x7085) == 0xA10E
                crc = (crc >> 1) ^ 0xA10E;
            }
            else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
```

The complete FOC unlocking sequence:
```
60.46832990646362   : ->FOC     : 20 01 01 | 03
60.49982523918152   : <-FOC     : 20 81 10 | 4d 55 31 33 33 37 33 42 33 33 32 30 30 32 31 36
60.67788124084473   : ->FOC     : 20 05 02 | 73 80
60.688647747039795  : <-FOC     : 20 85 01 | ce

73 80 = calc_serial_number_crc16(4d 55 31 33 33 37 33 42 33 33 32 30 30 32 31 36)
```

How to reach the 0xA10E number. Here are some of my niu scooters serial number and crc16:
```
(MU13373B33200216)
4d 55 31 33 33 37 33 42 33 33 32 30 30 32 31 36
73 80

(MUM3F83B43400949)
4d 55 4d 33 46 38 33 42 34 33 34 30 30 39 34 39
60 9a

(MM13593B22102462)
4d 4d 31 33 35 39 33 42 32 32 31 30 32 34 36 32
10 7b

(MU13482B42510971)
4d 55 31 33 34 38 32 42 34 32 35 31 30 39 37 31
1c 86
```

Feed the data pairs to some powerful AI, let it write python code to find the relationships. Although I have another idea if you only got one scooter: Remove the ECU from the scooter and connect it to a computer or MCU through RS485. After receive the query serial number packet from ECU, throw some made up serial number. The ECU will respond with the CRC16. Then you can get infinite query-response data pairs, should be much easier to find some patterns.

The second FOC unlock packet sent to FOC must be after delay 100ms (not exact) after the serial number packet received from FOC, or the FOC wound not respond (likely not fast enough to be ready to receive after sending).

CH32V003J4M6 is a SOP8 RISC-V MCU from WCH (WinChipHead 沁恒). It is one of the popular CH32V003 family. To save the scarce pins, the project used an unconventional 2-wire RS485 connection. I think it is rarely seen: The UART is configured as half-duplex mode, TX configured as Open Drain mode and has a pull up resistor, DI and RO are shorted together and connected to TX (via a 470 Ohm resistor in case programming error occurs). TX and RX are internally connected in half-duplex mode. It works because when DE is enabled, RO pin is high-z, so there is no shorting between DI and RO.
```
TX--R------------RO
      |  DIR-----RE#
      |      |
      |      ----DE
      -----------DI
```

MCU Pin usage. The project uses PD6 as TX/RX pin. Uses PA2 as direction control pin. Uses PC4 to measure battery voltage, and currently directly writes this realtime voltage to Dash as battery percent number. This leaves us with PC1 PC2 and the PD1 if SWDIO is not needed after flashing. PC1 and PC2 can be used as I2C SCL and SDA, and it is very useful. Like connect a RTC chip to get time, connect an OLED LCD to display things, connect an IO expander to control LED and receive buttons.

The I2C port can have IO expander and thus buttons. But there is a simpler way. The voltage sense pin can connect a button to ground. When button pushed the measured voltage goes to zero so we know there is a button push. The SWDIO can also be configured as GPIO. And there are other ways to connect multiple buttons and resistors to one ADC pin.


## niu
[niu U civic](docs/niu-u-civic.md)
