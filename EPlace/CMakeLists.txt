ADD_EXECUTABLE(
    ePlaceTest
    binDensity_test.cpp
    eplace.cpp
    ${PROJECT_SOURCE_DIR}/Parser/parser.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/Plot/plot.cpp
    ${PROJECT_SOURCE_DIR}/QPlace/qplace.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
)
INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/Parser
	${PROJECT_SOURCE_DIR}/PlaceCommon
	${PROJECT_SOURCE_DIR}/PlaceDB
    ${PROJECT_SOURCE_DIR}/Plot
    ${PROJECT_SOURCE_DIR}/Library/eigen-git-mirror
    ${PROJECT_SOURCE_DIR}/Library/CImg
    ${PROJECT_SOURCE_DIR}/QPlace
    ${PROJECT_SOURCE_DIR}/FFT
    )

target_link_libraries(ePlaceTest "-lgomp -lpthread -lm -ldl -lX11")