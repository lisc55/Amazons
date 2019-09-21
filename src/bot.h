#ifndef BOT

#define BOT

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <queue>
#include <vector>
#include <map>
#include <unordered_map>
using namespace std;

typedef long long LL;
typedef unsigned long long ULL;

//0blank  1black  2white  3obstacle

const int SIZE = 8;
const int dx[] = {1, 1, 0, -1, -1, -1, 0, 1};
const int dy[] = {0, -1, -1, -1, 0, 1, 1, 1};
const int init_pos[8][2] = {{0, 2}, {2, 0}, {5, 0}, {7, 2}, {0, 5}, {2, 7}, {5, 7}, {7, 5}};

ULL timing;

struct state {
	int a[SIZE][SIZE];
	int pos[8][2];
	state() {
		memset(a, 0, sizeof a);
		a[0][2] = a[2][0] = a[5][0] = a[7][2] = 1;
		a[0][5] = a[2][7] = a[5][7] = a[7][5] = 2;
		for (int i = 0; i < 8; i++)pos[i][0] = init_pos[i][0], pos[i][1] = init_pos[i][1];
	}
	int* operator [](int x) {
		return a[x];
	}
};

inline bool out_of_bound(int x, int y) {
	return x < 0 || x >= SIZE || y < 0 || y >= SIZE;
}

struct policy {
	int val;
	policy() {}
	policy(int val): val(val) {}
	policy(int x_0, int y_0, int x_1, int y_1, int x_2, int y_2) {
		val = (x_0 << 15) + (y_0 << 12) + (x_1 << 9) + (y_1 << 6) + (x_2 << 3) + y_2;
	}
	operator int() {
		return val;
	}
};

inline void trans_policy(int p, int &x_0, int &y_0, int &x_1, int &y_1, int &x_2, int &y_2) {
	y_2 = p & 7, p >>= 3;
	x_2 = p & 7, p >>= 3;
	y_1 = p & 7, p >>= 3;
	x_1 = p & 7, p >>= 3;
	y_0 = p & 7, p >>= 3;
	x_0 = p;
}

struct node {
	int vis;
	double val;
	node *fa, *bst_ch;
	map<int, node*> ch;
	int dep, pol;
	bool full;
	node(int pol, node *fa = nullptr): vis(0), val(0.0), fa(fa), bst_ch(nullptr), dep(fa ? fa->dep + 1 : 0), pol(pol), full(0) {}
	~node() {
		for (auto &p : ch)delete p.second;
	}
	node *new_child(int pol) {
		return ch[pol] = new node(pol, this);
	}
	void del_fa() {
		fa->ch[pol] = nullptr;
		delete fa;
		fa = nullptr;
	}
};

struct bot {

	static const int FACT8 = 40320;
	static const int FACT4 = 24;

	int PERM8[FACT8][8];
	int PERM4[FACT4][4];

	int cnt;
	bool placed[FACT8];

	static constexpr double C = 0.9;
	static constexpr double KAPPA = 0.0;
	static constexpr double SIG_THRES = 1;
	static constexpr double THRES = 40;
	static const int TRAIN_UNIT = 100;
	static constexpr double TIME_LIMIT = 0.95;
	static const int D = 5;

	node *root;
	state A;
	int role;

	int Queen_1[SIZE][SIZE], Queen_2[SIZE][SIZE];
	int King_1[SIZE][SIZE], King_2[SIZE][SIZE];
	int L[SIZE][SIZE];
	bool vis[SIZE][SIZE];

	bot(state A, int role): A(A), role(role) {
		root = new node(-1);
		memset(placed, 0, sizeof placed), cnt = 0;
		dfs8(0);
		memset(placed, 0, sizeof placed), cnt = 0;
		dfs4(0);
	}

	~bot() {
		delete root;
	}

	inline double UCB1(node *p);
	inline void update(node *p);
	inline bool is_fully_expanded(node *p);
	inline void move(int pol);
	inline node* random_move(node *p);
	inline void back_propagation(node *p, double eva);
	inline double Delta(int d1, int d2);
	inline void calc_Queen_1();
	inline void calc_Queen_2();
	inline double calc_Q();
	inline double calc_omega();
	inline void calc_King_1();
	inline void calc_King_2();
	inline double calc_K();
	inline void calc_Liberty();
	inline double calc_M();
	inline double sigmoid(double x);
	inline double sigma_1(double x);
	inline double sigma_2(double x);
	inline double sigma_3(double x);
	inline double eval();
	inline void expand(node *p);
	inline bool UCT_search();
	inline void train();
	inline void play(int pol);
	inline int play();
	inline void dfs8(int dep);
	inline void dfs4(int dep);
};

inline double bot::UCB1(node *p) {
	return ((p->dep & 1) == role ? (p->val) : (1 - p->val)) + C * sqrt(log(p->fa->vis) / p->vis);
}

inline void bot::update(node *p) {
	node *bst = nullptr;
	double mx = 0, tmp;
	for (auto &i : p->ch) {
		tmp = UCB1(i.second);
		if (tmp > mx) {
			mx = tmp;
			bst = i.second;
		}
	}
	p->bst_ch = bst;
}

inline bool bot::is_fully_expanded(node *p) {
	if (p->full)return 1;
	for (int id = (p->dep & 1) == role ? 4 : 0, U = (p->dep & 1) == role ? 8 : 4; id < U; id++) {
		int cur_x = A.pos[id][0], cur_y = A.pos[id][1];
		for (int dir1 = 0; dir1 < 8; dir1++) {
			for (int nxt_x = cur_x + dx[dir1], nxt_y = cur_y + dy[dir1]; !out_of_bound(nxt_x, nxt_y) && !A[nxt_x][nxt_y]; nxt_x += dx[dir1], nxt_y += dy[dir1]) {
				for (int dir2 = 0; dir2 < 8; dir2++) {
					for (int arr_x = nxt_x + dx[dir2], arr_y = nxt_y + dy[dir2]; !out_of_bound(arr_x, arr_y) && (!A[arr_x][arr_y] || cur_x == arr_x && cur_y == arr_y); arr_x += dx[dir2], arr_y += dy[dir2]) {
						if (!p->ch.count(policy(cur_x, cur_y, nxt_x, nxt_y, arr_x, arr_y)))return p->full = 0;
					}
				}
			}
		}
	}
	return p->full = 1;
}

inline void bot::move(int pol) {
	if (!~pol)return;
	int x = (pol & 229376) >> 15, y = (pol & 28672) >> 12;
	int role = A[x][y];
	for (int i = 0; i < 8; i++) {
		if (A.pos[i][0] == x && A.pos[i][1] == y) {
			A[A.pos[i][0] = (pol & 3584) >> 9][A.pos[i][1] = (pol & 448) >> 6] = role;
			A[x][y] = 0;
			A[(pol & 56) >> 3][pol & 7] = 3;
			break;
		}
	}
}

inline node* bot::random_move(node *p) {
	if (is_fully_expanded(p))return nullptr;
	int *shuffled_list1 = PERM8[rand() % FACT8];
	int *shuffled_list2 = PERM8[rand() % FACT8];
	int *shuffled_list3 = PERM4[rand() % FACT4];
	bool flag = (p->dep & 1) == role;
	for (int i = 0; i < 4; i++) {
		int id = shuffled_list3[i] + (flag ? 4 : 0); //printf("%d\n",id);
		int cur_x = A.pos[id][0], cur_y = A.pos[id][1];
		for (int dir1 = 0; dir1 < 8; dir1++) {
			int d1 = shuffled_list1[dir1];
			int CNT = 0, POL;
			for (int nxt_x = cur_x + dx[d1], nxt_y = cur_y + dy[d1]; !out_of_bound(nxt_x, nxt_y) && !A[nxt_x][nxt_y]; nxt_x += dx[d1], nxt_y += dy[d1]) {
				for (int dir2 = 0; dir2 < 8; dir2++) {
					int d2 = shuffled_list2[dir2];
					int cnt = 0, pol;
					for (int arr_x = nxt_x + dx[d2], arr_y = nxt_y + dy[d2]; !out_of_bound(arr_x, arr_y) && (!A[arr_x][arr_y] || cur_x == arr_x && cur_y == arr_y); arr_x += dx[d2], arr_y += dy[d2]) {
						int cur_pol = policy(cur_x, cur_y, nxt_x, nxt_y, arr_x, arr_y);
						if (!p->ch.count(cur_pol)) {
							if (rand() % (++cnt) == 0)pol = cur_pol;
						}
					}
					if (cnt) {
						if (rand() % (++CNT) == 0)POL = pol;
					}
				}
			}
			if (CNT)return p->new_child(POL);
		}
	}
	return nullptr;
}

inline void bot::back_propagation(node *p, double eva) {
	while (p) {
		p->val = (p->val * p->vis + eva) / (p->vis + 1);
		p->vis++;
		update(p);
		p = p->fa;
	}
}

inline double bot::Delta(int d1, int d2) {
	if ((!~d1) && (!~d2))return 0.0;
	else if (!~d1)return -1.0;
	else if (!~d2)return 1.0;
	else if (d1 == d2)return KAPPA;
	else if (d1 > d2)return -1.0;
	else return 1.0;
}

struct Qnode {
	int x, y, d;
	Qnode() {}
	Qnode(int x, int y, int d): x(x), y(y), d(d) {}
};

inline void bot::calc_Queen_1() {
	memset(Queen_1, -1, sizeof Queen_1);
	memset(vis, 0, sizeof vis);
	queue<Qnode> Q;
	for (int x, y, i = 0; i < 4; i++) {
		x = A.pos[i][0], y = A.pos[i][1];
		Q.push(Qnode(x, y, 0));
		Queen_1[x][y] = 0;
		vis[x][y] = 1;
	}
	while (!Q.empty()) {
		Qnode cur = Q.front(); Q.pop();
		int x = cur.x, y = cur.y, d = cur.d;
		for (int i = 0; i < 8; i++) {
			for (int _x = x + dx[i], _y = y + dy[i]; !out_of_bound(_x, _y) && !A[_x][_y]; _x += dx[i], _y += dy[i]) {
				if (!vis[_x][_y]) {
					vis[_x][_y] = 1;
					if ((!~Queen_1[_x][_y]) || Queen_1[_x][_y] > d + 1) {
						Queen_1[_x][_y] = d + 1;
					}
					Q.push(Qnode(_x, _y, Queen_1[_x][_y]));
				}
			}
		}
	}
}

inline void bot::calc_Queen_2() {
	memset(Queen_2, -1, sizeof Queen_2);
	memset(vis, 0, sizeof vis);
	queue<Qnode> Q;
	for (int x, y, i = 4; i < 8; i++) {
		x = A.pos[i][0], y = A.pos[i][1];
		Q.push(Qnode(x, y, 0));
		Queen_2[x][y] = 0;
		vis[x][y] = 1;
	}
	while (!Q.empty()) {
		Qnode cur = Q.front(); Q.pop();
		int x = cur.x, y = cur.y, d = cur.d;
		for (int i = 0; i < 8; i++) {
			for (int _x = x + dx[i], _y = y + dy[i]; !out_of_bound(_x, _y) && !A[_x][_y]; _x += dx[i], _y += dy[i]) {
				if (!vis[_x][_y]) {
					vis[_x][_y] = 1;
					if ((!~Queen_2[_x][_y]) || Queen_2[_x][_y] > d + 1) {
						Queen_2[_x][_y] = d + 1;
					}
					Q.push(Qnode(_x, _y, Queen_2[_x][_y]));
				}
			}
		}
	}
}

inline double bot::calc_Q() {
	calc_Queen_1();
	calc_Queen_2();
	double sum = 0.0;
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			sum += Delta(Queen_1[i][j], Queen_2[i][j]);
		}
	}
	return sum;
}

inline double bot::calc_omega() {
	double sum = 0.0;
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if ((~Queen_1[i][j]) && (~Queen_2[i][j])) {
				sum += pow(2, -abs(Queen_1[i][j] - Queen_2[i][j]));
			}
		}
	}
	return sum;
}

inline void bot::calc_King_1() {
	memset(King_1, -1, sizeof King_1);
	memset(vis, 0, sizeof vis);
	queue<Qnode> Q;
	for (int x, y, i = 0; i < 4; i++) {
		x = A.pos[i][0], y = A.pos[i][1];
		Q.push(Qnode(x, y, 0));
		King_1[x][y] = 0;
		vis[x][y] = 1;
	}
	while (!Q.empty()) {
		Qnode cur = Q.front(); Q.pop();
		int x = cur.x, y = cur.y, d = cur.d;
		for (int i = 0; i < 8; i++) {
			int _x = x + dx[i], _y = y + dy[i];
			if (!out_of_bound(_x, _y) && !A[_x][_y] && !vis[_x][_y]) {
				vis[_x][_y] = 1;
				if ((!~King_1[_x][_y]) || King_1[_x][_y] > d + 1) {
					King_1[_x][_y] = d + 1;
				}
				Q.push(Qnode(_x, _y, King_1[_x][_y]));
			}
		}
	}
}

inline void bot::calc_King_2() {
	memset(King_2, -1, sizeof King_2);
	memset(vis, 0, sizeof vis);
	queue<Qnode> Q;
	for (int x, y, i = 4; i < 8; i++) {
		x = A.pos[i][0], y = A.pos[i][1];
		Q.push(Qnode(x, y, 0));
		King_2[x][y] = 0;
		vis[x][y] = 1;
	}
	while (!Q.empty()) {
		Qnode cur = Q.front(); Q.pop();
		int x = cur.x, y = cur.y, d = cur.d;
		for (int i = 0; i < 8; i++) {
			int _x = x + dx[i], _y = y + dy[i];
			if (!out_of_bound(_x, _y) && !A[_x][_y] && !vis[_x][_y]) {
				vis[_x][_y] = 1;
				if ((!~King_2[_x][_y]) || King_2[_x][_y] > d + 1) {
					King_2[_x][_y] = d + 1;
				}
				Q.push(Qnode(_x, _y, King_2[_x][_y]));
			}
		}
	}
}

inline double bot::calc_K() {
	calc_King_1();
	calc_King_2();
	double sum = 0.0;
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			sum += Delta(King_1[i][j], King_2[i][j]);
		}
	}
	return sum;
}

inline void bot::calc_Liberty() {
	memset(L, 0, sizeof L);
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			for (int dir = 0; dir < 8; dir++) {
				if (!out_of_bound(i + dx[dir], j + dy[dir]) && !A[i + dx[dir]][j + dy[dir]]) {
					L[i][j]++;
				}
			}
		}
	}
}

inline double bot::calc_M() {
	calc_Liberty();
	double sum = 0;
	for (int x, y, i = 0; i < 4; i++) {
		x = A.pos[i][0], y = A.pos[i][1];
		double m = 0.0;
		for (int dir = 0; dir < 8; dir++) {
			for (int _x = x + dx[dir], _y = y + dy[dir], len = 1; !out_of_bound(_x, _y) && !A[_x][_y]; _x += dx[dir], _y += dy[dir], len++) {
				if (~Queen_2[_x][_y]) {
					m += pow(2, -len) * L[_x][_y];
				}
			}
		}
		sum -= 30 / (5 + m);
	}
	for (int x, y, i = 4; i < 8; i++) {
		x = A.pos[i][0], y = A.pos[i][1];
		double m = 0.0;
		for (int dir = 0; dir < 8; dir++) {
			for (int _x = x + dx[dir], _y = y + dy[dir], len = 1; !out_of_bound(_x, _y) && !A[_x][_y]; _x += dx[dir], _y += dy[dir], len++) {
				if (~Queen_1[_x][_y]) {
					m += pow(2, -len) * L[_x][_y];
				}
			}
		}
		sum += 30 / (5 + m);
	}
	return sum;
}

inline double bot::sigmoid(double x) { //\R -> (0,1)
	return 1.0 / (1.0 + exp(-SIG_THRES * x));
}

inline double bot::sigma_1(double x) {
	return 1.0;
}

inline double bot::sigma_2(double x) {
	return min(x / THRES, 1.0);
}

inline double bot::sigma_3(double x) {
	return 0.8 * min(x / THRES, 1.0);
}

inline double bot::eval() {
	double Q = calc_Q(); //printf("Q:%.3lf\n",Q);
	double K = calc_K(); //printf("K:%.3lf\n",K);
	double M = calc_M(); //printf("M:%.3lf\n",M);
	double omega = calc_omega(); //printf("omega:%.3lf\n",omega);
	return sigmoid(sigma_1(omega) * Q + sigma_2(omega) * K + sigma_3(omega) * M);
}

inline void bot::expand(node *p) {
	node *t = p, *ch;
	for (int i = 0; i < D; i++) {
		ch = random_move(t);
		if (!ch)break;
		t = ch;
		move(t->pol);
	}
	if (!p->ch.empty()) {
		delete p->ch.begin()->second;
		p->full = 0;
		p->ch.clear();
	}
	back_propagation(p, eval());
}

inline bool bot::UCT_search() {
	state copy = A;
	node *p = root;
	for (; p; p = p->bst_ch) {
		if (p != root)move(p->pol);
		if (!is_fully_expanded(p))break;
	}
	if (!p)return swap(A, copy), 0;
	p = random_move(p);
	move(p->pol);
	expand(p);
	swap(A, copy);
	return 1;
}

inline void bot::train() {
	timing = clock() + int(0.95 * CLOCKS_PER_SEC);
	while ((unsigned)clock() < timing) {
		for (int i = 0; i < TRAIN_UNIT; i++) {
			if (!UCT_search())return;
		}
	}
}

inline void bot::play(int pol) {
	if (!root->ch.count(pol))root = root->new_child(pol);
	else root = root->ch[pol];
	root->del_fa();
	move(root->pol);
}

inline int bot::play() {
	train();
	node *nxt = nullptr;
	if (root->ch.empty())return -1;
	for (auto &p : root->ch) {
		if (!nxt || p.second->vis > nxt->vis)nxt = p.second;
	}
	if (!nxt)return -1;
	root = nxt;
	root->del_fa();
	move(root->pol);
	return root->pol;
}

inline void bot::dfs8(int dep) {
	if (dep >= 8) {
		cnt++;
		for (int i = 0; i < 8; i++)PERM8[cnt][i] = PERM8[cnt - 1][i];
		return;
	}
	for (int i = 0; i < 8; i++) {
		if (!placed[i]) {
			placed[i] = 1;
			PERM8[cnt][dep] = i;
			dfs8(dep + 1);
			placed[i] = 0;
		}
	}
}

inline void bot::dfs4(int dep) {
	if (dep >= 4) {
		cnt++;
		for (int i = 0; i < 4; i++)PERM4[cnt][i] = PERM4[cnt - 1][i];
		return;
	}
	for (int i = 0; i < 4; i++) {
		if (!placed[i]) {
			placed[i] = 1;
			PERM4[cnt][dep] = i;
			dfs4(dep + 1);
			placed[i] = 0;
		}
	}
}

#endif
