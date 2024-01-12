--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

if not has_config('feature') then
    add_requires('efwmcwalnut')
end

target('wgpuapp')
set_languages('c++20')
set_kind('binary')
add_files('**.cpp')
add_includedirs('.')
add_defines('RESOURCE_DIR="./wgpu"')
add_defines('WEBGPU_BACKEND_WGPU')
-- packges with link need
add_packages('glfw3webgpu', 'glfw-walnut', 'imgui-walnut')
if has_config('feature') then
    add_deps('efwmcwalnut')
    add_includedirs('$(projectdir)/Source')
else
    add_packages('efwmcwalnut')
end
-- local packges with include and link need
add_includedirs('$(projectdir)/vendor/webgpu/include')
add_includedirs('$(projectdir)/vendor/webgpu/include/webgpu')
add_linkdirs('$(projectdir)/vendor/webgpu/bin/linux-x86_64')
add_links('wgpu')
after_build(function(target)
    os.cp('$(scriptdir)/resources/shaders/wgpu', target:targetdir())
end)
