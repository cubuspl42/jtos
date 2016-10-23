SECTIONS
{
	. = 0x100000;
	.text :
	{
		*(.head)
		*(.text)
	}
	.data :
	{
		*(.data)
	}
	.bss :
	{
		*(.bss)
	}
}
