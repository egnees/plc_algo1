project(algo)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pthread")

add_executable(zd_heurist zd_heurist.cpp ZD_heurist_QAP.cpp ZD_heurist_QAP1.cpp)
add_executable(test_new_heurist test_new_heurist.cpp new_heurist_QAP.h new_heurist_QAP.cpp ZD_heurist_QAP1.cpp)
add_executable(test_dp test_dp.cpp dp.h dp.cpp)
add_executable(test_goto goto.h goto.cpp test_goto.cpp)
add_executable(test_new_goto.cpp new_goto new_goto.h test_new_goto.cpp new_goto.cpp new_goto.h)
add_executable(test_zd_heurist_2 zd_heurist_2.cpp test_zd_heurist_2.cpp ZD_heurist_QAP1.cpp)