set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_SHARED_LIBRARY ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/glfw)

set_target_properties(glfw PROPERTIES FOLDER 3rdparty/glfw) # Override standard 'GLFW3' subfolder
set_target_properties(update_mappings PROPERTIES FOLDER 3rdparty/glfw) # Override standard 'GLFW3' subfolder

set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/glfw/include)
set(GLFW_DEFINITIONS -DGLFW_INCLUDE_NONE)
set(GLFW_LIBRARIES ${GLFW_LIBRARIES} glfw)
