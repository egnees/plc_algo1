#include "new_goto.h"

#include <numeric>
#include <cstring>
#include <random>
#include <queue>

// random

namespace NewGoto {

    enum {
        RANDOM_INT_LEFT = 0,
        RANDOM_INT_RIGHT = 1000000000 - 1
    };

    std::default_random_engine random_gen{};
    std::uniform_int_distribution<> int_dist(RANDOM_INT_LEFT, RANDOM_INT_RIGHT);
    std::uniform_real_distribution<> real_dist(0, 1);

} // namespace NewGoto

void NewGoto::get_best_k(const ans_t *x, const ans_t *y, int n, int m, int *ans_i, int *ans_j, int k) {
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

namespace NewGoto {

    void set_seed(uint32_t seed) {
        random_gen.seed(seed);
    }

    int rand_int(int l, int r) {
        return int_dist(random_gen) % (r - l + 1) + l;
    }

    float rand_real() {
        return real_dist(random_gen);
    }

} // namespace NewGoto

using namespace NewGoto;

// constructor

NewGotoHeurist::NewGotoHeurist(int m_, int n_, int stepx, int stepy,
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

NewGotoHeurist::~NewGotoHeurist() {
    deallocate_permanent();
}

// memory management

void NewGotoHeurist::allocate_permanent() {
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

void NewGotoHeurist::deallocate_permanent() {
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

void NewGotoHeurist::allocate_temp() {
    median_neib = new int[eps];
    median_vals = new ans_t[eps];

    temp_ans_i = new int[eps];
    temp_ans_j = new int[eps];

    sols = new Solution[eps];
    for (int i = 0; i < eps; ++i) {
        sols[i].perm = new int[devices];
        sols[i].rev_perm = new int[slots];
    }

    M = new Solution[S + 2];
    for (int i = 0; i < S + 2; ++i) {
        M[i].perm = new int[devices];
        M[i].rev_perm = new int[slots];
        M[i].prior = new float[devices];
    }
    gark_buf = new Solution*[GARK_BUF_MAX];

    init_util();
}

void NewGotoHeurist::deallocate_temp() {
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

    for (int i = 0; i < S + 2; ++i) {
        delete[] M[i].perm;
        delete[] M[i].rev_perm;
        delete[] M[i].prior;
    }

    delete[] M;

    delete[] gark_buf;

    free_util();
}


// Improvement
int NewGotoHeurist::idx(int i, int j) const {
    return i * n + j;
}

int NewGotoHeurist::idx_dev(int i, int j) const {
    return i * devices + j;
}

// GFDR
bool NewGotoHeurist::GFDR(NewGotoHeurist::Solution& sol, int device) { // improvement for the device
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

void NewGotoHeurist::copy(const NewGotoHeurist::Solution &from, NewGotoHeurist::Solution &to) const {
    if (!to.perm) {
        to.perm = new int[devices];
    }
    if (!to.rev_perm) {
        to.rev_perm = new int[slots];
    }
    if (!to.prior) {
        to.prior = new float[devices];
    }
    to.twl = from.twl;
    std::memcpy(to.perm, from.perm, devices * sizeof(int));
    std::memcpy(to.rev_perm, from.rev_perm, slots * sizeof(int));
    std::memcpy(to.prior, from.prior, devices * sizeof(float));
}

ans_t NewGotoHeurist::calc_twl(const NewGotoHeurist::Solution &sol) const {
    ans_t twl{0};
    for (int i = 0; i < devices; ++i) {
        for (int j = i + 1; j < devices; ++j) {
            twl += contrib(i, j, sol.perm[i], sol.perm[j]);
        }
    }
    return twl;
}

void NewGotoHeurist::swap(NewGotoHeurist::Solution &sol, int i, int j, ans_t twl_delta) {
    sol.twl += twl_delta;
    std::swap(sol.rev_perm[sol.perm[i]], sol.rev_perm[sol.perm[j]]);
    std::swap(sol.perm[i], sol.perm[j]);
}

ans_t NewGotoHeurist::delta(const NewGotoHeurist::Solution &sol, int i, int j) const {
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

ans_t NewGotoHeurist::contrib(int i, int j, int pos_i, int pos_j) const {
    if (i == j) {
        return 0;
    }
    return contrib(i, j, loc_x[pos_i], loc_x[pos_j], loc_y[pos_i], loc_y[pos_j]);
}

ans_t NewGotoHeurist::contrib(int i, int j, int xi, int xj, int yi, int yj) const {
    return contrib_x(i, j, xi, xj) + contrib_y(i, j, yi, yj);
}

ans_t NewGotoHeurist::contrib_x(int i, int j, int xi, int xj) const {
    int pair_id = idx_dev(i, j);
    return w[pair_id] * abs(xi - xj) + (xi == xj ? same_x[pair_id] : (xi < xj ? left_x[pair_id] : right_x[pair_id]));
}

ans_t NewGotoHeurist::contrib_y(int i, int j, int yi, int yj) const {
    int pair_id = idx_dev(i, j);
    return w[pair_id] * abs(yi - yj) + (yi == yj ? same_y[pair_id] : (yi < yj ? down_y[pair_id] : up_y[pair_id]));
}

void NewGotoHeurist::get_vals(ans_t *vals, int N, int step, const ans_t *pref_w, const ans_t *pref_s) {
    ans_t sum_w{0};
    ans_t sum_s{0};
    for (int i = 0; i < N; ++i) {
        sum_w += pref_w[i];
        sum_s += pref_s[i];
        vals[i] = i * step * sum_w + sum_s;
    }
}

void NewGotoHeurist::get_median(const NewGotoHeurist::Solution &sol, int device) {
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

std::vector<std::pair<double, std::vector<int>>> NewGotoHeurist::get_debug_info() const {
    return debug_info;
}

bool NewGotoHeurist::need_udpate() const {
    return debug_interval != -1 && (int)(clock() - last_time) >= debug_interval;
}

void NewGotoHeurist::update() {
    debug_info.emplace_back((clock() - start_time) / 1e6, std::vector<int>{best.perm, best.perm + devices});
    last_time = clock();
}

std::vector<int> NewGotoHeurist::solve(int n1_param, int n2_param, int S_param, int z_param, int lambda_max_param, int eps_param, double time, double deb_interval, int seed) {
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

    n1 = std::min(n1_param, devices);
    n2 = std::max(n2_param, n1);
    n2 = std::min(n2, devices);
    S = S_param;
    top = (S_param * z_param + 99) / 100;
    eps = std::min(eps_param, m * n);
    lambda_max = lambda_max_param;

    printf("new_goto: seed=%d, debug_interval=%d, "
           "m=%d, n=%d, n1=%d, n2=%d, S=%d, top=%d, eps=%d, lambda=%d\n", seed, debug_interval, m, n, n1, n2, S, top, eps, lambda_max);

    auto max_time = (clock_t)(time * 1e6);

    debug_info.clear();

    start_time = last_time = clock();

    allocate_temp();

    gen_M();

    if (debug_interval != -1) {
        update();
    }

    while ((clock() - start_time) <= max_time) {
        sort_M();
        upd_best();

        if (need_udpate()) {
            update();
        }

        int L = rand_int(0, top - 1);
        ces(M[L]); // heavy
        sort_M();

        gark(rand_int(1, 16), 5); // maybe heavy too
    }

    sort_M();
    upd_best();

    if (need_udpate()) {
        update();
    }

    std::vector<int> ret{best.perm, best.perm + devices};

    deallocate_temp();

    return ret;
}

void NewGotoHeurist::print_sol(const NewGotoHeurist::Solution &sol) const {
    for (int i = 0; i < devices; ++i) {
        printf("%d ", sol.perm[i]);
    }
    printf("\n");
    for (int i = 0; i < devices; ++i) {
        printf("%d ", sol.rev_perm[i]);
    }
    printf("\n");
}

ans_t NewGotoHeurist::contrib(const Solution& sol, int device) const {
    ans_t ret{0};
    for (int i = 0; i < devices; ++i) {
        if (i != device) {
            ret += contrib(device, i, sol.perm[device], sol.perm[i]);
        }
    }
    return ret;
}

// GARK part

void NewGotoHeurist::rand_sol(Solution& s) {
    rand_prior(s.prior);
    get_perm(s.prior, s.perm, s.rev_perm);
    s.twl = calc_twl(s);
}

void NewGotoHeurist::rand_prior(float *prior) const {
    for (int i = 0; i < devices; ++i) {
        prior[i] = rand_real();
    }
}

void NewGotoHeurist::get_perm(const float *prior, int *perm, int *rev_perm) const {
    for (int i = 0; i < devices; ++i) {
        rev_perm[i] = i;
    }
    std::sort(rev_perm, rev_perm + devices, [prior](int i, int j){
        return prior[i] < prior[j];
    });
    for (int i = 0; i < devices; ++i) {
        perm[rev_perm[i]] = i;
    }
}



// local search

void NewGotoHeurist::ls(Solution& s, int iters) {
    bool done = true;
    int it = 0;

    while (done && (iters != -1 && it < iters)) {
        done = false;

        for (int device = 0; device < devices; ++device) {
            if (GFDR(s, device)) {
                done = true;
            }
        }

        ++it;
    }

    if (s.twl < best.twl) {
        copy(s, best);
    }
}

// utils

void NewGotoHeurist::init_util() {
    temp_perm = new int[devices];
    temp_perm_S = new int[S];

    std::iota(temp_perm, temp_perm + devices, 0);
    std::iota(temp_perm_S, temp_perm_S + S, 0);
}

void NewGotoHeurist::free_util() {
    delete[] temp_perm;
    delete[] temp_perm_S;
}

void NewGotoHeurist::sort_M(int pref) {
    if (pref == 0) {
        return;
    }
    if (pref == -1) {
        pref = S;
    }
    std::sort(M, M + pref, [](const Solution &a, const Solution &b) {
        return a.twl < b.twl;
    });
}

void NewGotoHeurist::upd_best() {
    if (M[0].twl < best.twl) {
        copy(M[0], best);
    }
}

void NewGotoHeurist::gen_M() {
    for (int i = 0; i < S; ++i) {
        rand_sol(M[i]);
    }
    copy(M[0], best);
    sort_M();
    upd_best();
}

void NewGotoHeurist::gark(int type, int iters) {
    int sort_pref = 0;
    if (type == 1) {
        gark1(M[S]);

        ls(M[S], iters);

        sort_pref = S + 1;
    } else if (type == 2) {
        gark2(M[rand_int(0, S-1)], M[rand_int(0, S - 1)], M[S], M[S + 1]);

        ls(M[S], iters);
        ls(M[S + 1], iters);

        sort_pref = S + 2;
    } else if (type == 3) {
        int cnt = rand_int(GARK_BUF_MIN, GARK_BUF_MAX);
        std::sort(temp_perm_S, temp_perm_S + S); // generate cnt random vals from [0...top-1]
        std::shuffle(temp_perm_S, temp_perm_S + top, random_gen);
        for (int i = 0; i < cnt; ++i) {
            gark_buf[i] = M + temp_perm_S[i];
        }

        gark3(cnt, M[S]);

        ls(M[S]);

        sort_pref = S + 1;
    }

    if (sort_pref) {
        sort_M(sort_pref);
    }
}

void NewGotoHeurist::gark1(NewGotoHeurist::Solution &s) {
    rand_sol(s);
}

void NewGotoHeurist::gark2(const NewGotoHeurist::Solution &a, const NewGotoHeurist::Solution &b,
                           NewGotoHeurist::Solution &dest_a, NewGotoHeurist::Solution &dest_b) {

    for (int i = 0; i < n; ++i) {
        if (rand_int(0, 1)) { // flip fair coin
            dest_a.prior[i] = a.prior[i];
            dest_b.prior[i] = b.prior[i];
        } else {
            dest_a.prior[i] = b.prior[i];
            dest_b.prior[i] = a.prior[i];
        }
    }

    get_perm(dest_a.prior, dest_a.perm, dest_a.rev_perm);
    dest_a.twl = calc_twl(dest_a);

    get_perm(dest_b.prior, dest_b.perm, dest_b.rev_perm);
    dest_b.twl = calc_twl(dest_b);
}

void NewGotoHeurist::gark3(int cnt, NewGotoHeurist::Solution &dest) {
    for (int i = 0; i < n; ++i) {
        dest.prior[i] = 0;
        for (int j = 0; j < cnt; ++j) {
            dest.prior[i] += gark_buf[j]->prior[i];
        }
        dest.prior[i] /= (float) cnt;
    }

    get_perm(dest.prior, dest.perm, dest.rev_perm);
    dest.twl = calc_twl(dest);
}

void NewGotoHeurist::ces(NewGotoHeurist::Solution &sol) {
    for (int k = n1; k <= n2; ++k) {
        for (int device = 0; device < devices; ++device) {
            GFDR(sol, device);
            if (sol.twl < best.twl) {
                copy(sol, best);
            }
        }

        int p = rand_int(n1, k);
        jump(sol, p);

        if (sol.twl < best.twl) {
            copy(sol, best);
        }
    }
}

void NewGotoHeurist::jump(NewGotoHeurist::Solution &sol, int p) {
    std::shuffle(temp_perm, temp_perm + n, random_gen);
    for (int i = 0; i < p; ++i) {
        int a = temp_perm[i];
        int b = temp_perm[(i + 1) % p];
        swap(sol, a, b, 0);
    }
    sol.twl = calc_twl(sol);
}







