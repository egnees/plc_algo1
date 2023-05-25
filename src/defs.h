
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#ifndef PYBIND11_ALGO_DEFS_H
#define PYBIND11_ALGO_DEFS_H

#include <pybind11/pybind11.h>
#include "TaskSolver.h"
#include "IdleTaskSolver.h"

namespace py = pybind11;

// bind

py::list solvers();

py::list params(const py::str& solver_name);

py::list validate_and_estimate(const py::str& solver_name,
                               const py::str& input_path,
                               const py::str& output_path,
                               const py::kwargs& kwargs);

py::list solve(const py::str& solver_name,
               const py::str& input_path,
               const py::str& output_path,
               const py::kwargs& kwargs);

py::list gen_cluster_layout(
                    const std::string& path,
                    int seed,
                    int bbox_width, int bbox_height,
                    int step_out_x, int step_out_y,
                    int step_in_x, int step_in_y,
                    int rows_out, int cols_out,
                    int rows_int, int cols_in,
                    int inner_nets, int outer_nets,
                    int device_hwidth_left, int device_hwidth_right,
                    int device_hheight_left, int device_hheight_right,
                    int pin_hwidth_left, int pin_hwidth_right,
                    int pin_hheight_left, int pin_hheight_right,
                    int pd_count_left, int pd_count_right, // pins in device
                    int pn_count_left, int pn_count_right // pins in net
);

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
);

// not bind

std::unique_ptr<TaskSolver> create_solver(const py::str& solver_name);

#endif //PYBIND11_ALGO_DEFS_H
