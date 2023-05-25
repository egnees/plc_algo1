#include "ZD_heurist_QAP.h"

#include <random>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <chrono>

// Solution

ZD_heurist_QAP::Solution::Solution() : p{nullptr}, obv{-1}, size{0} {}

ZD_heurist_QAP::Solution::Solution(int* perm, int obvPerm, int n) : p{perm}, obv{obvPerm}, size{n} {}

ZD_heurist_QAP::Solution::Solution(const ZD_heurist_QAP::Solution& other) {
    p = new int[other.size]; 
    memcpy(p, other.p, other.size << 2); // assert that sizeof(int) == 4
    obv = other.obv;
    size = other.size;
}

ZD_heurist_QAP::Solution::Solution(ZD_heurist_QAP::Solution&& other) noexcept {
    p = other.p;
    other.p = nullptr;
    obv = other.obv;
    size = other.size;
}

ZD_heurist_QAP::Solution& ZD_heurist_QAP::Solution::operator=(const ZD_heurist_QAP::Solution& other) {
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

ZD_heurist_QAP::Solution& ZD_heurist_QAP::Solution::operator=(ZD_heurist_QAP::Solution&& other) noexcept {
    // size == other.size
    delete[] p;
    p = other.p;
    other.p = nullptr;
    obv = other.obv;
    size = other.size;
    return *this;
}

ZD_heurist_QAP::Solution::~Solution() {
    delete[] p;
}

void ZD_heurist_QAP::Solution::print() const {
    for (int i = 0; i < size; ++i) {
        // printf("%d ", p[i]);
    }
    // printf("\n");
}

// List

ZD_heurist_QAP::List::List() : size{0}, worst{nullptr}, K{0}, a{nullptr} {}

ZD_heurist_QAP::List::List(int k) : size{0}, worst{nullptr}, K{k} {
    a = new Solution*[K];
}

ZD_heurist_QAP::List::List(const ZD_heurist_QAP::List& other) {
    // puts("List(cost List& other)");
    K = other.K;
    a = new Solution*[K];
    size = other.size;
    for (int i = 0; i < size; ++i) {
        a[i] = new Solution(*other.a[i]);
    }
    worst = a + (other.worst - other.a);
}

ZD_heurist_QAP::List::List(ZD_heurist_QAP::List&& other) noexcept {
    K = other.K;
    a = other.a;
    worst = other.worst;
    size = other.size;
    other.a = nullptr;
    other.worst = nullptr;
    other.size = 0;
}

ZD_heurist_QAP::List& ZD_heurist_QAP::List::operator=(const ZD_heurist_QAP::List& other) {
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

ZD_heurist_QAP::List& ZD_heurist_QAP::List::operator=(ZD_heurist_QAP::List&& other) noexcept {
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

ZD_heurist_QAP::List::~List() {
    delete[] a;
}

void ZD_heurist_QAP::List::add(ZD_heurist_QAP::Solution* s) {
    a[size] = s;
    if (worst == nullptr || (*worst)->obv < s->obv) {
        worst = a + size;
    }
    ++size;
}

void ZD_heurist_QAP::List::clear() {
    worst = nullptr;
    size = 0;
}

// ZD_heurist_QAP

ZD_heurist_QAP::ZD_heurist_QAP(const std::vector<std::vector<int>>& cost, const std::vector<std::vector<int>>& dist, int maxListSize)
    : K{maxListSize}, n{(int) cost.size()}, d{0}, solutionFactory{n} {
    
    C = new int*[n];
    D = new int*[n];
    
    for (int i = 0; i < n; ++i) {
        C[i] = new int[n];
        D[i] = new int[n];

        if (cost[i][i] != 0) {
            // puts("cost-matrix is not zero-diagonal");
            break;
        }
        if (dist[i][i] != 0) {
            // puts("dist-matrix is not zero-diagonal");
            break;
        }

        for (int j = 0; j < n; ++j) {
            C[i][j] = cost[i][j];
            D[i][j] = dist[i][j];

            if (cost[i][j] != cost[j][i]) {
                // puts("cost-matrix is asymmetrical");
                break;
            }
            if (dist[i][j] != dist[j][i]) {
                // puts("dist-matrix is asymmetrical");
                break;
            }

        }
    }
}

ZD_heurist_QAP::~ZD_heurist_QAP() {
    for (int i = 0; i < n; ++i) {
        delete[] C[i];
        delete[] D[i];
    }
    delete[] C;
    delete[] D;
}

std::vector<int> ZD_heurist_QAP::solve() {

    int* p = randPerm();
    Solution center = Solution(p, obv(p), n);
    Solution bfs = center;

    // puts("center:");
    center.print();
    // puts("bfs:");
    bfs.print();

    std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    int c = 0;

    while (true) {
        d = n - (rnd() % 3 + 2); // n-4 <= d <= n-2

        if (d <= 0) {
            d = 1;
        }

        Solution bfs2, bfs3;
        List memory;
        QAP_iter(center, bfs2, bfs3, memory); // find new solutions and write it to bfs2 and bfs3

        // puts("\nafter QAP_iter bfs2:");
        bfs2.print();
        // puts("after QAP_iter bfs3:");
        bfs3.print();

        if (bfs.obv > bfs2.obv) { // found better solution
            c = 0;
            bfs = bfs2;
        }

        ++c;

        if (c == 1 || c == 3) {
            center = bestMemory(memory);
        } else if (c == 2 || c == 4) {
            center = std::move(bfs3);
        } else {
            break;
        }

        // puts("after QAP_iter bfs:");
        bfs.print();
    }

    solutionFactory.free();

    return {bfs.p, bfs.p + n};
}

ZD_heurist_QAP::Solution ZD_heurist_QAP::bestMemory(const List& memory) const {
    assert(memory.size >= 1);

    int best = 0;

    for (int i = 1; i < memory.size; ++i) {
        if (memory.a[best]->obv > memory.a[i]->obv) {
            best = i;
        }
    }

    return *memory.a[best];
}

void ZD_heurist_QAP::QAP_iter(Solution& center, Solution& bfs, Solution& bfs2, List& memory) {

    // puts("QAP iter...");

    int dp = 0; // distance between p and bfs

    List list0{K}; // best K permutations with dist = dp
    List list1{K}; // best K permutations with dist = dp+1 
    List list2{K}; // best K permutations with dist = dp+2

    list0.add(&center);

    bfs = center;

    // printf("d: %d\n", d);

    while (dp <= d) {

        // puts("\nnew QAP_iter...");

        int prevObv = bfs.obv;

        // printf("dp: %d\n", dp);
        // puts("bfs:");
        bfs.print();
        // puts("bfs2:");
        bfs2.print();
        newBfs(list0, bfs, bfs2); // find new solutions
        // puts("new bfs:");
        bfs.print();
        // puts("new bfs2: ");
        bfs2.print();

        if (prevObv != bfs.obv) {
            // updated
            list1.clear();
            list2.clear();
            dp = 0;
        }

        // printf("dp=%d\n", dp);
        // puts("updating lists...");
        // printf("list0.size=%d\n", list0.size);
        // printf("list1.size=%d\n", list1.size);
        // printf("list2.size=%d\n", list2.size);

        updLists(list0, list1, list2, dp, bfs); // update list1, list2 and find new solutions

        // puts("updated lists");
        // printf("list0.size=%d\n", list0.size);
        // printf("list1.size=%d\n", list1.size);
        // printf("list2.size=%d\n", list2.size);
        // puts("moving memory");

        memory = std::move(list0);

        // puts("memory moved");
        // printf("memory.size=%d\n", memory.size);

        // puts("bfs again:");
        bfs.print();
        // puts("bfs2 again:");
        bfs2.print();

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

bool ZD_heurist_QAP::inlist(List& x, Solution* s) {
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

void ZD_heurist_QAP::updLists(List& list0, List& list1, List& list2, int dp, const Solution& bfs) {
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

void ZD_heurist_QAP::newBfs(List& list0, Solution& bfs, Solution& bfs2) {

    // bfs inited, bfs may be not

    while (true) {

        int found = 0;
        
        for (int i = 0; i < list0.size; ++i) {

            Solution* curSol = list0.a[i];

            for (int j = 0; j + 1 < n; ++j) {
                for (int k = j + 1; k < n; ++k) {

                    int obvW = deltaObv(j, k, curSol->p) + curSol->obv;

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

int ZD_heurist_QAP::obv(const int* w) const {
    int ret = 0;
    for (int i = 0; i + 1 < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            ret += C[i][j] * D[w[i]][w[j]];
        }
    }
    return ret;
}

int* ZD_heurist_QAP::randPerm() const {
    std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    int* p = new int[n];
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }
    std::shuffle(p, p + n, rnd);
    return p;
}

int ZD_heurist_QAP::deltaObv(int r, int s, const int* w) const {
    int ret = 0;
    for (int i = 0; i < n; ++i) {
        if (i != r && i != s) {
            ret += (C[i][r] - C[i][s]) * (D[w[i]][w[s]] - D[w[i]][w[r]]);
        }
    }
    return ret;
}

int ZD_heurist_QAP::deltaP(const int* p, const int* w) const {
    int ret = 0;
    for (int i = 0; i < n; ++i) {
        if (p[i] != w[i]) {
            ++ret;
        }
    }
    return ret;
}

int ZD_heurist_QAP::deltaDeltaP(const int* w, const int* bfs, int j, int k) const {
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

// SolutionFactory

ZD_heurist_QAP::SolutionFactory::SolutionFactory(int size) : n{size} {}

ZD_heurist_QAP::Solution *ZD_heurist_QAP::SolutionFactory::create(const int* p, int obv) {
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

void ZD_heurist_QAP::SolutionFactory::free() {
    for (Solution* s : own) {
        freed.push_back(s);
    }
    own.clear();
}

void ZD_heurist_QAP::SolutionFactory::freeLast() {
    freed.push_back(own.back());
    own.pop_back();
}

void ZD_heurist_QAP::SolutionFactory::clear() {
    free();
    for (Solution* s : freed) {
        delete s;
    }
}

ZD_heurist_QAP::SolutionFactory::~SolutionFactory() {
    clear();
}


