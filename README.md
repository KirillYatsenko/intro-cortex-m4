# intro-cortex-m4

Implementing examples from Jonathan Valvano's books, without a dependency on Keil Uvision IDE which is not supported on Linux

## Supported Hardware
Only tm4c123gxl evaluation board is supported

## Implemented baremetal drivers and examples
- [x] PLL
- [x] Systick
- [x] UART
- [x] I2C
- [x] SPI
- [x] Matrix keypad
- [x] LED blinking
- [x] AHT20
- [x] Stopwatch
- [x] Button interrupt handling
- [x] Nokia5110
- [x] ADC

## Dependencies
Install flasher: ``sudo apt install lm4flash``

## Usage
1. Connect the tm4c123gxl evaluation board via usb cable
2. Navigate to the example you are interested in: `cd examples/blink/`
3. Build and flash the board: `make install`

## Credits
Template taken and stipped from: https://github.com/martinjaros/tm4c-gcc
