# This module defines
# Cairo_FOUND
# Cairo_INCLUDE_DIR
# Cairo_LIBRARY_DIR
# Cairo_LIBS

find_path(Cairo_INCLUDE_DIR cairo.h
    /usr/local/include/cairo
    /usr/include/cairo 
    /sw/include/cairo 
    /opt/local/include/cairo 
    /opt/csw/include/cairo 
    /opt/include/cairo
)

find_path(Cairo_LIBRARY_DIR
	NAMES
	libcairo.so
	PATHS
	/usr/local/lib
	/usr/lib
	/usr/lib/i386-linux-gnu
	/usr/lib/arm-linux-gnueabihf
	/sw/lib
	/opt/local/lib
	/opt/csw/lib
	/opt/lib
	)

set(Cairo_FOUND FALSE )
if(Cairo_LIBRARY_DIR AND Cairo_INCLUDE_DIR )
	set(Cairo_FOUND TRUE )
	set(Cairo_LIBS cairo )
endif(Cairo_LIBRARY_DIR AND Cairo_INCLUDE_DIR )


if(Cairo_FOUND)
	message(STATUS " Lib: ${Cairo_LIBS}")
	message(STATUS " - Includes: ${Cairo_INCLUDE_DIR}")
	message(STATUS " - Libraries: ${Cairo_LIBRARY_DIR}")
else(Cairo_FOUND)
	message(FATAL_ERROR "Could not find cairo installation.")
endif(Cairo_FOUND)
