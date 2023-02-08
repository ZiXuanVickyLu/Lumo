set(TINYEXPR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/TinyExpr)
set(TINYEXPR_LIBRARIES TinyExpr)

file(GLOB TINYEXPR_SOURCES ${TINYEXPR_INCLUDE_DIR}/tinyexpr.c)
file(GLOB TINYEXPR_HEADERS ${TINYEXPR_INCLUDE_DIR}/tinyexpr.h)
                 
add_library(${TINYEXPR_LIBRARIES} STATIC ${TINYEXPR_SOURCES} ${TINYEXPR_HEADERS})
    
set_target_properties(${TINYEXPR_LIBRARIES} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TINYEXPR_LIBRARIES} PROPERTIES FOLDER Lumo_Libs)

message(STATUS "TINYEXPR_INCLUDE_DIR : ${TINYEXPR_INCLUDE_DIR}")
message(STATUS "TINYEXPR_LIBRARIES : ${TINYEXPR_LIBRARIES}")