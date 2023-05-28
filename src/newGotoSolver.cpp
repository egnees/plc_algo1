
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 28.05.2023.
//

#include "newGotoSolver.h"

#include "../algo/new_goto.h"

#include <vector>

using namespace NewGoto;

Params newGotoTaskSolver::get_params() {
    return {
            {rows_name, "", false},
            {cols_name, "", false},
            {seed_name, std::to_string(DEFAULT_SEED), true},
            {time_name, std::to_string(DEFAULT_TIME), true},
            {step_x_name, std::to_string(DEFAULT_STEP_X), true},
            {step_y_name, std::to_string(DEFAULT_STEP_Y), true},
            {n1_name, std::to_string(DEFAULT_N1), true},
            {n2_name, std::to_string(DEFAULT_N2), true},
            {S_name, std::to_string(DEFAULT_S), true},
            {z_name, std::to_string(DEFAULT_Z), true},
            {lambda_name, std::to_string(DEFAULT_LAMBDA), true},
            {eps_name, std::to_string(DEFAULT_EPS), true},
            {debug_t_name, std::to_string(DEFAULT_DEBUG_T), true},
            {defaults_name, std::to_string(DEFAULT_DEFAULTS), true}
    };
}

Params newGotoTaskSolver::estimate() {
    return {
        {expect_time, my_round(time, 3) + " sec", false}
    };
}

void newGotoTaskSolver::init(const py::str &input_path, const py::str &output_path, const py::kwargs &kwargs) {
    init_layout(input_path.cast<std::string>());
    output_layout_path = output_path.cast<std::string>();

    get_value_nodef(kwargs, rows_name, rows);
    get_value_nodef(kwargs, cols_name, cols);

    get_value(kwargs, step_x_name, step_x, DEFAULT_STEP_X);
    get_value(kwargs, step_y_name, step_y, DEFAULT_STEP_Y);

    get_value_double(kwargs, time_name, time, DEFAULT_TIME);

    get_value(kwargs, n1_name, n1, DEFAULT_N1);
    get_value(kwargs, n2_name, n2, DEFAULT_N2);
    get_value(kwargs, S_name, S, DEFAULT_S);
    get_value(kwargs, z_name, z, DEFAULT_Z);

    get_value(kwargs, lambda_name, lambda, DEFAULT_LAMBDA);
    get_value(kwargs, eps_name, eps, DEFAULT_EPS);

    get_value(kwargs, seed_name, seed, DEFAULT_SEED);

    get_value_double(kwargs, debug_t_name, debug_t, DEFAULT_DEBUG_T);

    get_value(kwargs, defaults_name, defaults, DEFAULT_DEFAULTS);
}

Params newGotoTaskSolver::solve() {

    pin_acc_t left(device_count, std::vector<ans_t>(device_count, 0ll));
    pin_acc_t same_x(device_count, std::vector<ans_t>(device_count, 0ll));

    pin_acc_t up(device_count, std::vector<ans_t>(device_count, 0ll));
    pin_acc_t same_y(device_count, std::vector<ans_t>(device_count, 0ll));

    mul_t mul(device_count, std::vector<ans_t>(device_count, 0ll));

    long long LCM = 1;
    const int maxLCM = 1e9;

    for (int n_id = 0; n_id < net_count; ++n_id) {
        int size = (int) nets[n_id].pins.size();
        if (size <= 1) {
            continue;
        }
        LCM = 1ll * LCM * (size - 1) / gcd(LCM, 1ll * size - 1);
        if (LCM > maxLCM) {
            throw std::runtime_error("Too big nets");
        }
    }

    for (int n_id = 0; n_id < net_count; ++n_id) {
        int size = (int) nets[n_id].pins.size();
        long long coef = LCM / (size - 1);
        auto pins = nets[n_id].pins;
        for (int i = 0; i < (int) pins.size(); ++i) {
            Device* da = pins[i]->assigned_device;
            for (int j = 0; j < (int) pins.size(); ++j) {
                Device* db = pins[j]->assigned_device;
                if (da->id == db->id) {
                    continue;
                }

                mul[da->id][db->id] += coef;

                same_x[da->id][db->id] += coef * abs(pins[i]->relative.x - pins[j]->relative.x);
                same_y[da->id][db->id] += coef * abs(pins[i]->relative.y - pins[j]->relative.y);

                left[da->id][db->id] += coef * (-pins[i]->relative.x + pins[j]->relative.x);
                up[da->id][db->id] += coef * (pins[i]->relative.y - pins[j]->relative.y);
            }
        }
    }

    puts("newGotoSolver::inited");

    NewGotoHeurist solver(rows, cols, step_x, step_y, left, same_x, up, same_y, mul);
    if (defaults) {
        config_defaults();
    }

    puts("newGotoSolver::constructed");

    auto start = clock();

    auto perm = solver.solve(n1, n2, S, z, lambda, eps, time, debug_t, seed);

    double cpu_time = static_cast<double>(clock() - start) / 1e6;

    puts("newGotoSolver::solved");

    Point offset{screen_width/2 - (cols - 1) * step_x / 2, screen_height/2 - (rows - 1) * step_y / 2};

    for (int device = 0; device < device_count; ++device) {
        devices[device].center = Point{(perm[device] % cols) * step_x + offset.x,
                                       (perm[device] / cols) * step_y + offset.y};
    }

    write_layout(output_layout_path);

    puts("newGotoSolver::wrote layout");

    auto debug_info = solver.get_debug_info();

    Params params{
            {CPU_time, my_round(cpu_time, 3) + " sec", false},
            {TWL_manhattan, my_round(calc_metric(calc_manhattan)), false},
            {TWL_HP, my_round(calc_metric(calc_half_p)), false},
            {TWL_clique, my_round(calc_metric(calc_clique)), false},
            {TWL_hybrid, my_round(calc_metric(calc_hybrid)), false},
    };

    for (auto [t, perm] : debug_info) {
        for (int device = 0; device < device_count; ++device) {
            devices[device].center = Point{(perm[device] % cols) * step_x,
                                           (perm[device] / cols) * step_y};
        }
        params.push_back(Param{"di_" + std::to_string(t), std::to_string(calc_metric(calc_manhattan)), false});
    }

    puts("newGotoTaskSolver::constructed");

    return params;
}

void newGotoTaskSolver::config_defaults() {
    lambda = 4;
    eps = 4;
}
