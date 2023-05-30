
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 30.05.2023.
//

#include "zd_heurist_2.h"
#include "ZD_heurist_QAP1.h"

#include <random>
#include <chrono>
#include <ctime>

using cost_t = std::vector<std::vector<std::vector<std::vector<long long>>>>;
using dp_cost_t = std::vector<std::vector<long long>>;

cost_t random_cost(int n, int seed = 0) {
    std::mt19937 rnd((uint32_t) seed);
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

dp_cost_t random_dp_cost(int n, int seed = 0) {
    std::mt19937 rnd((uint32_t) seed);
    dp_cost_t ret(n, std::vector<long long>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            ret[i][j] = rnd() % 10;
        }
    }
    return ret;
}

long long calcObv(const cost_t& cost, const dp_cost_t& dp_cost, std::vector<int>& p) {
    int n = (int) p.size();
    long long ret = 0;
    for (int i = 0; i + 1 < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += cost[i][j][p[i]][p[j]];
        }
    }
    for (int i = 0; i < n; ++i) {
        ret += dp_cost[i][p[i]];
    }
    return ret;
}

long long brute_force(const cost_t& cost, const dp_cost_t &dp_cost, bool verbose = false) {
    int n = (int) cost.size();
    std::vector<int> p(n);
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }
    auto bestp = p;
    long long best = calcObv(cost, dp_cost, p);
    do {
        long long cur = calcObv(cost, dp_cost, p);
        if (cur < best) {
            best = cur;
            bestp = p;
        }
    } while (std::next_permutation(p.begin(), p.end()));

    if (verbose) {
        printf("\nbf x: %lld\n", best);
        puts("bf result is:");
        for (int i : bestp) {
            printf("%d ", i);
        }
        printf("\n");
    }
    return best;
}

void cmp_with_old(int n, double time, int seed = 0, int k = 2) {
    std::mt19937 rnd{(uint32_t) seed};
    ZD_heurist_2 solver1(n, k);
    auto cost = random_cost(n, rnd());
    auto dp_cost = dp_cost_t(n, std::vector<long long>(n, 0));

    solver1.set_cost(cost);
    solver1.set_dp_cost(dp_cost);

    auto p1 = solver1.solve(-1, rnd(), -1, 0);

    ZD_heurist_QAP1 solver2(cost, k);

    auto p2 = solver2.solve((int) (1e6 * time), rnd(), -1, 0);
    puts("solver2.solve() done");

    printf("cost(p1) = %lld,\ncost(p2) = %lld\n", calcObv(cost, dp_cost, p1), calcObv(cost, dp_cost, p2));
}

void cmp_with_bf(int n, int seed = 0, int k = 2) {

    std::mt19937 rnd{(uint32_t) seed};

    auto cost = random_cost(n, rnd());
    auto dp_cost = random_dp_cost(n, rnd());

    clock_t start = clock();

    ZD_heurist_2 solver1(n, k);

    solver1.set_cost(cost);
    solver1.set_dp_cost(dp_cost);

    auto best = solver1.solve();
    long long best_cost = calcObv(cost, dp_cost, best);

    for (int i = 0; i < 5; ++i) {
        auto p = solver1.solve();
        long long p_cost = calcObv(cost, dp_cost, p);

        if (p_cost < best_cost) {
            best = p;
            best_cost = p_cost;
        }
    }

    printf("cost(solver1.solve()) = %lld\n", best_cost);

    printf("cpu zd = %.10g\n", (clock() - start) / 1e6);

    start = clock();

    printf("cost(brute_force()) = %lld\n", brute_force(cost, dp_cost));

    printf("cpu bf = %.10g\n", (clock() - start) / 1e6);
}

int main() {
//    cmp_with_old(10, 10, 3281)
    cmp_with_bf(9, 32145, 2);
    return 0;
}