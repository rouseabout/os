#include <efi.h>
#include <efilib.h>

EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;
EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static const EFI_GUID acpi2_guid = ACPI_20_TABLE_GUID;
static const EFI_GUID acpi1_guid = ACPI_TABLE_GUID;

#if 0
double fabs (double f)
{
    return f < 0 ? -f : f;
}

size_t strlen(const char * s)
{
    size_t size = 0;
    while (*s++)
        size++;
    return size;
}

static  int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

#include "libc/generic_printf.h"
#include "kernel/utils.h"

static void kputc(void *cntx, int c)
{
    outb(0xE9, c);
}

int kprintf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    generic_vprintf(kputc, NULL, fmt, args);
    va_end(args);
    return 0; //FIXME: return bytes printed
}
#endif

int memcmp(const void * s1, const void * s2, size_t n)
{
    const unsigned char * m1 = s1, * m2 = s2;
    int ret = 0;
    for (size_t i = 0; i < n && !(ret = m1[i] - m2[i]); i++) ;
    return ret;
}

static EFI_STATUS load_file_at(EFI_SYSTEM_TABLE *systemTable, EFI_FILE *root, CHAR16 *name, EFI_ALLOCATE_TYPE type, EFI_PHYSICAL_ADDRESS *addr, UINTN *size)
{
    EFI_FILE *kernel;
    EFI_STATUS ret;

    systemTable->ConOut->OutputString(systemTable->ConOut, name);
    systemTable->ConOut->OutputString(systemTable->ConOut, L"...");

    ret = root->Open(root, &kernel, name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (ret != EFI_SUCCESS)
        return ret;

    UINTN fileInfoSize;
    kernel->GetInfo(kernel, &gEfiFileInfoGuid, &fileInfoSize, NULL);

    EFI_FILE_INFO *fileInfo;
    ret = systemTable->BootServices->AllocatePool(EfiLoaderData, fileInfoSize, (void**)&fileInfo);
    if (ret != EFI_SUCCESS)
        return ret;

    ret = kernel->GetInfo(kernel, &gEfiFileInfoGuid, &fileInfoSize, fileInfo);
    if (ret != EFI_SUCCESS)
        return ret;

    int pages = (fileInfo->FileSize + 4095) / 4096;
    ret = systemTable->BootServices->AllocatePages(type, EfiBootServicesData, pages, addr);
    if (ret != EFI_SUCCESS) {
        if (ret == EFI_NOT_FOUND)
            systemTable->ConOut->OutputString(systemTable->ConOut, L"AllocatePages failed");
        return ret;
    }

    *size = fileInfo->FileSize;
    ret = kernel->Read(kernel, size, (void*)*addr);
    if (ret != EFI_SUCCESS)
        return ret;

    systemTable->BootServices->FreePool(fileInfo);

    kernel->Close(kernel);

    systemTable->ConOut->OutputString(systemTable->ConOut, L"\r\n");
    return EFI_SUCCESS;
}

#define write_8(params, offset, value) *(uint8_t *)(params + offset) = value
#define write_16(params, offset, value) *(uint16_t *)(params + offset) = value
#define write_32(params, offset, value) *(uint32_t *)(params + offset) = value
#define write_64(params, offset, value) *(uint64_t *)(params + offset) = value
typedef struct {
    uint64_t addr;
    uint64_t size;
    int32_t type;
} __attribute__((packed)) e820_entry;

EFI_STATUS EFIAPI efi_main(void *imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    EFI_STATUS ret;

    systemTable->ConOut->EnableCursor(systemTable->ConOut, TRUE);

    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    ret = systemTable->BootServices->HandleProtocol(imageHandle, &gEfiLoadedImageProtocolGuid, (void**)&loadedImage);
    if (ret != EFI_SUCCESS)
        return ret;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem;
    ret = systemTable->BootServices->HandleProtocol(loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&fileSystem);
    if (ret != EFI_SUCCESS)
        return ret;

    EFI_PHYSICAL_ADDRESS info = 0x10000;
    ret = systemTable->BootServices->AllocatePages(AllocateAddress, EfiBootServicesData, 2, &info);
    if (ret != EFI_SUCCESS)
        return ret;

    EFI_FILE *root;
    ret = fileSystem->OpenVolume(fileSystem, &root);
    if (ret != EFI_SUCCESS)
        return ret;

    EFI_PHYSICAL_ADDRESS kernel_addr = 0x100000;
    UINTN kernel_size;
    ret = load_file_at(systemTable, root, L"kernel", AllocateAddress, &kernel_addr, &kernel_size);
    if (ret != EFI_SUCCESS)
        return ret;

    EFI_PHYSICAL_ADDRESS initrd_addr;
    UINTN initrd_size;
    ret = load_file_at(systemTable, root, L"initrd", AllocateAnyPages, &initrd_addr, &initrd_size);
    if (ret != EFI_SUCCESS)
        return ret;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    ret = systemTable->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void **)&gop);
    if (ret != EFI_SUCCESS)
        return ret;

    write_8(info, 0xf, 0x23);
    write_16(info, 0x12, gop->Mode->Info->HorizontalResolution);
    write_16(info, 0x14, gop->Mode->Info->VerticalResolution);
    write_16(info, 0x16, 32);
    write_32(info, 0x18, gop->Mode->FrameBufferBase);
    write_16(info, 0x24, gop->Mode->Info->PixelsPerScanLine*4);
    write_32(info, 0x36, 2);
    write_32(info, 0x3a, gop->Mode->FrameBufferBase >> 32);

    void *acpi2 = NULL, *acpi1 = NULL;
    for (size_t i = 0; i < systemTable->NumberOfTableEntries; i++) {
        const EFI_CONFIGURATION_TABLE *table = &systemTable->ConfigurationTable[i];
        if (!memcmp(&table->VendorGuid, &acpi2_guid, sizeof(EFI_GUID)))
            acpi2 = table->VendorTable;
        else if (!memcmp(&table->VendorGuid, &acpi1_guid, sizeof(EFI_GUID)))
            acpi1 = table->VendorTable;
    }
    write_64(info, 0x70, acpi2 ? (uint64_t)acpi2 : (uint64_t)acpi1);

    write_32(info, 0x218, initrd_addr);
    write_32(info, 0x21c, initrd_size);
    write_32(info, 0x228, info + 0x30); //cmdline
    write_8(info, 0x30, 0);

    EFI_MEMORY_DESCRIPTOR *Map;
    UINTN MapSize, MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    systemTable->BootServices->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    MapSize += 2*DescriptorSize;

    ret = systemTable->BootServices->AllocatePool(EfiLoaderData, MapSize, (void**)&Map);
    if (ret != EFI_SUCCESS)
        return ret;

    ret = systemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (ret != EFI_SUCCESS)
        return ret;

    write_8(info, 0x1e8, MapSize / DescriptorSize);
    e820_entry * e820 = (void *)(info + 0x2d0);
    for (EFI_MEMORY_DESCRIPTOR *m = Map; m < NextMemoryDescriptor(Map, MapSize); m = NextMemoryDescriptor(m, DescriptorSize)) {
        e820->type = (m->Type == EfiConventionalMemory || m->Type == EfiLoaderCode || m->Type == EfiLoaderData) ? 1 : 0;
        e820->addr = m->PhysicalStart;
        e820->size = m->NumberOfPages*4096;
        if (m->Type == EfiConventionalMemory)
            write_32(info, 0x1e0, m->PhysicalStart/1024 + m->NumberOfPages*4 - 640);
        e820++;
    }

    ret = systemTable->BootServices->ExitBootServices(imageHandle, MapKey);
    if (ret != EFI_SUCCESS)
        return ret;

    ((__attribute__((sysv_abi)) void (*)(int magic, void * info))0x100000)(0x1337, (void*)info);
    return 0;
}
