# ToDo

- Change UART port in examples/uart. Test using ftdi cable - DONE
- Refine blink example using systick interrupt instead of polling - DONE
- Improve UART driver using interrupts instead of polling, add FIFO buffering - DONE
- Implement printf functionality for debugging purposes - DONE
- Matrix keyboard driver - DONE
- SPI nokia 5110 lcd driver
- Move startup.c and scatter.ld to root directory
- Move common driver code to common src folder
- Implement Calculator using Matrix keyboard and 5110 nokia display
- Implement button edge triggering interrupt service, debouncing. - DONE
- Create an alarm clock which will be powered from powerbank using nokia 5110
  and a few buttons
- Write I2C driver by using only documentation - DONE
- Interface temperature/humidity sensor with I2C - DONE
- Show aht20 measurments on the LCD
- Create stopwatch which has two buttons(stop and start). Should be using interrupts
  for both buttons and timer interrupt. The resolution should be in mili seconds - DONE
