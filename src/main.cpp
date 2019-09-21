#include <ncurses.h>
#include <sys/stat.h>
#include <list>
#include "bot.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

char *main_choices[] = {
	"1. New ",
	"2. Load",
	"3. Exit"
};

char *player_choices[] = {
	"1. Man ",
	"2. Bot ",
	"3. Back"
};

char *exiting_choices[] = {
	"1. Save  ",
	"2. Delete",
	"3. Cancel"
};

char *replace_choices[] = {
	"1. Replace",
	"2. Cancel "
};

char *modify_choices[] = {
	"Open  ",
	"Delete",
	"Rename"
};

int n_main_choices = sizeof(main_choices) / sizeof(char *);
int n_player_choices = sizeof(player_choices) / sizeof(char *);
int n_exiting_choices = sizeof(exiting_choices) / sizeof(char *);
int n_replace_choices = sizeof(replace_choices) / sizeof(char *);
int n_modify_choices = sizeof(modify_choices) / sizeof(char *);

const int LEN = 500;
const int MAXLEN = 20;

int Y, X;

state A;

int p1, p2; // 0 man 1 bot

struct file_info {
	string file_name;
	int file_id;
	file_info() {}
	file_info(string file_name, int file_id): file_name(file_name), file_id(file_id) {}
};

char c_name[LEN];
string cur_name;
list<file_info> files;
int max_name = -1;

inline void Exit();

inline bool finished(int turn) {
	int x, y;
	if (turn == 1) {
		for (int id = 0; id < 4; id++) {
			x = A.pos[id][0], y = A.pos[id][1];
			for (int dir = 0; dir < 8; dir++) {
				if (!out_of_bound(x + dx[dir], y + dy[dir]) && !A[x + dx[dir]][y + dy[dir]]) {
					return 0;
				}
			}
		}
	}
	else if (turn == 2) {
		for (int id = 4; id < 8; id++) {
			x = A.pos[id][0], y = A.pos[id][1];
			for (int dir = 0; dir < 8; dir++) {
				if (!out_of_bound(x + dx[dir], y + dy[dir]) && !A[x + dx[dir]][y + dy[dir]]) {
					return 0;
				}
			}
		}
	}
	return 1;
}

inline void maketitle() {
	int s = Y * 0.2, t = (X - 81) / 2;
	mvprintw(s, t, "________  _____ ______   ________  ________  ________  ________   ________       ");
	mvprintw(s + 1, t, "|\\   __  \\|\\   _ \\  _   \\|\\   __  \\|\\_____  \\|\\   __  \\|\\   ___  \\|\\   ____\\     ");
	mvprintw(s + 2, t, "\\ \\  \\|\\  \\ \\  \\\\\\__\\ \\  \\ \\  \\|\\  \\\\|___/  /\\ \\  \\|\\  \\ \\  \\\\ \\  \\ \\  \\___|_    ");
	mvprintw(s + 3, t, " \\ \\   __  \\ \\  \\\\|__| \\  \\ \\   __  \\   /  / /\\ \\  \\\\\\  \\ \\  \\\\ \\  \\ \\_____  \\   ");
	mvprintw(s + 4, t, "  \\ \\  \\ \\  \\ \\  \\    \\ \\  \\ \\  \\ \\  \\ /  /_/__\\ \\  \\\\\\  \\ \\  \\\\ \\  \\|____|\\  \\  ");
	mvprintw(s + 5, t, "   \\ \\__\\ \\__\\ \\__\\    \\ \\__\\ \\__\\ \\__\\\\________\\ \\_______\\ \\__\\\\ \\__\\____\\_\\  \\ ");
	mvprintw(s + 6, t, "    \\|__|\\|__|\\|__|     \\|__|\\|__|\\|__|\\|_______|\\|_______|\\|__| \\|__|\\_________\\");
	mvprintw(s + 7, t, "                                                                     \\|_________|");
	refresh();
}

inline void raw_print(state A) {
	move(0, 0);
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (A[j][i] == 0)printw(". ");
			else if (A[j][i] == 1)printw("B ");
			else if (A[j][i] == 2)printw("W ");
			else if (A[j][i] == 3)printw("X ");
		}
		printw("\n");
	}
	printw("\n");
	for (int i = 0; i < 8; i++)printw("%d %d\n", A.pos[i][1], A.pos[i][0]);
	//getch();
}

inline vector<vector<bool>> get_valid(int cur_x, int cur_y, int pre_x = -1, int pre_y = -1) {
	vector<vector<bool>> ret(8, vector<bool>(8));
	for (int i = 0; i < 8; i++) {
		for (int nxt_x = cur_x + dx[i], nxt_y = cur_y + dy[i]; !out_of_bound(nxt_x, nxt_y) && (!A[nxt_x][nxt_y] || nxt_x == pre_x && nxt_y == pre_y); nxt_x += dx[i], nxt_y += dy[i]) {
			ret[nxt_x][nxt_y] = 1;
		}
	}
	return ret;
}

inline void print_board(int turn, int highlight_x = -1, int highlight_y = -1, int cur_x = -1, int cur_y = -1, int nxt_x = -1, int nxt_y = -1, int check_x = -1, int check_y = -1) {
	clear();
	vector<vector<bool>> valid(8, vector<bool>(8));
	if (~check_x)valid = get_valid(check_x, check_y, cur_x, cur_y);
	mvprintw(int(Y * 0.2), int(X * 0.35), "%s: ", cur_name.c_str());
	mvprintw(int(Y * 0.7) - 2, (X - 21) / 2, "Player 1 (Black): %s", p1 == 0 ? "Man" : "Bot");
	mvprintw(int(Y * 0.7) - 1, (X - 21) / 2, "Player 2 (White): %s", p2 == 0 ? "Man" : "Bot");
	mvprintw(int(Y * 0.7) + 1, (X - 12) / 2, "%s moves. ", turn == 2 ? "White" : "Black");
	mvprintw(int(Y * 0.7) + 5, (X - 18) / 2, "Press Esc to exit. ");
	int s = (Y - 18) * 0.4, t = (X - 37) / 2;
	mvprintw(s, t, "    0   1   2   3   4   5   6   7   x");
	mvprintw(s + 1, t, "  ┌");
	for (int i = 0; i < SIZE - 1; i++)printw("───┬"); printw("───┐");
	for (int i = 0; i < SIZE - 1; i++) {
		mvprintw(s + 2 + i * 2, t, "%d ", i);
		for (int j = 0; j < SIZE; j++) {
			printw("│");
			bool highlighted = (highlight_x == j && highlight_y == i), cur = (cur_x == j && cur_y == i), nxt = (nxt_x == j && nxt_y == i), check = valid[j][i];
			if (highlighted)attron(A_REVERSE);
			else if (cur)attron(COLOR_PAIR(1));
			else if (nxt)attron(COLOR_PAIR(2));
			else if (check)attron(COLOR_PAIR(3));
			if (A[j][i] == 1)printw(" ○ ");
			else if (A[j][i] == 2)printw(" ● ");
			else if (A[j][i] == 3)printw(" ✕ ");
			else printw("   ");
			if (highlighted)attroff(A_REVERSE);
			else if (cur)attroff(COLOR_PAIR(1));
			else if (nxt)attroff(COLOR_PAIR(2));
			else if (check)attroff(COLOR_PAIR(3));
		}
		printw("│ %d", i);
		mvprintw(s + 3 + i * 2, t, "  ├");
		for (int j = 0; j < SIZE - 1; j++)printw("───┼"); printw("───┤");
	}
	mvprintw(s + 2 + (SIZE - 1) * 2, t, "%d ", SIZE - 1);
	for (int j = 0; j < SIZE; j++) {
		printw("│");
		bool highlighted = (highlight_x == j && highlight_y == SIZE - 1), cur = (cur_x == j && cur_y == SIZE - 1), nxt = (nxt_x == j && nxt_y == SIZE - 1), check = valid[j][SIZE - 1];
		if (highlighted)attron(A_REVERSE);
		else if (cur)attron(COLOR_PAIR(1));
		else if (nxt)attron(COLOR_PAIR(2));
		else if (check)attron(COLOR_PAIR(3));
		if (A[j][SIZE - 1] == 1)printw(" ○ ");
		else if (A[j][SIZE - 1] == 2)printw(" ● ");
		else if (A[j][SIZE - 1] == 3)printw(" ✕ ");
		else printw("   ");
		if (highlighted)attroff(A_REVERSE);
		else if (cur)attroff(COLOR_PAIR(1));
		else if (nxt)attroff(COLOR_PAIR(2));
		else if (check)attroff(COLOR_PAIR(3));
	}
	printw("│ %d", SIZE - 1);
	mvprintw(s + 3 + (SIZE - 1) * 2, t, "  └");
	for (int i = 0; i < SIZE - 1; i++)printw("───┴"); printw("───┘");
	mvprintw(s + 4 + (SIZE - 1) * 2, t, "y   0   1   2   3   4   5   6   7  ");
	refresh();
//	raw_print(A);
}

inline void print_board_bot(int turn, int cur_x, int cur_y, int nxt_x, int nxt_y) {
	clear();
	mvprintw(int(Y * 0.2), int(X * 0.35), "%s: ", cur_name.c_str());
	mvprintw(int(Y * 0.7) - 2, (X - 21) / 2, "Player 1 (Black): %s", p1 == 0 ? "Man" : "Bot");
	mvprintw(int(Y * 0.7) - 1, (X - 21) / 2, "Player 2 (White): %s", p2 == 0 ? "Man" : "Bot");
	mvprintw(int(Y * 0.7) + 1, (X - 12) / 2, "%s moves. ", turn == 2 ? "White" : "Black");
	mvprintw(int(Y * 0.7) + 5, (X - 18) / 2, "Press Esc to exit. ");
	int s = (Y - 18) * 0.4, t = (X - 37) / 2;
	mvprintw(s, t, "    0   1   2   3   4   5   6   7   x");
	mvprintw(s + 1, t, "  ┌");
	for (int i = 0; i < SIZE - 1; i++)printw("───┬"); printw("───┐");
	for (int i = 0; i < SIZE - 1; i++) {
		mvprintw(s + 2 + i * 2, t, "%d ", i);
		for (int j = 0; j < SIZE; j++) {
			printw("│");
			bool cur = (cur_x == j && cur_y == i), nxt = (nxt_x == j && nxt_y == i);
			if (cur)attron(COLOR_PAIR(1));
			else if (nxt)attron(COLOR_PAIR(3));
			if (A[j][i] == 1)printw(" ○ ");
			else if (A[j][i] == 2)printw(" ● ");
			else if (A[j][i] == 3)printw(" ✕ ");
			else printw("   ");
			if (cur)attroff(COLOR_PAIR(1));
			else if (nxt)attroff(COLOR_PAIR(3));
		}
		printw("│ %d", i);
		mvprintw(s + 3 + i * 2, t, "  ├");
		for (int j = 0; j < SIZE - 1; j++)printw("───┼"); printw("───┤");
	}
	mvprintw(s + 2 + (SIZE - 1) * 2, t, "%d ", SIZE - 1);
	for (int j = 0; j < SIZE; j++) {
		printw("│");
		bool cur = (cur_x == j && cur_y == SIZE - 1), nxt = (nxt_x == j && nxt_y == SIZE - 1);
		if (cur)attron(COLOR_PAIR(1));
		else if (nxt)attron(COLOR_PAIR(3));
		if (A[j][SIZE - 1] == 1)printw(" ○ ");
		else if (A[j][SIZE - 1] == 2)printw(" ● ");
		else if (A[j][SIZE - 1] == 3)printw(" ✕ ");
		else printw("   ");
		if (cur)attroff(COLOR_PAIR(1));
		else if (nxt)attroff(COLOR_PAIR(3));
	}
	printw("│ %d", SIZE - 1);
	mvprintw(s + 3 + (SIZE - 1) * 2, t, "  └");
	for (int i = 0; i < SIZE - 1; i++)printw("───┴"); printw("───┘");
	mvprintw(s + 4 + (SIZE - 1) * 2, t, "y   0   1   2   3   4   5   6   7  ");
	refresh();
}

inline void preview_board(state A, int turn, int p1, int p2) {
	mvprintw(int(Y * 0.7) - 2, (X - 21) / 2 + X * 0.2, "Player 1 (Black): %s", p1 == 0 ? "Man" : "Bot");
	mvprintw(int(Y * 0.7) - 1, (X - 21) / 2 + X * 0.2, "Player 2 (White): %s", p2 == 0 ? "Man" : "Bot");
	mvprintw(int(Y * 0.7) + 1, (X - 12) / 2 + X * 0.2, "%s moves. ", turn == 2 ? "White" : "Black");
	int s = (Y - 18) * 0.4, t = (X - 37) / 2 + X * 0.2;
	mvprintw(s, t, "    0   1   2   3   4   5   6   7   x");
	mvprintw(s + 1, t, "  ┌");
	for (int i = 0; i < SIZE - 1; i++)printw("───┬"); printw("───┐");
	for (int i = 0; i < SIZE - 1; i++) {
		mvprintw(s + 2 + i * 2, t, "%d ", i);
		for (int j = 0; j < SIZE; j++) {
			printw("│");
			if (A[j][i] == 1)printw(" ○ ");
			else if (A[j][i] == 2)printw(" ● ");
			else if (A[j][i] == 3)printw(" ✕ ");
			else printw("   ");
		}
		printw("│ %d", i);
		mvprintw(s + 3 + i * 2, t, "  ├");
		for (int j = 0; j < SIZE - 1; j++)printw("───┼"); printw("───┤");
	}
	mvprintw(s + 2 + (SIZE - 1) * 2, t, "%d ", SIZE - 1);
	for (int j = 0; j < SIZE; j++) {
		printw("│");
		if (A[j][SIZE - 1] == 1)printw(" ○ ");
		else if (A[j][SIZE - 1] == 2)printw(" ● ");
		else if (A[j][SIZE - 1] == 3)printw(" ✕ ");
		else printw("   ");
	}
	printw("│ %d", SIZE - 1);
	mvprintw(s + 3 + (SIZE - 1) * 2, t, "  └");
	for (int i = 0; i < SIZE - 1; i++)printw("───┴"); printw("───┘");
	mvprintw(s + 4 + (SIZE - 1) * 2, t, "y   0   1   2   3   4   5   6   7  ");
	refresh();
}

inline void print_main_menu(int highlight) {
	maketitle();
	int s = Y / 2, t = (X - 7) / 2;
	for (int i = 0; i < n_main_choices; i++, s++) {
		if (i == highlight) {
			attron(A_REVERSE);
			mvprintw(s, t, "%s", main_choices[i]);
			attroff(A_REVERSE);
		}
		else mvprintw(s, t, "%s", main_choices[i]);
	}
	refresh();
}

inline void print_player_menu(int highlight) {
	int s = Y / 2, t = (X - 7) / 2;
	for (int i = 0; i < n_player_choices; i++, s++) {
		if (i == highlight) {
			attron(A_REVERSE);
			mvprintw(s, t, "%s", player_choices[i]);
			attroff(A_REVERSE);
		}
		else mvprintw(s, t, "%s", player_choices[i]);
	}
	refresh();
}

inline void print_exiting_menu(int highlight) {
	clear();
	mvprintw(Y * 0.35, (X - 46) / 2, "Do you want to save the current state to file?");
	int s = Y / 2, t = (X - 9) / 2;
	for (int i = 0; i < n_exiting_choices; i++, s++) {
		if (i == highlight) {
			attron(A_REVERSE);
			mvprintw(s, t, "%s", exiting_choices[i]);
			attroff(A_REVERSE);
		}
		else mvprintw(s, t, "%s", exiting_choices[i]);
	}
	refresh();
}

inline void preview_file(int id) {
	string file_name = ".file/" + to_string(id);
	FILE *f = fopen(file_name.c_str(), "r");
	state _A;
	int _turn, _p1, _p2;
	fscanf(f, "%d %d %d", &_p1, &_p2, &_turn);
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			fscanf(f, "%d", &_A[i][j]);
		}
	}
	fclose(f);
	preview_board(_A, _turn, _p1, _p2);
}

inline void print_file_list(int highlight) {
	clear();
	int size = files.size();
	int s = (Y - size) * 0.4, t = (X / 2 - 30) / 2;
	int i = 0;
	for (auto it : files) {
		if (i == highlight) {
			attron(A_REVERSE);
			mvprintw(s, t, "%02d: %30s", i + 1, it.file_name.c_str());
			attroff(A_REVERSE);
			preview_file(it.file_id);
		}
		else mvprintw(s, t, "%02d: %30s", i + 1, it.file_name.c_str());
		i++, s++;
	}
	refresh();
}

inline void print_replace_menu(int highlight, int flag = 0) {
	int s = Y * 0.6 + 5, t = (X - 10) / 2;
	if (flag)s = Y * 0.8 + 1, t = (X / 2 - 10) / 2;
	for (int i = 0; i < n_replace_choices; i++, s++) {
		if (i == highlight) {
			attron(A_REVERSE);
			mvprintw(s, t, "%s", replace_choices[i]);
			attroff(A_REVERSE);
		}
		else mvprintw(s, t, "%s", replace_choices[i]);
	}
	refresh();
}

inline void print_modify_menu(int s, int highlight) {
	int size = files.size();
	int t = (X / 2 - 30) / 2 + 34;
	for (int i = 0; i < n_modify_choices; i++, s++) {
		mvprintw(s, t, "|");
		if (i == highlight) {
			attron(A_REVERSE);
			mvprintw(s, t + 1, "%s", modify_choices[i]);
			attroff(A_REVERSE);
		}
		else mvprintw(s, t + 1, "%s", modify_choices[i]);
	}
	refresh();
}

inline void delete_file(int id) {
	list<file_info>::iterator pos;
	int i = 0;
	for (auto it = files.begin(); it != files.end(); it++) {
		if (i == id) {
			pos = it;
			break;
		}
		i++;
	}
	int file_id = pos->file_id;
	files.erase(pos);

	FILE *name_list = fopen(".file/name_list", "w");
	for (auto it : files) {
		fprintf(name_list, "%s\n%d\n", it.file_name.c_str(), it.file_id);
	}
	fclose(name_list);

	system(("rm .file/" + to_string(file_id)).c_str());
}

inline int modify_file(int cur_file) {
	int size = files.size();
	int highlight = 0, c, choice = -1;
	int s = (Y - size) * 0.4 + cur_file;
	print_modify_menu(s, highlight);
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight = (highlight - 1 + n_modify_choices) % n_modify_choices;
			break;
		case KEY_DOWN:
			highlight = (highlight + 1) % n_modify_choices;
			break;
		case KEY_LEFT:
			choice = -2;
			break;
		case 27:
			choice = -2;
			break;
		case 10:
			choice = highlight;
			break;
		default:
			if (isalpha(c)) {
				c = toupper(c);
				switch (c) {
				case 'O':
					choice = 0;
					break;
				case 'D':
					choice = 1;
					break;
				case 'R':
					choice = 2;
					break;
				}
			}
			break;
		}
		print_modify_menu(s, highlight);
		if (~choice)break;
	}
	if (choice == 0) {
		return 1;
	}
	else if (choice == 1) {
		delete_file(cur_file);
		return 2;
	}
	else if (choice == 2) {
		string name;
		while (1) {
			mvprintw(s + 2, (X / 2 - 30) / 2 + 41, ":");
			echo();
			curs_set(1);
			getstr(c_name);
			curs_set(0);
			noecho();
			name = c_name;
			if (name.length() > MAXLEN) {
				mvprintw(((Y - size) * 0.4 + size + Y) / 2, (X / 2 - 51) * 0.55, "The name is too long. Press any key to input again. ");
				getch();
				mvprintw(((Y - size) * 0.4 + size + Y) / 2, (X / 2 - 51) * 0.55, "                                                    ");
				mvprintw(s + 2, (X / 2 - 30) / 2 + 42, "                                             ");
				continue;
			}
			bool flag = 1;
			int i = 0;
			for (auto it = files.begin(); it != files.end(); it++) {
				if (i != cur_file && it->file_name == name) {
					flag = 0;
					break;
				}
				i++;
			}
			if (!flag) {
				mvprintw(((Y - size) * 0.4 + size + Y) / 2, (X / 2 - 54) * 0.55, "The name already exists. Press any key to input again. ");
				getch();
				mvprintw(((Y - size) * 0.4 + size + Y) / 2, (X / 2 - 54) * 0.55, "                                                       ");
				mvprintw(s + 2, (X / 2 - 30) / 2 + 42, "                                             ");
				continue;
			}
			else break;
		}
		int i = 0;
		for (auto &it : files) {
			if (i == cur_file) {
				it.file_name = name;
				break;
			}
			i++;
		}
		FILE *name_list = fopen(".file/name_list", "w");
		for (auto it : files) {
			fprintf(name_list, "%s\n%d\n", it.file_name.c_str(), it.file_id);
		}
		fclose(name_list);
		return 3;
	}
	else return -1;
}

inline int choose_player(int p) {
	mvprintw(int(Y * 0.3), (X - 41) / 2, "Choose the identity of player %d (%s):", p, p == 1 ? "Black" : "White");
	refresh();
	int highlight = 0, c, choice = -1;
	print_player_menu(highlight);
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight = (highlight - 1 + n_player_choices) % n_player_choices;
			break;
		case KEY_DOWN:
			highlight = (highlight + 1) % n_player_choices;
			break;
		case 27:
			choice = 2;
			break;
		case 10:
			choice = highlight;
			break;
		default:
			if (c >= '1' && c <= '3')choice = c - '1';
			else if (isalpha(c)) {
				c = toupper(c);
				switch (c) {
				case 'M':
					choice = 0;
					break;
				case 'B':
					choice = 1;
					break;
				}
			}
			break;
		}
		print_player_menu(highlight);
		if (~choice)break;
	}
	return choice;
}

inline void saving(int turn){
	string name;
	bool replace = 0;
	list<file_info>::iterator pos;
	while (1) {
		mvprintw(Y * 0.6, (X - 40) / 2, "Please input the name of the savefile: ");
		move(Y * 0.6 + 2, (X - 40) / 2);
		clrtoeol();
		echo();
		curs_set(1);
		getstr(c_name);
		curs_set(0);
		noecho();
		name = c_name;
		if (name.length() > MAXLEN) {
			mvprintw(Y * 0.6 + 4, (X - 51) / 2, "The name is too long. Press any key to input again. ");
			getch();
			move(Y * 0.6 + 4, (X - 51) / 2), clrtoeol();
			continue;
		}
		bool flag = 1;
		int i = 0;
		for (auto it = files.begin(); it != files.end(); it++) {
			if (it->file_name == name) {
				flag = 0;
				pos = it;
				break;
			}
		}
		if (!flag) {
			mvprintw(Y * 0.6 + 4, (X - 51) / 2, "The name already exists. Do you want to replace it? ");
			int highlight = 0, c, choice = -1;
			print_replace_menu(highlight);
			while (1) {
				c = getch();
				switch (c) {
				case KEY_UP:
					highlight = (highlight - 1 + n_replace_choices) % n_replace_choices;
					break;
				case KEY_DOWN:
					highlight = (highlight + 1) % n_replace_choices;
					break;
				case 27:
					choice = 1;
					break;
				case 10:
					choice = highlight;
					break;
				default:
					if (c >= '1' && c <= '2')choice = c - '1';
					else if (isalpha(c)) {
						c = toupper(c);
						switch (c) {
						case 'R':
							choice = 0;
							break;
						case 'C':
							choice = 1;
							break;
						}
					}
					break;
				}
				print_replace_menu(highlight);
				if (~choice)break;
			}
			if (choice == 0) {
				replace = 1;
				break;
			}
			else {
				move(Y * 0.6 + 4, (X - 51) / 2), clrtoeol();
				move(Y * 0.6 + 5, 0); clrtoeol();
				move(Y * 0.6 + 6, 0); clrtoeol();
			}
		}
		else break;
	}
	if (!replace) {
		files.push_back(file_info(name, ++max_name));
		pos = files.end(); pos--;
		FILE *name_list = fopen(".file/name_list", "a");
		fprintf(name_list, "%s\n%d\n", c_name, max_name);
		fclose(name_list);
	}
	string file_name = ".file/" + to_string(pos->file_id);
	FILE *f = fopen(file_name.c_str(), "w");
	fprintf(f, "%d %d %d\n", p1, p2, turn);
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			fprintf(f, "%d ", A[i][j]);
		}
		fprintf(f, "\n");
	}
	fclose(f);
}

inline bool exiting(int turn) {
	clear();
	int highlight = 0, c, choice = -1;
	print_exiting_menu(highlight);
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight = (highlight - 1 + n_exiting_choices) % n_exiting_choices;
			break;
		case KEY_DOWN:
			highlight = (highlight + 1) % n_exiting_choices;
			break;
		case 10:
			choice = highlight;
			break;
		case 27:
			choice = 2;
			break;
		default:
			if (c >= '1' && c <= '3')choice = c - '1';
			else if (isalpha(c)) {
				c = toupper(c);
				switch (c) {
				case 'S':
					choice = 0;
					break;
				case 'D':
					choice = 1;
					break;
				case 'C':
					choice = 2;
					break;
				}
			}
			break;
		}
		print_exiting_menu(highlight);
		if (~choice)break;
	}
	if (choice == 2)return 0;
	else if (choice == 1)return 1;
	else {
		saving(turn);
		return 1;
	}
}

inline void Main();

inline int move_cur_to_nxt(int turn, int x_0, int y_0, int x_1, int y_1) {
	int _dx = x_1 - x_0, _dy = y_1 - y_0, dir, len; //cout<<_dx<<' '<<_dy<<endl;
	if (_dx > 0 && !_dy)dir = 0;
	else if (_dx > 0 && _dx + _dy == 0)dir = 1;
	else if (!_dx && _dy < 0)dir = 2;
	else if (_dx < 0 && _dx == _dy)dir = 3;
	else if (_dx < 0 && !_dy)dir = 4;
	else if (_dx < 0 && _dx + _dy == 0)dir = 5;
	else if (!_dx && _dy > 0)dir = 6;
	else if (_dx > 0 && _dx == _dy)dir = 7;
	else return 1;
	len = abs(_dx ? _dx : _dy); //cout<<len<<endl;
	for (int i = 1; i <= len; i++) {
		if (A[x_0 + i * dx[dir]][y_0 + i * dy[dir]])return 2;
	}
	return 0;
}

inline int move_arrow(int turn, int x_0, int y_0, int x_1, int y_1, int x_2, int y_2) {
	int _dx = x_2 - x_1, _dy = y_2 - y_1, dir, len; //cout<<_dx<<' '<<_dy<<endl;
	if (_dx > 0 && !_dy)dir = 0;
	else if (_dx > 0 && _dx + _dy == 0)dir = 1;
	else if (!_dx && _dy < 0)dir = 2;
	else if (_dx < 0 && _dx == _dy)dir = 3;
	else if (_dx < 0 && !_dy)dir = 4;
	else if (_dx < 0 && _dx + _dy == 0)dir = 5;
	else if (!_dx && _dy > 0)dir = 6;
	else if (_dx > 0 && _dx == _dy)dir = 7;
	else return 1;
	len = abs(_dx ? _dx : _dy); //cout<<len<<endl;
	for (int i = 1; i <= len; i++) {
		int cur_x = x_1 + i * dx[dir], cur_y = y_1 + i * dy[dir];
		if ((cur_x != x_0 || cur_y != y_0) && A[cur_x][cur_y])return 2;
	}
	return 0;
}

inline int Round(int turn) {
	int highlight_x = 0, highlight_y = 0, c, choice_x = -1, choice_y = -1, cur_x = -1, cur_y = -1, nxt_x = -1, nxt_y = -1, arr_x = -1, arr_y = -1;
	print_board(turn, highlight_x, highlight_y);
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight_y = (highlight_y - 1 + SIZE) % SIZE;
			break;
		case KEY_DOWN:
			highlight_y = (highlight_y + 1) % SIZE;
			break;
		case KEY_LEFT:
			highlight_x = (highlight_x - 1 + SIZE) % SIZE;
			break;
		case KEY_RIGHT:
			highlight_x = (highlight_x + 1) % SIZE;
			break;
		case 27:
			if (exiting(turn))return -1;
			else break;
		case 10:
			choice_x = highlight_x, choice_y = highlight_y;
			if (A[choice_x][choice_y] != turn) {
				mvprintw(int(Y * 0.7) + 3, (X - 62) / 2, "Invalid: it's not your amazon. Press any key to choose again. "); getch();
				choice_x = -1, choice_y = -1;
			}
			break;
		}
		print_board(turn, highlight_x, highlight_y);
		//	printw("%d %d",highlight_x,highlight_y);
		if (~choice_x && ~choice_y)break;
	}
	cur_x = choice_x, cur_y = choice_y;
	print_board(turn, highlight_x, highlight_y, cur_x, cur_y, -1, -1, cur_x, cur_y);
	choice_x = -1, choice_y = -1;
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight_y = (highlight_y - 1 + SIZE) % SIZE;
			break;
		case KEY_DOWN:
			highlight_y = (highlight_y + 1) % SIZE;
			break;
		case KEY_LEFT:
			highlight_x = (highlight_x - 1 + SIZE) % SIZE;
			break;
		case KEY_RIGHT:
			highlight_x = (highlight_x + 1) % SIZE;
			break;
		case 27:
			if (exiting(turn))return -1;
			else break;
		case 10:
			choice_x = highlight_x, choice_y = highlight_y;
			int fail = move_cur_to_nxt(turn, cur_x, cur_y, choice_x, choice_y);
			if (fail == 1) {
				mvprintw(int(Y * 0.7) + 3, (X - 60) / 2, "Invalid: invalid direction. Press any key to choose again. "); getch();
				choice_x = -1, choice_y = -1;
			}
			else if (fail == 2) {
				mvprintw(int(Y * 0.7) + 3, (X - 60) / 2, "Invalid: crossing obstacle. Press any key to choose again. "); getch();
				choice_x = -1, choice_y = -1;
			}
			break;
		}
		print_board(turn, highlight_x, highlight_y, cur_x, cur_y, -1, -1, cur_x, cur_y);
		if (~choice_x && ~choice_y)break;
	}
	nxt_x = choice_x, nxt_y = choice_y;
	print_board(turn, highlight_x, highlight_y, cur_x, cur_y, nxt_x, nxt_y , nxt_x, nxt_y);
	choice_x = -1, choice_y = -1;
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight_y = (highlight_y - 1 + SIZE) % SIZE;
			break;
		case KEY_DOWN:
			highlight_y = (highlight_y + 1) % SIZE;
			break;
		case KEY_LEFT:
			highlight_x = (highlight_x - 1 + SIZE) % SIZE;
			break;
		case KEY_RIGHT:
			highlight_x = (highlight_x + 1) % SIZE;
			break;
		case 27:
			if (exiting(turn))return -1;
			else break;
		case 10:
			choice_x = highlight_x, choice_y = highlight_y;
			int fail = move_arrow(turn, cur_x, cur_y, nxt_x, nxt_y, choice_x, choice_y);
			if (fail == 1) {
				mvprintw(int(Y * 0.7) + 3, (X - 60) / 2, "Invalid: invalid direction. Press any key to choose again. "); getch();
				choice_x = -1, choice_y = -1;
			}
			else if (fail == 2) {
				mvprintw(int(Y * 0.7) + 3, (X - 60) / 2, "Invalid: crossing obstacle. Press any key to choose again. "); getch();
				choice_x = -1, choice_y = -1;
			}
			break;
		}
		print_board(turn, highlight_x, highlight_y, cur_x, cur_y, nxt_x, nxt_y , nxt_x, nxt_y);
		if (~choice_x && ~choice_y)break;
	}
	arr_x = choice_x, arr_y = choice_y;
	int id = -1;
	for (int i = 0; i < 8; i++) {
		if (A.pos[i][0] == cur_x && A.pos[i][1] == cur_y) {
			id = i;
			break;
		}
	}
	A[cur_x][cur_y] = 0;
	A[nxt_x][nxt_y] = turn;
	A[arr_x][arr_y] = 3;
	A.pos[id][0] = nxt_x, A.pos[id][1] = nxt_y;
	return policy(cur_x, cur_y, nxt_x, nxt_y, arr_x, arr_y);
}

inline void man_man(int turn = 1) {
	int c;
	while (1) {
		print_board(turn); c = getch();
		if (c == 27) {
			if (exiting(turn))return;
		}
		if (finished(turn)) {
			mvprintw(int(Y * 0.7) + 3, (X - 23) / 2, "player %d (%s) wins! ", 3 - turn, 3 - turn == 1 ? "Black" : "White"); getch();
			break;
		}
		int res = Round(turn);
		if (!~res)break;
		turn = 3 - turn;
	}
}

inline void bot_bot(int turn = 1) {
#define P(turn) (((turn) == 1)?P1:P2)
	int c;
	bot P1(A, 2 - turn), P2(A, 2 - turn);
	print_board(turn);
	while (1) {
		if (finished(turn)) {
			mvprintw(int(Y * 0.7) + 3, (X - 23) / 2, "player %d (%s) wins! ", 3 - turn, 3 - turn == 1 ? "Black" : "White"); getch();
			break;
		}
		int res = P(turn & 1).play(), x_0, y_0, x_1, y_1, x_2, y_2;
		trans_policy(res, x_0, y_0, x_1, y_1, x_2, y_2);
		print_board_bot(turn, x_0, y_0, x_1, y_1); c = getch();
		while (c == 27) {
			if (exiting(turn))return;
			else print_board_bot(turn, x_0, y_0, x_1, y_1), c = getch();
		}
		int id = -1;
		for (int i = 0; i < 8; i++) {
			if (A.pos[i][0] == x_0 && A.pos[i][1] == y_0) {
				id = i;
				break;
			}
		}
		A[x_0][y_0] = 0;
		A[x_1][y_1] = turn;
		A.pos[id][0] = x_1, A.pos[id][1] = y_1;
		print_board_bot(turn, x_1, y_1, x_2, y_2); c = getch();
		A[x_2][y_2] = 3;
		while (c == 27) {
			if (exiting(3 - turn))return;
			else A[x_2][y_2] = 0, print_board_bot(turn, x_1, y_1, x_2, y_2), c = getch(), A[x_2][y_2] = 3;
		}
		A[x_2][y_2] = 3;
		P((turn & 1) ^ 1).play(res);
		print_board(3 - turn);
		turn = 3 - turn;
	}
#undef P
}

inline void man_bot(int man, int turn = 1) {
	int c;
	bot P(A, 2 - turn);
	while (1) {
		if (finished(turn)) {
			mvprintw(int(Y * 0.7) + 3, (X - 23) / 2, 
				"player %d (%s) wins! ", 3 - turn, 3 - turn == 1 ? "Black" : "White"); getch();
			break;
		}
		if (turn == man) {
			int res = Round(turn);
			if (!~res)break;
			print_board(turn);
			P.play(res);
		}
		else {
			print_board(turn);
			int res = P.play(), x_0, y_0, x_1, y_1, x_2, y_2;
			trans_policy(res, x_0, y_0, x_1, y_1, x_2, y_2);
			print_board_bot(turn, x_0, y_0, x_1, y_1); c = getch();
			while (c == 27) {
				if (exiting(turn))return;
				else print_board_bot(turn, x_0, y_0, x_1, y_1), c = getch();
			}
			int id = -1;
			for (int i = 0; i < 8; i++) {
				if (A.pos[i][0] == x_0 && A.pos[i][1] == y_0) {
					id = i;
					break;
				}
			}
			A[x_0][y_0] = 0;
			A[x_1][y_1] = turn;
			A.pos[id][0] = x_1, A.pos[id][1] = y_1;
			print_board_bot(turn, x_1, y_1, x_2, y_2); c = getch();
			A[x_2][y_2] = 3;
			while (c == 27) {
				if (exiting(3 - turn))return;
				else A[x_2][y_2] = 0, 
					print_board_bot(turn, x_1, y_1, x_2, y_2), 
					c = getch(), A[x_2][y_2] = 3;
			}
			A[x_2][y_2] = 3;
		}
		turn = 3 - turn;
	}
}

inline void start(int p1, int p2, int turn = 1) {
	if (p1 == 0 && p2 == 0)man_man(turn);
	else if (p1 == 1 && p2 == 1)bot_bot(turn);
	else if (p1 == 0)man_bot(1, turn);
	else if (p2 == 0)man_bot(2, turn);
}

inline void game() {
	clear();
	p1 = choose_player(1);
	if (p1 == 2)return Main();
	p2 = choose_player(2);
	if (p2 == 2)return game();
	clear();
	start(p1, p2);
}

inline void Main() {
	clear();
	maketitle();
	int highlight = 0, c, choice = -1;
	print_main_menu(highlight);
	keypad(stdscr, 1);
	mvprintw(int(Y * 0.7), (X - 65) / 2, "Use arrow keys to go up and down. Press Enter to select a choice.");
	while (1) {
		c = getch();
		switch (c) {
		case KEY_UP:
			highlight = (highlight - 1 + n_main_choices) % n_main_choices;
			break;
		case KEY_DOWN:
			highlight = (highlight + 1) % n_main_choices;
			break;
		case 10:
			choice = highlight;
			break;
		case 27:
			choice = 2;
			break;
		default:
			if (c >= '1' && c <= '3')choice = c - '1';
			else if (isalpha(c)) {
				c = toupper(c);
				switch (c) {
				case 'N':
					choice = 0;
					break;
				case 'L':
					choice = 1;
					break;
				case 'E':
					choice = 2;
					break;
				}
			}
			break;
		}
		print_main_menu(highlight);
		if (~choice)break;
	}
	if (choice == 0) {
		A = state();
		cur_name = "New";
		game();
	}
	else if (choice == 1) {
		int highlight = 0, c, choice = -1;
		print_file_list(highlight);
		while (1) {
			int size = files.size();
			if (!size) {
				mvprintw(Y / 2, (X - 47) / 2, "There's no savefile. Press any key to continue. ");
				getch();
				return;
			}
			c = getch();
			switch (c) {
			case KEY_UP:
				highlight = (highlight - 1 + size) % size;
				break;
			case KEY_DOWN:
				highlight = (highlight + 1) % size;
				break;
			case 27:
				return;
			case 10:
				choice = highlight;
				break;
			case KEY_RIGHT:
				int res = modify_file(highlight);
				if (res == 1)choice = highlight;
				else if (res == 2)highlight = 0;
				break;
			}
			print_file_list(highlight);
			if (~choice)break;
		}
		int i = 0;
		list<file_info>::iterator pos;
		for (auto it = files.begin(); it != files.end(); it++) {
			if (i == choice) {
				cur_name = it->file_name;
				pos = it;
				break;
			}
			i++;
		}
		string file_name = ".file/" + to_string(pos->file_id);
		FILE *f = fopen(file_name.c_str(), "r");
		int turn;
		fscanf(f, "%d %d %d\n", &p1, &p2, &turn);
		int cnt1 = 0, cnt2 = 4;
		for (int i = 0; i < SIZE; i++) {
			for (int j = 0; j < SIZE; j++) {
				fscanf(f, "%d", &A[i][j]);
				if (A[i][j] == 1)A.pos[cnt1][0] = i, A.pos[cnt1++][1] = j;
				else if (A[i][j] == 2)A.pos[cnt2][0] = i, A.pos[cnt2++][1] = j;
			}
		}
		fclose(f);
		//	raw_print(A);getch();
		start(p1, p2, turn);
	}
	else if (choice == 2) {
		Exit();
	}
}

inline void load_savefile() {
	FILE *dir = fopen(".file/", "w");
	if (!dir)mkdir(".file/", 0775);
	else fclose(dir);
	FILE *name_list = fopen(".file/name_list", "r");
	if (name_list) {
		while (fgets(c_name, LEN, name_list) != NULL) {
			c_name[strlen(c_name) - 1] = 0;
			string name = c_name;
			int id;
			fscanf(name_list, "%d", &id);
			files.push_back(file_info(name, id));
			max_name = max(max_name, id);
			fgets(c_name, LEN, name_list);
		}
		fclose(name_list);
	}
}

inline void set_color(){
	start_color();
	init_color(COLOR_MAGENTA, 411, 420, 564);
	init_color(COLOR_CYAN, 436, 655, 900);
	init_pair(1, COLOR_WHITE, COLOR_CYAN);
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	init_pair(3, COLOR_WHITE, COLOR_MAGENTA);
}

inline void reset_color(){
	init_color(COLOR_MAGENTA, 816, 686, 828);
	init_color(COLOR_CYAN, 549, 823, 835);
}

inline void Exit() {
	reset_color();
	endwin();
	exit(0);
}

int main() {
	srand((unsigned)time(0));
	setlocale(LC_ALL, "");
	load_savefile();
	initscr();
	set_color();
	getmaxyx(stdscr, Y, X);
	curs_set(0);
	noecho();
	cbreak();
	while (1)Main();
	reset_color();
	endwin();
	return 0;
}

//Y  X
//52 208
