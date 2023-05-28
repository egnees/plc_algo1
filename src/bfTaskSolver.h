
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#ifndef PYBIND11_ALGO_BFTASKSOLVER_H
#define PYBIND11_ALGO_BFTASKSOLVER_H

#include "TaskSolver.h"

class bfTaskSolver : public TaskSolver {
public:
    Params get_params() override;
    Params estimate() override;
    Params solve() override;

    void init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) override;
private:
    int rows;
    int cols;
    int step_x;
    int step_y;

    [[nodiscard]] double calc_twl() const;
};


#endif //PYBIND11_ALGO_BFTASKSOLVER_H
