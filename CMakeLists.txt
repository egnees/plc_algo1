cmake_minimum_required(VERSION 3.22)
project(pybind11_algo)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(pybind11)
pybind11_add_module(placer src/module.cpp src/defs.h src/TaskSolver.cpp
        src/TaskSolver.h src/impl.cpp src/IdleTaskSolver.cpp src/IdleTaskSolver.h
        src/bfTaskSolver.cpp src/bfTaskSolver.h src/zdTaskSolver.cpp src/zdTaskSolver.h
        algo/ZD_heurist_QAP1.cpp algo/ZD_heurist_QAP1.h
        algo/new_heurist_QAP.h algo/new_heurist_QAP.cpp
        src/LayoutGenerator.h src/LayoutGenerator.cpp
        src/newTaskSolver.h src/newTaskSolver.cpp
        src/dpTaskSolver.h src/dpTaskSolver.cpp
        algo/dp.cpp algo/dp.h
        algo/goto.h algo/goto.cpp)

add_subdirectory(algo)

# add_executable(test_impl src/test_impl.cpp src/impl.cpp src/defs.h)

target_compile_definitions(placer
        PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})