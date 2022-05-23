OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS
{
    text PT_LOAD ;
    data PT_LOAD ;
}

SECTIONS
{
	.text : ALIGN(0x08) { *(.text) } :text
	.data : ALIGN(0x08) { *(.data) } :data
	.rodata : ALIGN(0x08) { *(.rodata) } :data
	.bss : ALIGN(0x08) { *(COMMON) *(.bss) } :data
}
