ADD_EXECUTABLE(
    QPPlacerTest
    qplace_test.cpp
    qplace.cpp
    ${PROJECT_SOURCE_DIR}/Parser/parser.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/Plot/plot.cpp
)
INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/Parser
	${PROJECT_SOURCE_DIR}/PlaceCommon
	${PROJECT_SOURCE_DIR}/PlaceDB
    ${PROJECT_SOURCE_DIR}/Plot
    ${PROJECT_SOURCE_DIR}/Library/eigen-git-mirror
    ${PROJECT_SOURCE_DIR}/Library/CImg
    )

target_link_libraries(QPPlacerTest "-lgomp -lpthread -lm -ldl -lX11")