cmake_minimum_required( VERSION 2.8 )

project( TritonCTS )

#####################################################################
# Define build type
#####################################################################
if( NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "" )
	set( CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE )
endif()

message( STATUS "Build type ${CMAKE_BUILD_TYPE}" )

#####################################################################
# Special handling for Lemon library
#####################################################################
if( NOT LEMON_HOME OR LEMON_HOME STREQUAL "" )
	message( FATAL_ERROR "LEMON_HOME not specified" )
endif()

message( STATUS "Lemon home: ${LEMON_HOME}" )

set( LEMON_INCLUDE "${LEMON_HOME}/include" )
set( LEMON_LIB "${LEMON_HOME}/lib" )

include_directories( ${LEMON_INCLUDE} )
link_directories( ${LEMON_LIB} )

#####################################################################
# OpenMP handling
#####################################################################
find_package(OpenMP)
if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

#####################################################################
# Define source files
#####################################################################
file( GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/ghtree/*.cpp )

####################################################################
# Define executable
#####################################################################
add_executable( tritoncts ${SRC_FILES} )
target_link_libraries( tritoncts emon )
