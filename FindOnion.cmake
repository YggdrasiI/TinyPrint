# Search libfreenect header and libary
find_path(Onion_INCLUDE_DIR
	NAMES
		onion/onion.h
		onion/log.h
		onion/extras/png.h
	PATHS
		/usr/include
		/usr/local/include
		/opt/local/include
		./lib/onion/include
	PATH_SUFFIXES
		libonion
)

find_path(Onion_LIBRARY
	NAMES
		libonion.so
	PATHS
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/opt/local/lib
		./lib/onion/lib
)

find_path(Onion_BIN_DIR
	NAMES
		otemplate
	PATHS
		/usr/bin
		/usr/local/bin
		/opt/local/bin
		./lib/onion/bin
)

if(Onion_BIN_DIR)
	set(OTEMPLATE ${Onion_BIN_DIR}/otemplate)
endif(Onion_BIN_DIR)

if(Onion_INCLUDE_DIR AND Onion_LIBRARY AND Onion_BIN_DIR)
	set(Onion_FOUND TRUE)
	set(Onion_LIBS onion onion_extras)
else(Onion_INCLUDE_DIR AND Onion_LIBRARY AND Onion_BIN_DIR)
	set(Onion_FOUND FALSE)
endif(Onion_INCLUDE_DIR AND Onion_LIBRARY AND Onion_BIN_DIR)

if(Onion_FOUND)
	message(STATUS " Lib: ${Onion_LIBS}")
	message(STATUS " - Includes: ${Onion_INCLUDE_DIR}")
	message(STATUS " - Libraries: ${Onion_LIBRARY}")
	message(STATUS " - Binaries: ${Onion_BIN_DIR}")
else(Onion_FOUND)
	message(FATAL_ERROR "Could not find onion installation.")
endif(Onion_FOUND)


