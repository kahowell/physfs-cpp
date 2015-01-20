cmake_minimum_required(VERSION 2.6)
project(PhysFS++)

OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" OFF)

find_package(PhysFS)

set(LIB_TYPE STATIC)
if(BUILD_SHARED_LIBS)
	set(LIB_TYPE SHARED)
endif()

add_subdirectory(src)