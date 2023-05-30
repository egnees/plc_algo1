#include <vector>
#include <cstdio>
#include <random>
#include <chrono>
#include <ctime>
#include <cassert>
#include <thread>

#include "new_heurist_QAP.h"

#include "ZD_heurist_QAP1.h"

using ans_t = long long;
using vl = std::vector<ans_t>;
using vvl = std::vector<vl>;
using vvvl = std::vector<vvl>;
using vvvvl = std::vector<vvvl>;
// cost_t[i][j][k][l] is cost between i and j if pos[i] = k, pos[j] = l
using cost_t = vvvvl;

void print_perm(const std::vector<int>& perm) {
    int n = (int) perm.size();
    printf("perm.size() = %d\n", n);
    for (int i = 0; i < n; ++i) {
        printf("%d ", perm[i] + 1);
    }
    puts("");
}
ans_t calcObv(const cost_t& cost, std::vector<int>& p) {

    int n = (int) p.size();
    ans_t ret = 0;
    for (int i = 0; i + 1 < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += cost[i][j][p[i]][p[j]];
        }
    }
    return ret;
}

ans_t brute_force(const cost_t& cost, bool verbose = false) {
    int n = (int) cost.size();
    std::vector<int> p(n);
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }
    auto bestp = p;
    ans_t best = calcObv(cost, p);
    do {
        int cur = calcObv(cost, p);
        if (cur < best) {
            best = cur;
            bestp = p;
        }
    } while (std::next_permutation(p.begin(), p.end()));

    if (verbose) {
        printf("\nbf x: %lld\n", best);
        puts("bf result is:");
        for (int i : bestp) {
            printf("%d ", i + 1);
        }
        printf("\n");
    }
    return best;
}

cost_t full_random_cost_matrix(int n, uint32_t seed) {
    std::mt19937 rnd(seed);
    cost_t ret(n, std::vector<std::vector<std::vector<long long>>>(n, std::vector<std::vector<long long>>(n, std::vector<long long>(n, 0))));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) {
                continue;
            }
            for (int k = 0; k < n; ++k) {
                for (int l = 0; l < n; ++l) {
                    if (k == l) {
                        continue;
                    }
                    ret[i][j][k][l] = ret[j][i][l][k] = rnd() % 100;
                }
            }
        }
    }
    return ret;
}

ans_t zd_solve(const cost_t &cost, int seed, double time) {
    int k = 2;
    ZD_heurist_2 solver(cost, k);
    auto start = clock();
    ans_t best = 1e18;
    clock_t max_time = time * 1e6;
    while ((clock() - start) <= max_time) {
        clock_t remain = max_time - (clock() - start);
        auto result = solver.solve(std::max(remain, (clock_t)0) / 1e6);
        ans_t x = calcObv(cost, result);
        if (x < best) {
            best = x;
        }
    }
    // printf("zd_time=%.10g\n", (clock() - start) / 1e6);
    return best;
}

ans_t new_qap_solve(const cost_t &cost, int seed, double time) {
    NewHeuristQAP solver(cost);
    auto perm = solver.solve(3, 6, 20, 100, 10, time, -1, seed, false);
    return calcObv(cost, perm);
}

std::vector<std::pair<int, int>> cases;

void compare(double t) {

    double sum_diff = 0;
    int cnt_better = 0;
    int cnt_worse = 0;

    for (auto [n, seed] : cases) {
        auto cost = full_random_cost_matrix(n, seed);

        ans_t new_qap_res = 0;
        ans_t zd_res = 0;

        std::thread t2{[&zd_res, &cost, &t](){
            zd_res = zd_solve(cost, -1, t);
        }};

        std::thread t1{[&new_qap_res, &seed, &cost, &t](){
            new_qap_res = new_qap_solve(cost, seed, t);
        }};

        t1.join();
        t2.join();

        printf("seed=%d, zd=%lld, new_qap=%lld\n", seed, zd_res, new_qap_res);
        sum_diff += static_cast<double>(zd_res - new_qap_res) / std::min(zd_res, new_qap_res);
        if (zd_res < new_qap_res) {
            ++cnt_worse;
        } else if (zd_res > new_qap_res) {
            ++cnt_better;
        }
    }

    printf("\nmean diff = %.10g%%\n", 100 * sum_diff / (int) cases.size());
    printf("cnt_better = %d, cnt_worse = %d\n", cnt_better, cnt_worse);
}

void init_cases(int n, int cnt) {
    cases.clear();
    std::mt19937 rnd(time(NULL));
    for (int i = 0; i < cnt; ++i) {
        cases.emplace_back(n, rnd());
    }
}


int main() {
    // printf("%lld\n", new_qap_solve(full_random_cost_matrix(100, 0), 0, 1, -1));
    init_cases(30, 100);
    compare(2);
    // int seed = 1251215;
    // int n = 11;
    // auto cost = full_random_cost_matrix(n, seed);
    // NewHeuristQAP solver(cost);
    // int iters = 40;
    // auto start = clock();
    // auto perm = solver.solve(3, 6, 5, 200, 10, -1, iters, seed, false);
    // printf("\nelapsed time: %.10g\n", (clock() - start) / 1e6);
    // printf("new_qap_cost: %lld\n", calcObv(cost, perm));
    // brute_force(cost, true);
    return 0;
}