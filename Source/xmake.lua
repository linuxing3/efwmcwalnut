--- @diagnostic disable:undefined-global

if has_config('local') then
    add_rules('mode.debug', 'mode.release')
    target('efwmcwalnut')
    set_languages('c++20', 'c99')
    set_kind('static')
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
else
    add_requires('imgui-walnut walnut', { configs = { glfw = true, vulkan = true, wgpu = true } })
    add_requires('glfw-walnut walnut', { configs = { glfw_include = 'vulkan' } })
    add_requires('glm')
    add_requires('stb')
    add_requires('tinyobjloader')
    add_requires('spdlog')
    add_requires('vulkan-headers')

    if is_os('windows') then
        add_cxxflags('/EHsc', '/Zc:preprocessor', '/Zc:__cplusplus')
    end
    add_rules('mode.debug', 'mode.release')
    target('efwmcwalnut')
    set_languages('c++20', 'c99')
    set_kind('static')
    add_files('**.cpp')
    add_includedirs('.')
    add_defines('RESOURCE_DIR="./wgpu"')
    add_defines('WEBGPU_BACKEND_WGPU')
    add_deps('glfw3webgpu')
    add_packages('glfw-walnut', 'imgui-walnut')
    -- packges without link need
    add_packages('vulkan-headers', 'stb', 'tinyobjloader')
    -- local packges with include and link need
    add_includedirs('$(projectdir)/vendor/glfw3webgpu')
    add_links('glfw3webgpu')
    add_includedirs('$(projectdir)/vendor/webgpu/include')
    add_includedirs('$(projectdir)/vendor/webgpu/include/webgpu')
    add_linkdirs('$(projectdir)/vendor/webgpu/bin/linux-x86_64')
    add_links('wgpu')
end
