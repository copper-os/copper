#include <efi/efi.h>
#include <efi/efilib.h>
#include <elf.h>

#define UEFI_MEMORY_DESCRIPTOR_BUFFER_SIZE (512)
#define MAX_PROGRAM_HEADER_TABLE_SIZE (512)
#define GRAPHIC_MODE_NUMBER (10)

static EFI_MEMORY_DESCRIPTOR descriptor_buffer[UEFI_MEMORY_DESCRIPTOR_BUFFER_SIZE];
static Elf64_Phdr program_header_table[MAX_PROGRAM_HEADER_TABLE_SIZE];

static int strcmp(const char* const a, const char* const b, uint64_t size)
{
    for (uint64_t i = 0; i < size; ++i) {
        if (a[i] != b[i]) {
            return 1;
        }
    }

    return 0;
}

#ifdef DEBUG_BOOT_MEMORY
static void print_memory_map(const EFI_MEMORY_DESCRIPTOR* const descriptor_buffer,
                             const UINTN descriptor_buffer_size, const UINTN descriptor_size)
{
    static char* memory_types[] = {"EfiReservedMemoryType",
                                   "EfiLoaderCode",
                                   "EfiLoaderData",
                                   "EfiBootServicesCode",
                                   "EfiBootServicesData",
                                   "EfiRuntimeServicesCode",
                                   "EfiRuntimeServicesData",
                                   "EfiConventionalMemory",
                                   "EfiUnusableMemory",
                                   "EfiACPIReclaimMemory",
                                   "EfiACPIMemoryNVS",
                                   "EfiMemoryMappedIO",
                                   "EfiMemoryMappedIOPortSpace",
                                   "EfiPalCode"};

    uint64_t total_frame_number = 0;

    uefi_memory_descriptor_for_each(d, descriptor_buffer, descriptor_buffer_size, descriptor_size)
    {
        Print(L"%a, %u KB\n", memory_types[d->Type], d->NumberOfPages * 4);
        total_frame_number += d->NumberOfPages;
    }

    Print(L"Total: %u KB\n", total_frame_number);
}
#endif

#ifdef DEBUG_BOOT_GRAPHIC
static EFI_STATUS print_graphic_modes(const EFI_GRAPHICS_OUTPUT_PROTOCOL* const graphics_output)
{
    EFI_STATUS status;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
    uint64_t info_size = 0;

    for (uint64_t i = 0; i < graphics_output->Mode->MaxMode; ++i) {
        status =
            uefi_call_wrapper(graphics_output->QueryMode, 4, graphics_output, i, &info_size, &info);
        if (EFI_ERROR(status)) {
            return status;
        }

        Print(L"Mode: %2d, Width: %4d, Height: %4d %s\n", i, info->HorizontalResolution,
              info->VerticalResolution, i == graphics_output->Mode->Mode ? "(current)" : " ");
    }

    return EFI_SUCCESS;
}
#endif

static EFI_STATUS open_file(const EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* const simple_file_system,
                            EFI_FILE** const out, EFI_FILE* const root, const CHAR16* const path)
{
    EFI_STATUS status;

    if (root == NULL) {
        status = uefi_call_wrapper(simple_file_system->OpenVolume, 2, simple_file_system, &root);
        if (EFI_ERROR(status)) {
            return status;
        }
    }

    status =
        uefi_call_wrapper(root->Open, 5, root, out, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(status)) {
        return status;
    }

    return EFI_SUCCESS;
}

static EFI_STATUS set_graphic_mode(const EFI_GRAPHICS_OUTPUT_PROTOCOL* const graphics_output)
{
    EFI_STATUS status =
        uefi_call_wrapper(graphics_output->SetMode, 2, graphics_output, GRAPHIC_MODE_NUMBER);
    return status;
}

static EFI_STATUS get_graphic_frame_buffer_data(
    const EFI_GRAPHICS_OUTPUT_PROTOCOL* const graphics_output, BootData* const boot_data)
{
    /* Unsupported pixel format. */
    if (graphics_output->Mode->Info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
        graphics_output->Mode->Info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
        return EFI_ABORTED;
    }

#ifdef DEBUG_BOOT_GRAPHIC
    print_graphic_modes(graphics_output);
#endif

    GraphicFrameBufferData* const buffer_data = &boot_data->frame_buffer_data;

    buffer_data->address = graphics_output->Mode->FrameBufferBase;
    buffer_data->size = graphics_output->Mode->FrameBufferSize;
    buffer_data->width = graphics_output->Mode->Info->HorizontalResolution;
    buffer_data->height = graphics_output->Mode->Info->VerticalResolution;
    buffer_data->pixel_format = graphics_output->Mode->Info->PixelFormat;
    buffer_data->pixel_per_scanline = graphics_output->Mode->Info->PixelsPerScanLine;

    return EFI_SUCCESS;
}

static EFI_STATUS load_font_psf1(const EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* const simple_file_system,
                                 BootData* const boot_data, EFI_FILE_PROTOCOL* const root,
                                 const CHAR16* const path)
{
    EFI_STATUS status;

    EFI_FILE_PROTOCOL* font_file = NULL;
    status = open_file(simple_file_system, &font_file, root, path);
    if (EFI_ERROR(status)) {
        return status;
    }

    uint64_t header_size = sizeof(boot_data->psf1_data.header);
    uefi_call_wrapper(font_file->Read, 3, font_file, &header_size, &boot_data->psf1_data.header);
    if (boot_data->psf1_data.header.magic[0] != PSF1_MAGIC0 &&
        boot_data->psf1_data.header.magic[1] != PSF1_MAGIC1) {
        return EFI_ABORTED;
    }

    uint64_t glyph_bufffer_size = 0;
    switch (boot_data->psf1_data.header.mode) {
    case PSF1_MODE256:
    case PSF1_MODEHASTAB:
        glyph_bufffer_size = boot_data->psf1_data.header.glyph_size * 256;
        break;
    case PSF1_MODE512:
    case PSF1_MODEHASSEQ:
    default:
        /* Unsupported font format. */
        return EFI_ABORTED;
    }

    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, glyph_bufffer_size,
                               (void**)&boot_data->psf1_data.glyph_buffer);
    if (EFI_ERROR(status)) {
        return status;
    }

    uefi_call_wrapper(font_file->SetPosition, 2, font_file, header_size);
    uefi_call_wrapper(font_file->Read, 3, font_file, &glyph_bufffer_size,
                      boot_data->psf1_data.glyph_buffer);

    return EFI_SUCCESS;
}

static EFI_STATUS load_kernel_elf(BootData* const boot_data,
                                  const EFI_FILE_PROTOCOL* const kernel_file)
{
    EFI_STATUS status;

    Elf64_Ehdr elf_header;
    uint64_t elf_header_size = sizeof(elf_header);
    uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &elf_header_size, &elf_header);

    if (strcmp((char*)elf_header.e_ident, ELFMAG, SELFMAG) ||
        elf_header.e_ident[EI_CLASS] != ELFCLASS64 || elf_header.e_ident[EI_DATA] != ELFDATA2LSB ||
        elf_header.e_ident[EI_VERSION] != EV_CURRENT || elf_header.e_machine != EM_X86_64 ||
        elf_header.e_version != EV_CURRENT ||
        (elf_header.e_type != ET_EXEC && elf_header.e_type != ET_DYN)) {
        Print(L"Kernel ELF header is invalid.\n");
        return EFI_ABORTED;
    }

    /* Read the program header table. */
    uint64_t program_header_table_size = elf_header.e_phentsize * elf_header.e_phnum;
    uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, elf_header.e_phoff);
    uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &program_header_table_size,
                      program_header_table);
    if (program_header_table_size != elf_header.e_phentsize * elf_header.e_phnum) {
        Print(L"Failed to read the program hdaer table.\n");
        Print(L"Read program header size %lu", program_header_table_size);
        return EFI_ABORTED;
    }

    /*
     * Get total memory size required to load the kernel.
     *
     * Note that loadable segment entries in the program header table appear in
     * ascending order, sorted on the `p_vaddr` member value.
     */

    /*
     * Find the smallest `p_vaddr` value first.
     *
     * As loadable segment entries are sorted on the `p_vaddr` value in
     * ascending order, the first appears has the smallest vaddr value.
     */
    uint64_t smallest_vaddr = 0;
    for (uint64_t i = 0; i < elf_header.e_phnum; ++i) {
        if (program_header_table[i].p_type != PT_LOAD) {
            continue;
        }

        smallest_vaddr = program_header_table[i].p_vaddr;
        break;
    }

    /* Find the largest `p_vaddr` value and get the segment size of the last
     * loadable segment. */
    uint64_t largest_vaddr = 0;
    uint64_t last_loadable_segment_size = 0;
    for (uint64_t i = 0; i < elf_header.e_phnum; ++i) {
        if (program_header_table[i].p_type != PT_LOAD) {
            continue;
        }

        largest_vaddr = program_header_table[i].p_vaddr;
        last_loadable_segment_size = program_header_table[i].p_memsz;
    }

    /* Calcuate the required memory size. */
    uint64_t total_kernel_memory_size = largest_vaddr - smallest_vaddr + last_loadable_segment_size;

    /* Allocate pages to load the kernel. */
    uint64_t number_of_pages = (total_kernel_memory_size % EFI_PAGE_SIZE) > 0
                                   ? (total_kernel_memory_size / EFI_PAGE_SIZE) + 1
                                   : (total_kernel_memory_size / EFI_PAGE_SIZE);
    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData,
                               number_of_pages, (void**)&boot_data->kernel_start_address);
    if (EFI_ERROR(status)) {
        return status;
    }

    /*
     * Load the kernel into memory.
     *
     * Because position-independent code uses relative addressing between
     * segments, the difference between virtual addresses in memory must match
     * the difference between virtual addresses in the file.
     *
     * The difference between the virtual address of any segment in memory and
     * the corresponding virtual address in the file is thus a single constant
     * value for any one executable or shared object in a given process. This
     * difference is "base address". One use of the base address is to reloacte
     * the memory image of the program during dynamic linking.
     *
     * An executable or shared object's base address is calculated during
     * execution from three values: the virtual memory load address, the maximum
     * page size, and the lowest virtual address of a program's loadable
     * segment.
     *
     * To compute the base address, one determines the memory address associated
     * with the lowest `p_vaddr` value for a `PT_LOAD` segment. This address is
     * truncated to the nearest multiple of the maximum page size. The
     * corresponding `p_vaddr` value itself is also truncated to the nearest
     * multiple of the maximum page size.
     *
     * The base address is the difference between the truncated memory address
     * and the truncated `p_vaddr` value.
     */
    uint64_t base_address = boot_data->kernel_start_address;
    uint64_t absolute_offset = base_address > smallest_vaddr ? base_address - smallest_vaddr
                                                             : smallest_vaddr - base_address;
    uint64_t current_segment_file_size = 0;
    uint64_t current_load_address = 0;

    for (uint64_t i = 0; i < elf_header.e_phnum; ++i) {
        if (program_header_table[i].p_type != PT_LOAD) {
            continue;
        }

        current_segment_file_size = program_header_table[i].p_filesz;
        current_load_address = base_address > smallest_vaddr
                                   ? program_header_table[i].p_vaddr + absolute_offset
                                   : program_header_table[i].p_vaddr - absolute_offset;

        uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file,
                          program_header_table[i].p_offset);
        uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &current_segment_file_size,
                          (void*)current_load_address);
        if (current_segment_file_size != program_header_table[i].p_filesz) {
            Print(L"Failed to load the kernel.\n");
            return EFI_ABORTED;
        }
    }

    boot_data->kernel_end_address = current_load_address + last_loadable_segment_size;

    return 0;
}

static EFI_STATUS get_memory_map_data(BootData* const boot_data,
                                      EFI_MEMORY_DESCRIPTOR* const descriptor_buffer)
{
    EFI_STATUS status;

    boot_data->memory_map_data.memory_descriptor_buffer = descriptor_buffer;
    boot_data->memory_map_data.memory_descriptor_buffer_size =
        sizeof(EFI_MEMORY_DESCRIPTOR) * UEFI_MEMORY_DESCRIPTOR_BUFFER_SIZE;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                               &boot_data->memory_map_data.memory_descriptor_buffer_size,
                               boot_data->memory_map_data.memory_descriptor_buffer,
                               &boot_data->memory_map_data.memory_map_key,
                               &boot_data->memory_map_data.memory_descriptor_size,
                               &boot_data->memory_map_data.memory_descriptor_version);
    if (EFI_ERROR(status)) {
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table)
{
    EFI_STATUS status;
    InitializeLib(image_handle, system_table);

    BootData boot_data;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* simple_file_system = NULL;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiSimpleFileSystemProtocolGuid, NULL,
                               (void**)&simple_file_system);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate SimpleFileSystemProtocol. %r\n", status);
        goto ERROR;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics_output = NULL;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL,
                               (void**)&graphics_output);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate GraphicsOutputProtocol. %r\n", status);
        goto ERROR;
    }

    Print(L"Set graphic mode.\n");
    status = set_graphic_mode(graphics_output);
    if (EFI_ERROR(status)) {
        Print(L"Failed to set graphic mode. %r\n", status);
        goto ERROR;
    }

    Print(L"Open kernel file.\n");
    EFI_FILE* kernel_file = NULL;
    status = open_file(simple_file_system, &kernel_file, NULL, L"kernel.elf");
    if (EFI_ERROR(status)) {
        Print(L"Failed to open kernel file. %r\n", status);
        goto ERROR;
    }

    Print(L"Load kernel ELF file.\n");
    status = load_kernel_elf(&boot_data, kernel_file);
    if (EFI_ERROR(status)) {
        Print(L"Failed to load kernel ELF file. %r\n", status);
        goto ERROR;
    }

    Print(L"Kernel Info:\n");
    Print(L"StartAddress: 0x%X, EndAddress: 0x%X\n", boot_data.kernel_start_address,
          boot_data.kernel_end_address);

    Print(L"Get graphic frame buffer info.\n");
    status = get_graphic_frame_buffer_data(graphics_output, &boot_data);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get graphic frame buffer information. %r\n", status);
        goto ERROR;
    }

    Print(L"Graphic Frame Buffer Info:\n");
    Print(L"BaseAddress: 0x%X, BufferSize: %u, Width: %u, Height: %u\n",
          boot_data.frame_buffer_data.address, boot_data.frame_buffer_data.size,
          boot_data.frame_buffer_data.width, boot_data.frame_buffer_data.height);

    Print(L"Load PSF1 font.\n");
    status = load_font_psf1(simple_file_system, &boot_data, NULL, L"zap-light16.psf");
    if (EFI_ERROR(status)) {
        Print(L"Failed to load PSF1 font. %r\n", status);
        goto ERROR;
    }

    Print(L"PSF1 Font Info:\n");
    Print(L"GlyphSize: %d\n", boot_data.psf1_data.header.glyph_size);

    Print(L"Get memory map of UEFI system.\n");
    status = get_memory_map_data(&boot_data, descriptor_buffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get memory map of UEFI system. %r\n", status);
        goto ERROR;
    }

    Print(L"Exit boot services.\n");
    uefi_call_wrapper(BS->ExitBootServices, 2, image_handle,
                      boot_data.memory_map_data.memory_map_key);

#ifdef DEBUG_BOOT_MEMORY
    print_memory_map(boot_data.memory_map_data.memory_descriptor_buffer,
                     boot_data.memory_map_data.memory_descriptor_buffer_size,
                     boot_data.memory_map_data.memory_descriptor_size);
#endif

    Print(L"Start kernel.\n");
    int (*start_kernel)(BootData) =
        (__attribute__((sysv_abi)) int (*)(const BootData))boot_data.kernel_start_address;

    Print(L"%d\n", start_kernel(boot_data));

    return EFI_SUCCESS;

ERROR:
    uefi_call_wrapper(BS->Exit, 4, image_handle, EFI_ABORTED, 0, NULL);

    return EFI_ABORTED;
}
