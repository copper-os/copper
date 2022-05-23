ARCH := x86_64
AS   := nasm
CC   := clang

CFLAGS = -Wall \
         -Wextra \
         -fpie \
         -ffreestanding \
         -fshort-wchar

LDFLAGS = -Bstatic \
          -Bsymbolic \
          -pie \
          -nostdlib \
          --gc-sections \
          --no-undefined-version


SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

ASMSRCS = $(shell find $(LIB_DIR) -name '*.S')
ASMOBJS = $(ASMSRCS:.S=.o)

KERNEL_LDS = kernel.lds

all: kernel.elf

kernel.elf: $(KERNEL_LDS) $(OBJS) $(ASMOBJS)
	$(LD) -T$(KERNEL_LDS) $(LDFLAGS) -o $@ $(OBJS) $(ASMOBJS)

$(ASMOBJS): $(ASMSRCS)
	$(AS) $(ASMFLAGS) -o $@ -f elf64 $(@:.o=.S)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean
clean:
	@rm -f kernel.elf $(OBJS) $(ASMOBJS) $(DEPS)

include $(DEPS)
