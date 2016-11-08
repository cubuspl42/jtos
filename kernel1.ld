SECTIONS
{
	/* . = 0xc000000000000000; */
	. = 0x100000;
	.text :
	{
		*(.head1)
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
