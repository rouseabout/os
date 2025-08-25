#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void perror2(const char * where)
{
    perror(where);
    exit(EXIT_FAILURE);
}

typedef struct {
    int fd;
    ElfHeader hdr;
    ElfSHeader * sections;
} File;

static int load(File * f, const char * path)
{
    f->fd = open(path, O_RDONLY);
    if (f->fd == -1)
        perror2(path);

    if (read(f->fd, &f->hdr, sizeof(f->hdr)) != sizeof(f->hdr)) {
        fprintf(stderr, "read error\n");
        return -1;
    }

    f->sections = malloc(f->hdr.e_shnum * sizeof(ElfSHeader));
    assert(f->sections);

    lseek(f->fd, f->hdr.e_shoff, SEEK_SET);
    if (read(f->fd, f->sections, f->hdr.e_shnum * sizeof(ElfSHeader)) != f->hdr.e_shnum * sizeof(ElfSHeader)) {
        fprintf(stderr, "read error\n");
        return -1;
    }

    for (int i = 0; i < f->hdr.e_shnum; i++) {
        ElfSHeader * sh = &f->sections[i];
        if (sh->sh_type == SHT_STRTAB || sh->sh_type == SHT_SYMTAB || sh->sh_type == SHT_REL) {
            sh->sh_addr = (uintptr_t)malloc(sh->sh_size);
            assert(sh->sh_addr);
            lseek(f->fd, sh->sh_offset, SEEK_SET);
            read(f->fd, (void *)sh->sh_addr, sh->sh_size);
        }
    }
    return 0;
}

typedef struct {
    uintptr_t origin;
    uint8_t * data;
    size_t size;
    size_t bss_size;
} Program;

static void align(Program * section, int align)
{
    int pad = (-section->size) & (align - 1);
    section->data = realloc(section->data, section->size + pad);
    assert(section->data);
    memset(section->data + section->size, 0xCC, pad);
    section->size += pad;
}

static void append(Program * section, File * file, int idx)
{
    ElfSHeader * sh = &file->sections[idx];
    align(section, sh->sh_addralign);
    sh->sh_addr = section->size; /* set base address */
    section->data = realloc(section->data, section->size + sh->sh_size);
    assert(section->data);
    lseek(file->fd, sh->sh_offset, SEEK_SET);
    read(file->fd, section->data + section->size, sh->sh_size);
    section->size += sh->sh_size;
}

static void append_section(Program * program, int phase, File * files, int nb_files)
{
    for (int i = 0; i < nb_files; i++) {
        for (int j = 0; j < files[i].hdr.e_shnum; j++) {
            ElfSHeader * sh = &files[i].sections[j];
            if (sh->sh_type == SHT_PROGBITS && phase == 0) {
                append(program, &files[i], j);
            } else if (sh->sh_type == SHT_NOBITS && phase == 1) {
                int pad = (-program->bss_size) & (sh->sh_addralign - 1);
                program->bss_size += pad += sh->sh_size;
            }
        }
    }
}

static uintptr_t find_symbol_addr(const char * symbol, File * files, int nb_files)
{
    for (int i = 0; i < nb_files; i++) {
        for (int j = 0; j < files[i].hdr.e_shnum; j++) {
            const ElfSHeader * shsym = &files[i].sections[j];
            if (shsym->sh_type != SHT_SYMTAB)
                continue;
            const ElfSym * sym = (ElfSym *)shsym->sh_addr;
            const ElfSHeader * shsymstr = &files[i].sections[shsym->sh_link];
            for (int k = 0; k < shsym->sh_info + 1; k++) {
                int stt = sym[k].st_info & 0xF;
                int stb = sym[k].st_info >> 4;
                if (stt == STT_NOTYPE && stb == STB_GLOBAL && sym[k].st_shndx > 0) {
                    if (!strcmp((char *)shsymstr->sh_addr + sym[k].st_name, symbol)) {
                        return files[i].sections[sym[k].st_shndx].sh_addr + sym[k].st_value;
                    }
                }
            }
        }
    }
    fprintf(stderr, "unresolved symbol: %s\n", symbol);
    exit(EXIT_FAILURE);
    return 0;
}

static void relocations(Program * program, File * files, int nb_files)
{
    for (int i = 0; i < nb_files; i++) {
        for (int j = 0; j < files[i].hdr.e_shnum; j++) {
            ElfSHeader * shrel = &files[i].sections[j];
            if (shrel->sh_type != SHT_REL)
                continue;
            const ElfSHeader * shsym = &files[i].sections[shrel->sh_link];
            const ElfSym * sym = (ElfSym *)shsym->sh_addr;
            const ElfSHeader * shsymstr = &files[i].sections[shsym->sh_link];
            const ElfSHeader * shwork = &files[i].sections[shrel->sh_info];
            const ElfRel * rel = (ElfRel *)shrel->sh_addr;
            for (int k = 0; k < shrel->sh_size / sizeof(ElfRel); k++) {
                uintptr_t workaddr = shwork->sh_addr + rel[k].r_offset;
                int rt = rel[k].r_info & 0xff; 
                int symtab_idx = rel[k].r_info >> 8;
                uintptr_t offset;

                int stt = sym[symtab_idx].st_info & 0xf;
                if (stt == STT_SECTION) {
                    int idx = sym[symtab_idx].st_shndx;
                    offset = program->origin + files[i].sections[ idx ].sh_addr;
                } else if (stt == STT_NOTYPE) {
                    const char * symbol = (const char *)shsymstr->sh_addr + sym[symtab_idx].st_name;
                    offset = program->origin + find_symbol_addr(symbol, files, nb_files);
                } else {
                    fprintf(stderr, "unsupported symbol type %d\n", stt);
                    exit(EXIT_FAILURE);
                }

                if (rt == R_386_32) {
                    *(uint32_t *)(program->data + workaddr) += offset;
                } else if (rt == R_386_PC32) {
                    *(uint32_t *)(program->data + workaddr) += offset - (program->origin + workaddr);
                } else {
                    fprintf(stderr, "unsupported relocation type %d\n", rt);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

static void write_elf(const char * path, Program * program)
{
    int fd = open(path, O_WRONLY|O_CREAT, 0755);
    if (fd == -1)
        perror2(path);
    ElfHeader hdr = {
#if defined(ARCH_i686)
        .e_ident = {127, 'E', 'L', 'F', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
#elif defined(ARCH_x86_64)
        .e_ident = {127, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
#endif
        .e_type = ET_EXEC,
#if defined(ARCH_i686)
        .e_machine = EM_386,
#elif defined(ARCH_x86_64)
        .e_machine = EM_X86_64,
#endif
        .e_version = 1,
        .e_phoff = sizeof(hdr),
        .e_phnum = 1,
        .e_ehsize = sizeof(ElfHeader),
        .e_phentsize = sizeof(ElfPHeader),
        .e_entry = program->origin,
    };
    write(fd, &hdr, sizeof(hdr));
    ElfPHeader phdr = {
        .p_type   = PT_LOAD,
        .p_vaddr  = program->origin - (sizeof(hdr) + sizeof(phdr)),
        .p_paddr  = program->origin - (sizeof(hdr) + sizeof(phdr)),
        .p_memsz  = sizeof(hdr) + sizeof(phdr) + program->size,
        .p_filesz = sizeof(hdr) + sizeof(phdr) + program->size,
        .p_offset = 0,
        .p_align  = 0x1000,
        .p_flags  = 5,
    };
    write(fd, &phdr, sizeof(phdr));
    write(fd, program->data, program->size);
    close(fd);
}

int main(int argc, char ** argv)
{
    const char * o = "a.out";
    int opt;
    while ((opt = getopt(argc, argv, "ho:")) != -1) {
        switch (opt) {
        case 'o':
            o = optarg;
            break;
        case 'h':
        default:
            fprintf(stderr, "usage: %s [-o output] input\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "no input file specified\n");
        return EXIT_FAILURE;
    }

    int nb_files = argc - optind;
    File * files = malloc(nb_files*sizeof(File));
    assert(files);

    for (int i = 0; i < nb_files; i++)
        if (load(&files[i], argv[optind + i]) < 0)
            return EXIT_FAILURE;

    Program program={0};
    program.origin = 0x10000 + sizeof(ElfHeader) + sizeof(ElfPHeader);
    append_section(&program, 0, files, nb_files);
    program.bss_size = program.size;
    append_section(&program, 1, files, nb_files);

    relocations(&program, files, nb_files);

    write_elf(o, &program);

    return EXIT_SUCCESS;
}
