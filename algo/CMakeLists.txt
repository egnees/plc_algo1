project(algo)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pthread")

add_executable(zd_heurist zd_heurist.cpp ZD_heurist_QAP.cpp ZD_heurist_QAP1.cpp)
add_executable(test_new_heurist test_new_heurist.cpp new_heurist_QAP.h new_heurist_QAP.cpp ZD_heurist_QAP1.cpp)
add_executable(test_dp test_dp.cpp dp.h dp.cpp)