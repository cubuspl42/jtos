SECTIONS
{
	. = 0x100000;
	.text :
	{
		*(.head0)
		*(.text)
	}
	.data : ALIGN(0x1000)
	{
		*(.data)
	}
	.bss :
	{
		*(.stack0)
		*(.bss)
		_kernel0_end = .;
	}
}
