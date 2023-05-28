//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#include "defs.h"
#include "TaskSolver.h"
#include "bfTaskSolver.h"
#include "zdTaskSolver.h"
#include "LayoutGenerator.h"
#include "newTaskSolver.h"
#include "dpTaskSolver.h"
#include "gotoSolver.h"
#include "newGotoSolver.h"

#include <pybind11/stl.h>
#include <random>
#include <chrono>

using PyParams = std::vector<std::tuple<std::string, std::string, bool>>;

PyParams convert_params(const Params& params) {
    PyParams ret;
    ret.reserve(params.size());
    for (auto [key, value, optional] : params) {
        ret.emplace_back(key, value, optional);
    }
    return ret;
}

PyParams create_from_exception(std::exception& e) {
    return {{"Error", e.what(), false}};
}

std::string idle_name{"idle"};
std::string bf_name{"brute_force"};
std::string zvi_drezner_name{"zd_heurist"};
std::string layout_generator_name{"layout_gen"};
std::string new_heurist_name{"new_heurist"};
std::string dp_name{"dp_linear"};
std::string goto_name{"goto"};
std::string new_goto_name{"new_goto"};

std::vector<std::string> solver_names = {
        idle_name,
        bf_name,
        zvi_drezner_name,
        layout_generator_name,
        new_heurist_name,
        dp_name,
        goto_name,
        new_goto_name
};

py::list solvers() {
    return py::cast(solver_names);
}

std::unique_ptr<TaskSolver> create_solver(const py::str& solver_name) {
    auto name = solver_name.cast<std::string>();
    if (name == idle_name) {
        return std::make_unique<IdleTaskSolver>();
    } else if (name == bf_name) {
        return std::make_unique<bfTaskSolver>();
    } else if (name == zvi_drezner_name) {
        return std::make_unique<zdTaskSolver>();
    } else if (name == layout_generator_name) {
        return std::make_unique<LayoutGenerator>();
    } else if (name == new_heurist_name) {
        return std::make_unique<newTaskSolver>();
    } else if (name == dp_name) {
        return std::make_unique<dpTaskSolver>();
    } else if (name == goto_name) {
        return std::make_unique<GotoTaskSolver>();
    } else if (name == new_goto_name) {
        return std::make_unique<newGotoTaskSolver>();
    } else {
        throw std::runtime_error("No such solver");
    }
}

py::list params(const py::str& solver_name) {
    try {
        auto solver = create_solver(solver_name);
        Params params = solver->get_params();
        return py::cast(convert_params(params));
    } catch (std::exception& e) {
        return py::cast(create_from_exception(e));
    }
}

py::list validate_and_estimate(const py::str& solver_name,
                               const py::str& input_path,
                               const py::str& output_path,
                               const py::kwargs& kwargs) {
    try {
        auto solver = create_solver(solver_name);
        solver->init(input_path, output_path, kwargs);
        Params params = solver->estimate();
        return py::cast(convert_params(params));
    } catch (std::exception& e) {
        return py::cast(create_from_exception(e));
    }
}

py::list solve(const py::str& solver_name,
               const py::str& input_path,
               const py::str& output_path,
               const py::kwargs& kwargs) {
    try {
        auto solver = create_solver(solver_name);
        solver->init(input_path, output_path, kwargs);
        Params result = solver->solve();
        return py::cast(convert_params(result));
    } catch (std::exception& e) {
        return py::cast(create_from_exception(e));
    }

}

py::list gen_cluster_layout(
                    const std::string& path,
                    int seed,
                    int bbox_width, int bbox_height,
                    int outer_step_x, int outer_step_y,
                    int inner_step_x, int inner_step_y,
                    int outer_rows, int outer_cols,
                    int inner_rows, int inner_cols,
                    int outer_net_cnt, int inner_net_cnt,
                    int device_hwidth_left, int device_hwidth_right,
                    int device_hheight_left, int device_hheight_right,
                    int pin_hwidth_left, int pin_hwidth_right,
                    int pin_hheight_left, int pin_hheight_right,
                    int pd_count_left, int pd_count_right, // pins in device
                    int pn_count_left, int pn_count_right // pins in net
) {
    if (seed == -1) {
        std::mt19937 rnd{(uint32_t) std::chrono::high_resolution_clock().now().time_since_epoch().count()};    
        seed = rnd();
    }
    std::mt19937 rnd{(uint32_t) seed};

    Layout res = random_cluster_layout(seed, bbox_width, bbox_height,
                    outer_step_x, outer_step_y,
                    inner_step_x, inner_step_y,
                    outer_rows, outer_cols,
                    inner_rows, inner_cols,
                    outer_net_cnt, inner_net_cnt,
                    device_hwidth_left, device_hwidth_right,
                    device_hheight_left, device_hheight_right,
                    pin_hwidth_left, pin_hwidth_right,
                    pin_hheight_left, pin_hheight_right,
                    pd_count_left, pd_count_right,
                    pn_count_left, pn_count_right);

    write_layout_to_file(path, res);

    destroy_layout(res);

    return py::list{};
}

py::list gen_layout(
                const std::string& path,
                int seed,
                int bbox_width, int bbox_height,
                int step_x, int step_y,
                int rows, int cols,
                int nets,
                int device_hwidth_left, int device_hwidth_right,
                int device_hheight_left, int device_hheight_right,
                int pin_hwidth_left, int pin_hwidth_right,
                int pin_hheight_left, int pin_hheight_right,
                int pd_count_left, int pd_count_right,
                int pn_count_left, int pn_count_right
) {
    if (seed == -1) {
        std::mt19937 rnd{(uint32_t) std::chrono::high_resolution_clock().now().time_since_epoch().count()};    
        seed = rnd();
    }

    Layout rand_layout = random_layout(
            seed, bbox_width, bbox_height, step_x, step_y, rows,cols, nets, device_hwidth_left, device_hwidth_right,
            device_hheight_left, device_hheight_right, pin_hwidth_left, pin_hwidth_right, pin_hheight_left, pin_hheight_right,
            pd_count_left, pd_count_right, pn_count_left, pn_count_right
    );

    write_layout_to_file(
        path,
        rand_layout
    );

    destroy_layout(rand_layout);

    return py::list{};
}