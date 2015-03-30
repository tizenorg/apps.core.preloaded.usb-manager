CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(usb-server C)

SET(SRCS
	src/um_common.c
	src/um_customize.c
	src/um_main.c
	src/um_usb_uevent_handler.c
	src/um_usb_accessory_manager.c
	src/um_usb_connection_manager.c
	src/um_usb_host_manager.c
	src/um_usb_notification.c
	src/um_usb_server.c)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(PACKAGE ${PROJECT_NAME})
SET(BINDIR "${PREFIX}/bin")
SET(DATADIR "${PREFIX}/share/${PACKAGE}/data")
SET(LOCALEDIR "${PREFIX}/share/locale")
SET(SYSCONFDIR ${SYS_CONF_DIR})

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	appcore-common
	ecore
	vconf
	dlog
	devman
	pmapi
	appsvc
	libudev
	libusb-1.0
	notification
	capi-system-usb-accessory
	libtzplatform-config
	)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -g -Wall")
MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")

IF("${SIMULATOR}" STREQUAL "yes")
	ADD_DEFINITIONS("-DSIMULATOR")
ENDIF()

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-ldl")

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/usb_icon.png DESTINATION ${DATADIR})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/start_dr.sh DESTINATION ${BINDIR})


ADD_SUBDIRECTORY(po)
