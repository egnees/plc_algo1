#pragma once

#include "TaskSolver.h"

class newTaskSolver : public TaskSolver {
public:
    Params get_params() override;
    Params estimate() override;
    Params solve() override;

    void init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) override;

    void config_defaults(int rows, int cols, int time);

private:
    int rows;
    int cols;
    int step_x;
    int step_y;

    int time;
    int seed;
    int tabu_tenure;
    int n1;
    int n2;
    int S;
    int z;
    int defaults;

    const int DEFAULT_TIME{1};
    const int DEFAULT_SEED{-1};
    const int DEFAULT_STEP_X{70};
    const int DEFAULT_STEP_Y{70};
    const int DEFAULT_TABU_TENURE{1};
    const int DEFAULT_N1{2};
    const int DEFAULT_N2{7};
    const int DEFAULT_S{100};
    const int DEFAULT_Z{10};
    const int DEFAULT_DEFAULTS{1};

    const std::string time_name{"time"};
    const std::string seed_name{"seed"};
    const std::string tabu_tenure_name{"tabu_tenure"};
    const std::string n1_name{"n1"};
    const std::string n2_name{"n2"};
    const std::string S_name{"S"};
    const std::string z_name{"z"};
    const std::string defaults_name{"defaults"};
};