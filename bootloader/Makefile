ARCH := x86_64
CC   := clang

GNU_EFI_LDS = /usr/lib/elf_$(ARCH)_efi.lds
GNU_EFI_CRT = /usr/lib/crt0-efi-$(ARCH).o

LIBRARIES = efi gnuefi

CFLAGS = -fpie \
         -ffreestanding \
         -fno-stack-protector \
         -fno-stack-check \
         -fshort-wchar \
         -mno-red-zone \
         -Wall \
         -Wextra \
         -DDEBUG_KERNEL_LOAD

LDFLAGS = -T$(GNU_EFI_LDS) \
          -L/usr/lib \
          -nostdlib \
          -znocombreloc \
          -shared \
          -Bsymbolic

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)


all: bootloader.efi

bootloader.efi: bootloader.so
	objcopy \
		-j .text \
		-j .sdata \
		-j .data \
		-j .dynamic \
		-j .dynsym \
		-j .rel \
		-j .rela \
		-j .rel.* \
		-j .rela.* \
		-j .reloc \
		--target efi-app-$(ARCH) \
		--subsystem=10 \
		$< $@

bootloader.so: $(GNU_EFI_CRT) $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(addprefix -l,$(LIBRARIES))

.PHONY: clean
clean: clean-deps-graph
	@rm -f bootloader.efi bootloader.so $(OBJS)

include $(MAKE_DIR)/deps-graph.mk
