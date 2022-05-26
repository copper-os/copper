export MAKE_DIR := $(CURDIR)/make

COPPER_IMAGE := copper.img


all: image

image: bootloader kernel
	./build-image.sh $(COPPER_IMAGE)

.PHONY: bootloader
bootloader:
	$(MAKE) -C bootloader

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

.PHONY: run
run: image
	@qemu-system-x86_64 \
		-m 4G \
		-cpu qemu64 \
		-net none \
		-drive if=pflash,format=raw,unit=0,file=/usr/share/qemu/OVMF.fd,readonly=on \
		-drive format=raw,file=$(COPPER_IMAGE)

.PHONY: format
format:
	@find . -iname '*.[ch]' -or -iname '*.cpp' -or -iname '*.hpp' | xargs clang-format -i -style=file

.PHONY: check
check:
	@find . -iname '*.[ch]' -or -iname '*.cpp' -or -iname '*.hpp' | xargs clang-tidy -checks=*,clang-analyzer-*,-modernize-use-trailing-return-type,-llvmlibc-implementation-in-namespace,-performance-no-int-to-ptr,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-llvmlibc-restrict-system-libc-headers,-altera-unroll-loops

.PHONY: header-guard
header-guard:
	@find . -iname '*.[ch]' -or -iname '*.cpp' -or -iname '*.hpp' | xargs clang-tidy -checks=-*,llvm-header-guard -fix-errors

.PHONY: namespace-comment
namespace-comment:
	@find . -iname '*.[ch]' -or -iname '*.cpp' -or -iname '*.hpp' | xargs clang-tidy -checks=-*,llvm-namespace-comment -fix-errors

.PHONY: clean
clean:
	@$(MAKE) -C bootloader clean
	@$(MAKE) -C kernel clean
	@rm -f $(COPPER_IMAGE)
