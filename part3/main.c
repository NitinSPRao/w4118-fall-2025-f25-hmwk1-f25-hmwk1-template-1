short int get_ip();

void main(void)
{
	__asm__("mov $0x0, %ax");
	__asm__("mov %ax, %ds");


	__asm__ volatile (
		"mov %%sp, %0"
		: "=r" (sp_value)
		:
		: "memory"
	);

}

