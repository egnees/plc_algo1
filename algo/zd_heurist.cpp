//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 05.05.2023.
//

#include "ZD_heurist_QAP.h"
#include "ZD_heurist_QAP1.h"

#include <vector>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <random>

using cost_t = std::vector<std::vector<std::vector<std::vector<long long>>>>;

int calcObv(const cost_t& cost, std::vector<int>& p) {
    int n = (int) p.size();
    int ret = 0;
    for (int i = 0; i + 1 < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += cost[i][j][p[i]][p[j]];
        }
    }
    return ret;
}

int calcObv(const std::vector<std::vector<int>>& dist, const std::vector<std::vector<int>>& cost, const std::vector<int>& p) {
    int ret = 0;
    int n = (int) dist.size();
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += cost[i][j] * dist[p[i]][p[j]];
        }
    }
    return ret;
}

int brute_force(const cost_t& cost, bool verbose = false) {
    int n = (int) cost.size();
    std::vector<int> p(n);
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }
    auto bestp = p;
    int best = calcObv(cost, p);
    do {
        int cur = calcObv(cost, p);
        if (cur < best) {
            best = cur;
            bestp = p;
        }
    } while (std::next_permutation(p.begin(), p.end()));

    if (verbose) {
        printf("\nbf x: %d\n", best);
        puts("bf result is:");
        for (int i : bestp) {
            printf("%d ", i);
        }
        printf("\n");
    }
    return best;
}

int brute_force(const std::vector<std::vector<int>>& dist, const std::vector<std::vector<int>>& cost, bool verbose = false) {
    int n = (int) dist.size();
    std::vector<int> p(n);
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }
    auto bestp = p;
    int best = calcObv(dist, cost, bestp);
    do {
        int cur = calcObv(dist, cost, p);
        if (cur < best) {
            best = cur;
            bestp = p;
        }
    } while (std::next_permutation(p.begin(), p.end()));

    if (verbose) {
        printf("\nbf x: %d\n", best);
        puts("bf result is:");
        for (int i: p) {
            printf("%d ", i);
        }
        printf("\n");
    }
    return best;
}

int zd_solve(const std::vector<std::vector<int>>& dist, const std::vector<std::vector<int>>& cost, bool verbose = false) {
    ZD_heurist_QAP solver(cost, dist, 2);
    auto result = solver.solve();
    int x = calcObv(dist, cost, result);
    if (verbose) {
        printf("\nzd_solve x: %d\n", x);
        puts("zd_solve result is:");
        for (int i : result) {
            printf("%d ", i);
        }
        printf("\n");
    }
    return x;
}

int zd_solve(const cost_t& cost, bool verbose = false, int k = 2) {
    ZD_heurist_QAP1 solver(cost, k);
    auto result = solver.solve();
    int x = calcObv(cost, result);
    if (verbose) {
        printf("\nzd_solve x: %d\n", x);
        puts("zd_solve result is:");
        for (int i : result) {
            printf("%d ", i);
        }
        printf("\n");
    }
    return x;
}

std::vector<std::vector<int>> random_sym_matrix(int n) {
    std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::vector<std::vector<int>> ret(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret[i][j] = ret[j][i] = rnd() % 10;
        }
    }
    return ret;
}

cost_t not_random_cost_matrix(int n) {
    auto dist = random_sym_matrix(n);
    auto cost = random_sym_matrix(n);
    cost_t ret(n, std::vector<std::vector<std::vector<long long>>>(n, std::vector<std::vector<long long>>(n, std::vector<long long>(n))));
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                for (int l = k + 1; l < n; ++l) {
                    ret[i][j][k][l] = ret[j][i][k][l] = ret[i][j][l][k] = ret[j][i][l][k] = cost[i][j] * dist[k][l];
                }
            }
        }
    }
    return ret;
}

cost_t full_random_cost_matrix(int n) {
    std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
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

void test_heurist_1(int n, int k = 2) {
    auto cost = not_random_cost_matrix(n);
    ZD_heurist_QAP1 solver(cost, k);
    auto result = solver.solve();
    auto real_result = brute_force(cost);
    printf("heurist: %d\nreal: %d\n", calcObv(cost, result), real_result);
}

void test_heurist_2(int n, int k = 2) {
    auto cost = full_random_cost_matrix(n);
    ZD_heurist_QAP1 solver(cost, k);
    auto result = solver.solve();
    for (int i : result) {
        printf("%d ", i);
    }
    puts("");
    auto real_result = brute_force(cost, true);
    printf("\n\nheurist: %d\nreal: %d\n", calcObv(cost, result), real_result);
}

void test3() {
    std::vector<std::vector<int>> dist{
            {0, 3, 5},
            {4, 0, 3},
            {5, 3, 0}
    };
    std::vector<std::vector<int>> cost{
            {0, 1, 2},
            {1, 0, 1},
            {2, 1, 0}
    };
    zd_solve(dist, cost, true);
    brute_force(dist, cost, true);
}

void random_test1(int n) {
    int missed = 0;
    int iter_count = 100;
    int one_iters_count = 50;
    double perc = 0;
    double sum_cpu = 0;
    for (int i = 0; i < iter_count; ++i) {
        auto cost = full_random_cost_matrix(n);
        int best_zd = 1e9;
        auto start = clock();
        for (int it = 0; it < one_iters_count; ++it) {
            best_zd = std::min(best_zd, zd_solve(cost, false, 2));
        }
        auto cpu = (clock() - start) / 1e6;
        sum_cpu += cpu;
        int bf = brute_force(cost, false);
//        printf("bf: %d\n", bf);
//        printf("zd: %d\n", best_zd);
//        printf("cpu: %lf\n", cpu);
        perc += 1.0 * (best_zd - bf) / bf;
        if (best_zd != bf) {
            ++missed;
        }

        if (i % 10 == 0) {
            printf("%0.2g done...\n", 100.0 * (i + 1) / iter_count);
        }
    }

    printf("\n\nrandom_test n=%d, iters=%d\n", n, one_iters_count);
    printf("missed=%lf\navg perc=%lf\n", 100.0 * missed / iter_count, 100.0 * perc / iter_count);
    printf("avg cpu=%lf sec\n\n", sum_cpu / iter_count);
}

void benchmark(int n) {
    int iter_count = 100;
    double sum_cpu = 0;
    for (int i = 0; i < iter_count; ++i) {
        auto dist = random_sym_matrix(n);
        auto cost = random_sym_matrix(n);
        int best_zd = 1e9;
        auto start = clock();
//        for (int it = 0; it < 120; ++it) {
        best_zd = zd_solve(dist, cost, false);
//        }
        auto cpu = (clock() - start) / 1e6;
        sum_cpu += cpu;
//        int bf = brute_force(dist, cost, false);
//        printf("bf: %d\n", bf);
//        printf("zd: %d\n", best_zd);
//        printf("cpu: %lf\n", cpu);

        if (i % 10 == 0) {
            printf("%0.2g done...\n", 100.0 * (i + 1) / iter_count);
        }
    }
    printf("n=%d, avg cpu=%lf sec\n\n", n, sum_cpu / iter_count);
}

void benchmark1(int n, int k = 2) {
    int iter_count = 100;
    double sum_cpu = 0;
    auto cost = full_random_cost_matrix(n);
    for (int i = 0; i < iter_count; ++i) {
        int best_zd = 1e9;
        auto start = clock();
//        for (int it = 0; it < 120; ++it) {
        best_zd = zd_solve(cost, false, k);
//        }
        auto cpu = (clock() - start) / 1e6;
        sum_cpu += cpu;
//        int bf = brute_force(dist, cost, false);
//        printf("bf: %d\n", bf);
//        printf("zd: %d\n", best_zd);
//        printf("cpu: %lf\n", cpu);

        if (i % 10 == 0) {
            printf("%0.2g done...\n", 100.0 * (i + 1) / iter_count);
        }
    }
    printf("n=%d, avg cpu=%lf sec\n\n", n, sum_cpu / iter_count);
}

void testCPU() {
    auto start = clock();
    long long x = 0;
    for (int i = 0; i < 5e8; ++i) {
        x += 1ll * i * i;
    }
    printf("%lld\n", x);
    double cpu = (clock() - start) / 1e6;
    printf("cpu: %lf\n", cpu);
}

int main() {
   random_test1(2);
//    random_test(10);
//    test_heurist_1(4);
//    test_heurist_2(5);
//    test3();
//    random_test(8);
//    random_test(10);
//    testCPU();
    // benchmark(10);
//    benchmark(15);
//    benchmark1(20);
//    benchmark1(30);
//    benchmark1(50);
//    benchmark(50);
    return 0;
}
