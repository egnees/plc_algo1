#include "dp.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cstring>

SolverDP::SolverDP(const std::vector<int>& locations, const mut_t& cost, const pin_add_t& add_pins) {
    n = (int) locations.size();
    if ((int) cost.size() != n) {
        throw std::invalid_argument("cost.shape != loc.size");
    }
    
    x = new int[n];
    std::memcpy(x, &locations.front(), n * sizeof(int));
    std::sort(x, x + n);

    printf("x: ");
    for (int i = 0; i < n; ++i) {
        printf("%d ", x[i]);
    }
    puts("");

    mut = new ans_t[n * n];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            mut[idx(i, j)] = cost[i][j];
        }
    }

    add = new ans_t[n * n];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            add[idx(i, j)] = add_pins[i][j];
        }
    }

    dp = new ans_t[1ll << n];

    p = new int[1ll << n];
}

SolverDP::~SolverDP() {
    delete[] x;
    delete[] mut;
    delete[] dp;
    delete[] p;
}

int SolverDP::idx(int i, int j) {
    return i * n + j;
}

bool SolverDP::bit(long long msk, int b) {
    return (msk >> b) & 1;
}

std::vector<int> SolverDP::solve() {
    std::fill(dp, dp + (1ll << n), INF);
    std::fill(p, p + (1ll << n), -1);

    dp[0] = 0;
    p[0] = 0;

    std::vector<int> conn[n];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (mut[idx(i, j)] != 0) {
                conn[i].push_back(j);
            }
        }
    }

    for (long long msk = 1; msk < (1ll << n); ++msk) {
        int placed{0};
        
        for (int i = 0; i < n; ++i) {
            if (bit(msk, i)) {
                ++placed;
            }    
        }

        int cur_x = x[placed-1];

        for (int i = 0; i < n; ++i) {
            if (!bit(msk, i)) {
                continue;
            }
            ans_t contribution{0};
            for (int j : conn[i]) {
                if (bit(msk, j)) { // j ... i
                    contribution += mut[idx(i, j)] * cur_x + add[idx(j, i)];
                } else { // i ... j
                    contribution -= mut[idx(i, j)] * cur_x;
                }
            }
            if (dp[msk] > dp[msk ^ (1ll << i)] + contribution) {
                dp[msk] = dp[msk ^ (1ll << i)] + contribution;
                p[msk] = i;
            }
        }
    }

    return get_ans();
}

std::vector<int> SolverDP::get_ans() const {
    std::vector<int> perm{n};
    long long msk = (1ll << n) - 1;
    while (msk) {
        int i = p[msk];
        perm.push_back(i);
        msk ^= 1ll << i;
    }
    std::reverse(perm.begin(), perm.end());
    return perm;
}