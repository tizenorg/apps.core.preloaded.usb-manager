CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(usb-server C)

SET(SRCS
	src/um_common.c
	src/um_customize.c
	src/um_main.c
	src/um_usb_accessory_manager.c
	src/um_usb_connection_manager.c
	src/um_usb_host_manager.c
	src/um_usb_server.c)
 
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	appcore-common
	ecore
	vconf
	heynoti
	dlog
	syspopup-caller
	devman
	pmapi
	appsvc
	libusb-1.0
	)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag} -g")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
IF("${SIMULATOR}" STREQUAL "yes")
	ADD_DEFINITIONS("-DSIMULATOR")
ENDIF()

SET(UDEV_RULES_PATH share/usb-server/udev-rules)
SET(UDEV_RULES udev-rules/91-usb-server.rules)

CONFIGURE_FILE(${UDEV_RULES}.in ${UDEV_RULES} @ONLY)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-ldl")

INSTALL(FILES ${UDEV_RULES} DESTINATION ${UDEV_RULES_PATH})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/set_usb_debug.sh DESTINATION bin)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/start_dr.sh DESTINATION bin)
ADD_SUBDIRECTORY(po)
