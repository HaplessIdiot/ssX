srcs = [
  'amdgpu_bo_helper.c',
  'amdgpu_dri2.c',
  'amdgpu_dri3.c',
  'amdgpu_drm_queue.c',
  'amdgpu_glamor.c',
  'amdgpu_glamor_wrappers.c',
  'amdgpu_kms.c',
  'amdgpu_misc.c',
  'amdgpu_pixmap.c',
  'amdgpu_probe.c',
  'amdgpu_present.c',
  'amdgpu_sync.c',
  'amdgpu_video.c',
  'drmmode_display.c',
]

cc = meson.get_compiler('c')

# FORCED PATH: We point directly to the current directory for the flat header
# and we add the standard include path as a backup.
current_dir = meson.current_source_dir()
drm_args = ['-I' + current_dir]

# GFX12 Test: Explicitly forcing the check to look at our local file
gfx12_args = []
gfx12_test = '''
#include "fourcc.h"
int main() {
#ifdef AMD_FMT_MOD_TILE_VER_GFX12
    int x = AMD_FMT_MOD_TILE_VER_GFX12;
    return 0;
#else
#error GFX12_NOT_DEFINED
#endif
}
'''

# Use 'args' to inject the current directory into the test compiler call
if cc.compiles(gfx12_test, args: drm_args)
    message('ssX Sovereign: Internal fourcc.h detected via flat path.')
    gfx12_args = ['-DHAVE_AMD_FMT_MOD_TILE_GFX12']
else
    # Try the libdrm/ subdirectory style if that's how it's stored in ssX
    if cc.compiles('#include "libdrm/fourcc.h"\nint main(){return 0;}', args: drm_args)
        message('ssX Sovereign: Internal libdrm/fourcc.h detected.')
        gfx12_args = ['-DHAVE_AMD_FMT_MOD_TILE_GFX12']
    else
        message('WARNING: fourcc.h not found in local tree. GFX12 constants disabled.')
    endif
endif

amdgpu_drv_deps = [
    fontsproto_dep,
    gbm_dep,
    libdrm_amdgpu_dep,
    libdrm_dep,
    randrproto_dep,
    renderproto_dep,
    videoproto_dep,
    xextproto_dep,
    xf86driproto_dep,
    xorg_deps,
    xproto_dep
]

if udev_dep.found()
    amdgpu_drv_deps += udev_dep
endif

# THE STATIC/DYNAMIC SWITCH
if get_option('static_loader')
    amdgpu_drv_lib = static_library(
        'amdgpu_drv',
        srcs,
        # 'include_directories' needs to include the local dir '.' for "fourcc.h"
        include_directories: [inc, include_directories('.'), xorg_inc],
        dependencies: amdgpu_drv_deps,
        c_args: drm_args + gfx12_args + ['-DCONFIG_STATIC_LOADER'],
        pic: true
    )
    set_variable('amdgpu_drv_lib', amdgpu_drv_lib)
else
    shared_module(
        'amdgpu_drv',
        srcs,
        include_directories: [inc, include_directories('.'), xorg_inc],
        dependencies: amdgpu_drv_deps,
        c_args: drm_args + gfx12_args,
        install: true,
        install_dir: join_paths(get_option('module_dir'), 'drivers'),
        name_prefix: '',
    )
endif
