set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "")
set(USE_STATIC_CRT OFF CACHE BOOL "" FORCE)

set(ZLIB_INCLUDE_DIRS 
	${CMAKE_SOURCE_DIR}/3rdparty/zlib 
	${CMAKE_CURRENT_BINARY_DIR}/3rdparty/zlib
)
set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIRS})

set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
##EXCLUDE_FROM_ALL reject install for this target
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/zlib EXCLUDE_FROM_ALL)
set(ZLIB_LIBRARIES zlib)
set(ZLIB::ZLIB zlib)
set_target_properties(zlib PROPERTIES FOLDER 3rdparty/Shared)
set_target_properties(zlib PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
set_target_properties(zlib PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(zlib PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(zlib PROPERTIES SUFFIX ".dll")

set(ZLIB_FOUND TRUE)
