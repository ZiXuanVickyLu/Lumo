
if(USE_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
	set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "")
else()
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(LLVM_USE_CRT_DEBUG MTd CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_MINSIZEREL MT CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELEASE MT CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELWITHDEBINFO MT CACHE STRING "" FORCE)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "")
endif()

if (BUILD_SHARED_LIBS)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rtmidi EXCLUDE_FROM_ALL)
	target_compile_definitions(rtmidi INTERFACE BUILD_SHARED_LIBS)
	set_target_properties(rtmidi PROPERTIES POSITION_INDEPENDENT_CODE ON)
	set_target_properties(rtmidi PROPERTIES DEFINE_SYMBOL "RTMIDI_EXPORT")
else()
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rtmidi EXCLUDE_FROM_ALL)
endif()

set_target_properties(rtmidi PROPERTIES RUNTIME_OUTPUT_NAME_DEBUG "rtmidid")

if(USE_SHARED_LIBS)
	set_target_properties(rtmidi PROPERTIES FOLDER 3rdparty/Shared)
else()
	set_target_properties(rtmidi PROPERTIES FOLDER 3rdparty/Static)
endif()

set_target_properties(rtmidi PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
set_target_properties(rtmidi PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(rtmidi PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")

set(RTMIDI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/rtmidi/include)
set(RTMIDI_LIBRARIES rtmidi)
set(RTMIDI_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR})
