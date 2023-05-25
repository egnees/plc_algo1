#include "new_heurist_QAP.h"

#include <random>
#include <cassert>
#include <chrono>
#include <memory>
#include <cstring>
#include <utility>
#include <algorithm>

// sol

void print_sol(int n, const Sol* sol) {
    for (int i = 0; i < n; ++i) {
        printf("%d ", sol->perm[i] + 1);
    }
    printf(": %lld\n", sol->cost);
}

void write_sol(int n, const float *prior, const int* perm, NewHeuristQAP::ans_t cost, Sol *to) { // to != nullptr
    std::memcpy(to->prior, prior, n * sizeof(float));
    std::memcpy(to->perm, perm, n * sizeof(int));
    to->cost = cost;
}

// random

enum {
    RANDOM_INT_LEFT = 0,
    RANDOM_INT_RIGHT = 1000000000-1
};

std::default_random_engine random_gen{};
std::uniform_real_distribution<> real_dist(0, 1);
std::uniform_int_distribution<> int_dist(RANDOM_INT_LEFT, RANDOM_INT_RIGHT);

void set_seed(uint32_t s) {
    random_gen.seed(s);
}

int rand_int(int l, int r) {
    return l + int_dist(random_gen) % (r - l + 1);
}

float rand_real() {
    return real_dist(random_gen);
}

// NewHeuristQAP

NewHeuristQAP::NewHeuristQAP(const cost_t &cost) {
    n = (int) cost.size();
    n_2 = n * n;
    n_3 = n * n * n;
    C = new ans_t[n_3 * n];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                for (int l = 0; l < n; ++l) {
                    C[idx(i, j, k, l)] = cost[i][j][k][l];
                    assert(cost[i][i][l][k] == 0);
                    assert(cost[i][j][l][k] == cost[j][i][k][l]);
                    if (cost[i][i][l][k] != 0) {
                        return;
                    }
                }
            }
        }
    }
}

NewHeuristQAP::~NewHeuristQAP() {
    delete[] C;
}

std::vector<int> NewHeuristQAP::solve(int n1_new, int n2_new, int tabu_tenure_new, int S_new,
						   int z_new, double max_time_new,
						   int max_iters_new, int seed_new, bool verbose_new, int debug_t) {
    init_all(n1_new, n2_new, tabu_tenure_new, S_new, z_new, max_time_new, max_iters_new, seed_new, verbose_new, debug_t);

    work();

    sort_M();
    upd_best();

    std::vector<int> perm(n);
    for (int i = 0; i < n; ++i) {
        perm[i] = best->perm[i];
    }

    free_all();

    return perm;
}

std::vector<std::pair<double, std::vector<int>>> NewHeuristQAP::get_debug_info() const {
    return debug_info;
}

void NewHeuristQAP::init_all(int n1_new, int n2_new, int tabu_tenure_new, int S_new,
				  int z_new, double max_time_new, int max_iters_new, int seed_new, bool verbose_new, int debug_t) {
    n2_new = std::min(n2_new, n);
    n1_new = std::min(n1_new, n2_new);
    assert(n1_new <= n2_new && n2_new <= n); // check valid
    assert(tabu_tenure_new > 0);
    assert(S_new > 0);
    assert(z_new > 0 && z_new <= 100);
    assert(max_time_new != -1 || max_iters_new != -1);

    n1 = n1_new; // init vars
    n2 = n2_new;
    tabu_tenure = tabu_tenure_new;
    S = S_new;
    top = (z_new * S_new + 99) / 100;
    verbose = verbose_new;

    if (max_iters_new != -1) { // init stop criteria
        assert(max_iters_new > 0);
        max_iters = max_iters_new;
        use_time = false;
    }
    if (max_time_new != -1) {
        assert(max_time_new > 0);
        assert(max_time_new < 1000);
        max_time = static_cast<clock_t>(max_time_new * 1e6);
        use_time = true;
    }

    if (seed_new == -1) { // init seed
        std::mt19937 rnd1{(uint32_t) std::chrono::high_resolution_clock().now().time_since_epoch().count()};
        seed = rnd1();
    } else {
        seed = static_cast<uint32_t>(seed_new);
    }
    set_seed(seed);

    // init parts
    init_M();
    init_cets();
    init_gark();
    init_util();

    //debug
    debug_interval = debug_t;
    debug_info.clear();
}

void NewHeuristQAP::free_all() {
    free_cets();
    free_gark();
    free_M();
    free_util();
}

bool NewHeuristQAP::need_stop(int iter) {
    if (use_time) {
        return clock() - start_clock > max_time;
    } else {
        assert(iter != -1);
        return iter > max_iters;
    }
}

void NewHeuristQAP::work() {
    int iter = 0;
    start_clock = clock();

    gen_M();

    printf("new_heurist: ");
    printf("tabu_tenure=%d, ", tabu_tenure);
    printf("S=%d, ", S);
    printf("top=%d, ", top);
    printf("n1,n2=(%d,%d), ", n1, n2);
    printf("seed=%d, ", seed);
    printf("time=%.2g, ", (double) max_time / 1e6);
    printf("debug_interval=%d\n", debug_interval);

    clock_t last_t = clock();

    if (debug_interval != -1) {
        sort_M();
        upd_best();
        debug_info.emplace_back((clock() - start_clock) / 1e6, std::vector<int>{best->perm, best->perm + n});
    }

    while (!need_stop(iter)) {
        sort_M();
        upd_best();

        int L = rand_int(0, top - 1);
        cets(M + L); // the most heavy part
        sort_M();

        gark(rand_int(1, 100), 5); // maybe here heavy too

        if (debug_interval != -1) {
            clock_t cur_t = clock();
            if (cur_t - last_t >= debug_interval) {
                debug_info.emplace_back((cur_t - start_clock) / 1e6, std::vector<int>{best->perm, best->perm + n});
                last_t = cur_t;
            }
        }

        ++iter;
    }

    if (debug_interval != -1) {
        sort_M();
        upd_best();
        debug_info.emplace_back((clock() - start_clock) / 1e6, std::vector<int>{best->perm, best->perm + n});
    }
}

// perm

NewHeuristQAP::ans_t NewHeuristQAP::cost(const int *perm) {
    ans_t ret = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += C[idx(i, j, perm[i], perm[j])]; 
        }
    }
    return ret;
}

NewHeuristQAP::ans_t NewHeuristQAP::exchange_delta(const int *perm, 
					   int r, int s) {
    ans_t ret = 0;
    for (int i = 0; i < n; ++i) {
        if (i != r && i != s) {
            ret += C[idx(r, i, perm[s], perm[i])] - C[idx(r, i, perm[r], perm[i])]
                     + C[idx(s, i, perm[r], perm[i])] - C[idx(s, i, perm[s], perm[i])];
        }
    }
    ret += C[idx(s, r, perm[r], perm[s])] - C[idx(s, r, perm[s], perm[r])];
    return ret;
}

void NewHeuristQAP::exchange(Sol* sol, ans_t delta, int r, int s) {
    std::swap(sol->perm[r], sol->perm[s]);
    std::swap(sol->prior[r], sol->prior[s]);
    sol->cost += delta;
}

// cets funcional

void NewHeuristQAP::init_cets() {
    tabu = new int[n * n];
}

void NewHeuristQAP::init_tabu() {
    memset(tabu, -tabu_tenure, sizeof(int) * n_2);
}

void NewHeuristQAP::cets(Sol *sol) {
    for (int k = n1; k <= n2; ++k) {
        for (int r = 0; r < n; ++r) {
            for (int s = r + 1; s < n; ++s) {
                ans_t d = exchange_delta(sol->perm, r, s);
                if (d < 0) {
                    exchange(sol, d, r, s);
                    if (sol->cost < best->cost) {
                        write_sol(n, sol->prior, sol->perm, sol->cost, best);
                    }
                } else {
                    assert(sol->cost + d >= best->cost);
                }
            }
        }

        int p = rand_int(n1, k);
        jump(sol, p);
        // jump1(sol, p);

        if (sol->cost < best->cost) {
            write_sol(n, sol->prior, sol->perm, sol->cost, best);
        }
    }
}

void NewHeuristQAP::jump(Sol *s, int p) {
    std::shuffle(temp_perm, temp_perm + n, random_gen);
    for (int i = 0; i < p; ++i) {
        int a = temp_perm[i];
        int b = temp_perm[(i + 1) % p];
        exchange(s, 0, a, b);
    }
    s->cost = cost(s->perm);
}

void NewHeuristQAP::jump1(Sol *s, int p) {
    for (int i = 0; i < p; ++i) {
        int x = rand_int(0, n - 1);
        int y = rand_int(0, n - 1);
        exchange(s, 0, x, y);
    }
    s->cost = cost(s->perm);
}

void NewHeuristQAP::free_cets() {
    delete[] tabu;
}

// local search
void NewHeuristQAP::ls(Sol *s, int iters) {
    bool done = true;
    int it = 0;
    while (done && (iters != -1 && it < iters)) {
        done = false;

        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                ans_t d = exchange_delta(s->perm, i, j);
                if (d < 0) {
                    exchange(s, d, i, j);
                    done = true;
                }
            }
        }

        ++it;
    }

    if (s->cost < best->cost) {
        write_sol(n, s->prior, s->perm, s->cost, best);
    }
}

// GARK
void NewHeuristQAP::init_gark() {
    gark_buf = new Sol*[GARK_BUF_MAX];
}

void NewHeuristQAP::gark(int type, int iters) {
    int sort_pref = 0;
    if (type == 1) {
        gark1(M + S);

        ls(M + S, iters);

        sort_pref = S + 1;
    } else if (type == 2) {
        gark2(M + rand_int(0, S-1), M + rand_int(0, S-1), M + S, M + S + 1);

        ls(M + S, iters);
        ls(M + S + 1, iters);

        sort_pref = S + 2;
    } else if (type == 3) {
        int cnt = rand_int(GARK_BUF_MIN, GARK_BUF_MAX);
        std::sort(temp_perm_S, temp_perm_S + S); // generate cnt random vals from [0...top-1] 
        std::shuffle(temp_perm_S, temp_perm_S + top, random_gen);
        for (int i = 0; i < cnt; ++i) {
            gark_buf[i] = M + temp_perm_S[i];
        }
        
        gark3(cnt, M + S);

        ls(M + S);
        
        sort_pref = S + 1;
    }

    if (sort_pref) {
        sort_M(sort_pref);
    }
}

void NewHeuristQAP::gark1(Sol *s) {
    rand_sol(s);
}

void NewHeuristQAP::gark2(const Sol *a, const Sol *b, Sol *dest_a, Sol *dest_b) {
    for (int i = 0; i < n; ++i) {
        if (rand_int(0, 1)) { // flip fair coin
            dest_a->prior[i] = a->prior[i];
            dest_b->prior[i] = b->prior[i];
        } else {
            dest_a->prior[i] = b->prior[i];
            dest_b->prior[i] = a->prior[i];
        }
    }

    get_perm(dest_a->prior, dest_a->perm);
    dest_a->cost = cost(dest_a->perm);

    get_perm(dest_b->prior, dest_b->perm);
    dest_b->cost = cost(dest_b->perm);
}

void NewHeuristQAP::gark3(int cnt, Sol *dest) {
    for (int i = 0; i < n; ++i) {
        dest->prior[i] = 0;
        for (int j = 0; j < cnt; ++j) {
            dest->prior[i] += gark_buf[j]->prior[i];
        }
        dest->prior[i] /= cnt;
    }

    get_perm(dest->prior, dest->perm);
    dest->cost = cost(dest->perm);
}

void NewHeuristQAP::free_gark() {
    delete[] gark_buf;
}

void NewHeuristQAP::init_M() {
    M = new Sol[S + 2];
    for (int i = 0; i < S + 2; ++i) {
        M[i].prior = new float[n];
        M[i].perm = new int[n];
        M[i].cost = 0;
    }
    best = new Sol();
    best->prior = new float[n];
    best->perm = new int[n];
}

void NewHeuristQAP::gen_M() {
    for (int i = 0; i < S; ++i) {
        rand_sol(M + i);
    }
    write_sol(n, M[0].prior, M[0].perm, M[0].cost, best);
    sort_M();
    upd_best();
}

void NewHeuristQAP::upd_best() {
    // here M is sorted
    if (M[0].cost < best->cost) {
        write_sol(n, M[0].prior, M[0].perm, M[0].cost, best);
    }
}

void NewHeuristQAP::sort_M(int pref) {
    if (pref == 0) {
        return;
    }
    if (pref == -1) {
        pref = S;
    }
    std::sort(M, M + pref, [](const Sol &a, const Sol &b) {
        return a.cost < b.cost;
    });
}

void NewHeuristQAP::free_M() {
    for (int i = 0; i < S + 2; ++i) {
        delete[] M[i].perm;
        delete[] M[i].prior;
    }
    delete[] M;
    delete[] best->prior;
    delete[] best->perm;
    delete best;
}

void NewHeuristQAP::print_M(int head) {
    if (head == -1) {
        head = S;
    }
    for (int i = 0; i < head; ++i) {
        for (int j = 0; j < n; ++j) {
            printf("%d ", M[i].perm[j] + 1);
        }
        printf(": %lld\n", M[i].cost);
    }
}

// util
void NewHeuristQAP::init_util() {
    temp_perm = new int[n];
    for (int i = 0; i < n; ++i) {
        temp_perm[i] = i;
    }
    temp_perm_S = new int[S];
    for (int i = 0; i < S; ++i) {
        temp_perm_S[i] = i;
    }
}

void NewHeuristQAP::rand_sol(Sol* s) {
    rand_prior(s->prior);
    get_perm(s->prior, s->perm);
    s->cost = cost(s->perm);
}

void NewHeuristQAP::rand_prior(float *prior) {
    for (int i = 0; i < n; ++i) {
        prior[i] = rand_real();
    }
}

void NewHeuristQAP::get_perm(const float *prior, int *perm) {
    for (int i = 0; i < n; ++i) {
        temp_perm[i] = i;
    }
    std::sort(temp_perm, temp_perm + n, [prior](int i, int j){
        return prior[i] < prior[j];
    });
    for (int i = 0; i < n; ++i) {
        perm[temp_perm[i]] = i;
    }
}

int NewHeuristQAP::idx(int i, int j, int k, int l) {
    return i * n_3 + j * n_2 + k * n + l;
}

int NewHeuristQAP::idx(int i, int j) {
    return i * n + j;
}

void NewHeuristQAP::free_util() {
    delete[] temp_perm;
    delete[] temp_perm_S;
}


