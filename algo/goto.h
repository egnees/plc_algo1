#include <vector>
#include <ctime>

namespace Goto {

    using ans_t = long long;
    using pin_acc_t = std::vector<std::vector<ans_t>>;
    using mul_t = std::vector<std::vector<ans_t>>;

// write minimal k elements from x_i + y_j to ans; k <= n * m required
// works in O(k log n)
// write minimal k elements from x_i + y_j to ans; k <= n * m required
    void get_best_k(const ans_t *x, const ans_t *y, int n, int m, int *ans_i, int *ans_j, int k);

// i-th location corresponds i/n - th row, i%n - th col
// so location (row, col) corresponds to row * n + col

    class GotoHeurist {

        struct Solution {
            int *perm; // i-th device -> perm[i]-th location
            int *rev_perm; // i-th location -> rev_perm[i]-th device
            ans_t twl;
        };

        // creates deep copy of Solution other
        void copy(const Solution &from, Solution &to) const;

        void print_sol(const Solution &sol) const;

        [[nodiscard]] ans_t calc_twl(const Solution &sol) const;

    public:
        GotoHeurist(int m, int n, int stepx, int stepy,
                    const pin_acc_t &leftx, const pin_acc_t &samex,
                    const pin_acc_t &upy, const pin_acc_t &samey,
                    const mul_t &mul); // mul is symmetric matrix, mut[i][i] = 0

        ~GotoHeurist();

        // returns perm where i-th device is located in the perm[i]-th location
        std::vector<int> solve(int lambda_max, int eps, double time, double debug_interval = -1, int seed = -1);

        // returns array debug_info, where debug_info[i] = {time, best_form_perm}
        [[nodiscard]] std::vector<std::pair<double, std::vector<int>>> get_debug_info() const;

    private:
        int lambda_max{}; // >= 2
        int eps{}; // >= 1

        int m, n;
        int slots;
        int devices;
        int step_x, step_y;

        [[nodiscard]] int idx(int i, int j) const; // i * n + j
        [[nodiscard]] int idx_dev(int i, int j) const; // i * devices + j

        ans_t *left_x{}, *same_x{}, *right_x{}; // contribution of pins if i [left/same/right] than j
        ans_t *up_y{}, *same_y{}, *down_y{}; // contribution of pins if i [up/same/down] than j
        ans_t *w{}; // mult for dist between centers

        // Generalized-Force-Directed relaxation
        bool GFDR(Solution &sol, int i); // start local update from i-th device

        void swap(Solution &sol, int i, int j, ans_t twl_delta); // swap i, j and update sol
        [[nodiscard]] ans_t delta(const Solution &sol, int i, int j) const;

        [[nodiscard]] ans_t contrib_x(int i, int j, int xi, int xj) const;

        [[nodiscard]] ans_t contrib_y(int i, int j, int yi, int yj) const;

        [[nodiscard]] ans_t contrib(int i, int j, int xi, int xj, int yi, int yj) const;

        [[nodiscard]] ans_t contrib(int i, int j, int pos_i, int pos_j) const;

        [[nodiscard]] ans_t contrib(const Solution &sol, int device) const;

        // value for i is i*step*{\sum_{j <= i} pref_w_j} + \sum_{j <= i} pref_s_j
        // len(vals) = N
        void get_vals(ans_t *vals, int N, int step, const ans_t *pref_w, const ans_t *pref_s);

        void get_median(const Solution &sol, int device); // write median-eps-[vals/neibs] to [median_vals/median_neib]
        void get_median_1(const Solution &sol, int device); // write medin-eps-[vals/neibs/ to [median_vals/median_neib]

        // Sub-Optimum-Random-Generation
        Solution SORG();

        Solution SORG1();

        // debug
        clock_t start_time{}; // starts with solve()
        clock_t last_time{};
        std::vector<std::pair<double, std::vector<int>>> debug_info{}; // clear with solve()
        [[nodiscard]] bool need_udpate() const;

        void update(); // update debug_info
        int debug_interval{-1};

        // info
        Solution best{}; // best of all launches

        Solution *sols{}; // size = eps

        int *median_neib{}; // size = eps
        ans_t *median_vals{};

        int *temp_ans_i{}; // size = eps
        int *temp_ans_j{}; // size = eps

        int *help_ans_i{};
        int *help_ans_j{};

        ans_t *pref_s_x{}; // pref_s for x, size = n
        ans_t *pref_s_y{}; // pref_s for y, size = m

        ans_t *pref_w_x{}; // pref_w for x, size = n
        ans_t *pref_w_y{}; // pref_w for y, size = m

        ans_t *vals_x{}; // vals for x, size = n
        ans_t *vals_y{}; // vals for y, size = m

        int *loc_x{}; // x coord of i-th location, (i%n)*step_x
        int *loc_y{}; // y coord of i-th location, (i/n)*step_y

        // memory management
        void allocate_permanent();

        void deallocate_permanent();

        void allocate_temp();

        void deallocate_temp();
    };


} // namespace Goto
