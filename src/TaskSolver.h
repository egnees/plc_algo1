
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#ifndef PYBIND11_ALGO_TASKSOLVER_H
#define PYBIND11_ALGO_TASKSOLVER_H

#include <map>
#include <vector>
#include <string>

#include <pybind11/pybind11.h>

namespace py = pybind11;

void get_value(const py::kwargs& kwargs, const std::string& name, int& val, int def);

void get_value_double(const py::kwargs& kwargs, const std::string& name, double& val, double def);

void get_value_nodef(const py::kwargs& kwargs, const std::string& name, int& val);

struct Param {
    std::string key;
    std::string value;
    bool optional{false};
};

using Params = std::vector<Param>;

struct Point {
    int x, y;
};

struct Pin;

struct Device {
    int id;
    int half_width;
    int half_height;
    Point center;
    std::vector<Pin*> pins;
};

struct Pin {
    int id;
    int half_width;
    int half_height;
    Device* assigned_device;
    Point relative;

    [[nodiscard]] Point absolute() const;
};

struct Net {
    int id;
    std::vector<Pin*> pins;
};

struct Layout {
    Device* devices{nullptr};
    Pin* pins{nullptr};
    Net* nets{nullptr};

    int device_cnt{0};
    int pin_cnt{0};
    int net_cnt{0};

    int bbox_width{0};
    int bbox_height{0};
}; 

void destroy_layout(Layout& layout);

Layout init_layout_from_file(const std::string& input_path);
void write_layout_to_file(const std::string& output_path, const Layout& layout);

Point get_offset(int width, int height, int step_x, int step_y, int rows, int cols, int device_hwidth, int device_hheight);

Layout random_layout(
                    int seed,
                    int bbox_width, int bbox_height,
                    int step_x, int step_y,
                    int rows, int cols, 
                    int nets_cnt, 
                    int device_hwidth_left, int device_hwidth_right,
                    int device_hheight_left, int device_hheight_right,
                    int pin_hwidth_left, int pin_hwidth_right,
                    int pin_hheight_left, int pin_hheight_right,
                    int pd_count_left, int pd_count_right, // pins in device
                    int pn_count_left, int pn_count_right // pins in net);
);

Layout random_cluster_layout(
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
);

class TaskSolver {
public:
    virtual void init(const py::str& input_path,
                      const py::str& output_path,
                      const py::kwargs& kwargs) = 0;

    virtual Params get_params() = 0;
    virtual Params estimate() = 0;
    virtual Params solve() = 0;

    virtual ~TaskSolver();

    static double get_cpu(int start);

    double calc_metric(std::function<double(const Net*)>&& metric) const;

    void add_debug_info(Params& p, double t, std::function<double(const Net*)>&& metric) const;

protected:
    void init_layout(const std::string& path_to_layout);
    void write_layout(const std::string& path_to_file);

    Device* devices{nullptr};
    Pin* pins{nullptr};
    Net* nets{nullptr};

    int device_count{0};
    int pin_count{0};
    int net_count{0};

    const double DEFAULT_DEBUG_T{0};

    int screen_width{1280-360};
    int screen_height{720-100};
    int margin_x{30};
    int margin_y{30};
    double debug_t{DEFAULT_DEBUG_T};


    std::string output_layout_path{};

    std::string expect_time{"Expect time"};
    std::string CPU_time{"CPU time"};

    std::string TWL_manhattan{"TWL manh"};
    std::string TWL_HP{"TWL HP"};
    std::string TWL_clique{"TWL clique"};
    std::string TWL_hybrid{"TWL hybrid"};


    std::string rows_name{"rows"};
    std::string cols_name{"cols"};
    std::string step_x_name{"step_x"};
    std::string step_y_name{"step_y"};

    std::string debug_t_name{"debug_t"};

};

double calc_half_p(const Net* net);
double calc_clique(const Net* net);
double calc_hybrid(const Net* net);
double calc_manhattan(const Net* net);

std::string my_round(double x, int e = 2);

#endif //PYBIND11_ALGO_TASKSOLVER_H
