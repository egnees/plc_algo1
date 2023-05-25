
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 08.05.2023.
//

#include "TaskSolver.h"

#include <cmath>
#include <random>
#include <set>
#include <utility>

void get_value(const py::kwargs& kwargs, const std::string& name, int& val, int def) {
    if (!kwargs.contains(name)) {
        val = def;
    } else {
        val = std::stoi(kwargs[name.c_str()].cast<std::string>());
    }
}

void get_value_double(const py::kwargs& kwargs, const std::string& name, double& val, double def) {
    if (!kwargs.contains(name)) {
        val = def;
    } else {
        val = std::stod(kwargs[name.c_str()].cast<std::string>());
    }
}

void get_value_nodef(const py::kwargs& kwargs, const std::string& name, int& val) {
    if (!kwargs.contains(name)) {
        throw std::runtime_error("No " + name);
    } else {
        val = std::stoi(kwargs[name.c_str()].cast<std::string>());
    }
}

void destroy_layout(Layout& layout) {
    delete[] layout.devices;
    delete[] layout.pins;
    delete[] layout.nets;
}

Point get_offset(int width, int height, int step_x, int step_y, int rows, int cols, int device_hwidth, int device_hheight) {
    int w = 2 * cols * device_hwidth + (cols - 1) * step_x;
    int h = 2 * rows * device_hheight + (rows - 1) * step_y; 
    return {(width - w) / 2, (height - h) / 2};
}

Layout init_layout_from_file(const std::string &path_to_layout) {
    FILE* file = fopen(path_to_layout.c_str(), "r");
    const int BUFLEN = 1024;
    char temp_buffer[BUFLEN];

    Layout layout;

    fscanf(file, "%s", temp_buffer); // devices
    int device_count;
    fscanf(file, "%d", &device_count);
    Device* devices = new Device[device_count];
    for (int i = 0; i < device_count; ++i) {
        int id;
        fscanf(file, "%d", &id);
        devices[id].id = id;
        fscanf(file, "%d %d %d %d",
              &devices[id].center.x, &devices[id].center.y,
              &devices[id].half_width, &devices[id].half_height);
    }

    fscanf(file, "%s", temp_buffer); // pins
    int pin_count;
    fscanf(file, "%d", &pin_count);
    Pin* pins = new Pin[pin_count];
    for (int i = 0; i < pin_count; ++i) {
        int id;
        fscanf(file, "%d", &id);
        pins[id].id = id;
        int assigned_device_id;
        fscanf(file, "%d", &assigned_device_id);
        pins[id].assigned_device = devices + assigned_device_id;
        fscanf(file, "%d %d %d %d",
               &pins[id].relative.x,
               &pins[id].relative.y,
               &pins[id].half_width,
               &pins[id].half_height);
    }

    fscanf(file, "%s", temp_buffer); // nets
    int net_count;
    fscanf(file, "%d", &net_count);
    Net* nets = new Net[net_count];
    for (int i = 0; i < net_count; ++i) {
        int id;
        fscanf(file, "%d", &id);
        nets[id].id = id;
        int net_size;
        fscanf(file, "%d", &net_size);
        nets[id].pins.reserve(net_size);
        for (int j = 0; j < net_size; ++j) {
            int pin_id;
            fscanf(file, "%d", &pin_id);
            nets[id].pins.push_back(pins + pin_id);
        }
    }

    int x, y;
    int bbox_width{0};
    int bbox_height{0};
    if (fscanf(file, "%d%d", &x, &y)) {
        bbox_width = x;
        bbox_height = y;
        // printf("inited width, height: %d %d\n", bbox_width, bbox_height);
    }

    fclose(file);

    return Layout{
        devices, pins, nets,
        device_count, pin_count, net_count,
        bbox_width, bbox_height
    };
}

void write_layout_to_file(const std::string& path_to_file, const Layout& layout) {
    // puts("writing layout to file...");
    FILE* file = fopen(path_to_file.c_str(), "w");
    // puts("opened file");

    fprintf(file, "Devices\n"); // devices
    // puts("printed devices (name)");
    int device_count = layout.device_cnt;
    auto devices = layout.devices;
    fprintf(file, "%d\n", device_count);
    for (int i = 0; i < device_count; ++i) {
        fprintf(file, "%d\n%d %d %d %d\n",
                devices[i].id, devices[i].center.x, devices[i].center.y,
                devices[i].half_width, devices[i].half_height);
    }
    // puts("printed all devices");

    fprintf(file, "Pins\n"); // pins
    // puts("printed pins (name)");
    int pin_count = layout.pin_cnt;
    auto pins = layout.pins;
    fprintf(file, "%d\n", pin_count);
    for (int i = 0; i < pin_count; ++i) {
        fprintf(file, "%d\n%d %d %d %d %d\n", pins[i].id, pins[i].assigned_device->id,
                pins[i].relative.x, pins[i].relative.y,
                pins[i].half_width,
                pins[i].half_height);
    }
    // puts("printer all pins");

    fprintf(file, "Nets\n"); // nets
    // puts("printed nets (name)");
    int net_count = layout.net_cnt;
    auto nets = layout.nets;
    fprintf(file, "%d\n", net_count);
    for (int i = 0; i < net_count; ++i) {
        int net_size = (int) nets[i].pins.size();
        fprintf(file, "%d\n%d ", nets[i].id, net_size);
        for (int j = 0; j < net_size; ++j) {
            fprintf(file, "%d", nets[i].pins[j]->id);
            if (j + 1 < (int) net_size) {
                fprintf(file, " ");
            } 
        }
        fprintf(file, "\n");
    }
    // puts("printed all nets");

    fprintf(file, "%d %d\n", layout.bbox_width, layout.bbox_height);

    fclose(file);
}

int rand_int(int l, int r, std::mt19937& rnd) {
    return rnd() % (r - l + 1) + l;
}

Layout random_layout(
                    int seed,
                    int bbox_width, int bbox_height,
                    int step_x, int step_y,
                    int rows, int cols, 
                    int net_cnt, 
                    int device_hwidth_left, int device_hwidth_right,
                    int device_hheight_left, int device_hheight_right,
                    int pin_hwidth_left, int pin_hwidth_right,
                    int pin_hheight_left, int pin_hheight_right,
                    int pd_count_left, int pd_count_right, // pins in device
                    int pn_count_left, int pn_count_right // pins in net);
) {
    std::mt19937 rnd{static_cast<uint32_t>(seed)};

    int device_cnt = rows * cols;

    std::vector<int> pd_cnt(device_cnt, 0); // pd_cnt[i] = pins in device[i]
    int pin_cnt = 0;
    for (int& i : pd_cnt) {
        i = rand_int(pd_count_left, pd_count_right, rnd);
        // printf("i:%d, ", i);
        pin_cnt += i;
    }
    // puts("");

    Layout layout{
        new Device[device_cnt],
        new Pin[pin_cnt],
        new Net[net_cnt],
        device_cnt, pin_cnt, net_cnt,
        bbox_width, bbox_height
    };

    int pin_id = 0;

    Point offset{bbox_width/2 - (cols - 1) * step_x / 2, bbox_height/2 - (rows - 1) * step_y / 2};
    auto [offset_x, offset_y] = offset;

    std::vector<std::pair<int, int>> order;
    order.reserve(rows * cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            order.emplace_back(i, j);
        }
    }
    std::shuffle(order.begin(), order.end(), rnd);

    for (auto [i, j] : order) {
        int id = i * cols + j;
        layout.devices[id].id = id;
        layout.devices[id].half_width = rand_int(device_hwidth_left, device_hwidth_right, rnd);
        layout.devices[id].half_height = rand_int(device_hheight_left, device_hheight_right, rnd);
        layout.devices[id].center.x = i * step_x;

        layout.devices[id].pins.reserve(pd_cnt[id]);
        layout.devices[id].center.x = offset_x + step_x * j;
        layout.devices[id].center.y = offset_y + step_y * i;

        for (int k = 0; k < pd_cnt[id]; ++k) {
            layout.pins[pin_id].id = pin_id;
            layout.pins[pin_id].half_width = rand_int(pin_hwidth_left, pin_hwidth_right, rnd);
            layout.pins[pin_id].half_height = rand_int(pin_hheight_left, pin_hheight_right, rnd);
            layout.pins[pin_id].assigned_device = layout.devices + id;
            layout.pins[pin_id].relative.x = rand_int(-layout.devices[id].half_width, layout.devices[id].half_width, rnd);
            layout.pins[pin_id].relative.y = rand_int(-layout.devices[id].half_height, layout.devices[id].half_height, rnd);
            layout.devices[id].pins.push_back(layout.pins + pin_id);
            ++pin_id;
        }
    }

    std::set<std::pair<int, int>> idxes;
    for (int i = 0; i < pin_cnt; ++i) {
        idxes.insert({0, i});
    }

    for (int i = 0; i < net_cnt; ++i) {
        layout.nets[i].id = i;
        // printf("pn_count_left=%d, pn_count_right=%d\n", pn_count_left, pn_count_right);
        int cnt = rand_int(pn_count_left, pn_count_right, rnd);
        // printf("cnt: %d, ", cnt);
        if (cnt > pin_cnt) {
            cnt = pin_cnt;
        }
        // puts("");
        layout.nets[i].pins.reserve(cnt);
        while (cnt > 0) {
            auto [x, id] = *idxes.begin();
            idxes.erase(idxes.begin());
            layout.nets[i].pins.push_back(layout.pins + id);
            idxes.insert({x + 1, id});
            --cnt;
        }
        // puts("did layout.nets[i].pins.reserve(cnt)");
        // printf("inited layout.nets[%d]\n", i);
    }
    // puts("generated layout");

    return layout;
}

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
) {
    std::mt19937 rnd{(uint32_t) seed};

    int unit_bbox_height = (unit_bbox_height + outer_rows - 1) / outer_rows;
    int unit_bbox_width = (unit_bbox_width + outer_cols - 1) / outer_cols;

    int total_pins_count = 0;

    std::vector<Layout> layouts;
    layouts.reserve(outer_rows * outer_cols);
    for (int i = 0; i < outer_rows * outer_cols; ++i) {
        layouts.push_back(random_layout(
            rnd(), unit_bbox_width, unit_bbox_height,
            inner_step_x, inner_step_y,
            inner_rows, inner_cols,
            inner_net_cnt,
            device_hwidth_left, device_hwidth_right,
            device_hheight_left, device_hheight_right,
            pin_hwidth_left, pin_hwidth_right,
            pin_hheight_left, pin_hheight_right,
            pd_count_left, pd_count_right,
            pn_count_left, pn_count_right)
        );
        total_pins_count += layouts.back().pin_cnt;
    }

    if (outer_rows == 1 && outer_cols == 1 && outer_net_cnt != 0) {
        throw std::runtime_error("outer_rows == 1 && outer_cols == 1 && outer_net_cnt != 0");
    }

    if (pn_count_left <= 1) {
        throw std::runtime_error("pn_count_left <= 1");
    }

    std::vector<Net*> addiction_nets;

    for (int i = 0; i < outer_net_cnt; ++i) {
        int x = rnd() % (outer_rows * outer_cols);
        int y;
        do {
            y = rnd() % (outer_rows * outer_cols);
        } while (x == y);
        
        int cnt_tot = rnd() % (pn_count_right - pn_count_left + 1) + pn_count_left;
        int cnt_first = rnd() % (cnt_tot - 1) + 1;
        int cnt_second = cnt_tot - cnt_first;

        cnt_first = std::min(cnt_first, (int) layouts[x].pin_cnt);
        cnt_second = std::min(cnt_second, (int) layouts[y].pin_cnt);

        if (cnt_first + cnt_second <= 1) {
            continue;
        }

        std::vector<int> a((int) layouts[x].pin_cnt);
        for (int j = 0; j < (int) layouts[x].pin_cnt; ++j) {
            a[j] = j;
        }
        std::shuffle(a.begin(), a.end(), rnd);

        addiction_nets.push_back(new Net());
        addiction_nets.back()->id = (int) addiction_nets.size() - 1 + inner_net_cnt * outer_cols * outer_rows;
        for (int j = 0; j < cnt_first; ++j) {
            addiction_nets.back()->pins.push_back(layouts[x].pins + a[j]);
        }

        std::vector<int> b((int) layouts[y].pin_cnt);
        for (int j = 0; j < (int) layouts[y].pin_cnt; ++j) {
            b[j] = j;
        }
        std::shuffle(b.begin(), b.end(), rnd);

        for (int j = 0; j < cnt_second; ++j) {
            addiction_nets.back()->pins.push_back(layouts[y].pins + b[j]);
        }
    }

    Layout res{};

    res.bbox_width = bbox_width;
    res.bbox_height = bbox_height;

    res.device_cnt = inner_cols * inner_rows * outer_cols * outer_rows;
    res.devices = new Device[res.device_cnt];
    
    res.pin_cnt = total_pins_count;
    res.pins = new Pin[res.pin_cnt];

    res.net_cnt = inner_net_cnt * outer_cols * outer_rows + (int) addiction_nets.size();
    res.nets = new Net[res.net_cnt];

    std::map<Device*, Device*> dev_map; // old to new
    std::map<Pin*, Pin*> pin_map; // old to new

    int current_device = 0;
    int current_pin = 0;
    int current_net = 0;

    for (int i = 0; i < outer_rows * outer_cols; ++i) {
        // printf("layouts[%d].device_cnt=%d\n", i, (int) layouts[i].device_cnt);
        for (int d = 0; d < layouts[i].device_cnt; ++d) {
            Device *to = res.devices + current_device;
            Device *from = layouts[i].devices + d;

            to->id = current_device;
            to->center = {i % outer_cols * (outer_step_x + (inner_cols - 1) * inner_step_x) + d % inner_cols * inner_step_x,
                          i / outer_cols * (outer_step_y + (inner_rows - 1) * inner_step_y) + d / inner_cols * inner_step_y};
            to->half_width = from->half_width;
            to->half_height = from->half_height;

            dev_map[from] = to;

            ++current_device;
        }
    }

    for (int i = 0; i < outer_rows * outer_cols; ++i) {
        for (int p = 0; p < layouts[i].pin_cnt; ++p) {
            Pin *to = res.pins + current_pin;
            Pin* from = layouts[i].pins + p;

            to->id = current_pin;
            to->relative = from->relative;
            to->half_width = from->half_width;
            to->half_height = from->half_height;
            to->assigned_device = dev_map[from->assigned_device];

            pin_map[from] = to;

            ++current_pin;
        }
    }

    for (int i = 0; i < outer_rows * outer_cols; ++i) {
        for (int n = 0; n < layouts[i].net_cnt; ++n) {
            Net *to = res.nets + current_net;
            Net *from = layouts[i].nets + n;

            to->id = current_net;
            to->pins.reserve(from->pins.size());

            for (Pin* pin_from : from->pins) {
                to->pins.push_back(pin_map[pin_from]);
            }

            ++current_net;
        }
    }

    for (Net* from : addiction_nets) {
        Net *to = res.nets + current_net;

        to->id = current_net;
        to->pins.reserve(from->pins.size());

        for (Pin* pin_from : from->pins) {
            to->pins.push_back(pin_map[pin_from]);
        }

        ++current_net;
    }

    for (int i = 0; i < outer_rows * outer_cols; ++i) {
        destroy_layout(layouts[i]);       
    }

    int width = (outer_cols - 1) * outer_step_y + inner_cols * inner_step_y;
    int height = (outer_rows - 1) * outer_step_x + inner_rows * inner_step_x;

    int offset_x = (bbox_width - width) / 2;
    int offset_y = (bbox_height - height) / 2;

    for (int i = 0; i < res.device_cnt; ++i) {
        res.devices[i].center.x += offset_x;
        res.devices[i].center.y += offset_y;
    }

    return res;
}

void TaskSolver::init_layout(const std::string &path_to_layout) {
    FILE* file = fopen(path_to_layout.c_str(), "r");
    const int BUFLEN = 1024;
    char temp_buffer[BUFLEN];

    fscanf(file, "%s", temp_buffer); // devices
    fscanf(file, "%d", &device_count);
    devices = new Device[device_count];
    for (int i = 0; i < device_count; ++i) {
        int id;
        fscanf(file, "%d", &id);
        devices[id].id = id;
        fscanf(file, "%d %d %d %d",
              &devices[id].center.x, &devices[id].center.y,
              &devices[id].half_width, &devices[id].half_height);
    }

    fscanf(file, "%s", temp_buffer); // pins
    fscanf(file, "%d", &pin_count);
    pins = new Pin[pin_count];
    for (int i = 0; i < pin_count; ++i) {
        int id;
        fscanf(file, "%d", &id);
        pins[id].id = id;
        int assigned_device_id;
        fscanf(file, "%d", &assigned_device_id);
        pins[id].assigned_device = devices + assigned_device_id;
        fscanf(file, "%d %d %d %d",
               &pins[id].relative.x,
               &pins[id].relative.y,
               &pins[id].half_width,
               &pins[id].half_height);
    }

    fscanf(file, "%s", temp_buffer); // nets
    fscanf(file, "%d", &net_count);
    nets = new Net[net_count];
    for (int i = 0; i < net_count; ++i) {
        int id;
        fscanf(file, "%d", &id);
        nets[id].id = id;
        int net_size;
        fscanf(file, "%d", &net_size);
        nets[id].pins.reserve(net_size);
        for (int j = 0; j < net_size; ++j) {
            int pin_id;
            fscanf(file, "%d", &pin_id);
            nets[id].pins.push_back(pins + pin_id);
        }
    }

    int x, y;
    if (fscanf(file, "%d%d", &x, &y)) {
        screen_width = x;
        screen_height = y;
        // printf("inited width, height: %d %d\n", screen_width, screen_height);
    }

    fclose(file);
}

void TaskSolver::write_layout(const std::string &path_to_file) {
    FILE* file = fopen(path_to_file.c_str(), "w");

    fprintf(file, "Devices\n"); // devices
    fprintf(file, "%d\n", device_count);
    for (int i = 0; i < device_count; ++i) {
        fprintf(file, "%d\n%d %d %d %d\n",
                devices[i].id, devices[i].center.x, devices[i].center.y,
                devices[i].half_width, devices[i].half_height);
    }

    fprintf(file, "Pins\n"); // pins
    fprintf(file, "%d\n", pin_count);
    for (int i = 0; i < pin_count; ++i) {
        fprintf(file, "%d\n%d %d %d %d %d\n", pins[i].id, pins[i].assigned_device->id,
                pins[i].relative.x, pins[i].relative.y,
                pins[i].half_width,
                pins[i].half_height);
    }

    fprintf(file, "Nets\n"); // nets
    fprintf(file, "%d\n", net_count);
    for (int i = 0; i < net_count; ++i) {
        int net_size = (int) nets[i].pins.size();
        fprintf(file, "%d\n%d ", nets[i].id, net_size);
        for (int j = 0; j < net_size; ++j) {
            fprintf(file, "%d", nets[i].pins[j]->id);
            if (j + 1 < (int) net_size) {
                fprintf(file, " ");
            } 
        }
        fprintf(file, "\n");
    }

    fprintf(file, "%d %d\n", screen_width, screen_height);

    fclose(file);
}

TaskSolver::~TaskSolver() {
    delete[] devices;
    delete[] pins;
    delete[] nets;
}

double TaskSolver::get_cpu(int start) {
    return round((static_cast<double>(clock() - start) / 1e6) * 100) / 100;
}

double TaskSolver::calc_metric(std::function<double(const Net *)> &&metric) const {
    double res = 0;
    for (int i = 0; i < net_count; ++i) {
        res += metric(nets + i);
    }
    return res;
}

void TaskSolver::add_debug_info(Params& p, double t, std::function<double(const Net*)>&& metric) const {
    p.emplace_back("di_" + std::to_string(t), std::to_string(calc_metric(std::move(metric))), false);
}

double calc_half_p(const Net* net) {
    if (net->pins.empty()) {
        return 0;
    }

    int max_x = (int) -1e8;
    int max_y = max_x;
    int min_x = -max_x;
    int min_y = -max_y;

    for (const Pin* p : net->pins) {
        auto [x, y] = p->absolute();
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
    }

    return ((max_x - min_x) + (max_y - min_y)) / 2.0;
}

double calc_clique(const Net* net) {
    auto pins = net->pins;
    if (pins.size() <= 1) {
        return 0;
    }

    double ret = 0;

    for (int i = 0; i < (int) pins.size(); ++i) {
        auto [x1, y1] = pins[i]->absolute();
        for (int j = i + 1; j < (int) pins.size(); ++j) {
            auto [x2, y2] = pins[j]->absolute();
            int dx = x2 - x1;
            int dy = y2 - y1;
            ret += std::sqrt(dx * dx + dy * dy);
        }
    }

    return ret / static_cast<double>(pins.size() - 1);
}

double calc_hybrid(const Net* net) {
    auto pins = net->pins;
    if (pins.size() <= 3) {
        return calc_clique(net);
    } else {
        return calc_clique(net) * (double) pins.size();
    }
}

double calc_manhattan(const Net* net) {
    auto pins = net->pins;
    if (pins.size() <= 1) {
        return 0.0;
    }
    double cf = 1.0 / static_cast<double>(pins.size() - 1);
    double ret = 0;
    for (int i = 0; i < (int) pins.size(); ++i) {
        auto [x1, y1] = pins[i]->absolute();
        for (int j = i + 1; j < (int) pins.size(); ++j) {
            auto [x2, y2] = pins[j]->absolute();
            ret += abs(x2 - x1) + abs(y2 - y1);
        }
    }
    return ret * cf;
}

std::string my_round(double x, int e) {
    char buffer[100];
    std::string format = "%." + std::to_string(e) + "lf";
    sprintf(buffer, format.c_str(), x);
    return buffer;
}

Point Pin::absolute() const {
    return {relative.x + assigned_device->center.x,
            relative.y + assigned_device->center.y};
}