# Lab 4 Pong

**GitHub Repository URL: https://github.com/nsheth19/ese5190f26-lab4-sheth.git**

| Team Member Name | Email Address       | GitHub Handle       |
|------------------|---------------------|---------------------|
| Nikhil Sheth         | nsheth1@engineering.upenn.edu          | nsheth19          |


I1

<img width="760" height="545" alt="Screenshot 2026-11-06 at 23 19 41" src="https://github.com/user-attachments/assets/e3368498-d252-4a95-810f-6356fd653701" />

The Saleae Logic Analyzer capture shows the SPI communication between the ATmega and the LCD display. D0 is the SPI clock (SCK), and each group of 8 clock pulses represents one byte being transmitted. D1 is MOSI, which carries the actual command and data bytes sent to the display. The first command shown is 0x2A, which is the Column Address Set command. The following bytes 0x00, 0x0A, 0x00, 0x1D set the column range from 10 to 29. The next command is 0x2B, the Row Address Set command, which begins setting the row range. D2 is the D/C line, which tells the display whether the byte on MOSI should be treated as a command or as data. When D/C = 0, the byte is a command such as 0x2A or 0x2B. When D/C = 1, the following bytes are data values such as the row or column addresses. D3 is the chip select signal and stays high in this captured section. Overall, the packet shows the MCU sending display commands followed by the address data needed to define the region of the screen that will be written to.
