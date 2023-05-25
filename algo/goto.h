#include <vector>

using ans_t = long long;
using pin_acc_t = std::vector<std::vector<ans_t>>;
using mul_t = std::vector<std::vector<ans_t>>;

// write minimal k elements from x_i + y_j to ans; k <= n * m required
// works in O(n log m), where n <= m
void get_best_k(const ans_t *x, const ans_t *y, int n, int m, int *ans_i, int *ans_j, int k); // write minimal k elements from x_i + y_j to ans; k <= n * m required

// i-th location corresponds i/n row, i%n col
// so location (row, col) corresponds to row * n + col

class GotoHeurist {

    struct Solution {
        int *perm; // i-th device -> perm[i]-th location
        ans_t twl;
    };

    // creates deep copy of Solution other
    Solution copy(const Solution& other) const;

    ans_t calc_twl(const Solution& sol) const;

public:
    GotoHeurist(int m, int n, int stepx, int stepy, 
                const pin_acc_t& leftx, const pin_acc_t& samex, const pin_acc_t& rightx,
                const pin_acc_t& upy, const pin_acc_t& samey, const pin_acc_t& downy,
                const mul_t& mul);

    ~GotoHeurist();

    // returns perm where i-th device is located in the perm[i]-th location
    std::vector<int> solve(double time, double debug_interval, int lambda_max, int eps);

    // returns array debug_info, where debug_info[i] = {time, best_form_perm}
    std::vector<double, std::vector<int>> get_debug_info() const;

private:
    int lambda_max; // >= 2
    int eps; // >= 1

    int m, n;
    int slots;
    int devices;
    int step_x, step_y;

    int idx(int i, int j) const; // i * n + j
    int idx_dev(int i, int j) const; // i * devices + j

    ans_t *left_x, *same_x, *right_x; // contribution of pins if i [left/same/right] than j
    ans_t *up_y, *same_y, *down_y; // contribution of pins if i [up/same/down] than j
    ans_t *w; // mult for dist between centers

    // Generalized-Force-Dirrected relaxation
    bool GFDR(Solution *sol, int i); // start local update from i-th device

    void swap(Solution* sol, int i, int j, ans_t d); // swap i, j and update sol
    ans_t delta(Solution* sol, int i, int j) const;

    ans_t contrib_x(int i, int j, int xi, int xj) const;
    ans_t contrib_y(int i, int j, int yi, int yj) const;
    ans_t contrib(int i, int j, int xi, int xj, int yi, int yj) const;

    // value for i is i*step*{\sum_{j <= i} pref_w_j} + \sum_{j <= i} pref_s_j
    void get_vals(ans_t *vals, int cnt, int step, ans_t *pref_w, ans_t *pref_s);

    // Sub-Optimum-Random-Generation
    Solution SORG();

    // debug
    clock_t start_time; // starts with solve()
    std::vector<clock_t, Solution> debug_info; // clear with solve()

    // info
    Solution best;

    int *median_neib; // size = eps

    int *temp_ans_i; // size = eps
    int *temp_ans_j; // size = eps

    ans_t *pref_s_x; // pref_s for x, size = n
    ans_t *pref_s_y; // pref_s for y, size = m

    ans_t *pref_w_x; // pref_w for x, size = n
    ans_t *pref_w_y; // pref_w for y, size = m

    ans_t *vals_x; // vals for x, size = n
    ans_t *vals_y; // vals for y, size = m

    int *loc_x; // x coord of i-th location, (i%n)*step_x
    int *loc_y; // y coord of i-th location, (i/n)*step_y

    // memory management
    void allocate_permanent();
    void deallocate_permanent();

    void allocate_temp();
    void deallocate_temp();
};

