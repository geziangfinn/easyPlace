CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (ePlace)
SET(CMAKE_C_FLAGS "-O3 -Wall -fomit-frame-pointer -funroll-loops -ffast-math -funit-at-a-time -fPIC")
SET(CMAKE_CXX_FLAGS "-O3 -Wall -fomit-frame-pointer -funroll-loops -ffast-math -funit-at-a-time -fPIC -DNDEBUG")
SET(CMAKE_EXE_LINKER_FLAGS "-static")
add_executable(
    ParserTest 
    Parser/parser_test.cpp
    Parser/parser.cpp
    PlaceCommon/arghandler.cpp
    PlaceCommon/objects.cpp
    PlaceDB/placedb.cpp
    PlaceCommon/global.h
)
INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/Parser
	${PROJECT_SOURCE_DIR}/PlaceCommon
	${PROJECT_SOURCE_DIR}/PlaceDB
)