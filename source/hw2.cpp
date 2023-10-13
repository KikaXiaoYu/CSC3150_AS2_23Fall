#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>

/* const numbers define */
#define ROW 10
#define COLUMN 50
#define LOG_LEN 15
#define LOG_DELAY 120000
#define PRINT_DELAY 13000

/* struct for the frog */
struct Node
{
	int x, y;
	Node(int _x, int _y) : x(_x), y(_y){};
	Node(){};
} frog;

/* global variables */
char map[ROW + 10][COLUMN];
int log_center[ROW + 1];
int game_status; // 0 for run, 1 for win, 2 for lose, 3 for quit

/* global pthread variables */
pthread_mutex_t wood_lock;

/* useful helping functions */
int kbhit(void);
void map_print(void);
int rand_num_gen(void);
int get_mod(int, int);
bool is_frog_on_log(int, int, int);
/* logs helping functions */
void logs_shifting(int, int);
void logs_initial(int);
/* frog move helping functions */
void frog_move_help(char ch);
/* pthread functions */
void *screen_print(void *t);
void *logs_move(void *t);
void *status_judge(void *t);

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0.
int kbhit(void)
{
	struct termios oldt, newt; // to specify the attributes of a serial port
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt); // to get the current attributes of a terminal

	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO); // not canonical mode and not block typing response

	tcsetattr(STDIN_FILENO, TCSANOW, &newt); // change take effect immediately
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);	 // return fd status

	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); // set fd status

	ch = getchar(); // get the char

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // change take effect immediately
	fcntl(STDIN_FILENO, F_SETFL, oldf);		 // set fd status

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}

/* print the map */
void map_print(void)
{
	printf("\033[H\033[2J"); // clean the terminal
	int i;
	for (i = 0; i <= ROW; ++i)
		puts(map[i]);
}

/* generate random number from 0 to COLUMN-1 */
int rand_num_gen(void)
{
	int rand_num = rand() % (COLUMN - 1);
	return rand_num;
}

/* get the num with mod */
int get_mod(int num, int mod)
{
	return (num + mod) % (mod);
}

bool is_frog_on_log(int row, int x, int y)
{
	if (x != row)
		return false;
	int half_log = LOG_LEN / 2;
	int center = log_center[row];
	if (((y - center <= half_log) && (y - center >= -half_log)) ||
		((y - center >= COLUMN - 1 - half_log) || (y - center <= half_log - COLUMN + 1)))
		return true;
	else
		return false;
}

/* shift the logs in map */
void logs_shifting(int row, int direc)
{
	int left_end;
	int right_end;
	int center = get_mod(log_center[row], COLUMN - 1);
	int half_log = LOG_LEN / 2;

	switch (direc)
	{
	case 1: // left
		left_end = get_mod(center - half_log - 1, COLUMN - 1);
		right_end = get_mod(center + half_log, COLUMN - 1);
		map[row][left_end] = '=';
		map[row][right_end] = ' ';
		if (is_frog_on_log(row, frog.x, frog.y))
		{
			map[row][frog.y] = '=';
			if (frog.y == right_end)
				map[row][frog.y] = ' ';
			frog.y = get_mod(frog.y - 1, COLUMN - 1);
			map[row][frog.y] = '0';
		}
		log_center[row] = get_mod(center - 1, COLUMN - 1);
		break;
	case 0: // right
		left_end = get_mod(center - half_log, COLUMN - 1);
		right_end = get_mod(center + half_log + 1, COLUMN - 1);
		map[row][left_end] = ' ';
		map[row][right_end] = '=';
		if (is_frog_on_log(row, frog.x, frog.y))
		{
			map[row][frog.y] = '=';
			if (frog.y == left_end)
				map[row][frog.y] = ' ';
			frog.y = get_mod(frog.y + 1, COLUMN - 1);
			map[row][frog.y] = '0';
		}
		log_center[row] = get_mod(center + 1, COLUMN - 1);
		break;
	default:
		break;
	}
}

/* initalize the logs randomly in map */
void logs_initial(int row)
{
	int center = rand_num_gen();
	log_center[row] = center;

	int j;
	int half_log = LOG_LEN / 2;
	int idx;

	for (j = (center - half_log); j <= (center + half_log); j++)
	{
		idx = get_mod(j, COLUMN - 1);
		map[row][idx] = '=';
	}
}

/* screen print thread */
void *screen_print(void *t)
{
	while (1)
	{
		map_print();

		/* Check game's status */
		if (game_status != 0)
			pthread_exit(NULL);

		usleep(PRINT_DELAY);
	}
};

/* log threads function */
void *logs_move(void *t)
{
	long row = (long)t;	  // log number
	long direc = row % 2; // log direction, 1 for left, 0 for right
	pthread_mutex_lock(&wood_lock);
	logs_initial(row);
	pthread_mutex_unlock(&wood_lock);
	while (1)
	{
		/* Move the logs */
		pthread_mutex_lock(&wood_lock);
		logs_shifting(row, direc);
		/* Move the frog */
		if (kbhit())
		{
			char ch = getchar();
			frog_move_help(ch);
		}
		pthread_mutex_unlock(&wood_lock);
		/* Check game's status */
		if (game_status != 0)
			pthread_exit(NULL);

		usleep(LOG_DELAY);
	}
}

/* help moving the frog */
void frog_move_help(char ch)
{
	int old_x = frog.x;
	int old_y = frog.y;
	int half_log = LOG_LEN / 2;
	int center = log_center[old_x];
	/* process previous entry */
	if (old_x == ROW || old_x == 0) // frog is on the bottom bank
		map[old_x][old_y] = '|';
	else if (old_x != ROW && old_x != 0) // frog is on the river
	{
		if (is_frog_on_log(old_x, old_x, old_y))
			map[old_x][old_y] = '=';
		else
			map[old_x][old_y] = ' ';
	}
	/* move the frog index */
	switch (ch)
	{
	case 'W':
	case 'w':
		if (frog.x != 0)
			frog.x -= 1;
		break;
	case 'S':
	case 's':
		if (frog.x != ROW)
			frog.x += 1;
		break;
	case 'A':
	case 'a':
		if (frog.y != 0)
			frog.y -= 1;
		break;
	case 'D':
	case 'd':
		if (frog.y != COLUMN - 2)
			frog.y += 1;
		break;
	case 'Q':
	case 'q':
		game_status = 3;
		break;
	default:
		break;
	}
	/* set new frog on map */
	map[frog.x][frog.y] = '0';
}

/* judge the status all the time */
void *status_judge(void *t)
{
	while (1)
	{
		pthread_mutex_lock(&wood_lock);
		/* Check game's status */
		if (game_status != 0)
		{
			pthread_mutex_unlock(&wood_lock);
			pthread_exit(NULL);
		}
		/* Check if the frog is on the top bank */
		if (frog.x == 0)
		{
			game_status = 1; // win the game
			pthread_mutex_unlock(&wood_lock);
			pthread_exit(NULL);
		}
		/* Check if the frog touches the edge */
		else if (frog.y == 0 || frog.y == COLUMN - 2)
		{
			game_status = 2; // lose the game
			pthread_mutex_unlock(&wood_lock);
			pthread_exit(NULL);
		}
		/* Check if the frog is on the river */
		else if (frog.x != ROW && frog.x != 0)
		{
			if (!is_frog_on_log(frog.x, frog.x, frog.y))
			{
				game_status = 2; // lose the game
				pthread_mutex_unlock(&wood_lock);
				pthread_exit(NULL);
			}
		}
		pthread_mutex_unlock(&wood_lock);
	}
}

int main(int argc, char *argv[])
{
	srand(time(NULL)); // set random seeds (for logs init usage)
	int i, j;		   // iterating variables

	/* Initialize the river map and frog's starting position */
	memset(map, 0, sizeof(map)); // set all the chars(1 byte each) of the map to '0'
	for (i = 1; i < ROW; ++i)	 // create the empty river
	{
		for (j = 0; j < COLUMN - 1; ++j)
			map[i][j] = ' ';
	}
	for (j = 0; j < COLUMN - 1; ++j) // create the bottom bank with len (COLUMM - 1)
		map[ROW][j] = map[0][j] = '|';
	for (j = 0; j < COLUMN - 1; ++j) // create the top bank with len (COLUMM - 1)
		map[0][j] = map[0][j] = '|';
	frog = Node(ROW, (COLUMN - 1) / 2); // create the frog
	map[frog.x][frog.y] = '0';			// put the frog on the map

	/* clean the terminal and set game status */
	printf("\033[2J");
	map_print();
	game_status = 0;

	/* Initialize the mutex for wood move */
	pthread_mutex_init(&wood_lock, NULL);

	/* Create pthreads for wood move and frog control. */
	pthread_t wood_threads[ROW]; // 0 for frog, others for wood
	for (i = 1; i <= ROW - 1; i++)
		pthread_create(&wood_threads[i], NULL, logs_move, (void *)(long)i);

	/* Create pthreads for screen print and game status */
	pthread_t control_threads[2]; // 0 for print, and 1 for status
	pthread_create(&control_threads[0], NULL, screen_print, NULL);
	pthread_create(&control_threads[1], NULL, status_judge, NULL);

	/* join all the threads */
	for (i = 1; i <= ROW - 1; i++)
		pthread_join(wood_threads[i], NULL);
	for (i = 0; i <= 1; i++)
		pthread_join(control_threads[i], NULL);

	/* Display the output for user: win, lose or quit. */
	printf("\033[2J"); // clean the terminal
	printf("\033[H");  // move the cursor to the top left corner
	switch (game_status)
	{
	case 1:
		printf("You win the game!!\n");
		break;
	case 2:
		printf("You lose the game!!\n");
		break;
	case 3:
		printf("You exit the game.\n");
		break;
	default:
		break;
	}

	/* destroy mutex and exit the pthread */
	pthread_mutex_destroy(&wood_lock);
	pthread_exit(NULL);

	return 0;
}
