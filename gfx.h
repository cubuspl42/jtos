#include <efi.h>

typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *protocol;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION mode_info;
    uint32_t mode_id;
    void *buffer_base;
    uint64_t buffer_size;
} Gfx;

void init_gfx(Gfx *gfx);
