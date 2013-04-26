SET(THIRDPARTY_BUILD_METIS ON CACHE BOOL
    "Build ModMetis library from ThirdParty")

IF (THIRDPARTY_BUILD_METIS)
    INCLUDE(ExternalProject)
    EXTERNALPROJECT_ADD(
        modmetis-5.1.0
        PREFIX ${TPSRC}
        URL http://xyloid.org/metis-5.1.0.tar.gz
        URL_MD5 "c48620f7b5b1cebebc00ccbf73fca5d0"
        DOWNLOAD_DIR ${TPSRC}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX:PATH=${TPSRC}/dist -DCMAKE_C_FLAGS:STRING=-fPIC -DGKLIB_PATH:PATH=${TPSRC}/src/modmetis-5.1.0/GKlib ${TPSRC}/src/modmetis-5.1.0
    )
    SET(METIS_LIB metis CACHE FILEPATH
        "METIS library" FORCE)
    MARK_AS_ADVANCED(METIS_LIB)
    LINK_DIRECTORIES(${TPSRC}/dist/lib)
    INCLUDE_DIRECTORIES(${TPSRC}/dist/include)
    MESSAGE(STATUS "Build Metis: ${TPSRC}/dist/lib/lib${METIS_LIB}.a")
ELSE (THIRDPARTY_BUILD_METIS)
    INCLUDE (FindMetis)
ENDIF (THIRDPARTY_BUILD_METIS)

