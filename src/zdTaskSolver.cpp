
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 09.05.2023.
//

#include "zdTaskSolver.h"
#include "../algo/ZD_heurist_QAP1.h"

#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>

Params zdTaskSolver::get_params() {
    return {
            {rows_name, "", false},
            {cols_name, "", false},
            {seed_name, std::to_string(DEFAULT_SEED), true},
            {step_x_name, std::to_string(DEFAULT_STEP_X), true},
            {step_y_name, std::to_string(DEFAULT_STEP_Y), true},
            {time_name, std::to_string(DEFAULT_TIME), true},
            {iters_name, std::to_string(DEFAULT_ITERS), true},
            {k_name, std::to_string(DEFAULT_K), true},
            {debug_t_name, std::to_string(DEFAULT_DEBUG_T), true}
    };
}

Params zdTaskSolver::estimate() {
    return {
        {expect_time, std::to_string(time) + " sec", false}
    };
}

void zdTaskSolver::init(const py::str &input_path, const py::str &output_path, const py::kwargs &kwargs) {
    init_layout(input_path.cast<std::string>());
    output_layout_path = output_path.cast<std::string>();

    get_value_nodef(kwargs, rows_name, rows);
    get_value_nodef(kwargs, cols_name, cols);

    if (rows * cols != device_count) {
        // printf("Dev cnt not equals Loc cnt");
        throw std::runtime_error("Dev cnt not Loc cnt");
    }

    get_value(kwargs, step_x_name, step_x, DEFAULT_STEP_X);
    get_value(kwargs, step_y_name, step_y, DEFAULT_STEP_Y);
    get_value(kwargs, iters_name, iters, DEFAULT_ITERS);
    get_value(kwargs, time_name, time, DEFAULT_TIME);
    get_value(kwargs, k_name, k, DEFAULT_K);
    get_value(kwargs, seed_name, seed, DEFAULT_SEED);

    get_value_double(kwargs, debug_t_name, debug_t, DEFAULT_DEBUG_T);
}

Params zdTaskSolver::solve() {
    Point offset{screen_width/2 - (cols - 1) * step_x / 2, screen_height/2 - (rows - 1) * step_y / 2};

    std::vector<Point> locations;
    locations.reserve(device_count);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            locations.push_back({offset.x + j * step_x, offset.y + i * step_y});
        }
    }


    int n = device_count;

    using vl = std::vector<long long>;
    using vvl = std::vector<vl>;
    using vvvl = std::vector<vvl>;
    using vvvvl = std::vector<vvvl>;


    auto cost = vvvvl(n, vvvl(n, vvl(n, vl(n, 0))));

    long long LCM = 1;
    const int maxLCM = 1e9;

    for (int n_id = 0; n_id < net_count; ++n_id) {
        int size = nets[n_id].pins.size();
        if (size <= 1) {
            continue;
        }
        LCM = 1ll * LCM * (size - 1) / gcd(LCM, 1ll * size - 1);
        if (LCM > maxLCM) {
            throw std::runtime_error("Too big nets");
        }
    }

    for (int n_id = 0; n_id < net_count; ++n_id) {
        Net* cur_net = nets + n_id;
        auto cur_pin = cur_net->pins;
        int size = cur_pin.size();
        if (size <= 1) {
            continue;
        }
        int w = LCM / (size - 1);
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {

                if (i == j) {
                    continue;
                }

                Pin* a = cur_pin[i];
                Pin* b = cur_pin[j];
                if (a->assigned_device->id == b->assigned_device->id) {
                    continue;
                }

                int da = a->assigned_device->id;
                int db = b->assigned_device->id;
                Point old_cent_da = a->assigned_device->center;
                Point old_cent_db = b->assigned_device->center;

                for (int p1 = 0; p1 < (int) locations.size(); ++p1) {
                    a->assigned_device->center = locations[p1];
                    for (int p2 = 0; p2 < (int) locations.size(); ++p2) {
                        if (p1 == p2) {
                            continue;
                        }
                        b->assigned_device->center = locations[p2];

                        Point pos_a = a->absolute();
                        Point pos_b = b->absolute();

                        int dx = pos_a.x - pos_b.x;
                        int dy = pos_a.y - pos_b.y;

                        int dist = abs(dx) + abs(dy);

                        cost[da][db][p1][p2] += 1ll * w * dist;
                    }
                }

                a->assigned_device->center = old_cent_da;
                b->assigned_device->center = old_cent_db;
            }
        }
    }

    std::vector<int> best;
    long long best_twl = 1e18;

    ZD_heurist_QAP1 solver(cost, k);

    clock_t max_time = 1e6 * time;
    if (seed == -1) {
        std::mt19937 rnd{(uint32_t) std::chrono::high_resolution_clock().now().time_since_epoch().count()};
        seed = rnd();
    }
    std::mt19937 rnd{(uint32_t) seed};
    int debug_interval = -1;
    if (debug_t != 0) {
        debug_interval = 1e6 * debug_t;
    }
    auto start = clock();
    while ((clock() - start) <= max_time) {
        int rem = max_time - (clock() - start);
        auto cur = solver.solve(rem, rnd(), debug_interval, ((double)(clock() - start)) / 1e6);
        for (int j = 0; j < n; ++j) {
            devices[j].center = locations[cur[j]];
        }
        double cur_twl = calc_metric(calc_manhattan);
        if (cur_twl < best_twl) {
            best_twl = cur_twl;
            best = cur;
        }
    }

    if (best.empty()) {
        throw std::runtime_error("Not solved");
    }

    for (int i = 0; i < n; ++i) {
        devices[i].center = locations[best[i]];
    }

    write_layout(output_layout_path);

    double cpu_time = static_cast<double>(clock() - start) / 1e6;

    Params params{
            {CPU_time, my_round(cpu_time, 3) + " sec", false},
            {TWL_manhattan, my_round(calc_metric(calc_manhattan)), false},
            {TWL_HP, my_round(calc_metric(calc_half_p)), false},
            {TWL_clique, my_round(calc_metric(calc_clique)), false},
            {TWL_hybrid, my_round(calc_metric(calc_hybrid)), false},
    };

    auto debug_info = solver.get_debug_info();

    double last_twl;
    bool first{true};

    for (auto [t, perm] : debug_info) {
        for (int i = 0; i < n; ++i) {
            devices[i].center = locations[perm[i]];
        }
        if (first) {
            last_twl = calc_metric(calc_manhattan);
            first = false;
        } else {
            double cur_twl = calc_metric(calc_manhattan);
            if (cur_twl < last_twl) {
                last_twl = cur_twl;
            }
        }
        params.push_back(Param{"di_" + std::to_string(t), std::to_string(last_twl), false});
    }

    return params;
}

