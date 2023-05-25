#include "newTaskSolver.h"

#include "../algo/new_heurist_QAP.h"

#include <random>
#include <chrono>

Params newTaskSolver::get_params() {
    return {
        {rows_name, "", false},
        {cols_name, "", false},
        {seed_name, std::to_string(DEFAULT_SEED), true},
        {time_name, std::to_string(DEFAULT_TIME), true},
        {step_x_name, std::to_string(DEFAULT_STEP_X), true},
        {step_y_name, std::to_string(DEFAULT_STEP_Y), true},
        {tabu_tenure_name, std::to_string(DEFAULT_TABU_TENURE), true},
        {n1_name, std::to_string(DEFAULT_N1), true},
        {n2_name, std::to_string(DEFAULT_N2), true},
        {S_name, std::to_string(DEFAULT_S), true},
        {z_name, std::to_string(DEFAULT_Z), true},
        {debug_t_name, std::to_string(DEFAULT_DEBUG_T), true},
        {defaults_name, std::to_string(DEFAULT_DEFAULTS), true}
    };
}

Params newTaskSolver::estimate() {
    return {
        {expect_time, std::to_string(time) + " sec", false}
    };
}

void newTaskSolver::config_defaults(int rows, int cols, int time) {
    int n = rows * cols;
    if (n <= 30) {
        n1 = 2;
        n2 = 7;
        S = 100;
        z = 10;
    } else if (n <= 55) {
        n1 = 2;
        n2 = 8;
        if (time <= 5) {
            S = 50;
        } else {
            S = 100;
        }
        z = 10;
    } else {
        n1 = 2;
        n2 = 8;
        if (time <= 10) {
            S = 25;
        } else {
            S = 100;
        }
        z = 10;
    }
}

Params newTaskSolver::solve() {
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
        LCM = 1ll * LCM * (size - 1) / std::__gcd(LCM, 1ll * size - 1);
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

                Pin *a = cur_pin[i];
                Pin *b = cur_pin[j];
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

    NewHeuristQAP solver(cost);
    if (seed == -1) {
        std::mt19937 rnd{(uint32_t) std::chrono::high_resolution_clock().now().time_since_epoch().count()};
        seed = rnd();
    }

    int debug_interval = -1;
    if (debug_t != 0) {
        debug_interval = debug_t * 1e6;
    }

    // if (defaults) {
    //     config_defaults(rows, cols, time);
    // }

    auto start = clock();
    auto best = solver.solve(n1, n2, tabu_tenure, S, z, time, -1, seed, false, debug_interval);

    for (int i = 0; i < n; ++i) {
        devices[i].center = locations[best[i]];
    }

    write_layout(output_layout_path);

    double cpu_time = static_cast<double>(clock() - start) / 1e6;

    auto debug_info = solver.get_debug_info();

    Params params{
            {CPU_time, my_round(cpu_time, 3) + " sec", false},
            {TWL_manhattan, my_round(calc_metric(calc_manhattan)), false},
            {TWL_HP, my_round(calc_metric(calc_half_p)), false},
            {TWL_clique, my_round(calc_metric(calc_clique)), false},
            {TWL_hybrid, my_round(calc_metric(calc_hybrid)), false},
    };

    for (auto [t, perm] : debug_info) {
        for (int i = 0; i < n; ++i) {
            devices[i].center = locations[perm[i]];
        }
        params.push_back(Param{"di_" + std::to_string(t), std::to_string(calc_metric(calc_manhattan)), false});
    }

    return params;
}

void newTaskSolver::init(const py::str& input_path,
        const py::str& output_path,
        const py::kwargs& kwargs) {

    init_layout(input_path.cast<std::string>());
    output_layout_path = output_path.cast<std::string>();

    get_value_nodef(kwargs, rows_name, rows);
    get_value_nodef(kwargs, cols_name, cols);

    get_value(kwargs, step_x_name, step_x, DEFAULT_STEP_X);
    get_value(kwargs, step_y_name, step_y, DEFAULT_STEP_Y);

    get_value(kwargs, time_name, time, DEFAULT_TIME);

    get_value(kwargs, n1_name, n1, DEFAULT_N1);
    get_value(kwargs, n2_name, n2, DEFAULT_N2);
    get_value(kwargs, tabu_tenure_name, tabu_tenure, DEFAULT_TABU_TENURE);
    get_value(kwargs, S_name, S, DEFAULT_S);
    get_value(kwargs, z_name, z, DEFAULT_Z);
    get_value(kwargs, seed_name, seed, DEFAULT_SEED);

    get_value_double(kwargs, debug_t_name, debug_t, DEFAULT_DEBUG_T);
    get_value(kwargs, defaults_name, defaults, DEFAULT_DEFAULTS);
}