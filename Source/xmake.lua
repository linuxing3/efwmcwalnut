--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

target('efwmcwalnut')
set_languages('c++20')
set_kind('static')
add_files('**.cpp')
add_includedirs('.')
add_defines('RESOURCE_DIR="./wgpu"')
add_defines('WEBGPU_BACKEND_WGPU')
add_packages('glfw3webgpu', 'glfw-walnut', 'imgui-walnut', 'vulkan-headers', 'stb', 'tinyobjloader')
add_includedirs('$(projectdir)/vendor/webgpu/include')
add_includedirs('$(projectdir)/vendor/webgpu/include/webgpu')
add_linkdirs('$(projectdir)/vendor/webgpu/bin/linux-x86_64')
add_links('wgpu', 'glfw3webgpu', 'glfw')
