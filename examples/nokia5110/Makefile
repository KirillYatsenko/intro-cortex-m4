PART    ?= TM4C123GH6PM
TARGET  ?= TM4C123_RB1
CFLAGS  ?= -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp \
	   -ffunction-sections -fdata-sections -std=c99 -Os -Wall \
	   -I../../inc -I../../drivers -DPRINTF_INCLUDE_CONFIG_H

LDFLAGS ?= --gc-sections

SIZE := arm-none-eabi-size

LIBGCC  := ${shell arm-none-eabi-gcc ${CFLAGS} -print-libgcc-file-name}
LIBC    := ${shell arm-none-eabi-gcc ${CFLAGS} -print-file-name=libc.a}
LIBM    := ${shell arm-none-eabi-gcc ${CFLAGS} -print-file-name=libm.a}

.PHONY: all clean install

DRIVERS_DIR := ../../drivers
SRC_DIRS := . $(DRIVERS_DIR)
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(patsubst %.c,%.o,$(SRCS))

all: output.bin output.lst

install: output.bin
	lm4flash $<

size: output.axf
	@$(SIZE) $^

clean:
	rm -f *.d *.o $(DRIVERS_DIR)/*.o $(DRIVERS_DIR)/*.d output.* \
		compile_commands.json

output.axf: $(OBJS)
	arm-none-eabi-ld ${LDFLAGS} -T scatter.ld --entry ResetISR \
		-Map output.map -o $@ $^ ${LIBGCC} ${LIBC} ${LIBM}

%.o: %.c
	arm-none-eabi-gcc ${CFLAGS} -MD -c -o $@ $<

%.bin: %.axf
	arm-none-eabi-objcopy -O binary $< $@

%.lst: %.axf
	arm-none-eabi-objdump -h -S $< > $@
