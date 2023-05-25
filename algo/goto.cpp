#include "goto.h"

#include <numeric>
#include <cstring>

// constructors

GotoHeurist::GotoHeurist(int m_, int n_, int stepx, int stepy, 
                const pin_acc_t& leftx, const pin_acc_t& samex, const pin_acc_t& rightx,
                const pin_acc_t& upy, const pin_acc_t& samey, const pin_acc_t& downy,
                const mul_t& mul) {
    m = m_;
    n = n_;
    slots = m * n;
    devices = m * n;
    step_x = stepx;
    step_y = stepy;

    allocate_permanent();

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            int q = idx(i, j);
            
            left_x[q] = leftx[i][j];
            same_x[q] = samex[i][j];
            right_x[q] = rightx[i][j];

            up_y[q] = upy[i][j];
            same_y[q] = samey[i][j];
            down_y[q] = downy[i][j];

            w[q] = mul[i][j];
        } 
    }

    std::iota(best.perm, best.perm + slots, 0);

    for (int j = 0; j < n; ++j) {
        loc_x[j] = step_x * j;
    }

    for (int i = 0; i < m; ++i) {
        loc_y[i] = step_y * i;
    }
}

GotoHeurist::~GotoHeurist() {
    deallocate_permanent();
}

// memory management

void GotoHeurist::allocate_permanent() {
    left_x = new ans_t[slots * slots];
    same_x = new ans_t[slots * slots];
    right_x = new ans_t[slots * slots];

    up_y = new ans_t[slots * slots];
    same_y = new ans_t[slots * slots];
    down_y = new ans_t[slots * slots];

    pref_s_x = new ans_t[n];
    pref_s_y = new ans_t[m];

    pref_w_x = new ans_t[n];
    pref_w_y = new ans_t[m];

    vals_x = new ans_t[n];
    vals_y = new ans_t[m];

    loc_x = new int[n];
    loc_y = new int[m];
}

void GotoHeurist::deallocate_permanent() {
    delete[] left_x;
    delete[] same_x;
    delete[] right_x;

    delete[] up_y;
    delete[] same_y;
    delete[] down_y;

    delete[] pref_s_x;
    delete[] pref_s_y;

    delete[] pref_w_x;
    delete[] pref_w_y;

    delete[] vals_x;
    delete[] vals_y;

    delete[] loc_x;
    delete[] loc_y;    
}

void GotoHeurist::allocate_temp() {
    median_neib = new int[eps];

    temp_ans_i = new int[eps];
    temp_ans_j = new int[eps];
}

void GotoHeurist::deallocate_temp() {
    delete[] median_neib;

    delete[] temp_ans_i;
    delete[] temp_ans_j;
}

// SORG
GotoHeurist::Solution GotoHeurist::SORG() {
    int is_placed[devices]; // placed[device]
    int is_taken[slots]; // taken[slot]
    std::memset(is_placed, 0, devices * sizeof(int));
    std::memset(is_taken, 0, slots * sizeof(int));

    ans_t IOC[devices];
    std::memset(IOC, 0, devices * sizeof(ans_t));

    for (int i = 0; i < devices; ++i) {
        for (int j = 0; j < devices; ++j) {
            if (i != j) {
                IOC[i] -= w[idx_dev(i, j)];
            }
        }
    }

    Solution sol{
        new int[devices], // slots == devices
        0
    };

    for (int i = 0; i < slots; ++i) {
        int dev1{-1}, dev2{-1};
        for (int j = 0; j < devices; ++j) {
            if (is_placed[j]) {
                continue;
            }
            if (dev1 == -1) {
                dev1 = j;
                continue;
            }
            if (IOC[j] >= IOC[dev1]) {
                dev2 = dev1;
                dev1 = j;
            } else if (dev2 == -1 || IOC[j] > IOC[dev2]) {
                dev2 = j;
            }
        }

        int dev{dev1};

        if (rand() % 2) {
            dev = dev2;
        }

        int slot1{-1}, slots2{-1};
        for (int j = 0; j < slots; ++j) {
            if (is_taken[j]) {
                continue;
            }
        }

        is_placed[dev] = 1;
        for (int j = 0; j < devices; ++j) {
            if (j != dev) {
                IOC[j] += w[idx_dev(dev, j)];
            }
        }
    }
} 