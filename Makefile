# =============================
#  Project paths
# =============================
SRC_DIR ?= ./
SOURCES := $(shell find $(SRC_DIR) -name '*.c' -or -name '*.S')
OBJECTS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
LINKER  := $(SRC_DIR)/dtekv-script.lds

# =============================
#  Toolchain
# =============================
TOOLCHAIN ?= riscv32-unknown-elf-
CC      := $(TOOLCHAIN)gcc
LD      := $(TOOLCHAIN)ld
OBJCOPY := $(TOOLCHAIN)objcopy
OBJDUMP := $(TOOLCHAIN)objdump

# =============================
#  Optimization selection
# =============================
ifeq ($(OPT),0)
  OPTFLAG = -O0
  TARGET  = main_O0
else
  OPTFLAG = -O3
  TARGET  = main_O3
endif

# CPU + common flags
COMMON_CFLAGS := -Wall -nostdlib -mabi=ilp32 -march=rv32imzicsr -fno-builtin
CFLAGS := $(COMMON_CFLAGS) $(OPTFLAG)

# =============================
#  Default build
# =============================
all: main_O0.bin main_O3.bin

# =============================
#  Compilation
# =============================
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	$(CC) -c $(CFLAGS) -o $@ $<

# =============================
#  Linking
# =============================
$(TARGET).elf: $(OBJECTS)
	$(LD) -o $@ -T $(LINKER) $(filter-out boot.o, $(OBJECTS)) softfloat.a

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) --output-target binary $< $@
	$(OBJDUMP) -D $< > $<.txt

# Explicit build targets
main_O0: OPT=0
main_O0: main_O0.bin

main_O3: OPT=3
main_O3: main_O3.bin

# =============================
#  Cleaning
# =============================
clean:
	rm -f *.o *.elf *.bin *.txt

# =============================
#  Running on DTEK-V tools
# =============================
TOOL_DIR ?= ./tools

run: $(TARGET).bin
	make -C $(TOOL_DIR) FILE_TO_RUN=$(CURDIR)/$<
