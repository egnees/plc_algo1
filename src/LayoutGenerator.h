#pragma once

#include "TaskSolver.h"

class LayoutGenerator : public TaskSolver {
public:
    Params get_params() override;
    Params estimate() override;
    Params solve() override;

    void init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) override;

private:
    std::string path;
    int seed;
    int bbox_width, bbox_height;
    int step_x, step_y;
    int rows, cols;
    int net_cnt;
    int device_hwidth_left, device_hwidth_right;
    int device_hheight_left, device_hheight_right;
    int pin_hwidth_left, pin_hwidth_right;
    int pin_hheight_left, pin_hheight_right;
    int pd_count_left, pd_count_right; // pins in device
    int pn_count_left, pn_count_right; // pins in net

    const int DEFAULT_SEED{-1};
    const int DEFAULT_BBOX_WIDTH{1488};
    const int DEFAULT_BBOX_HEIGHT{873};
    const int DEFAULT_STEP_X{70};
    const int DEFAULT_STEP_Y{70};
    const int DEFAULT_DEVICE_WIDTH{25};
    const int DEFAULT_DEVICE_HEIGHT{25};
    const int DEFAULT_PIN_WIDTH{5};
    const int DEFAULT_PIN_HEIGHT{5};
    const int DEFAULT_PD_COUNT_LEFT{1};
    const int DEFAULT_PD_COUNT_RIGHT{4};
    const int DEFAULT_PN_COUNT_LEFT{2};
    const int DEFAULT_PN_COUNT_RIGHT{6};
};