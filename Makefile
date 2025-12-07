# ------------------------
# Standalone Makefile for DTEK-V (no tools/ dependency)
# - Builds main_O0.{elf,bin} and main_O3.{elf,bin}
# - Use: make OPT=3 main_O3     or   make main_O3
# - Use: make OPT=0 main_O0     to build unoptimized
# - No automatic flashing; see run_hint
# ------------------------

# Sources and objects (current directory)
SRC_DIR ?= ./
SOURCES := $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c' -or -name '*.S')
OBJECTS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))

# Linker script and softfloat library (adjust if named differently)
LINKER  := $(SRC_DIR)/dtekv-script.lds
SOFTFLOAT_LIB := softfloat.a

# Toolchain
TOOLCHAIN ?= riscv32-unknown-elf-
CC      := $(TOOLCHAIN)gcc
LD      := $(TOOLCHAIN)ld
OBJCOPY := $(TOOLCHAIN)objcopy
OBJDUMP := $(TOOLCHAIN)objdump

# CPU + common flags
COMMON_CFLAGS := -Wall -nostdlib -mabi=ilp32 -march=rv32imzicsr -fno-builtin

# Select optimization with OPT variable (default O3)
ifeq ($(OPT),0)
  OPTFLAG = -O0
  TARGET  = main_O0
else
  OPTFLAG = -O3
  TARGET  = main_O3
endif

CFLAGS := $(COMMON_CFLAGS) $(OPTFLAG)

# Default target: build optimized (main_O3) if user runs plain `make`
.PHONY: default
default: OPT=3
default: main_O3

# Compile rules
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	$(CC) -c $(CFLAGS) -o $@ $<

# Link
$(TARGET).elf: $(OBJECTS)
	$(LD) -o $@ -T $(LINKER) $(filter-out boot.o, $(OBJECTS)) $(SOFTFLOAT_LIB)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) --output-target binary $< $@
	$(OBJDUMP) -D $< > $<.txt

# Convenience explicit targets
.PHONY: main_O0 main_O3 all
main_O0: OPT=0
main_O0: main_O0.bin

main_O3: OPT=3
main_O3: main_O3.bin

all: main_O0 main_O3

# Clean
.PHONY: clean
clean:
	rm -f *.o *.elf *.bin *.txt

# Run hint (no automatic flashing): tells user how to flash/copy
.PHONY: run_hint
run_hint:
	@echo "No automatic flashing configured in this Makefile."
	@echo "Manual flashing suggestions:"
	@echo "  - If your board boots from an SD card: copy the desired binary to the SD card"
	@echo "      cp $(CURDIR)/$(TARGET).bin /path/to/sdcard/monitor.bin"
	@echo "    then insert SD and power-cycle the board."
	@echo "  - If you use OpenOCD + JTAG, flash the ELF with OpenOCD (edit interface/target):"
	@echo "      openocd -f interface/<your>.cfg -f target/<your>.cfg -c \"program $(CURDIR)/$(TARGET).elf verify reset exit\""
	@echo "  - If you use vendor GUI (Quartus, etc): open the GUI and program $(CURDIR)/$(TARGET).bin or .elf"
	@echo ""
	@echo "To build optimized binary: make OPT=3 main_O3"
	@echo "To build unoptimized binary: make OPT=0 main_O0"

# Show variables (debug)
.PHONY: info
info:
	@echo "SOURCES = $(SOURCES)"
	@echo "OBJECTS = $(OBJECTS)"
	@echo "LINKER  = $(LINKER)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "TARGET  = $(TARGET)"
