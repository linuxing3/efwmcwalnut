--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

if not has_config('feature') then
    add_requires('efwmcwalnut')
    add_requires('glfw3webgpu')
    add_requires('imgui-walnut walnut', { configs = { glfw = true, vulkan = true, wgpu = true } })
    add_requires('glfw-walnut walnut', { configs = { glfw_include = 'vulkan' } })
    add_requires('spdlog')
end

target('wgpuapp')
set_languages('c++20')
set_kind('binary')
add_files('**.cpp')
add_includedirs('.')
add_defines('RESOURCE_DIR="./wgpu"')
add_defines('WEBGPU_BACKEND_WGPU')
-- packges with link need
if has_config('feature') then
    add_deps('efwmcwalnut')
    add_includedirs('$(projectdir)/Source')
    add_packages('glfw-walnut', 'imgui-walnut')
else
    add_packages('spdlog')
    add_packages('efwmcwalnut')
    add_packages('glfw3webgpu')
    add_packages('glfw-walnut', 'imgui-walnut')
end
-- local packges with include and link need
add_includedirs('$(projectdir)/vendor/webgpu/include')
add_includedirs('$(projectdir)/vendor/webgpu/include/webgpu')
add_linkdirs('$(projectdir)/vendor/webgpu/bin/linux-x86_64')
add_links('wgpu')
after_build(function(target)
    os.cp('$(scriptdir)/resources/shaders/wgpu', target:targetdir())
end)
