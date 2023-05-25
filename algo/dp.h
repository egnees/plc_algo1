#pragma once

#include <vector>
#include <cstdint>

using ans_t = int64_t;

using mut_t = std::vector<std::vector<ans_t>>;
using pin_add_t = std::vector<std::vector<ans_t>>;
// total_cost is (x[b] - x[a]) * mul + pin_add[a][b]

class SolverDP  {
public:
    SolverDP(const std::vector<int>& locations, const mut_t& cost, const pin_add_t& add);
    ~SolverDP();

    std::vector<int> solve();

private:
    int n; // device count

    int *x; // x-coordinates of locations
    ans_t *mut; // mut[i][j] is total mut between i and j 
    ans_t *add; // add[i][j] is add between i and j in view of their pins

    int idx(int i, int j); // i * n + j
    bool bit(long long msk, int b);

    ans_t *dp; // dynamic programming array
    int *p; // parents array to build ans

    const ans_t INF{(ans_t) 1e18};

    std::vector<int> get_ans() const;
};