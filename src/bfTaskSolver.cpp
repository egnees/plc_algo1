
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#include "bfTaskSolver.h"

Params bfTaskSolver::get_params() {
    return {
            {rows_name, "", false},
            {cols_name, "", false},
            {step_x_name, "", true},
            {step_y_name, "", true}
    };
}

Params bfTaskSolver::estimate() {
    long long n_fact = 1;
    for (int i = 1; i <= device_count; ++i) {
        n_fact *= i;
    }

    long long net_sum = 0;
    for (int i = 0; i < net_count; ++i) {
        int x = (int) nets[i].pins.size();
        net_sum += 1ll * x * (x - 1) / 2;
    }

    double expect_time_result = static_cast<double>(n_fact * net_sum) / 1e8;
    return {
            {expect_time, my_round(expect_time_result, 3) + " sec", false}
    };
}

Params bfTaskSolver::solve() {

    auto start = clock();

    Point offset{screen_width/2 - (cols - 1) * step_x / 2, screen_height/2 - (rows - 1) * step_y / 2};
    // printf("screen width, screen_height %d %d\n", screen_width, screen_height);

    std::vector<Point> locations;
    locations.reserve(device_count);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            locations.push_back({offset.x + j * step_x, offset.y + i * step_y});
        }
    }

    std::vector<int> perm(device_count);
    for (int i = 0; i < device_count; ++i) {
        perm[i] = i;
    }

    auto best = perm;
    double best_twl = 1e9;

    do {
        for (int i = 0; i < device_count; ++i) {
            devices[i].center = locations[perm[i]];
        }

        double cur_twl = calc_twl();
        if (cur_twl < best_twl) {
            best = perm;
            best_twl = cur_twl;
        }

    } while (std::next_permutation(perm.begin(), perm.end()));

    for (int i = 0; i < device_count; ++i) {
        devices[i].center = locations[best[i]];
    }

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

void bfTaskSolver::init(const py::str &input_path, const py::str &output_path, const py::kwargs &kwargs) {
    init_layout(input_path.cast<std::string>());
    output_layout_path = output_path.cast<std::string>();

    if (!kwargs.contains(rows_name)
    || kwargs[rows_name.c_str()].is_none()
    || kwargs[rows_name.c_str()].cast<std::string>().empty()) {
        // printf("no rows\n");
        throw std::runtime_error("No " + rows_name);
    }
    if (!kwargs.contains(cols_name)
    || kwargs[cols_name.c_str()].is_none()
    || kwargs[cols_name.c_str()].cast<std::string>().empty()) {
        // printf("no cols\n");
        throw std::runtime_error("No " + cols_name);
    }

    try {
        rows = std::stoi(kwargs[rows_name.c_str()].cast<std::string>(), nullptr);
        cols = std::stoi(kwargs[cols_name.c_str()].cast<std::string>(), nullptr);
    } catch (std::exception& e) {
        // printf("Cant get Rows or Columns\n");
        throw std::runtime_error("Cant get " + rows_name + " or " + cols_name);
    }

    if (rows * cols != device_count) {
        // printf("Rows x Columns is not Device count");
        throw std::runtime_error("Dev cnt not equals Loc cnt");
    }

    try {

        if (!kwargs.contains(step_x_name)
        || kwargs[step_x_name.c_str()].is_none()
        || kwargs[step_x_name.c_str()].cast<std::string>().empty()) {
            int mx_half_width = (int) -1e8;
            for (int i = 0; i < device_count; ++i) {
                mx_half_width = std::max(mx_half_width, devices[i].half_width);
            }

            step_x = 2 * mx_half_width + margin_x;
        } else {
            step_x = std::stoi(kwargs[step_x_name.c_str()].cast<std::string>(), nullptr);
        }

        if (!kwargs.contains(step_y_name)
        || kwargs[step_y_name.c_str()].is_none()
        || kwargs[step_y_name.c_str()].cast<std::string>().empty()) {
            int mx_half_height = (int) -1e8;
            for (int i = 0; i < device_count; ++i) {
                mx_half_height = std::max(mx_half_height, devices[i].half_height);
            }

            step_y = 2 * mx_half_height + margin_y;
        } else {
            step_y = std::stoi(kwargs[step_y_name.c_str()].cast<std::string>(), nullptr);
        }

    } catch (std::exception& e) {
        throw std::runtime_error("Cant get " + step_x_name + " or " + step_y_name);
    }
}

double bfTaskSolver::calc_twl() const {
    return calc_metric(calc_manhattan);
}
