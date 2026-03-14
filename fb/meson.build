srcs_fb = [
    'fballpriv.c', 'fbarc.c', 'fbbits.c', 'fbblt.c', 'fbbltone.c',
    'fbcmap_mi.c', 'fbcopy.c', 'fbfill.c', 'fbfillrect.c', 'fbfillsp.c',
    'fbgc.c', 'fbgetsp.c', 'fbglyph.c', 'fbimage.c', 'fbline.c',
    'fboverlay.c', 'fbpict.c', 'fbpixmap.c', 'fbpoint.c', 'fbpush.c',
    'fbscreen.c', 'fbseg.c', 'fbsetsp.c', 'fbsolid.c', 'fbtrap.c',
    'fbutil.c', 'fbwindow.c',
]

# RESTORED: The SDK headers required for external drivers
hdrs_fb = [
    'fb.h', 'fboverlay.h', 'fbpict.h', 'fbrop.h', 'wfbrename.h'
]

# Internal Static Library
libxserver_fb = static_library('libxserver_fb',
    srcs_fb,
    include_directories: inc,
    dependencies: common_dep,
    pic: true,
)

# PROMOTED: Proper library to kill the 'shadow' deprecation
fb_lib = shared_library('fb',
    srcs_fb,
    include_directories: inc,
    dependencies: common_dep,
    install: true,
    install_dir: join_paths(get_option('module_dir'), 'lib'),
    override_options: ['b_lundef=false'],
    pic: true
)

wfb_args = '-DFB_ACCESS_WRAPPER'

# WFB Internal Static
libxserver_wfb = static_library('libxserver_wfb',
    srcs_fb,
    c_args: wfb_args,
    include_directories: inc,
    dependencies: common_dep,
    pic: true,
    build_by_default: false,
)

# WFB Proper Library
wfb_lib = shared_library('wfb',
    srcs_fb,
    c_args: wfb_args,
    include_directories: inc,
    dependencies: common_dep,
    install: true,
    install_dir: join_paths(get_option('module_dir'), 'lib'),
    override_options: ['b_lundef=false'],
    pic: true
)

# RESTORED: Install headers to the SDK directory
install_data(hdrs_fb, install_dir: xorgsdkdir)

# Export for shadow/monolith consumers
set_variable('fb_lib', fb_lib)
set_variable('wfb_lib', wfb_lib)
