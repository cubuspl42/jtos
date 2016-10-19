#include "gfx.h"
#include "efi.h"

#include <efilib.h>


static void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;

    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}


static void set_pixel(Gfx *gfx, int w, int h, uint32_t rgb)
{
    w *= 4;
    h *= 4;
    char *framebuffer = (char *) gfx->buffer_base;
    int32_t *addr = (int32_t *)(framebuffer + w + h * gfx->mode_info.PixelsPerScanLine);
    *addr = rgb;
}


void init_gfx(Gfx *gfx)
{
    Print(L"init_gfx\r\n");
    Print(L"%lx\r\n", efi.st->BootServices->LocateProtocol);

    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_CHECK(efi.st->BootServices->LocateProtocol(
        &gop_guid, NULL, (void **) &gfx->protocol));

    // Print(L"LocateProtocol\r\n");

    UINT32 mode = gfx->protocol->Mode->Mode;
    UINTN size;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;

    EFI_CHECK(gfx->protocol->QueryMode(gfx->protocol, mode, &size, &info));

    memcpy(&gfx->mode_info, info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

    Print(L"%dx%d\r\n", info->HorizontalResolution, info->VerticalResolution);

    EFI_CHECK(gfx->protocol->SetMode(gfx->protocol, mode));

    gfx->buffer_base = (void *) gfx->protocol->Mode->FrameBufferBase;
    gfx->buffer_size = gfx->protocol->Mode->FrameBufferSize;

    for(int i = 0; i < 64; ++i) {
        for(int j = 0; j < 64; ++j) set_pixel(gfx, j, i, 0xFFFFFFFF);
    }

    for(;;);

}
