
SET(XXT_SEARCH_PATHS 
	${CMAKE_SOURCE_DIR}/ThirdParty/gsmpi-1.2/
	${CMAKE_SOURCE_DIR}/ThirdParty/gsmpi-1.2/build/
	${CMAKE_SOURCE_DIR}/../ThirdParty/gsmpi-1.2/
	${CMAKE_SOURCE_DIR}/../ThirdParty/gsmpi-1.2/build 
    ${CMAKE_SOURCE_DIR}/ThirdParty/dist/lib 
    ${CMAKE_SOURCE_DIR}/../ThirdParty/dist/lib)

FIND_LIBRARY(XXT_LIBRARY NAMES xxt PATHS ${XXT_SEARCH_PATHS})

SET(XXT_FOUND FALSE)
IF (XXT_LIBRARY)
  SET(XXT_FOUND TRUE)
  MARK_AS_ADVANCED(XXT_LIBRARY)
ENDIF (XXT_LIBRARY)

IF (XXT_FOUND)
  IF (NOT XXT_FIND_QUIETLY)
     MESSAGE(STATUS "Found XXT")
  ENDIF (NOT XXT_FIND_QUIETLY)
ELSE(XXT_FOUND)
  IF (XXT_FIND_REQUIRED)
     MESSAGE(FATAL_ERROR "Could not find XXT")
  ENDIF (XXT_FIND_REQUIRED)
ENDIF (XXT_FOUND)
