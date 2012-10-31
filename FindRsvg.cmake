# This module defines
# Rsvg_FOUND
# Rsvg_INCLUDE_DIR
# Rsvg_LIBRARY_DIR
# Rsvg_LIBS

find_path(Rsvg_INCLUDE_DIR librsvg/rsvg.h
    /usr/local/include/librsvg-2.0
    /usr/include/librsvg-2.0 
    /sw/include/librsvg-2.0 
    /opt/local/include/librsvg-2.0 
    /opt/csw/include/librsvg-2.0 
    /opt/include/librsvg-2.0
)

find_path(Rsvg_LIBRARY_DIR
	NAMES
	librsvg-2.so
	PATHS
	/usr/local/lib
	/usr/lib
	/usr/lib/i386-linux-gnu
	/usr/lib/x86_64-linux-gnu
	/usr/lib/arm-linux-gnueabihf
	/usr/lib/arm-linux-gnueabi
	/sw/lib
	/opt/local/lib
	/opt/csw/lib
	/opt/lib
	)

set(Rsvg_FOUND FALSE )
if(Rsvg_LIBRARY_DIR AND Rsvg_INCLUDE_DIR )
	set(Rsvg_FOUND TRUE )
	set(Rsvg_LIBS rsvg-2  )
endif(Rsvg_LIBRARY_DIR AND Rsvg_INCLUDE_DIR )


if(Rsvg_FOUND)
	message(STATUS " Lib: ${Rsvg_LIBS}")
	message(STATUS " - Includes: ${Rsvg_INCLUDE_DIR}")
	message(STATUS " - Libraries: ${Rsvg_LIBRARY_DIR}")
else(Rsvg_FOUND)
	message(FATAL_ERROR "Could not find rsvg installation.")
endif(Rsvg_FOUND)
