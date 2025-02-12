# CMakeList.txt : CMake project for MathRenderer, include source and define
# project specific logic here.
#
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(IMGUI_SRC
	imgui/imgui.h
	imgui/imgui.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_demo.cpp
	imgui/backends/imgui_impl_glfw.h
	imgui/backends/imgui_impl_glfw.cpp
	imgui/backends/imgui_impl_vulkan.h
	imgui/backends/imgui_impl_vulkan.cpp
)

#add_executable (MathRenderer "MathRenderer.cpp" ${IMGUI_SRC})
add_executable (MathRenderer "MathRenderer.cpp" "VulkanEngine.cpp" ${IMGUI_SRC})
target_include_directories(MathRenderer PRIVATE "imgui" "imgui/backends")
target_link_libraries(MathRenderer glfw ${Vulkan_LIBRARIES} Vulkan::Vulkan)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MathRenderer PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
