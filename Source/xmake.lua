--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

target('efwmcwalnut')
set_languages('c++20', 'c99')
set_kind('static')
add_files('**.cpp')
add_files('*.c')
add_includedirs('.')
add_defines('RESOURCE_DIR="./wgpu"')
add_defines('WEBGPU_BACKEND_WGPU')
-- packges with link need
add_packages('glfw-walnut', 'imgui-walnut')
add_links('glfw-walnut')
-- packges without link need
add_packages('vulkan-headers', 'stb', 'tinyobjloader')
-- local packges with include and link need
add_includedirs('$(projectdir)/vendor/webgpu/include')
add_includedirs('$(projectdir)/vendor/webgpu/include/webgpu')
add_linkdirs('$(projectdir)/vendor/webgpu/bin/linux-x86_64')
add_links('wgpu')
