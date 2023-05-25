#include "ZD_heurist_QAP1.h"

#include <random>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <exception>

// Solution

ZD_heurist_QAP1::Solution::Solution() : p{nullptr}, obv{-1}, size{0} {}

ZD_heurist_QAP1::Solution::Solution(int* perm, long long obvPerm, int n) : p{perm}, obv{obvPerm}, size{n} {}

ZD_heurist_QAP1::Solution::Solution(const ZD_heurist_QAP1::Solution& other) {
    p = new int[other.size];
    memcpy(p, other.p, other.size << 2); // assert that sizeof(int) == 4
    obv = other.obv;
    size = other.size;
}

ZD_heurist_QAP1::Solution::Solution(ZD_heurist_QAP1::Solution&& other) noexcept {
    p = other.p;
    other.p = nullptr;
    obv = other.obv;
    size = other.size;
}

ZD_heurist_QAP1::Solution& ZD_heurist_QAP1::Solution::operator=(const ZD_heurist_QAP1::Solution& other) {
    if (&other == this) {
        return *this;
    }
    if (p == nullptr && other.p != nullptr) {
        p = new int[other.size];
    }
    if (other.p != nullptr) {
        memcpy(p, other.p, other.size << 2);
    }
    obv = other.obv;
    size = other.size;
    return *this;
}

ZD_heurist_QAP1::Solution& ZD_heurist_QAP1::Solution::operator=(ZD_heurist_QAP1::Solution&& other) noexcept {
    // size == other.size
    delete[] p;
    p = other.p;
    other.p = nullptr;
    obv = other.obv;
    size = other.size;
    return *this;
}

ZD_heurist_QAP1::Solution::~Solution() {
    delete[] p;
}

void ZD_heurist_QAP1::Solution::print() const {
    for (int i = 0; i < size; ++i) {
        // printf("%d ", p[i]);
    }
    // printf("\n");
}

// List

ZD_heurist_QAP1::List::List() : a{nullptr}, size{0}, worst{nullptr}, K{0} {}

ZD_heurist_QAP1::List::List(int k) : size{0}, worst{nullptr}, K{k} {
    a = new Solution*[K];
}

ZD_heurist_QAP1::List::List(const ZD_heurist_QAP1::List& other) {
    // puts("List(cost List& other)");
    K = other.K;
    a = new Solution*[K];
    size = other.size;
    for (int i = 0; i < size; ++i) {
        a[i] = new Solution(*other.a[i]);
    }
    worst = a + (other.worst - other.a);
}

ZD_heurist_QAP1::List::List(ZD_heurist_QAP1::List&& other) noexcept {
    K = other.K;
    a = other.a;
    worst = other.worst;
    size = other.size;
    other.a = nullptr;
    other.worst = nullptr;
    other.size = 0;
}

ZD_heurist_QAP1::List& ZD_heurist_QAP1::List::operator=(const ZD_heurist_QAP1::List& other) {
    if (this == &other) {
        return *this;
    }
    delete[] a;
    K = other.K;
    a = new Solution*[K];
    size = other.size;
    for (int i = 0; i < size; ++i) {
        a[i] = new Solution(*other.a[i]);
    }
    worst = a + (other.worst - other.a);
    return *this;
}

ZD_heurist_QAP1::List& ZD_heurist_QAP1::List::operator=(ZD_heurist_QAP1::List&& other) noexcept {
    delete[] a;
    K = other.K;
    a = other.a;
    worst = other.worst;
    size = other.size;
    other.a = nullptr;
    other.worst = nullptr;
    other.size = 0;
    return *this;
}

ZD_heurist_QAP1::List::~List() {
    delete[] a;
}

void ZD_heurist_QAP1::List::add(ZD_heurist_QAP1::Solution* s) {
    a[size] = s;
    if (worst == nullptr || (*worst)->obv < s->obv) {
        worst = a + size;
    }
    ++size;
}

void ZD_heurist_QAP1::List::clear() {
    worst = nullptr;
    size = 0;
}

// ZD_heurist_QAP1

// constructor
ZD_heurist_QAP1::ZD_heurist_QAP1(const cost_t& cost, int maxListSize)
        : K{maxListSize}, d{0}, n{(int) cost.size()}, n2{n * n}, n3{n * n * n},
        n4{n * n * n * n}, solutionFactory{n} {

    C = new long long[n4];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                for (int l = 0; l < n; ++l) {
                    
                    if (cost[i][i][k][l] != 0 || cost[i][j][k][k] != 0) {
                        throw std::runtime_error("Cost not zero diag");
                    }

                    if (cost[i][j][k][l] != cost[j][i][l][k]) {
                        throw std::runtime_error("Cost not symmetric");
                    }

                    C[idx(i, j, k, l)] = cost[i][j][k][l];
                }
            }
        }
    }
}

ZD_heurist_QAP1::~ZD_heurist_QAP1() {
    delete[] C;
}

std::vector<int> ZD_heurist_QAP1::solve(int time, int seed, int debug_t, double start_t) {

    debug_interval = debug_t;

    printf("zd_heurist: ");
    printf("seed=%d, ", seed);
    printf("k=%d, ", K);
    printf("time=%.2g, ", time / 1e6);
    printf("debug_interval=%d\n", debug_interval);

    int* p = randPerm(seed);
    Solution center = Solution(p, obv(p), n);
    Solution bfs = center;

    center.print();
    bfs.print();

    std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    int c = 0;


    bool check_time = false;
    // clock_t max_time;
    // clock_t start;
    if (time != -1) {
        check_time = true;
        max_time = time;
        start = clock();
    }

    clock_t last = clock();

    debug_info.emplace_back(start_t + ((double)(clock() - start)) / 1e6, std::vector<int>{bfs.p, bfs.p + n});

    while (!check_time || (clock() - start) <= max_time) {
        d = n - (rnd() % 3 + 2); // n-4 <= d <= n-2

        if (d <= 0) {
            d = 1;
        }

        Solution bfs2, bfs3;
        List memory;
        QAP_iter(center, bfs2, bfs3, memory); // find new solutions and write it to bfs2 and bfs3

        if (bfs.obv > bfs2.obv) { // found better solution
            c = 0;
            bfs = bfs2;
        }

        ++c;

        if (debug_interval != -1) {
            clock_t cur = clock();
            if ((cur - last) >= debug_interval) {
                debug_info.emplace_back(start_t + ((double)(cur - start)) / 1e6, std::vector<int>{bfs.p, bfs.p + n});
                last = cur;
            }
        }

        if (c == 1 || c == 3) {
            center = bestMemory(memory);
        } else if (c == 2 || c == 4) {
            center = std::move(bfs3);
        } else {
            break;
        }
    }

    if (debug_interval != -1) {
        debug_info.back() = {start_t + ((double)(clock() - start)) / 1e6, std::vector<int>{bfs.p, bfs.p + n}};
    }

    solutionFactory.free();

    return {bfs.p, bfs.p + n};
}

std::vector<std::pair<double, std::vector<int>>> ZD_heurist_QAP1::get_debug_info() const {
    return debug_info;
}

ZD_heurist_QAP1::Solution ZD_heurist_QAP1::bestMemory(const List& memory) const {
    assert(memory.size >= 1);

    int best = 0;

    for (int i = 1; i < memory.size; ++i) {
        if (memory.a[best]->obv > memory.a[i]->obv) {
            best = i;
        }
    }

    return *memory.a[best];
}

void ZD_heurist_QAP1::QAP_iter(Solution& center, Solution& bfs, Solution& bfs2, List& memory) {

    // puts("QAP iter...");

    int dp = 0; // distance between p and bfs

    List list0{K}; // best K permutations with dist = dp
    List list1{K}; // best K permutations with dist = dp+1
    List list2{K}; // best K permutations with dist = dp+2

    list0.add(&center);

    bfs = center;

    // printf("d: %d\n", d);

    while (dp <= d) {

        if ((clock() - start) > max_time) {
            break;
        }

        long long prevObv = bfs.obv;

        newBfs(list0, bfs, bfs2); // find new solutions
        bfs.print();
        bfs2.print();

        if (prevObv != bfs.obv) {
            list1.clear();
            list2.clear();
            dp = 0;
        }

        updLists(list0, list1, list2, dp, bfs); // update list1, list2 and find new solutions

        memory = std::move(list0);

        if (list1.size == 0) {
            list0 = list2;
            list1.clear();
            list2.clear();
            ++dp;
        } else {
            list0 = std::move(list1);
            list1 = std::move(list2);
            list2 = List{K};
        }

        ++dp;
    }
}

bool ZD_heurist_QAP1::inlist(List& x, Solution* s) {
    for (int i = 0; i < x.size; ++i) {
        if (x.a[i]->obv == s->obv && deltaP(x.a[i]->p, s->p) == 0) {
            return false;
        }
    }

    if (x.size < K) {
        x.add(s);
        return true;
    }

    *x.worst = s;

    for (int i = 0; i < x.size; ++i) {
        if ((*x.worst)->obv < x.a[i]->obv) {
            x.worst = x.a + i;
        }
    }

    return true;
}

void ZD_heurist_QAP1::updLists(List& list0, List& list1, List& list2, int dp, const Solution& bfs) {
    // puts("\nbfs:");
    bfs.print();
    for (int i = 0; i < list0.size; ++i) {

        Solution* curSol = list0.a[i];

        // puts("list0 cur sol:");
        curSol->print();

        for (int j = 0; j + 1 < n; ++j) {
            for (int k = j + 1; k < n; ++k) {

                int deltaW = deltaDeltaP(curSol->p, bfs.p, j, k);
                // printf("swap %d, %d\n", j, k);
                // printf("deltaW: %d\n", deltaW);
                if (deltaW <= 0) {
                    continue;
                }

                Solution* newSol = solutionFactory.create(curSol->p, deltaObv(j, k, curSol->p) + curSol->obv);
                std::swap(newSol->p[j], newSol->p[k]);

                bool owned = false;

                if (deltaW == 1) {
                    owned = inlist(list1, newSol);
                } else if (deltaW == 2) {
                    owned = inlist(list2, newSol);
                } else {
                    assert(deltaW <= 2);
                }

                if (!owned) {
                    solutionFactory.freeLast();
                }
            }
        }
    }
}

void ZD_heurist_QAP1::newBfs(List& list0, Solution& bfs, Solution& bfs2) {

    // bfs inited, bfs may be not

    while (true) {

        if ((clock() - start) > max_time) {
            break;
        }

        int found = 0;

        for (int i = 0; i < list0.size; ++i) {

            Solution* curSol = list0.a[i];

            for (int j = 0; j + 1 < n; ++j) {
                for (int k = j + 1; k < n; ++k) {

                    long long obvW = deltaObv(j, k, curSol->p) + curSol->obv;

                    if (obvW < bfs.obv) {
                        bfs = *curSol;
                        std::swap(bfs.p[j], bfs.p[k]);
                        bfs.obv = obvW;
                        found = 1;
                    } else if (bfs2.obv == -1 || obvW < bfs2.obv) {
                        bfs2 = *curSol;
                        std::swap(bfs2.p[j], bfs2.p[k]);
                        bfs2.obv = obvW;
                    }

                }
            }
        }

        if (found) {
            list0.clear();
            list0.add(&bfs);
        } else {
            break;
        }
    }
}

long long ZD_heurist_QAP1::obv(const int* w) const {
    long long ret = 0;
    for (int i = 0; i + 1 < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += C[idx(i, j, w[i], w[j])];
        }
    }
    return ret;
}

int* ZD_heurist_QAP1::randPerm(int seed) const {
    if (seed == -1) {
        std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        seed = rnd();
    }
    std::mt19937 rnd(seed);
    int* p = new int[n];
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }
    std::shuffle(p, p + n, rnd);
    return p;
}

long long ZD_heurist_QAP1::deltaObv(int r, int s, const int* w) const {
    long long ret = 0;
    for (int i = 0; i < n; ++i) {
        if (i != r && i != s) {
            ret += C[idx(r, i, w[s], w[i])] - C[idx(r, i, w[r], w[i])]
                     + C[idx(s, i, w[r], w[i])] - C[idx(s, i, w[s], w[i])];
        }
    }
    ret += C[idx(s, r, w[r], w[s])] - C[idx(s, r, w[s], w[r])];
    return ret;
}

int ZD_heurist_QAP1::deltaP(const int* p, const int* w) const {
    int ret = 0;
    for (int i = 0; i < n; ++i) {
        if (p[i] != w[i]) {
            ++ret;
        }
    }
    return ret;
}

int ZD_heurist_QAP1::deltaDeltaP(const int* w, const int* bfs, int j, int k) const {
    int ret = 0;

    if (w[j] == bfs[j]) {
        ++ret;
    } else if (w[k] == bfs[j]) {
        --ret;
    }

    if (w[k] == bfs[k]) {
        ++ret;
    } else if (w[j] == bfs[k]) {
        --ret;
    }

    return ret;
}

int ZD_heurist_QAP1::idx(int i, int j, int k, int l) const {
    return i * n3 + j * n2 + k * n + l;
}

// SolutionFactory

ZD_heurist_QAP1::SolutionFactory::SolutionFactory(int size) : n{size} {}

ZD_heurist_QAP1::Solution *ZD_heurist_QAP1::SolutionFactory::create(const int* p, long long obv) {
    Solution* ret;
    if (!freed.empty()) {
        ret = freed.back();
        freed.pop_back();
    } else {
        ret = new Solution();
        ret->p = new int[n];
        ret->size = n;
    }
    memcpy(ret->p, p, n << 2);
    ret->obv = obv;
    own.push_back(ret);
    return ret;
}

void ZD_heurist_QAP1::SolutionFactory::free() {
    for (Solution* s : own) {
        freed.push_back(s);
    }
    own.clear();
}

void ZD_heurist_QAP1::SolutionFactory::freeLast() {
    freed.push_back(own.back());
    own.pop_back();
}

void ZD_heurist_QAP1::SolutionFactory::clear() {
    free();
    for (Solution* s : freed) {
        delete s;
    }
}

ZD_heurist_QAP1::SolutionFactory::~SolutionFactory() {
    clear();
}


