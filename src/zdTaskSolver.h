
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 09.05.2023.
//

#ifndef PYBIND11_ALGO_ZDTASKSOLVER_H
#define PYBIND11_ALGO_ZDTASKSOLVER_H

#include "TaskSolver.h"

class zdTaskSolver : public TaskSolver {
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

    int iters;
    int k;
    int time;
    int seed;

    const int DEFAULT_ITERS{2000};
    const int DEFAULT_K{2};
    const int DEFAULT_TIME{1};
    const int DEFAULT_SEED{-1};
    const int DEFAULT_STEP_X{70};
    const int DEFAULT_STEP_Y{70};

    const std::string iters_name{"iters"};
    const std::string k_name{"k"};
    const std::string time_name{"time"};
    const std::string seed_name{"seed"};
};


#endif //PYBIND11_ALGO_ZDTASKSOLVER_H
