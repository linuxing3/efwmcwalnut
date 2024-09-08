if has_config('local') then
    target('glfw3webgpu')
    set_kind('static')
    add_files('*.c')
    set_targetdir('.')
    add_includedirs('.')
    add_includedirs('../webgpu/include/webgpu')
    add_includedirs('../glfw-walnut/include/')
    add_linkdirs('../webgpu/bin/windows-x86_64')
    add_links('wgpu')
    if is_os('windows') then
        add_cxxflags('/EHsc', '/Zc:preprocessor', '/Zc:__cplusplus')
    end
else
    add_requires('glfw-walnut')
    target('glfw3webgpu')
    set_kind('static')
    add_files('*.c')
    set_targetdir('.')
    add_includedirs('.')
    add_includedirs('../webgpu/include/webgpu')
    add_packages('glfw-walnut')
    add_linkdirs('../webgpu/bin/linux-x86_64')
    add_links('wgpu')
end
