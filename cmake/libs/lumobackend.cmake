set(COMMON_INCLUDE_DIR)
set(LUMO_BACKEND_LIBRARIES)
set(LUMO_BACKEND_LIB_DIR)

set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "")

add_subdirectory(${CMAKE_SOURCE_DIR}/libs/LumoBackend)

set_target_properties(LumoBackend PROPERTIES FOLDER Libs/Shared)

set_target_properties(LumoBackend PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
set_target_properties(LumoBackend PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(LumoBackend PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")


