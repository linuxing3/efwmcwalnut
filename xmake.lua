--- @diagnostic disable:undefined-global

set_project('efwmcwalnut')

set_version('0.0.1')

set_xmakever('2.7.9')

set_warnings('all')
set_languages('c++20')

set_allowedplats('windows', 'linux', 'macosx')

includes('xmake')
includes('vendor')
includes('Source')

if has_config('example') then
    includes('app')
end
