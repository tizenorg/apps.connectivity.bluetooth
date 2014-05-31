CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(bluetooth C)

SET(SRCS
	src/bt-main.c
	src/bt-main-view.c
	src/bt-handler.c
	src/bt-popup.c
	src/bt-util.c
	src/bt-dbus-method.c
	)

SET(VENDOR "tizen")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "org.${VENDOR}.${PACKAGE}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(INSTALL_DIR_APPS "/usr/apps/${PKGNAME}")
SET(INSTALL_DIR_PRE "/usr")

SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(DATADIR "${PREFIX}/data")
SET(ICONDIR "${RESDIR}/icons")
SET(DESKTOPICONDIR "${ICONDIR}/default/small")
SET(EDJDIR "${RESDIR}/edje")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

SET(GC_SECTIONS_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GC_SECTIONS_FLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GC_SECTIONS_FLAGS}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	elementary
	dlog
	evas
	edje
	vconf
	ecore-x
	ecore
	ecore-input
	capi-appfw-application
	efl-assist
	appcore-efl
	capi-network-bluetooth
	syspopup-caller
	dbus-glib-1
	utilX
	deviced
)

FIND_LIBRARY(LIB_M m)

ADD_DEFINITIONS(${pkgs_CFLAGS})
ADD_DEFINITIONS("-fpie")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")
ADD_DEFINITIONS("-DSLP_DEBUG")
ADD_DEFINITIONS("-DSLP_PROF")

ADD_DEFINITIONS("-DVENDOR=\"${VENDOR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${PKGNAME}\"")
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")

ADD_DEFINITIONS("-DINSTALL_DIR_APPS=\"${INSTALL_DIR_APPS}\"")
ADD_DEFINITIONS("-DBINDIR=\"${BINDIR}\"")
ADD_DEFINITIONS("-DICONDIR=\"${ICONDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-pie" ${LIB_M})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})

# install application HOME directory
INSTALL(DIRECTORY DESTINATION ${DATADIR})

# desktop icon
#INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.bluetooth.png DESTINATION /usr/share/icons/default/small)
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.bluetooth.xml DESTINATION /usr/share/packages)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.bluetooth.rule DESTINATION  /etc/smack/accesses2.d/)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/images/ DESTINATION /usr/apps/org.tizen.bluetooth/res/images)
