CMAKE_MINIMUM_REQUIRED(VERSION 2.4.8)

PROJECT(gydpdict)
SET(GYDP_VERSION "0.2.0")

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE(PkgConfig)

# dependency checks
CHECK_PKG_CONFIG_PACKAGE(glib-2.0 2.16)
CHECK_PKG_CONFIG_PACKAGE(gio-2.0 2.16)
CHECK_PKG_CONFIG_PACKAGE(gtk+-2.0 2.12)

# compilation flags
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${glib-2.0_CFLAGS} ${gio-2.0_CFLAGS} ${gtk+-2.0_CFLAGS}")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra")

# configuration file
SET(GYDP_CONFIG ${PROJECT_BINARY_DIR}/gydp_config.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/gydp_config.h.in "${GYDP_CONFIG}" ESCAPE_QUOTES)

# sources
ADD_EXECUTABLE(gydpdict src/main.c src/gydp_global.c src/gydp_util.c src/gydp_app.c src/gydp_conf.c
	src/gydp_window.c src/gydp_list_view.c src/gydp_list_data.c
	src/gydp_dict.c src/gydp_dict_ydp.c src/gydp_dict_sap.c
	src/gydp_convert_ydp.c src/gydp_convert_sap.c)

# linker and additional flags
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
TARGET_LINK_LIBRARIES(gydpdict ${glib-2.0_LIBS} ${gio-2.0_LIBS} ${gtk+-2.0_LIBS})
SET_TARGET_PROPERTIES(gydpdict PROPERTIES
	LINK_FLAGS "-Wl,-O1 -Wl,--as-needed"
	DEFINE_SYMBOL G_LOG_DOMAIN=\\"gydpdict\\"
)

# SAP dictionary files
FILE(GLOB GYDP_SAP dict/dvp_[12].dic)

# install files
INSTALL(TARGETS gydpdict RUNTIME DESTINATION bin)
INSTALL(FILES ${GYDP_SAP} DESTINATION share/gydpdict)
INSTALL(FILES gydpdict.desktop DESTINATION share/applications)

