enable_testing()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

add_executable(run_tests
        fake_data.cpp
        fake_data.h
        fs_utils.cpp
        fs_utils.h
        performance_test.cpp
        run_and_measure.h
        svpng.inc
        )

