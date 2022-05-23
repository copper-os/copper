#include <efi/efi.h>
#include <efi/efilib.h>
#include <elf.h>

#define MAX_PROGRAM_HEADER_TABLE_SIZE (512)
#define GRAPHIC_MODE_NUMBER (10)

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

static EFI_STATUS load_kernel_elf(uint64_t* kernel_start_address, const EFI_FILE_PROTOCOL* const kernel_file)
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
        Print(u"Kernel ELF header is invalid.\n");
        return EFI_ABORTED;
    }

    /* Read the program header table. */
    uint64_t program_header_table_size = elf_header.e_phentsize * elf_header.e_phnum;
    uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, elf_header.e_phoff);
    uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &program_header_table_size,
                      program_header_table);
    if (program_header_table_size != elf_header.e_phentsize * elf_header.e_phnum) {
        Print(u"Failed to read the program hdaer table.\n");
        Print(u"Read program header size %lu", program_header_table_size);
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
                               number_of_pages, (void**)kernel_start_address);
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
    uint64_t base_address = *kernel_start_address;
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
            Print(u"Failed to load the kernel.\n");
            return EFI_ABORTED;
        }
    }

#ifdef DEBUG_KERNEL_LOAD
    Print(u"KernelStartAddress: 0x%X", kernel_start_address);
    Print(u"KernelEndAddress: 0x%X\n", current_load_address + last_loadable_segment_size);
#endif

    return 0;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table)
{
    EFI_STATUS status;
    InitializeLib(image_handle, system_table);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* simple_file_system = NULL;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiSimpleFileSystemProtocolGuid, NULL,
                               (void**)&simple_file_system);
    if (EFI_ERROR(status)) {
        Print(u"Failed to locate SimpleFileSystemProtocol. %r\n", status);
        goto ERROR;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics_output = NULL;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL,
                               (void**)&graphics_output);
    if (EFI_ERROR(status)) {
        Print(u"Failed to locate GraphicsOutputProtocol. %r\n", status);
        goto ERROR;
    }

    Print(u"Set graphic mode.\n");
    status = set_graphic_mode(graphics_output);
    if (EFI_ERROR(status)) {
        Print(u"Failed to set graphic mode. %r\n", status);
        goto ERROR;
    }

    Print(u"Open kernel file.\n");
    EFI_FILE* kernel_file = NULL;
    status = open_file(simple_file_system, &kernel_file, NULL, u"kernel.elf");
    if (EFI_ERROR(status)) {
        Print(u"Failed to open kernel file. %r\n", status);
        goto ERROR;
    }

    Print(u"Load kernel ELF file.\n");
    uint64_t kernel_start_address;
    status = load_kernel_elf(&kernel_start_address, kernel_file);
    if (EFI_ERROR(status)) {
        Print(u"Failed to load kernel ELF file. %r\n", status);
        goto ERROR;
    }

    Print(u"Start kernel.\n");
    int (*start_kernel)() = (__attribute__((sysv_abi)) int (*)())kernel_start_address;
    Print(u"%d\n", start_kernel());

    // This should not be reached...
    return EFI_ABORTED;

ERROR:
    uefi_call_wrapper(BS->Exit, 4, image_handle, EFI_ABORTED, 0, NULL);

    return EFI_ABORTED;
}
