srcs_fb = [
    'fballpriv.c', 'fbarc.c', 'fbbits.c', 'fbblt.c', 'fbbltone.c',
    'fbcmap_mi.c', 'fbcopy.c', 'fbfill.c', 'fbfillrect.c', 'fbfillsp.c',
    'fbgc.c', 'fbgetsp.c', 'fbglyph.c', 'fbimage.c', 'fbline.c',
    'fboverlay.c', 'fbpict.c', 'fbpixmap.c', 'fbpoint.c', 'fbpush.c',
    'fbscreen.c', 'fbseg.c', 'fbsetsp.c', 'fbsolid.c', 'fbtrap.c',
    'fbutil.c', 'fbwindow.c',
]

hdrs_fb = [
    'fb.h', 'fboverlay.h', 'fbpict.h', 'fbrop.h', 'wfbrename.h'
]

# 1. The Internal Static Library (Keep this for core linkage)
libxserver_fb = static_library('libxserver_fb',
    srcs_fb,
    include_directories: inc,
    dependencies: common_dep,
    pic: true,
)

# 2. The FB "Module" (Now built as a proper library to stop the shadow warning)
# We use library() instead of shared_module()
fb_lib = library('fb',
    srcs_fb,
    include_directories: inc,
    dependencies: common_dep,
    install: true,
    install_dir: join_paths(get_option('module_dir'), 'lib'),
    # This is the secret sauce: tell the linker to allow undefined symbols
    # which is standard for Xorg modules but usually bugs out shared_library()
    override_options: ['b_lundef=false'],
    pic: true
)

wfb_args = '-DFB_ACCESS_WRAPPER'

# 3. The WFB Static Library
libxserver_wfb = static_library('libxserver_wfb',
    srcs_fb,
    c_args: wfb_args,
    include_directories: inc,
    dependencies: common_dep,
    pic: true,
    build_by_default: false,
)

# 4. The WFB "Module" 
wfb_lib = library('wfb',
    srcs_fb,
    c_args: wfb_args,
    include_directories: inc,
    dependencies: common_dep,
    install: true,
    install_dir: join_paths(get_option('module_dir'), 'lib'),
    override_options: ['b_lundef=false'],
    pic: true
)

install_data(hdrs_fb, install_dir: xorgsdkdir)

# Export these for the shadow module and the Monolith
set_variable('fb_lib', fb_lib)
set_variable('wfb_lib', wfb_lib)
