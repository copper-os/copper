#!/bin/bash
set -o errexit -o errtrace -o nounset -o pipefail

readonly IMAGE="${1}"

dd if=/dev/zero of="${IMAGE}" bs=256M count=1
mkfs -t fat "${IMAGE}"

tmpdir=$(mktemp -d)
sudo mount "${IMAGE}" "${tmpdir}"

sudo mkdir -p "${tmpdir}/EFI/BOOT"
sudo install -v bootloader/bootloader.efi "${tmpdir}/EFI/BOOT"
sudo install -v startup.nsh "${tmpdir}"
sudo install -v kernel/kernel.elf "${tmpdir}"

sudo sync

sudo umount "${IMAGE}"
