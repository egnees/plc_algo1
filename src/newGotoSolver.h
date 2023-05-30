
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 28.05.2023.
//

#ifndef PYBIND11_ALGO_NEWGOTOSOLVER_H
#define PYBIND11_ALGO_NEWGOTOSOLVER_H

#include "TaskSolver.h"

#include "../algo/new_goto.h"
#include "../algo/zd_heurist_2.h"

class newGotoTaskSolver : public TaskSolver {
public:
    Params get_params() override;
    Params estimate() override;
    Params solve() override;

    void init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) override;

    void config_defaults();

    std::vector<int> do_local_upd(const std::vector<int>& perm,
                                  const NewGoto::pin_acc_t& left, const NewGoto::pin_acc_t& same_x,
                                  const NewGoto::pin_acc_t& up, const NewGoto::pin_acc_t& same_y,
                                  const NewGoto::mul_t& mul,
                                  int a = 4, int b = 4) const;

private:
    int rows;
    int cols;
    int step_x;
    int step_y;

    double time;
    int seed;
    int n1;
    int n2;
    int S;
    int z;
    int lambda;
    int eps;
    int defaults;
    int local_upd;

    const int DEFAULT_TIME{1};
    const int DEFAULT_SEED{-1};
    const int DEFAULT_STEP_X{70};
    const int DEFAULT_STEP_Y{70};
    const int DEFAULT_N1{2};
    const int DEFAULT_N2{7};
    const int DEFAULT_S{100};
    const int DEFAULT_Z{10};
    const int DEFAULT_DEFAULTS{1};
    const int DEFAULT_LAMBDA{4};
    const int DEFAULT_EPS{4};
    const int DEFAULT_LOCAL_UPD{0};

    const std::string time_name{"time"};
    const std::string seed_name{"seed"};
    const std::string n1_name{"n1"};
    const std::string n2_name{"n2"};
    const std::string S_name{"S"};
    const std::string z_name{"z"};
    const std::string lambda_name{"lambda"};
    const std::string eps_name{"eps"};
    const std::string defaults_name{"defaults"};
    const std::string local_upd_name{"local_upd"};
};

#endif //PYBIND11_ALGO_NEWGOTOSOLVER_H
