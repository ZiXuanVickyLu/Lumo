set(MESHER_INCLUDE_DIR)
set(MESHER_LIBRARIES)
set(MESHER_LIB_DIR)

add_subdirectory(${CMAKE_SOURCE_DIR}/libs/Mesher)
set_target_properties(${MESHER_LIBRARIES} PROPERTIES FOLDER Lumo_Libs)
##set_target_properties(${VKPROFILER_LIBRARIES} PROPERTIES LINK_FLAGS "/ignore:4244")

message(STATUS "MESHER_INCLUDE_DIR : ${MESHER_INCLUDE_DIR}")
message(STATUS "MESHER_LIBRARIES : ${MESHER_LIBRARIES}")
message(STATUS "MESHER_LIB_DIR : ${MESHER_LIB_DIR}")


