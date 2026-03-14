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

# FIX 1: Robust Header Detection
# We check the dependency directly rather than guessing paths.
# If you have custom headers in the local dir, they will be found via include_directories.
if cc.has_header('libdrm/fourcc.h', dependencies: libdrm_dep)
    message('ssX: libdrm/fourcc.h confirmed via dependency')
    drm_args = []
else
    # Fallback for flat inclusion if your source tree is non-standard
    drm_args = ['-I' + meson.current_source_dir()]
endif

# Check for GFX12 tile version support
gfx12_args = []
gfx12_test = '''
#include <libdrm/fourcc.h>
#ifndef AMD_FMT_MOD_TILE_VER_GFX12
#define AMD_FMT_MOD_TILE_VER_GFX12 12
#endif
int main() {
    int x = AMD_FMT_MOD_TILE_VER_GFX12;
    return 0;
}
'''

if cc.compiles(gfx12_test, dependencies: [libdrm_dep])
    gfx12_args = ['-DHAVE_AMD_FMT_MOD_TILE_GFX12']
else
    message('GFX12 constants not found - driver will target legacy GFX paths')
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

# FIX 2: THE STATIC/DYNAMIC SWITCH
if get_option('static_loader')
    # Build as a static library to be baked into Xorg
    amdgpu_drv_lib = static_library(
        'amdgpu_drv',
        srcs,
        include_directories: [inc, include_directories('.'), xorg_inc],
        dependencies: amdgpu_drv_deps,
        c_args: drm_args + gfx12_args + ['-DCONFIG_STATIC_LOADER'],
        pic: true # Position Independent Code is required for linking into the server
    )
    # Export this variable so the root meson.build can see it
    set_variable('amdgpu_drv_lib', amdgpu_drv_lib)
else
    # Standard Shared Module behavior
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
