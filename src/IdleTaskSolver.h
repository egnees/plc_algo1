
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#ifndef PYBIND11_ALGO_IDLETASKSOLVER_H
#define PYBIND11_ALGO_IDLETASKSOLVER_H


#include "TaskSolver.h"

class IdleTaskSolver : public TaskSolver {
public:
    Params get_params() override;
    Params estimate() override;
    Params solve() override;

    void init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) override;
};


#endif //PYBIND11_ALGO_IDLETASKSOLVER_H
