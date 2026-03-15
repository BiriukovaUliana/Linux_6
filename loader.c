#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/mman.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <ELF file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    Elf64_Ehdr ehdr;
    read(fd, &ehdr, sizeof(ehdr));

    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        printf("Not an ELF file\n");
        return 1;
    }

    printf("ELF entry point: 0x%lx\n", ehdr.e_entry);

    // Масив для всіх заголовків програм
    Elf64_Phdr *phdrs = malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
    lseek(fd, ehdr.e_phoff, SEEK_SET);
    read(fd, phdrs, ehdr.e_phnum * sizeof(Elf64_Phdr));

    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD)
            continue;

        printf("Loading segment %d at 0x%lx\n", i, phdrs[i].p_vaddr);

        // Вирівнювання адреси по сторінці (4KB)
        uint64_t virt_addr = phdrs[i].p_vaddr & ~0xfff;
        uint64_t offset_in_page = phdrs[i].p_vaddr & 0xfff;
        uint64_t map_len = phdrs[i].p_memsz + offset_in_page;

        void *addr = mmap(
            (void *)virt_addr,
            map_len,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
            -1,
            0);

        if (addr == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }

        // Копіюємо дані сегмента з файлу в пам'ять
        lseek(fd, phdrs[i].p_offset, SEEK_SET);
        read(fd, (void *)phdrs[i].p_vaddr, phdrs[i].p_filesz);

        // Обнуляємо залишок сегмента (.bss), якщо p_memsz > p_filesz
        if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
            memset((void *)(phdrs[i].p_vaddr + phdrs[i].p_filesz), 0, 
                   phdrs[i].p_memsz - phdrs[i].p_filesz);
        }
    }

    free(phdrs);
    close(fd);

    printf("Jumping to entry point...\n");

    // Важливо: переконайся, що прототип функції правильний
    void (*entry)() = (void (*)())ehdr.e_entry;
    entry();

    return 0;
}
