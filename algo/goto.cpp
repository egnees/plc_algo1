#include "goto.h"

#include <numeric>
#include <cstring>
#include <random>
#include <queue>

// random

namespace Goto {

    enum {
        RANDOM_INT_LEFT = 0,
        RANDOM_INT_RIGHT = 1000000000 - 1
    };

    std::default_random_engine random_gen{};
    std::uniform_int_distribution<> int_dist(RANDOM_INT_LEFT, RANDOM_INT_RIGHT);

}

void Goto::get_best_k(const ans_t *x, const ans_t *y, int n, int m, int *ans_i, int *ans_j, int k) {
    // size(x) = size(ans_i) = n
    // size(y) = size(ans_j) = m
    // x and y sorted

    using pair = std::pair<int, int>;
    auto comparator = [x, y](const pair& left, const pair& right) {
        return x[left.first] + y[left.second] > x[right.first] + y[right.second];
    };
    std::priority_queue<pair, std::vector<pair>, decltype(comparator)> q(comparator);
    for (int i = 0; i < n; ++i) {
        q.emplace(i, 0);
    }
    for (int iter = 0; iter < k; ++iter) {
        auto [i, j] = q.top();
        ans_i[iter] = i;
        ans_j[iter] = j;
        q.pop();
        if (j + 1 < m) {
            q.emplace(i, j + 1);
        }
    }
}

namespace Goto {

    void set_seed(uint32_t seed) {
        random_gen.seed(seed);
    }

    int rand_int(int l, int r) {
        return int_dist(random_gen) % (r - l + 1) + l;
    }

} // namespace Goto

// constructor

using namespace Goto;

GotoHeurist::GotoHeurist(int m_, int n_, int stepx, int stepy, 
                const pin_acc_t& leftx, const pin_acc_t& samex,
                const pin_acc_t& upy, const pin_acc_t& samey,
                const mul_t& mul) {
    m = m_;
    n = n_;
    slots = m * n;
    devices = m * n;
    step_x = stepx;
    step_y = stepy;

    allocate_permanent();

    for (int i = 0; i < devices; ++i) {
        for (int j = 0; j < devices; ++j) {
            int q = idx_dev(i, j);
            
            left_x[q] = leftx[i][j];
            same_x[q] = samex[i][j];
            right_x[q] = leftx[j][i];

            up_y[q] = upy[i][j];
            same_y[q] = samey[i][j];
            down_y[q] = upy[j][i];

            w[q] = mul[i][j];
        } 
    }

    for (int s = 0; s < slots; ++s) {
        loc_x[s] = step_x * (s % n);
        loc_y[s] = step_y * (s / n);
    }

    std::iota(best.perm, best.perm + devices, 0);
    std::iota(best.rev_perm, best.rev_perm + slots, 0);
    best.twl = calc_twl(best);
}

GotoHeurist::~GotoHeurist() {
    deallocate_permanent();
}

// memory management

void GotoHeurist::allocate_permanent() {
    help_ans_i = new int[n];
    help_ans_j = new int[m];

    w = new ans_t[slots * slots];

    left_x = new ans_t[slots * slots];
    same_x = new ans_t[slots * slots];
    right_x = new ans_t[slots * slots];

    up_y = new ans_t[slots * slots];
    same_y = new ans_t[slots * slots];
    down_y = new ans_t[slots * slots];

    pref_s_x = new ans_t[n];
    pref_s_y = new ans_t[m];

    pref_w_x = new ans_t[n];
    pref_w_y = new ans_t[m];

    vals_x = new ans_t[n];
    vals_y = new ans_t[m];

    loc_x = new int[slots];
    loc_y = new int[slots];

    best.perm = new int[devices];
    best.rev_perm = new int[slots];
}

void GotoHeurist::deallocate_permanent() {
    delete[] left_x;
    delete[] same_x;
    delete[] right_x;

    delete[] up_y;
    delete[] same_y;
    delete[] down_y;

    delete[] pref_s_x;
    delete[] pref_s_y;

    delete[] pref_w_x;
    delete[] pref_w_y;

    delete[] vals_x;
    delete[] vals_y;

    delete[] loc_x;
    delete[] loc_y;

    delete[] help_ans_i;
    delete[] help_ans_j;
}

void GotoHeurist::allocate_temp() {
    median_neib = new int[eps];
    median_vals = new ans_t[eps];

    temp_ans_i = new int[eps];
    temp_ans_j = new int[eps];

    sols = new Solution[eps];
    for (int i = 0; i < eps; ++i) {
        sols[i].perm = new int[devices];
        sols[i].rev_perm = new int[slots];
    }
}

void GotoHeurist::deallocate_temp() {
    delete[] median_neib;

    delete[] temp_ans_i;
    delete[] temp_ans_j;

    delete[] best.perm;
    delete[] best.rev_perm;

    for (int i = 0; i < eps; ++i) {
        delete[] sols[i].perm;
        delete[] sols[i].rev_perm;
    }

    delete[] sols;
}

// SORG
GotoHeurist::Solution GotoHeurist::SORG() {
    int is_placed[devices]; // placed[device]
    int is_taken[slots]; // taken[slot]
    std::memset(is_placed, 0, devices * sizeof(int));
    std::memset(is_taken, 0, slots * sizeof(int));

    ans_t IOC[devices];
    std::memset(IOC, 0, devices * sizeof(ans_t));

    for (int i = 0; i < devices; ++i) {
        for (int j = 0; j < devices; ++j) {
            if (i != j) {
                IOC[i] -= w[idx_dev(i, j)];
            }
        }
    }

    Solution sol{
        new int[devices], // slots == devices
        new int[slots],
        0
    };

    for (int i = 0; i < slots; ++i) {
        int dev1{-1}, dev2{-1};
        for (int j = 0; j < devices; ++j) {
            if (is_placed[j]) {
                continue;
            }
            if (dev1 == -1) {
                dev1 = j;
                continue;
            }
            if (IOC[j] >= IOC[dev1]) {
                dev2 = dev1;
                dev1 = j;
            } else if (dev2 == -1 || IOC[j] > IOC[dev2]) {
                dev2 = j;
            }
        } // got two devices with highest IOC

        int dev{dev1};

        if (dev2 != -1 && rand_int(0, 1)) {
            dev = dev2;
        }

        int slot{-1};
        ans_t best_cost{-1};
        for (int j = 0; j < slots; ++j) {
            if (is_taken[j]) {
                continue;
            }
            ans_t cost{0};
            for (int d = 0; d < devices; ++d) {
                if (!is_placed[d]) {
                    continue;
                }
                cost += contrib(d, dev, sol.perm[d], j);
            }
            if (slot == -1 || cost < best_cost) {
                slot = j;
                best_cost = cost;
            }
        }

        is_placed[dev] = 1;
        is_taken[slot] = 1;
        sol.perm[dev] = slot;
        sol.rev_perm[slot] = dev;

        for (int j = 0; j < devices; ++j) {
            if (j != dev) {
                IOC[j] += w[idx_dev(dev, j)];
            }
        }

    } // O(devices^3)

    sol.twl = calc_twl(sol);

    return sol;
}



// Improvement
int GotoHeurist::idx(int i, int j) const {
    return i * n + j;
}

int GotoHeurist::idx_dev(int i, int j) const {
    return i * devices + j;
}

// GFDR
bool GotoHeurist::GFDR(GotoHeurist::Solution& sol, int device) { // improvement for the device
//    get_median_1(sol, device);
    get_median(sol, device);

    {
        int opt_device = sol.rev_perm[median_neib[0]];
        ans_t best_swap_delta = delta(sol, device, opt_device);
        if (best_swap_delta < 0) {
            swap(sol, device, opt_device, best_swap_delta);
            return true; // improved
        }
    } // check if first swap can improve

    if (lambda_max == 2) {
        return false;
    }

    // lambda_max >= 3

    ans_t total_delta[eps];
    for (int q = 0; q < eps; ++q) {
        copy(sol, sols[q]);

        int swap_device = sol.rev_perm[median_neib[q]];
        ans_t swap_delta = delta(sol, device, swap_device);

        swap(sols[q], device, swap_device, swap_delta);
        total_delta[q] = swap_delta; // >= 0
    }

    for (int lambda = 3; lambda <= lambda_max; ++lambda) {
        for (int q = 0; q < eps; ++q) {
//            get_median_1(sols[q], device);
            get_median(sols[q], device);

            int swap_device = sols[q].rev_perm[median_neib[0]];
            ans_t swap_delta = delta(sols[q], device, swap_device);

            swap(sols[q], device, swap_device, swap_delta);
            total_delta[q] += swap_delta;

            if (total_delta[q] < 0) { // improved
                copy(sols[q], sol);
                return true; // improved
            }
        }
    }

    return false; // not improved
}

// Solution

void GotoHeurist::copy(const GotoHeurist::Solution &from, GotoHeurist::Solution &to) const {
    if (!to.perm) {
        to.perm = new int[devices];
    }
    if (!to.rev_perm) {
        to.rev_perm = new int[slots];
    }
    to.twl = from.twl;
    std::memcpy(to.perm, from.perm, devices * sizeof(int));
    std::memcpy(to.rev_perm, from.rev_perm, slots * sizeof(int));
}

ans_t GotoHeurist::calc_twl(const GotoHeurist::Solution &sol) const {
    ans_t twl{0};
    for (int i = 0; i < devices; ++i) {
        for (int j = i + 1; j < devices; ++j) {
            twl += contrib(i, j, sol.perm[i], sol.perm[j]);
        }
    }
    return twl;
}

void GotoHeurist::swap(GotoHeurist::Solution &sol, int i, int j, ans_t twl_delta) {
    sol.twl += twl_delta;
    std::swap(sol.rev_perm[sol.perm[i]], sol.rev_perm[sol.perm[j]]);
    std::swap(sol.perm[i], sol.perm[j]);
}

ans_t GotoHeurist::delta(const GotoHeurist::Solution &sol, int i, int j) const {
    if (i == j) {
        return 0;
    }
    ans_t ret{0};
    int pos_i = sol.perm[i];
    int pos_j = sol.perm[j];
    for (int q = 0; q < devices; ++q) {
        if (q == i || q == j) {
            continue;
        }
        int pos_q = sol.perm[q];
        ret += contrib(i, q, pos_j, pos_q) - contrib(i, q, pos_i, pos_q)
                + contrib(j, q, pos_i, pos_q) - contrib(j, q, pos_j, pos_q);
    }
    ret += contrib(i, j, pos_j, pos_i) - contrib(i, j, pos_i, pos_j);
    return ret;
}

ans_t GotoHeurist::contrib(int i, int j, int pos_i, int pos_j) const {
    if (i == j) {
        return 0;
    }
    return contrib(i, j, loc_x[pos_i], loc_x[pos_j], loc_y[pos_i], loc_y[pos_j]);
}

ans_t GotoHeurist::contrib(int i, int j, int xi, int xj, int yi, int yj) const {
    return contrib_x(i, j, xi, xj) + contrib_y(i, j, yi, yj);
}

ans_t GotoHeurist::contrib_x(int i, int j, int xi, int xj) const {
    int pair_id = idx_dev(i, j);
    return w[pair_id] * abs(xi - xj) + (xi == xj ? same_x[pair_id] : (xi < xj ? left_x[pair_id] : right_x[pair_id]));
}

ans_t GotoHeurist::contrib_y(int i, int j, int yi, int yj) const {
    int pair_id = idx_dev(i, j);
    return w[pair_id] * abs(yi - yj) + (yi == yj ? same_y[pair_id] : (yi < yj ? down_y[pair_id] : up_y[pair_id]));
}

void GotoHeurist::get_vals(ans_t *vals, int N, int step, const ans_t *pref_w, const ans_t *pref_s) {
    ans_t sum_w{0};
    ans_t sum_s{0};
    for (int i = 0; i < N; ++i) {
        sum_w += pref_w[i];
        sum_s += pref_s[i];
        vals[i] = i * step * sum_w + sum_s;
    }
}

void GotoHeurist::get_median(const GotoHeurist::Solution &sol, int device) {
    std::memset(pref_s_x, 0, sizeof(ans_t) * n);
    std::memset(pref_s_y, 0, sizeof(ans_t) * m);
    std::memset(pref_w_x, 0, sizeof(ans_t) * n);
    std::memset(pref_w_y, 0, sizeof(ans_t) * m);

    for (int i = 0; i < devices; ++i) {
        if (i == device) {
            continue;
        }

        int pair_id = idx_dev(device, i);
        ans_t cur_w = w[pair_id];

        int xi = sol.perm[i] % n; // < n
        int yi = sol.perm[i] / n; // < m

        {
            pref_w_x[0] -= cur_w;
            pref_w_x[xi] += cur_w;
            if (xi + 1 < n) {
                pref_w_x[xi + 1] += cur_w;
            }

            pref_s_x[0] += step_x * xi * cur_w + left_x[pair_id];
            pref_s_x[xi] += -step_x * xi * cur_w - left_x[pair_id] + same_x[pair_id];
            if (xi + 1 < n) {
                pref_s_x[xi + 1] += -step_x * xi * cur_w - same_x[pair_id] + right_x[pair_id];
            }
        }

        {
            pref_w_y[0] -= cur_w;
            pref_w_y[yi] += cur_w;
            if (yi + 1 < m) {
                pref_w_y[yi + 1] += cur_w;
            }

            pref_s_y[0] += step_y * yi * cur_w + down_y[pair_id];
            pref_s_y[yi] += -step_y * yi * cur_w - down_y[pair_id] + same_y[pair_id];
            if (yi + 1 < m) {
                pref_s_y[yi + 1] += -step_y * yi * cur_w - same_y[pair_id] + up_y[pair_id];
            }
        }
    }

    get_vals(vals_x, n, step_x, pref_w_x, pref_s_x);
    get_vals(vals_y, m, step_y, pref_w_y, pref_s_y);

    std::iota(help_ans_i, help_ans_i + n, 0);
    std::sort(help_ans_i, help_ans_i + n, [vals_x = this->vals_x](int i, int j) {
        return vals_x[i] < vals_x[j];
    });

    std::iota(help_ans_j, help_ans_j + m, 0);
    std::sort(help_ans_j, help_ans_j + m, [vals_y = this->vals_y](int i, int j) {
        return vals_y[i] < vals_y[j];
    });

    std::sort(vals_x, vals_x + n);
    std::sort(vals_y, vals_y + m);

    get_best_k(vals_x, vals_y, n, m, temp_ans_i, temp_ans_j, eps);

    for (int q = 0; q < eps; ++q) {
        int i = help_ans_i[temp_ans_i[q]];
        int j = help_ans_j[temp_ans_j[q]];
        median_neib[q] = idx(j, i);
        median_vals[q] = vals_x[temp_ans_i[q]] + vals_y[temp_ans_j[q]];
    }
}

std::vector<std::pair<double, std::vector<int>>> GotoHeurist::get_debug_info() const {
    std::vector<std::pair<double, std::vector<int>>> res;
    res.reserve(debug_info.size());
    for (auto [t, sol] : debug_info) {
        res.emplace_back((double) t / 1e6, std::vector<int>{best.perm, best.perm + devices});
    }
    return res;
}

bool GotoHeurist::need_udpate() const {
    return debug_interval != -1 && (int)(clock() - last_time) >= debug_interval;
}

void GotoHeurist::update() {
    debug_info.emplace_back((clock() - start_time) / 1e6, std::vector<int>{best.perm, best.perm + devices});
    last_time = clock();
}

std::vector<int> GotoHeurist::solve(int lambda_max_param, int eps_param, double time, double deb_interval, int seed) {
    if (seed == -1) {
        std::mt19937 rnd{(uint32_t) std::chrono::high_resolution_clock::now().time_since_epoch().count()};
        seed = (int) rnd();
    }
    set_seed(seed);

    if (deb_interval == -1) {
        debug_interval = -1;
    } else {
        debug_interval = (int)(deb_interval * 1e6);
    }

    eps = std::min(eps_param, m * n);
    lambda_max = lambda_max_param;

    auto max_time = (clock_t)(time * 1e6);

    debug_info.clear();

    start_time = last_time = clock();

    allocate_temp();

    if (debug_interval != -1) {
        update();
    }

    while ((clock() - start_time) <= max_time) {

        if (need_udpate()) {
            update();
        }

        Solution initial{SORG()};
        for (int d = 0; d < devices; ++d) {
            GFDR(initial, d);
            if (initial.twl < best.twl) {
                copy(initial, best);
            }
        }
    }

    if (debug_interval != -1) {
        update();
    }

    std::vector<int> ret{best.perm, best.perm + devices};

    deallocate_temp();

    return ret;
}

void GotoHeurist::print_sol(const GotoHeurist::Solution &sol) const {
    for (int i = 0; i < devices; ++i) {
        printf("%d ", sol.perm[i]);
    }
    printf("\n");
    for (int i = 0; i < devices; ++i) {
        printf("%d ", sol.rev_perm[i]);
    }
    printf("\n");
}

ans_t GotoHeurist::contrib(const Solution& sol, int device) const {
    ans_t ret{0};
    for (int i = 0; i < devices; ++i) {
        if (i != device) {
            ret += contrib(device, i, sol.perm[device], sol.perm[i]);
        }
    }
    return ret;
}

GotoHeurist::Solution GotoHeurist::SORG1() {
    Solution ret{
        new int[devices],
        new int[slots],
        0
    };
    std::iota(ret.perm, ret.perm + devices, 0);
    std::iota(ret.rev_perm, ret.rev_perm + slots, 0);
    std::shuffle(ret.perm, ret.perm + devices, random_gen);
    for (int i = 0; i < devices; ++i) {
        ret.rev_perm[ret.perm[i]] = i;
    }
    return ret;
}

void GotoHeurist::get_median_1(const GotoHeurist::Solution &sol, int device) {
    std::vector<ans_t> contr(slots, 0);
    for (int i = 0; i < slots; ++i) {
        for (int d = 0; d < devices; ++d) {
            if (d != device) {
                ans_t cur_contr = contrib(device, d, i, sol.perm[d]);
                contr[i] += cur_contr;
            }
        }
    }
    std::vector<int> S(slots);
    std::iota(S.begin(), S.end(), 0);
    std::sort(S.begin(), S.end(), [&](int i, int j){
        return contr[i] < contr[j];
    });
    for (int i = 0; i < eps; ++i) {
        median_neib[i] = S[i];
        median_vals[i] = contr[median_neib[i]];
    }
}









