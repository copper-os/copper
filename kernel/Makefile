export ARCH := x86_64
export AR   := ar
export AS   := nasm
export CC   := clang

SOURCE_DIR  := $(CURDIR)/src
HEADER_DIR  := $(CURDIR)/include
LIBRARY_DIR := $(CURDIR)/lib

export HEADER_DIR

LIBRARIES = console

export CFLAGS = -I$(HEADER_DIR) \
         -Wall \
         -Wextra \
         -fpie \
         -ffreestanding \
         -fshort-wchar

LDFLAGS = $(addprefix -L$(LIBRARY_DIR)/,$(LIBRARIES)) \
          -Bstatic \
          -Bsymbolic \
          -pie \
          -nostdlib \
          --gc-sections \
          --no-undefined-version

SRCS = $(wildcard $(SOURCE_DIR)/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

ASM_SRCS = $(wildcard $(SOURCE_DIR)/*.S)
ASM_OBJS = $(ASM_SRCS:.S=.o)

LINKER_SCRIPT = kernel.lds


all: kernel.elf

kernel.elf: $(LINKER_SCRIPT) $(OBJS) $(ASM_OBJS) $(LIBRARIES)
	$(LD) -T$(LINKER_SCRIPT) $(LDFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(addprefix -l,$(LIBRARIES))

$(ASM_OBJS): $(ASM_SRCS)
	$(AS) $(ASMFLAGS) -o $@ -f elf64 $^

.PHONY: $(LIBRARIES)
$(LIBRARIES):
	$(MAKE) -C $(addprefix $(LIBRARY_DIR)/,$@)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean
clean:
	@rm -f kernel.elf $(OBJS) $(ASM_OBJS) $(DEPS)
	@for libdir in $(addprefix $(LIBRARY_DIR)/,$(LIBRARIES)); do \
		$(MAKE) -C $$libdir clean; \
	done

include $(DEPS)
