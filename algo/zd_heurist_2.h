#pragma once

#include <vector>
#include <utility>

class ZD_heurist_2 { // version to call solve one time
public:
    // cost_t[i][j][k][l] is cost between i and j if pos[i] = k, pos[j] = l
    using cost_t = std::vector<std::vector<std::vector<std::vector<long long>>>>;
    // cost to assign i-th device to j-th pos
    using dev_pos_cost_t = std::vector<std::vector<long long>>;

    /// @brief Z. Drezner, A New Heuristic for the Quadratic Assignment Problem implementation.
    /// @brief Journal of Applied Mathematics and Decision Sciences, 6(3), 2002, 143-153.
    /// @param cost is symmetric zero-diagonal matrix.
    /// @param dist is symmetric zero-diagonal matrix.
    /// @param maxListSize is the maximum size of list0, list1, list2.

    ZD_heurist_2(int n, int maxListSize);

    void set_cost(const cost_t& cost);
    void set_dp_cost(const dev_pos_cost_t& dp_cost);

    /// @brief solves QAP problem using Z. Drezner heuristic
    /// @return permutation \param p where i-th facility assigned to p[i]-th position.
    std::vector<int> solve(int time = -1, int seed = -1, int debug_t = -1, double start_t = 0);

    [[nodiscard]] std::vector<std::pair<double, std::vector<int>>> get_debug_info() const;

    ~ZD_heurist_2();

private:

    long long* C;
    long long* C1;
    int K;
    int d{};

    int n;
    int n2, n3, n4;

    [[nodiscard]] inline int idx(int i, int j, int k, int l) const;
    [[nodiscard]] inline int idx(int i, int j) const;

    struct Solution {
        int* p;
        long long obv;
        int size;

        Solution();
        Solution(int* perm, long long obvPerm, int n);
        Solution(const Solution& other);
        Solution(Solution&& other) noexcept;
        Solution& operator=(const Solution& other);
        Solution& operator=(Solution&& other) noexcept;
        ~Solution();

        void print() const;
    };

    struct SolutionFactory {

        explicit SolutionFactory(int size);

        SolutionFactory() = delete;
        SolutionFactory(const SolutionFactory&) = delete;
        SolutionFactory(SolutionFactory&&) = delete;
        SolutionFactory& operator=(const SolutionFactory&) = delete;
        SolutionFactory& operator=(SolutionFactory&&) = delete;
        ~SolutionFactory();

        Solution* create(const int* p, long long obv);

        void free();
        void freeLast();
        void clear();

        int n;

        std::vector<Solution*> own;
        std::vector<Solution*> freed;
    } solutionFactory;

    struct List {
        Solution** a;
        int size;
        Solution** worst;
        int K;

        void add(Solution* s);
        void clear();

        List();
        explicit List(int k);
        List(const List& other);
        List(List&& other) noexcept;
        List& operator=(const List& other);
        List& operator=(List&& other) noexcept;
        ~List();
    };

    [[nodiscard]] long long obv(const int* w) const;
    [[nodiscard]] int* randPerm(int seed = -1) const;

    [[nodiscard]] long long deltaObv(int r, int s, const int* w) const;
    [[nodiscard]] int deltaP(const int* p, const int* w) const;
    [[nodiscard]] int deltaDeltaP(const int* w, const int* bfs, int j, int k) const;

    void newBfs(List& list0, Solution& bfs, Solution& bfs2);
    void updLists(List& list0, List& list1, List& list2, int dp, const Solution& bfs);
    bool inlist(List& x, Solution* s);
    void QAP_iter(Solution& center, Solution& bfs, Solution& bfs2, List& memory);

    [[nodiscard]] Solution bestMemory(const List& memory) const;

    //debug
    int debug_interval{-1};
    std::vector<std::pair<double, std::vector<int>>> debug_info{};

    //time
    clock_t max_time;
    clock_t start;
};