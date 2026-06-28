# =============================================================================
# Makefile — Water Level System | ATmega328P | avr-gcc
# =============================================================================

MCU      = atmega328p
F_CPU    = 16000000UL
TARGET   = water_level

# ---- Programmer ----
PROGRAMMER   = arduino
PORT         = COM3         # <-- Schimba cu portul tau (Device Manager)
AVRDUDE_BAUD = 115200

# ---- Toolchain ----
CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size
AVRDUDE = avrdude

# ---- Directoare sursa ----
SRC_DIRS = src utils \
           drivers/adc \
           drivers/buzzer \
           drivers/eeprom \
           drivers/gpio \
           drivers/i2c \
           drivers/interrupt \
           drivers/ssd1306 \
           drivers/timer \
           drivers/uart

# ---- Surse si obiecte ----
SRC = $(foreach d, $(SRC_DIRS), $(wildcard $(d)/*.c))
OBJ = $(patsubst %.c, build/%.o, $(SRC))

# ---- Include paths ----
INC_DIRS = bsp src utils \
           drivers/adc \
           drivers/buzzer \
           drivers/eeprom \
           drivers/gpio \
           drivers/i2c \
           drivers/interrupt \
           drivers/ssd1306 \
           drivers/timer \
           drivers/uart

INCLUDES = $(addprefix -I, $(INC_DIRS))

# ---- Flags compilare ----
CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)
CFLAGS += -Os -Wall -Wextra -Wshadow -std=c99
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += $(INCLUDES)

LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections

# ---- Reguli ----
.PHONY: all flash clean size disasm dirs

all: dirs $(TARGET).hex size

dirs:
	@mkdir -p $(so