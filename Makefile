# =============================
#  Project paths
# =============================
SRC_DIR ?= ./
OBJ_DIR ?= ./
SOURCES := $(shell find $(SRC_DIR) -name '*.c' -or -name '*.S')
OBJECTS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
LINKER  := $(SRC_DIR)/dtekv-script.lds

# =============================
#  Toolchain
# =============================
TOOLCHAIN ?= riscv32-unknown-elf-
CC  := $(TOOLCHAIN)gcc
LD  := $(TOOLCHAIN)ld
OBJCOPY := $(TOOLCHAIN)objcopy
OBJDUMP := $(TOOLCHAIN)objdump

# =============================
#  Optimization selection
#  make OPT=0   (no optim)
#  make OPT=3   (aggressive)
#  default = O3
# =============================
ifeq ($(OPT),0)
  OPTFLAG = -O0
  TARGET = main_O0
else
  OPTFLAG = -O3
  TARGET = main_O3
endif

# Your CPU features:
COMMON_CFLAGS := -Wall -nostdlib -mabi=ilp32 -march=rv32imzicsr -fno-builtin

CFLAGS := $(COMMON_CFLAGS) $(OPTFLAG)

# =============================
#  Build Rules
# =============================
all: main_O0.bin main_O3.bin

$(TARGET).elf: $(SOURCES)
	$(CC) -c $(CFLAGS) $(SOURCES)
	$(LD) -o $@ -T $(LINKER) $(filter-out boot.o, $(OBJECTS)) softfloat.a

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) --output-target binary $< $@
	$(OBJDUMP) -D $< > $<.txt

# Explicit targets for convenience:
main_O0.elf:
	make OPT=0 $(MAKECMDGOALS)

main_O0.bin: main_O0.elf

main_O3.elf:
	make OPT=3 $(MAKECMDGOALS)

main_O3.bin: main_O3.elf

# =============================
#  Cleaning
# =============================
clean:
	rm -f *.o *.elf *.bin *.txt

# =============================
#  Run on DTEK-V tools
# =============================
TOOL_DIR ?= ./tools

run: $(TARGET).bin
	make -C $(TOOL_DIR) "FILE_TO_RUN=$(CURDIR)/$<"
