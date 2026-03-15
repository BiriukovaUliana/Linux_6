const char msg[] = "Hello from loaded ELF!\n";

void _start() {
    asm(
        "mov $1, %%rax\n"
        "mov $1, %%rdi\n"
        "lea msg(%%rip), %%rsi\n"
        "mov $23, %%rdx\n"
        "syscall\n"

        "mov $60, %%rax\n"
        "xor %%rdi, %%rdi\n"
        "syscall\n"
        ::: "rax", "rdi", "rsi", "rdx"
    );
}
