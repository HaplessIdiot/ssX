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

# FIX 1: Use an absolute path for the compiler to find your flat headers.
# Relative paths in 'has_header' often fail in sub-sub-directories.
cc = meson.get_compiler('c')
current_dir = meson.current_source_dir()

if cc.has_header('fourcc.h', args: '-I' + current_dir)
  message('Sovereign Check: fourcc.h confirmed at ' + current_dir)
  drm_args = ['-I' + current_dir]
else
  # Fallback to the system libdrm path if it exists
  drm_incdir = libdrm_dep.get_variable(pkgconfig: 'includedir')
  drm_args = ['-I' + drm_incdir]
endif

# Check for GFX12 tile version support using the FLAT header path
gfx12_args = []
gfx12_test = '''
#include <fourcc.h>
int main() {
    int x = AMD_FMT_MOD_TILE_VER_GFX12;
    return 0;
}
'''
if cc.compiles(gfx12_test, dependencies: [libdrm_dep], args: drm_args)
  gfx12_args = ['-DHAVE_AMD_FMT_MOD_TILE_GFX12']
else
  message('GFX12 constants not found - driver will target legacy GFX paths')
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
  xorg_deps, # plural to match ssX master
  xproto_dep
]

if udev_dep.found()
  amdgpu_drv_libs += udev_dep
endif

# FIX 2: Manually reconstruct the module path because 'xorg_module_dir' is out of scope.
# This pulls from the '-Dmodule_dir' option you set at the root.
module_path = get_option('module_dir')
if module_path == ''
  # Standard fallback if not explicitly set
  module_path = join_paths(get_option('libdir'), 'xorg/modules')
endif

shared_module(
  'amdgpu_drv',
  srcs,
  include_directories: [include_directories('..'), include_directories('.')],
  dependencies: amdgpu_drv_libs,
  c_args: drm_args + gfx12_args,
  install: true,
  install_dir: join_paths(module_path, 'drivers'),
  name_prefix: '',
)
