project(B9CreatorLinux)
cmake_minimum_required(VERSION 2.6)
option(FLIP_COLORS "Flip byte order of colors in DirectFB images. (For Raspberry PI)" ON)

set(EXTRA_C_FLAGS "${PG_FLAGS}")
set(EXTRA_C_FLAGS_DEBUG "")
set(EXTRA_CXX_FLAGS "${PG_FLAGS}")
set(EXTRA_CXX_FLAGS_DEBUG "")

## required and optional packages
#set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_SOURCE_DIR})
#set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
find_package(OpenCV REQUIRED)
#find_package(DirectFB REQUIRED)#source?

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
find_package(Onion REQUIRED)
find_package(Cairo REQUIRED)
find_package(Rsvg REQUIRED)

set(Boost_USE_STATIC_LIBS        OFF)
#set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME    OFF)
#set(Boost_DEBUG TRUE)
find_package(Boost 
	COMPONENTS signals system filesystem iostreams regex
	REQUIRED)
#message(STATUS "Boost libs: ${Boost_LIBRARIES}" )

set(Directfb_LIBRARY_DIR /usr/lib )
set(Directfb_LIBS directfb )
set(Directfb_INCLUDE_DIR /usr/include/directfb )

find_package(GTK2 REQUIRED)
if(GTK2_FOUND )
message(STATUS " Lib: gtk-2.0" )
message(STATUS " - Includes: ${GTK2_INCLUDE_DIRS}" )
message(STATUS " - Libraries: ${GTK2_LIBRARIES}" )
else(GTK2_FOUND)
message(FATAL_ERROR "Could not find gtk2 installation.")
endif(GTK2_FOUND)

## end required and optional packages

## set definitions
if(FLIP_COLORS)
	add_definitions(-DFLIP_COLORS)
endif(FLIP_COLORS)
## end set definitions


## add include dirs and lib dirs of above packages
include_directories(
		${CMAKE_SOURCE_DIR}/include
	)
link_directories (
	${Onion_LIBRARY_DIR}
	${Boost_LIBRARY_DIR}
	${Directfb_LIBRARY_DIR}
	${Cairo_LIBRARY_DIR}
	${Rsvg_LIBRARY_DIR}
	) 
## end add include dis and lib dirs of above packages

add_subdirectory(lib)
add_subdirectory(src)

# Copy htlm folder content to build dir
add_custom_target(htmlfolder)
add_custom_command(
	TARGET htmlfolder
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/html ${CMAKE_CURRENT_BINARY_DIR}/html
	)

add_dependencies(main htmlfolder) 

