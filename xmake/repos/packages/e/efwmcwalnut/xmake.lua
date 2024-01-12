package('efwmcwalnut')
set_homepage('https://github.com/linuxing3/efwmcwalnut')
set_description('Bloat-free Immediate App Framework for C++ with minimal dependencies')
set_license('MIT')

add_urls('https://github.com/linuxing3/efwmcwalnut.git')
add_versions('v1.0.0', '7724c97844596f8f6c9eee249097e2e38eefc87e')

add_includedirs('include', 'include/Source')

on_install('macosx', 'linux', 'windows', 'mingw', 'android', 'iphoneos', function(package)
    os.cp('Source', package:installdir('include'))
    import('package.tools.xmake').install(package, {})
end)
