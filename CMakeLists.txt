project(B9CreatorLinux)
cmake_minimum_required(VERSION 2.6)

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

## Copy html folder to build directory. (On configuation time)
file(COPY html/ DESTINATION html PATTERN b9creator_settings.js EXCLUDE )

macro(copy_files GLOBPAT DESTINATION)
  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLOBPAT})
  add_custom_target(copy ALL
    COMMENT "Copying files: ${GLOBPAT}")

  foreach(FILENAME ${COPY_FILES})
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
    set(DST "${DESTINATION}/${FILENAME}")

    add_custom_command(
      TARGET copy
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
      )
  endforeach(FILENAME)
endmacro(copy_files)
## end TODO

#add_dependencies(main copy_files "html/*.js" "${CMAKE_CURRENT_BINARY_DIR}/html" )

