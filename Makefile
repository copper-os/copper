COPPER_IMAGE := copper.img


all: image

image: bootloader
	dd if=/dev/zero of=$(COPPER_IMAGE) bs=512 count=93750
	mformat -i $(COPPER_IMAGE) -f 1440 ::
	mmd -i $(COPPER_IMAGE) ::/EFI
	mmd -i $(COPPER_IMAGE) ::/EFI/BOOT
	mcopy -i $(COPPER_IMAGE) bootloader/bootloader.efi ::/EFI/BOOT
	mcopy -i $(COPPER_IMAGE) startup.nsh ::

.PHONY: bootloader
bootloader:
	$(MAKE) -C bootloader

.PHONY: run
run: image
	@qemu-system-x86_64 \
		-m 4G \
		-cpu qemu64 \
		-net none \
		-drive if=pflash,format=raw,unit=0,file=/usr/share/qemu/ovmf/OVMF.fd,readonly=on \
		-drive format=raw,file=$(COPPER_IMAGE)

PHONY += format
format:
	@find . -iname '*.[ch]' | xargs clang-format -i -style=file

PHONY += clean
clean:
	@$(MAKE) -C bootloader clean
	@$(MAKE) -C kernel clean
	@rm -f $(COPPER_IMAGE)
