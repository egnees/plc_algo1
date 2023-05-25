
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#include "defs.h"

#include <pybind11/pybind11.h>

PYBIND11_MODULE(placer, m) {
    m.def("solvers", &solvers);
    m.def("params", &params, py::arg("solver"));
    m.def("validate_and_estimate", &validate_and_estimate,
          py::arg("solver"), py::arg("input"), py::arg("output"));
    m.def("solve", &solve, py::arg("solver"), py::arg("input"), py::arg("output"));

    m.def("gen_cluster_layout", &gen_cluster_layout, 
        py::arg("path"),
        py::arg("seed") = -1,
        py::arg("bbox_width") = 1488, py::arg("bbox_height") = 873,
        py::arg("outer_step_x") = 180, py::arg("outer_step_y") = 180,
        py::arg("inner_step_x") = 70, py::arg("inner_step_y") = 70,
        py::arg("outer_rows"), py::arg("outer_cols"),
        py::arg("inner_rows"), py::arg("inner_cols"),
        py::arg("outer_nets"), py::arg("inner_nets"),
        py::arg("device_hwidth_left") = 25, py::arg("device_hwidth_right") = 25,
        py::arg("device_hheight_left") = 25, py::arg("device_hheight_right") = 25,
        py::arg("pin_hwidth_left") = 5, py::arg("pin_hwidth_right") = 5,
        py::arg("pin_hheight_left") = 5, py::arg("pin_hheight_right") = 5,
        py::arg("pd_count_left") = 2, py::arg("pd_count_right") = 5,
        py::arg("pn_count_left") = 4, py::arg("pn_count_right") = 9);
    
    m.def("gen_layout", &gen_layout,
        py::arg("path"),
        py::arg("seed") = -1,
        py::arg("bbox_width") = 1488, py::arg("bbox_height") = 873,
        py::arg("step_x") = 70, py::arg("step_y") = 70,
        py::arg("rows"), py::arg("cols"),
        py::arg("nets"),
        py::arg("device_hwidth_left") = 25, py::arg("device_hwdith_right") = 25,
        py::arg("device_hheight_left") = 25, py::arg("device_hheight_right") = 25,
        py::arg("pin_hwdith_left") = 5, py::arg("pin_hwidth_right") = 5,
        py::arg("pin_hheight_left") = 5, py::arg("pin_hheight_right") = 5,
        py::arg("pd_count_left") = 2, py::arg("pd_count_right") = 5,
        py::arg("pn_count_left") = 4, py::arg("pn_count_right") = 9);
}