#include "dpTaskSolver.h"
#include "../algo/dp.h"
#include <algorithm>
#include <cmath>
#include <numeric>

Params dpTaskSolver::get_params() {
    return {
        {step_x_name, std::to_string(DEFAULT_STEP_X), true}
    };
}

Params dpTaskSolver::estimate() {
    long long n2 = 1;
    for (int i = 1; i <= device_count; ++i) {
        n2 *= 2;
    }

    n2 *= device_count * device_count;

    double expect_time_result = static_cast<double>(n2) / 1e8;
    return {
        {expect_time, my_round(expect_time_result, 3) + " sec", false}
    };
}

int dpTaskSolver::get_lcm() const {
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
    return LCM;
}

std::pair<mut_t, pin_add_t> dpTaskSolver::get_input(int LCM) const {
    mut_t mut(n, std::vector<ans_t>(n, 0));
    pin_add_t add(n, std::vector<ans_t>(n, 0));

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

                Pin *a = cur_pin[i];
                Pin *b = cur_pin[j];
                if (a->assigned_device->id == b->assigned_device->id) {
                    continue;
                }

                int d1 = a->relative.x;
                int d2 = b->relative.x;

                add[a->assigned_device->id][b->assigned_device->id] += w * (d2 - d1);
                mut[a->assigned_device->id][b->assigned_device->id] += w;
            }
        }
    }

    return {mut, add};
}

Params dpTaskSolver::solve() {
    std::vector<int> locations(n, 0);
    Point offset{screen_width/2 - (device_count - 1) * step_x / 2, screen_height/2};
    for (int i = 0; i < n; ++i) {
        locations[i] = offset.x + step_x * i;
    }

    auto [mut, add] = get_input(get_lcm());

    SolverDP solver(locations, mut, add);
    auto start = clock();
    auto best = solver.solve();
    for (int i = 0; i < n; ++i) {
        devices[best[i]].center = {locations[i], offset.y};
    }

    double elapsed_cpu = static_cast<double>(clock() - start) / 1e6;

    write_layout(output_layout_path);

    return {
        {CPU_time, my_round(elapsed_cpu, 3) + " sec", false},
        {TWL_manhattan, my_round(calc_metric(calc_manhattan)), false},
        {TWL_HP, my_round(calc_metric(calc_half_p)), false},
        {TWL_clique, my_round(calc_metric(calc_clique)), false},
        {TWL_hybrid, my_round(calc_metric(calc_hybrid)), false},
    };
}

void dpTaskSolver::init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) {
    
    init_layout(input_path.cast<std::string>());
    output_layout_path = output_path.cast<std::string>();

    n = device_count;

    get_value(kwargs, step_x_name, step_x, DEFAULT_STEP_X);
}