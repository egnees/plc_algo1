
//
// Created by Sergei Yakovlev <3gnees@gmail.com> on 28.05.2023.
//

#include "new_goto.h"

#include <chrono>
#include <random>

namespace testing {

    std::mt19937 rnd{(uint32_t) std::chrono::high_resolution_clock::now().time_since_epoch().count()};

    using namespace NewGoto;

    int rand_int(int l, int r) {
        return rnd() % (r - l + 1) + l;
    }

    void set_seed(uint32_t seed) {
        rnd.seed(seed);
    }

    template<typename T>
    void print_matrix(const std::vector<std::vector<T>>& a) {
        for (int i = 0; i < (int)a.size(); ++i) {
            for (int j = 0; j < (int)a[i].size(); ++j) {
                printf("%d ", a[i][j]);
            }
            puts("");
        }
    }

    mul_t random_symmetric_matrix(int m, int n) {
        mul_t ret(m, std::vector<long long>(n, 0));
        for (int i = 0; i < m; ++i) {
            for (int j = i + 1; j < n; ++j) {
                ret[i][j] = ret[j][i] = rand_int(0, 10);
            }
        }
        return ret;
    }

    pin_acc_t random_pin_matrix(int m, int n) {
        pin_acc_t ret(m, std::vector<long long>(n, 0));
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) {
                    ret[i][j] = 0;
                } else {
                    ret[i][j] = rand_int(0, 10);
                }
            }
        }
        return ret;
    }

    pin_acc_t null_matrix(int m, int n) {
        return pin_acc_t(m, std::vector<long long>(n, 0));
    }

    void test_best_k_1() {
        ans_t x[]{-1, 2, 3, 5};
        ans_t y[]{0, 7, 9};
        int k = 10;
        int ans_i[k];
        int ans_j[k];
        get_best_k(x, y, 4, 3, ans_i, ans_j, k);
        for (int q = 0; q < k; ++q) {
            printf("(%d %d) -> %lld\n", ans_i[q], ans_j[q], x[ans_i[q]] + y[ans_j[q]]);
        }
    }

    void test_best_k_2() {
        int n = 500;
        int m = 1000;
        ans_t x[n];
        ans_t y[m];
        for (int i = 0; i < n; ++i) {
            x[i] = rand_int(-10000, 10000);
        }
        for (int j = 0; j < m; ++j) {
            y[j] = rand_int(-10000, 10000);
        }
        int k = 30000;
        int ans_i[k];
        int ans_j[k];
        std::sort(x, x + n);
        std::sort(y, y + m);
        get_best_k(x, y, n, m, ans_i, ans_j, k);
        ans_t real[n * m];
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                real[i * m + j] = x[i] + y[j];
            }
        }
        std::sort(real, real + n * m);
        for (int q = 0; q < k; ++q) {
            assert(real[q] == x[ans_i[q]] + y[ans_j[q]]);
        }
    }

    void test_best_k() {
        test_best_k_1();
        test_best_k_2();
    }

    ans_t calc_twl(int m, int n,
                   int step_x, int step_y,
                   const std::vector<int>& perm,
                   const pin_acc_t& left, const pin_acc_t& same_x,
                   const pin_acc_t& up, const pin_acc_t& same_y,
                   const mul_t& mul) {
        ans_t res{0};
//        printf("m=%d n=%d\n", m, n);
//        printf("step_x=%d, step_y=%d\n", step_x, step_y);
        for (int i = 0; i < m * n; ++i) {
            for (int j = i + 1; j < m * n; ++j) {
                int xi = (perm[i] % n) * step_x;
                int yi = (perm[i] / n) * step_y;
                int xj = (perm[j] % n) * step_x;
                int yj = (perm[j] / n) * step_y;
                res += mul[i][j] * (abs(xi - xj) + abs(yi - yj));

                if (xi == xj) {
                    res += same_x[i][j];
//                    printf("+same_x=%lld\n", same_x[i][j]);
                } else if (xi < xj) {
                    res += left[i][j];
//                    printf("+left[i][j]=%lld\n", left[i][j]);
                } else {
                    res += left[j][i];
                }

                if (yi == yj) {
                    res += same_y[i][j];
//                    printf("+same_y=%lld\n", same_y[i][j]);
                } else if (yi > yj) {
                    res += up[i][j];
                } else {
                    res += up[j][i];
                }

//                printf("(xi, yi, xj, yj) = (%d, %d, %d, %d)\n", xi, yi, xj, yj);
            }
        }
        return res;
    }

    std::vector<int> bf(int m, int n, int step_x, int step_y,
                        const pin_acc_t& left, const pin_acc_t& same_x,
                        const pin_acc_t& up, const pin_acc_t& same_y,
                        const mul_t& mul) {
        int devices = m * n;
        std::vector<int> perm(devices);
        std::iota(perm.begin(), perm.end(), 0);
        auto best_perm = perm;
        auto calc = [&](){
            return calc_twl(m, n, step_x, step_y, perm, left, same_x, up, same_y, mul);
        };
        ans_t best_twl{calc()};
        do {
            ans_t cur_twl{calc()};
            if (cur_twl < best_twl) {
                best_perm = perm;
                best_twl = cur_twl;
            }
        } while (std::next_permutation(perm.begin(), perm.end()));
        return best_perm;
    }

    void test_center_pins() {

        for (int it = 0; it < 100; ++it) {

            int m = 2;
            int n = 4;
            int step_x = 5;
            int step_y = 5;
            auto mul = random_symmetric_matrix(m * n, m * n);
//        puts("mul: ");
//        print_matrix(mul);

            auto left_x = random_pin_matrix(m * n, m * n);
            auto same_x = random_symmetric_matrix(m * n, m * n);

            auto up_y = random_pin_matrix(m * n, m * n);
            auto same_y = random_symmetric_matrix(m * n, m * n);

//        {
//            puts("left_x: ");
//            print_matrix(left_x);
//
//            puts("same_x: ");
//            print_matrix(same_x);
//
//            puts("up_y: ");
//            print_matrix(up_y);
//
//            puts("same_y: ");
//            print_matrix(same_y);
//        }

            NewGotoHeurist solver(m, n, step_x, step_y, left_x, same_x, up_y, same_y, mul);
            auto perm = solver.solve(2, 7, 100, 10, 4, 4, 1, -1, rnd());
            printf("\ngoto_perm:\n");
            for (int i = 0; i < n * m; ++i) {
                printf("%d ", perm[i]);
            }
            printf("\ngoto_perm.twl = %lld\n", calc_twl(m, n, step_x, step_y, perm, left_x, same_x, up_y, same_y, mul));

            printf("\nbf_perm:\n");
            auto bf_perm = bf(m, n, step_x, step_y, left_x, same_x, up_y, same_y, mul);
            for (int i = 0; i < n * m; ++i) {
                printf("%d ", bf_perm[i]);
            }
            printf("\nbf_perm.twl = %lld\n",
                   calc_twl(m, n, step_x, step_y, bf_perm, left_x, same_x, up_y, same_y, mul));
        }
    }

} // testing

int main() {
//    testing::test_best_k();
    testing::test_center_pins();
    return 0;
}