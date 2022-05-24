export ARCH := x86_64
export AR   := ar
export AS   := nasm
export CC   := clang

SOURCE_DIR        := $(CURDIR)/src
HEADER_DIR        := $(CURDIR)/include
LIBRARY_BASE_DIR  := $(CURDIR)/lib

export HEADER_DIR

LIBRARIES = console
LIBRARY_DIRS = $(foreach lib,$(LIBRARIES),$(LIBRARY_BASE_DIR)/$(lib))

LIBRARY_LINK_DIRS    = $(foreach libdir,$(LIBRARY_DIRS),-L$(libdir))
LIBRARY_LINK_TARGETS = $(foreach lib,$(LIBRARIES),-l$(lib))

CFLAGS = -I$(HEADER_DIR) \
         -Wall \
         -Wextra \
         -fpie \
         -ffreestanding \
         -fshort-wchar

LDFLAGS = $(LIBRARY_LINK_DIRS) \
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

kernel.elf: $(LINKER_SCRIPT) $(OBJS) $(ASM_OBJS) $(LIBRARY_DIRS)
	$(LD) -T$(LINKER_SCRIPT) $(LDFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(LIBRARY_LINK_TARGETS)

$(ASM_OBJS): $(ASM_SRCS)
	$(AS) $(ASMFLAGS) -o $@ -f elf64 $^

.PHONY: $(LIBRARY_DIRS)
$(LIBRARY_DIRS):
	$(MAKE) -C $@

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean
clean:
	@rm -f kernel.elf $(OBJS) $(ASM_OBJS) $(DEPS)
	@for libdir in $(LIBRARY_DIRS); do \
		$(MAKE) -C $$libdir clean; \
	done

include $(DEPS)
