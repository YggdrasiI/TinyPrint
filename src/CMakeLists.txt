## extra include directories
include_directories(
	${CMAKE_SOURCE_DIR}/lib/cJSON
	${Onion_INCLUDE_DIR}
	${Cairo_INCLUDE_DIR}
	${Rsvg_INCLUDE_DIR}
	${GTK2_INCLUDE_DIRS}
	${Boost_INCLUDE_DIR}
	${Directfb_INCLUDE_DIR}
	)
## end extra include directories

## json files
set(json_SRCS
	${CMAKE_SOURCE_DIR}/lib/cJSON/cJSON.c
	JsonConfig.cpp	
	B9CreatorSettings.cpp
	)
## end json files

## http server
#add_custom_target(server)
set(server_SRCS
	OnionServer.cpp
	index_html.c
	b9creator_settings_js.c
	)

# create sourcefile from html template
add_custom_command(
	OUTPUT index_html.c
	#	COMMAND ${OPACK} ${CMAKE_SOURCE_DIR}/html/index.html -o ${CMAKE_CURRENT_BINARY_DIR}/index_html.c
	COMMAND ${OTEMPLATE} ${CMAKE_SOURCE_DIR}/html/index.html ${CMAKE_CURRENT_BINARY_DIR}/index_html.c
	DEPENDS ${CMAKE_SOURCE_DIR}/html/index.html
	)
add_custom_command(
	OUTPUT b9creator_settings_js.c
	COMMAND ${OTEMPLATE} ${CMAKE_SOURCE_DIR}/html/b9creator_settings.js ${CMAKE_CURRENT_BINARY_DIR}/b9creator_settings_js.c
	DEPENDS ${CMAKE_SOURCE_DIR}/html/b9creator_settings.js
	)

add_library (server SHARED ${server_SRCS})
## end http server


## main executable
set(main_SRCS
	main.cpp
	${json_SRCS}
	SerialManager.cpp
	DisplayManager.cpp
	JobManager.cpp
	JobFile.cpp
	JobFileSvg.cpp
	JobFileList.cpp
	JobFileB9J.cpp
	)

add_executable (main ${main_SRCS} )

# Link again this libs
target_link_libraries(main server 
	${Directfb_LIBS}
	${OpenCV_LIBS}
	${Onion_LIBS}
	${Cairo_LIBS}
	${Rsvg_LIBS}
	${GTK2_LIBARIES}
	${Boost_LIBRARIES}
	)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#g++ flags 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${EXTRA_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EXTRA_CXX_FLAGS_DEBUG}")

# Das Programm soll in den folgenden Unterordnern
# installiert werden.
install(TARGETS main 
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)
## end main executable

## Create job_files folder
# TODO: Does not work
set(JOB_FILES_CREATE_FOLDER test -d "${PROJECT_BINARY_DIR}/job_files" || mkdir "${PROJECT_BINARY_DIR}/job_files" )
add_custom_command(
	TARGET main
	COMMAND ${JOB_FILES_CREATE_FOLDER}
	COMMENT "Create default folder for job files."
	)
## End create job_files folder

## Link start script into build dir
set(LINK_SCRIPT_INTO_BIN test -f "${EXECUTABLE_OUTPUT_PATH}/start.sh" || ln -s "${CMAKE_SOURCE_DIR}/start.sh" "${EXECUTABLE_OUTPUT_PATH}/start.sh" )
add_custom_command(
	TARGET main
	COMMAND ${LINK_SCRIPT_INTO_BIN}
	COMMENT "Create symbolic link to start.sh."
	)

