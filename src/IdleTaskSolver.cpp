
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#include "IdleTaskSolver.h"

Params IdleTaskSolver::get_params() {
    return {};
}

Params IdleTaskSolver::estimate() {
    return {
        {expect_time, "0 sec", false}
    };
}

Params IdleTaskSolver::solve() {
    auto start = clock();
    write_layout(output_layout_path);
    double cpu_time = static_cast<double>(clock() - start) / 1e6;
    return {
        {CPU_time, my_round(cpu_time, 3) + " sec", false},
        {TWL_manhattan, my_round(calc_metric(calc_manhattan)), false},
        {TWL_HP, my_round(calc_metric(calc_half_p)), false},
        {TWL_clique, my_round(calc_metric(calc_clique)), false},
        {TWL_hybrid, my_round(calc_metric(calc_hybrid)), false},
    };
}

void IdleTaskSolver::init(const py::str &input_path, const py::str &output_path, const py::kwargs &kwargs) {
    init_layout(input_path.cast<std::string>());
    output_layout_path = output_path.cast<std::string>();
}
