SECTIONS
{
	. = 0x100000;
	.text :
	{
		*(.head0)
		*(.text)
	}
	.data :
	{
		*(.data)
	}
	.bss :
	{
		*(.stack)
		*(.bss)
	}
}
