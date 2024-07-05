# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	 *.c
	 *.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
	 *.h
	 *.hpp)

# Define the executable	 
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_source_files_properties(${ASSETS_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)



target_compile_definitions(${PROJECT_NAME} PRIVATE LIBRARY_SUFFIX="")

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
												  ${stb_image_SOURCE_DIR}
												  ${imgui_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} stb_image)
target_link_libraries(${PROJECT_NAME} assimp)
target_link_libraries(${PROJECT_NAME} imgui)
target_link_libraries(${PROJECT_NAME} spdlog)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
				   COMMAND ${CMAKE_COMMAND} -E create_symlink 
				   ${CMAKE_SOURCE_DIR}/res 
				   ${CMAKE_CURRENT_BINARY_DIR}/res)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
endif()
