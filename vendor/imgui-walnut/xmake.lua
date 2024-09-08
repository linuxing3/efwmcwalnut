add_rules('mode.debug', 'mode.release')

target('imgui-walnut')
set_languages('c++20')
set_kind('static')
add_includedirs('.')
add_includedirs('../glfw-walnut/include')
add_files('imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp', 'imgui_demo.cpp')
