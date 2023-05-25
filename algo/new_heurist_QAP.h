#pragma once

#include <vector>
#include <ctime>
#include <cstdint>

struct Sol {
	using ans_t = long long;

	float *prior{nullptr};
	int *perm{nullptr};
	ans_t cost{0};
};

void print_sol(int n, const Sol* sol);

void write_sol(int n, const float *prior, const int* perm, Sol::ans_t cost, Sol *to);

void set_seed(uint32_t s);
int rand_int(int l, int r);
float rand_real();

class NewHeuristQAP {

public:
	using ans_t = Sol::ans_t;
	using vl = std::vector<ans_t>;
	using vvl = std::vector<vl>;
	using vvvl = std::vector<vvl>;
	using vvvvl = std::vector<vvvl>;
	// cost_t[i][j][k][l] is cost between i and j if pos[i] = k, pos[j] = l
	using cost_t = vvvvl;

	explicit NewHeuristQAP(const cost_t &cost);
	~NewHeuristQAP();

	std::vector<int> solve(int n1_new, int n2_new, int tabu_tenure_new, int S_new,
						   int z_new, double max_time_new = -1,
						   int max_iters_new = -1, int seed_new = -1,
						   bool verbose = false, int debug_t = -1); // time in seconds or iters

	std::vector<std::pair<double, std::vector<int>>> get_debug_info() const;

private:
	void init_all(int n1_new, int n2_new, int tabu_tenure_new, int S_new,
				  int z_new, double max_time_new = -1, int max_iters_new = -1, 
				  int seed_new = -1, bool verbose_new = false, int debug_t = -1);
	void free_all();
	bool need_stop(int iter = -1);
	void work();

	// perm
	ans_t cost(const int *perm); // calc cost of perm
	ans_t exchange_delta(const int *perm, 
					   int r, int s); // calc (became - was), if < 0 then improves
	void exchange(Sol *sol, ans_t delta, int r, int s);

	// cets funcional
	void init_cets(); // call to init cets part
	void init_tabu(); // call to clear tabu
	void cets(Sol *s); // critical event tabu search
	void jump(Sol *s, int p); // jump from local optimum
	void jump1(Sol *s, int p); // other jump strategy
	void free_cets(); // call to free cets part

	// cets data
	int *tabu;
	int n1, n2;
	int tabu_tenure;

	// local search
	void ls(Sol *s, int it = -1); // Algorithm1

	// gark functional
	void init_gark(); // call to init gark part
	void gark(int type, int iters = -1); // type in [1, 100]
	void gark1(Sol *dest);
	void gark2(const Sol *a, const Sol *b, Sol *dest_a, Sol *dest_b);
	void gark3(int cnt, Sol *dest); // sols for comb in gark_buf
	void free_gark(); // call to free gark part

	// gark data
	Sol **gark_buf; // len is gark_max
	enum {
		GARK_BUF_MIN = 2,
		GARK_BUF_MAX = 5
	};

	// M functional
	void init_M(); // call to init M
	void gen_M();
	void upd_best();
	void sort_M(int pref = -1);
	void free_M(); // call to free M
	void print_M(int head = -1);

	// M data
	Sol *M; // len is S+2
	Sol *best;
	int S;
	int top; // top solutions from M is RefSet

	// util
	void init_util();
	void rand_sol(Sol* s);
	void rand_prior(float *prior);				  // generate rand prior of len n
	void get_perm(const float *prior, int *perm); // get perm by prior O(n log n)
	int idx(int i, int j, int k, int l);
	int idx(int i, int j);
	void free_util();
	int *temp_perm; // some perm of [0...n-1] 
	int *temp_perm_S; // some perm of [0...S-1]

	int n, n_2, n_3; // n, n^2, n^3
	ans_t *C;

	// stop condition
	clock_t max_time{-1};
	int max_iters{-1};
	bool use_time{false};
	clock_t start_clock{0};

	// random
	uint32_t seed;

	//verbose
	bool verbose{false};

	//debug
	int debug_interval{-1};
	std::vector<std::pair<double, std::vector<int>>> debug_info{};
};