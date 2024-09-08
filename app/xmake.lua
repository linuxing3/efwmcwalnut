--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

if has_config('local') then
    add_rules('mode.debug', 'mode.release')
    target('wgpuapp')
    set_languages('c++20', 'c99')
    set_kind('binary')
    add_files('**.cpp')
    add_includedirs('.')
    add_defines('RESOURCE_DIR="./wgpu"')
    add_defines('WEBGPU_BACKEND_WGPU')

    if is_os('windows') then
        add_cxxflags('/EHsc', '/Zc:preprocessor', '/Zc:__cplusplus')
    end

    -- packges without link need
    add_includedirs('$(projectdir)/vendor')
    add_includedirs('$(projectdir)/vendor/glm')
    add_includedirs('$(projectdir)/vendor/stb')
    add_includedirs('$(projectdir)/vendor/spdlog/include')
    add_includedirs('$(projectdir)/vendor/tinyobjloader')
    -- local packges with include and link need
    add_deps('efwmcwalnut')
    add_includedirs('$(projectdir)/Source')
    add_links('efwmcwalnut')
    add_deps('glfw3webgpu')
    add_includedirs('$(projectdir)/vendor/glfw3webgpu')
    add_links('glfw3webgpu')
    add_deps('imgui-walnut')
    add_includedirs('$(projectdir)/vendor/imgui-walnut')
    add_links('imgui-walnut')
    add_deps('glfw-walnut')
    add_includedirs('$(projectdir)/vendor/glfw-walnut/include')
    add_links('glfw-walnut')
    add_includedirs('$(projectdir)/vendor/webgpu/include')
    add_includedirs('$(projectdir)/vendor/webgpu/include/webgpu')
    add_linkdirs('$(projectdir)/vendor/webgpu/bin/linux-x86_64')
    add_links('wgpu')
    add_includedirs(path.join(os.getenv("VULKAN_SDK"), "include"))
    add_linkdirs(path.join(os.getenv("VULKAN_SDK"), "lib"))
    add_links('vulkan')
    after_build(function(target)
        os.cp('$(scriptdir)/resources/shaders/wgpu', target:targetdir())
        os.cp('$(scriptdir)/vendor/webgpu/bin/linux-x86_64/wgpu.*', target:targetdir())
        os.cp(path.join(os.getenv("VULKAN_SDK"), "lib/libvulkan.*.*"), target:targetdir())
    end)
else
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
    if is_os('windows') then
        add_cxxflags('/EHsc', '/Zc:preprocessor', '/Zc:__cplusplus')
    end
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
        os.cp('$(scriptdir)/vendor/vulkan/lib/libvulkan.so', target:targetdir())
    end)
end
