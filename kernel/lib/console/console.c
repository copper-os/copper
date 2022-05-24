#include <console/console.h>
#include <efi/efi.h>
#include <efi/efilib.h>

int console_init(void)
{
    EFI_STATUS status = 0;

    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics_output = NULL;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL,
                               (void**)&graphics_output);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate GraphicsOutputProtocol. %r\n", status);
        return 1;
    }

    return 0;
}
