# CPM - package manager
include(CPM)

# stb_image
set(STB_IMAGE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb_image)
set(stb_image_SOURCE_DIR ${STB_IMAGE_DIR} CACHE INTERNAL "")
add_library(stb_image STATIC ${STB_IMAGE_DIR}/stb_image.cpp)
target_include_directories(stb_image PRIVATE ${STB_IMAGE_DIR})

# Set the VML directory
set(VML_DIR ${CMAKE_CURRENT_LIST_DIR}/vml)

# Collect all header files in the VML include directory
file(GLOB_RECURSE VML_HEADERS
    "${VML_DIR}/include/*.h"
    "${VML_DIR}/include/detail/*.h"
    "${VML_DIR}/include/uniform/*.h"
)

# Add the VML library with the collected header files
add_library(vml INTERFACE)
# Explicitly set the language for the VML target
set_target_properties(vml PROPERTIES LINKER_LANGUAGE CXX)
# Set the include directories for VML
target_include_directories(vml INTERFACE ${VML_DIR}/include)

set(dx12dir ${CMAKE_CURRENT_LIST_DIR}/directx)
add_library(dx12 INTERFACE)
target_include_directories(dx12 INTERFACE ${dx12dir})



# other
CPMAddPackage("gh:assimp/assimp@5.2.5")
CPMAddPackage("gh:ocornut/imgui#v1.90.9-docking")
CPMAddPackage("gh:gabime/spdlog@1.10.0")
CPMAddPackage("gh:microsoft/DirectXTK12#sep2024")

set(imgui_SOURCE_DIR ${imgui_SOURCE_DIR} CACHE INTERNAL "")
add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp
					     ${imgui_SOURCE_DIR}/imgui_demo.cpp
					     ${imgui_SOURCE_DIR}/imgui_draw.cpp
						 ${imgui_SOURCE_DIR}/imgui_tables.cpp
					     ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
						 
set_target_properties(stb_image 
                      assimp 
                      imgui
		      vml
		      dx12
              DirectXTK12
                      spdlog PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()