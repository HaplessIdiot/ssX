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

# We know where our headers are. No need for the Archon's logic.
if meson.get_compiler('c').has_header('drm_fourcc.h')
    message('Found DRM headers in root include path.')
    drm_args = []
else
    # Check if libdrm_dep actually has the path we need
    drm_incdir = libdrm_dep.get_variable(pkgconfig: 'includedir')
    drm_args = ['-I' + drm_incdir]
endif

# Check for GFX12 tile version support in libdrm
gfx12_args = []
gfx12_test = '''
#include <drm_fourcc.h>
int main() {
    int x = AMD_FMT_MOD_TILE_VER_GFX12;
    x = AMD_FMT_MOD_TILE_GFX12_256B_2D;
    x = AMD_FMT_MOD_TILE_GFX12_4K_2D;
    x = AMD_FMT_MOD_TILE_GFX12_64K_2D;
    x = AMD_FMT_MOD_TILE_GFX12_256K_2D;
    return 0;
}
'''
if meson.get_compiler('c').compiles(gfx12_test, dependencies: [libdrm_dep])
  gfx12_args = ['-DHAVE_AMD_FMT_MOD_TILE_GFX12']
else
  message('Some GFX12 tile constants missing - disabling GFX12 support')
endif

amdgpu_drv_libs = [
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
if libudev_dep.found()
  amdgpu_drv_libs += libudev_dep
endif

shared_module(
  'amdgpu_drv',
  srcs,
  # Add '.' to ensure it looks in the current driver folder for local headers
  include_directories: [include_directories('..'), include_directories('.')],
  dependencies: amdgpu_drv_libs,
  c_args: drm_args + gfx12_args,
  install: true,
  install_dir: join_paths(moduledir, 'drivers'),
  name_prefix: '',
)
