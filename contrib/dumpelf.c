#include <elf.h>
#include <stdio.h>
#include <stdlib.h>

static void dump_hex(const void * ptr, unsigned int size)
{
    const uint8_t * buf = ptr;
    for (unsigned i = 0 ; i < size; i += 16) {
        printf("%5x:", i);
        for (unsigned int j = i; j < size && j < i + 16; j++) {
            printf(" %02x", buf[j]);
        }
        printf(" | ");
        for (unsigned int j = i; j < size && j < i + 16; j++) {
            printf("%c", buf[j] >= 32 && buf[j] <= 127 ? buf[j] : '.');
        }
        printf("\n");
    }
}

static int fdump(FILE * f, int offset, int size)
{
    void * buf = malloc(size);
    if (!buf)
        return -1;
    fseek(f, offset, SEEK_SET);
    fread(buf, size, 1, f);
    dump_hex(buf, size);
    free(buf);
    return 0;
}

int main(int argc, char ** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        return -1;
    }

    FILE * f = fopen(argv[1], "rb");
    if (!f)
        return -1;

    ElfHeader e;
    if (fread(&e, sizeof(e), 1, f) != 1)
        return -1;

    printf("e_ident:");
    for (int i = 0; i < sizeof(e.e_ident); i++)
        printf(" %d", e.e_ident[i]);
    printf("\n");

    printf("e_type: %d\n", e.e_type);
    printf("e_machine: %d\n", e.e_machine);
    printf("e_version: %d\n", e.e_version);
    printf("e_entry: 0x%x\n", e.e_entry);
    printf("e_phoff: 0x%x\n", e.e_phoff);
    printf("e_shoff: 0x%x\n", e.e_shoff);
    printf("e_flags: 0x%x\n", e.e_flags);
    printf("e_ehsize: %d bytes (expect %d)\n", e.e_ehsize, sizeof(ElfHeader));
    printf("e_phentsize: %d bytes (expect %d)\n", e.e_phentsize, sizeof(ElfPHeader));
    printf("e_phnum: %d\n", e.e_phnum);
    printf("e_shentsize: %d bytes (expect %d)\n", e.e_shentsize, sizeof(ElfSHeader));
    printf("e_shnum: %d\n", e.e_shnum);
    printf("e_shstrndx: %d\n", e.e_shstrndx);
    //printf("sizeof(ElfHeader): %d\n", sizeof(ElfHeader));
    printf("\n");

    switch(e.e_machine) {
#if defined(ARCH_i686)
    case 0x3: break;
#elif defined(ARCH_x86_64)
    case 0x3E: break;
#endif
    default:
        printf("unsupported architecture\n");
        return -1;
    }

    for (int i = 0; i < e.e_phnum; i++) {
        fseek(f, e.e_phoff + i * sizeof(ElfPHeader), SEEK_SET);
        ElfPHeader p;
        printf("---Program Header %d---\n", i);
        if (fread(&p, sizeof(p), 1, f) != 1)
            return -1;

        printf("p_type: 0x%x ", p.p_type);
        switch(p.p_type) {
        case PT_NULL: printf("NULL"); break;
        case PT_LOAD: printf("LOAD"); break;
        case PT_NOTE: printf("NOTE"); break;
        default: printf("(unknown)"); break;
        }
        printf("\n");

        printf("p_offset: 0x%x\n", p.p_offset);
        printf("p_vaddr: 0x%x\n", p.p_vaddr);
        printf("p_paddr: 0x%x\n", p.p_paddr);
        printf("p_filesz: 0x%x\n", p.p_filesz);
        printf("p_memsz: 0x%x\n", p.p_memsz);
        printf("p_flags: 0x%x\n", p.p_flags);
        printf("p_align: 0x%x\n", p.p_align);
        printf("\n");
        fdump(f, p.p_offset, p.p_filesz);
    }

    for (int i = 0; i < e.e_shnum; i++) {
        fseek(f, e.e_shoff + i * sizeof(ElfSHeader), SEEK_SET);
        ElfSHeader s;
        printf("---Section Header %d---\n", i);
        if (fread(&s, sizeof(s), 1, f) != 1)
            return -1;

        printf("sh_shname: 0x%x\n", s.sh_name);
        printf("sh_type: 0x%x ", s.sh_type);
        switch (s.sh_type) {
        case SHT_NULL: printf("NULL"); break;
        case SHT_PROGBITS: printf("PROGBITS"); break;
        case SHT_SYMTAB: printf("SYMTAB"); break;
        case SHT_NOBITS: printf("NOBITS"); break;
        case SHT_STRTAB: printf("STRTAB"); break;
        case SHT_REL: printf("REL"); break;
        default: printf("(unknown)"); break;
        }
        printf("\n");

        printf("sh_flags: 0x%x", s.sh_flags);
        if (s.sh_flags & SHF_WRITE) printf(" WRITE");
        if (s.sh_flags & SHF_ALLOC) printf(" ALLOC");
        if (s.sh_flags & SHF_EXECINSTR) printf(" EXECUTABLE");
        printf("\n");

        printf("sh_addr: 0x%x\n", s.sh_addr);
        printf("sh_offet: 0x%x\n", s.sh_offset);
        printf("sh_size: 0x%x\n", s.sh_size);
        printf("sh_link: %d\n", s.sh_link);
        printf("sh_info: 0x%x\n", s.sh_info);
        printf("sh_addralign: 0x%x\n", s.sh_addralign);
        printf("sh_entsize: 0x%x\n", s.sh_entsize);
        printf("\n");
        fdump(f, s.sh_offset, s.sh_size);
    }

    fclose(f);
    return 0;
}
