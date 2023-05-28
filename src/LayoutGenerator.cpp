#include "LayoutGenerator.h"

#include <random>
#include <chrono>

Params LayoutGenerator::get_params() {
    return {
        {"path", "", false},
        {"seed", "-1", true},
        {"bbox_width", std::to_string(DEFAULT_BBOX_WIDTH), true},
        {"bbox_height", std::to_string(DEFAULT_BBOX_HEIGHT), true},
        {"step_x", std::to_string(DEFAULT_STEP_X), true},
        {"step_y", std::to_string(DEFAULT_STEP_Y), true},
        {rows_name, "", false},
        {cols_name, "", false},
        {"nets", "", false},
        {"device_hwidth_left", std::to_string(DEFAULT_DEVICE_WIDTH), true},
        {"device_hwidth_right", std::to_string(DEFAULT_DEVICE_WIDTH), true},
        {"device_hheight_left", std::to_string(DEFAULT_DEVICE_HEIGHT), true},
        {"device_hheight_right", std::to_string(DEFAULT_DEVICE_HEIGHT), true},
        {"pin_hwidth_left", std::to_string(DEFAULT_PIN_WIDTH), true},
        {"pin_hwidth_right", std::to_string(DEFAULT_PIN_WIDTH), true},
        {"pin_hheight_left", std::to_string(DEFAULT_PIN_HEIGHT), true},
        {"pin_hheight_right", std::to_string(DEFAULT_PIN_HEIGHT), true},
        {"pd_count_left", std::to_string(DEFAULT_PD_COUNT_LEFT), true},
        {"pd_count_right", std::to_string(DEFAULT_PD_COUNT_RIGHT), true},
        {"pn_count_left", std::to_string(DEFAULT_PN_COUNT_LEFT), true},
        {"pn_count_right", std::to_string(DEFAULT_PN_COUNT_RIGHT), true}
    };
}

Params LayoutGenerator::estimate() {
    return {
        {"Expect time", "0 sec", false}
    };
}

Params LayoutGenerator::solve() {
    if (seed == -1) {
        uint32_t tmp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::mt19937 rnd{tmp};
        seed = rnd();
    }
    write_layout_to_file(
        path, 
        random_layout(
            seed, 
            bbox_width, bbox_height, 
            step_x, step_y, 
            rows, cols, 
            net_cnt, 
            device_hwidth_left, device_hwidth_right,
            device_hheight_left, device_hheight_right,
            pin_hwidth_left, pin_hwidth_right,
            pin_hheight_left, pin_hheight_right,
            pd_count_left, pd_count_right,
            pn_count_left, pn_count_right
        )
    );

    return {
        {"seed", std::to_string(seed), false}
    };
}

void LayoutGenerator::init(
            const py::str& input_path,
            const py::str& output_path,
            const py::kwargs& kwargs) {
    path = output_path.cast<std::string>();

    get_value(kwargs, "seed", seed, DEFAULT_SEED);
    get_value(kwargs, "bbox_width", bbox_width, DEFAULT_BBOX_WIDTH);
    get_value(kwargs, "bbox_height", bbox_height, DEFAULT_BBOX_HEIGHT);
    get_value(kwargs, "step_x", step_x, DEFAULT_STEP_X);
    get_value(kwargs, "step_y", step_y, DEFAULT_STEP_Y);
    get_value_nodef(kwargs, rows_name, rows);
    get_value_nodef(kwargs, cols_name, cols);
    get_value_nodef(kwargs, "nets", net_cnt);
    get_value(kwargs, "device_hwidth_left", device_hwidth_left, DEFAULT_DEVICE_WIDTH);
    get_value(kwargs, "device_hwidth_right", device_hwidth_right, DEFAULT_DEVICE_WIDTH);
    get_value(kwargs, "pin_hwidth_left", pin_hwidth_left, DEFAULT_PIN_WIDTH);
    get_value(kwargs, "pin_hwidth_right", pin_hwidth_right, DEFAULT_PIN_WIDTH);
    get_value(kwargs, "device_hheight_left", device_hheight_left, DEFAULT_DEVICE_HEIGHT);
    get_value(kwargs, "device_hheight_right", device_hheight_right, DEFAULT_DEVICE_HEIGHT);
    get_value(kwargs, "pin_hheight_left", pin_hheight_left, DEFAULT_PIN_HEIGHT);
    get_value(kwargs, "pin_hheight_right", pin_hheight_right, DEFAULT_PIN_HEIGHT);
    get_value(kwargs, "pd_count_left", pd_count_left, DEFAULT_PD_COUNT_LEFT);
    get_value(kwargs, "pd_count_right", pd_count_right, DEFAULT_PD_COUNT_RIGHT);
    get_value(kwargs, "pn_count_left", pn_count_left, DEFAULT_PN_COUNT_LEFT);
    get_value(kwargs, "pn_count_right", pn_count_right, DEFAULT_PN_COUNT_RIGHT);
}