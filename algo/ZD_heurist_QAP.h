#pragma once

#include <vector>

class ZD_heurist_QAP {
public:
    /// @brief Z. Drezner, A New Heuristic for the Quadratic Assignment Problem implementation.
    /// @brief Journal of Applied Mathematics and Decision Sciences, 6(3), 2002, 143-153.
    /// @param cost is symmetric zero-diagonal matrix.
    /// @param dist is symmetric zero-diagonal matrix.
    /// @param maxListSize is the maximum size of list0, list1, list2.
    ZD_heurist_QAP(const std::vector<std::vector<int>>& cost, const std::vector<std::vector<int>>& dist, int maxListSize);

    /// @brief solves QAP problem using Z. Drezner heuristic
    /// @return permutation \param p where i-th facility assigned to p[i]-th position.
    std::vector<int> solve();

    ~ZD_heurist_QAP();

private:

    int** C;
    int** D;
    int K;
    int d{};

    int n;

    struct Solution {
        int* p;
        int obv;
        int size;

        Solution();
        Solution(int* perm, int obvPerm, int n);
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

        Solution* create(const int* p, int obv);

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

    [[nodiscard]] int obv(const int* w) const;
    [[nodiscard]] int* randPerm() const;

    [[nodiscard]] int deltaObv(int r, int s, const int* w) const;
    [[nodiscard]] int deltaP(const int* p, const int* w) const;
    [[nodiscard]] int deltaDeltaP(const int* w, const int* bfs, int j, int k) const;

    void newBfs(List& list0, Solution& bfs, Solution& bfs2);
    void updLists(List& list0, List& list1, List& list2, int dp, const Solution& bfs);
    bool inlist(List& x, Solution* s);
    void QAP_iter(Solution& center, Solution& bfs, Solution& bfs2, List& memory);

    [[nodiscard]] Solution bestMemory(const List& memory) const;
};