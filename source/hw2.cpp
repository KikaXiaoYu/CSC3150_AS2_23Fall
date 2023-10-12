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
pthread_mutex_t lock;
pthread_mutex_t screen_lock;

int kbhit(void);

void Delay();

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
void map_print()
{
	// printf("\033[2J");

	printf("\033[H\033[2J"); // clean the terminal
	int i;
	for (i = 0; i <= ROW; ++i)
		puts(map[i]);
	printf("\033[H");
}

void map_clean()
{
	// printf("\033[2J");
	// system("clear");
}

/* generate random number from 0 to COLUMN-1 */
int rand_num_gen()
{
	int rand_num = rand() % (COLUMN - 1);
	return rand_num;
}

int get_mod(int num, int mod)
{
	return (num + mod) % (mod);
}

/* shift the logs */
void logs_shifting(int row, int direc)
{
	int left_end;
	int right_end;
	int center = log_center[row];
	int half_log = LOG_LEN / 2;

	switch (direc)
	{
	case 1: // left
		left_end = get_mod(center - half_log - 1, COLUMN - 1);
		right_end = get_mod(center + half_log, COLUMN - 1);
		map[row][left_end] = '=';
		map[row][right_end] = ' ';
		log_center[row] = get_mod(center - 1, COLUMN - 1);
		break;
	case 0: // right
		left_end = get_mod(center - half_log, COLUMN - 1);
		right_end = get_mod(center + half_log + 1, COLUMN - 1);
		map[row][left_end] = ' ';
		map[row][right_end] = '=';
		log_center[row] = get_mod(center + 1, COLUMN - 1);
		break;
	default:
		break;
	}
}

/* initalize the logs randomly */
static void logs_initial(long row)
{
	int center = rand_num_gen();
	// int center = 45;
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

void Delay(int num)
{
	for (int i = 0; i < num; i++)
		;
}

void *screen_print(void *t)
{
	while (1)
	{
		// pthread_mutex_lock(&screen_lock);
		// Delay(0x000FFFFF);
		pthread_mutex_lock(&screen_lock);

		map_print();
		pthread_mutex_unlock(&screen_lock);
		usleep(10000);
		// pthread_mutex_unlock(&screen_lock);
	}
};

void *screen_clean(void *t)
{
	while (1)
	{
		// pthread_mutex_lock(&screen_lock);
		map_clean();
		// pthread_mutex_unlock(&screen_lock);
		// sleep(1);
	}
};

/* threads function */
void *logs_move(void *t)
{
	long row = (long)t;	  // log number
	long direc = row % 2; // log direction, 1 for left, 0 for right

	pthread_mutex_lock(&lock);
	logs_initial(row);
	// map_print();
	pthread_mutex_unlock(&lock);

	while (1)
	{
		/* Move the logs */
		pthread_mutex_lock(&lock);

		logs_shifting(row, direc);
		// map_print((void *)1);
		pthread_mutex_unlock(&lock);
		// Delay(0x000FFFFF);
		usleep(30000);
	}

	/* Check keyboard hits, to change frog's position or quit the game. */

	/* Check game's status */

	/* Print the map on the screen */
}

int main(int argc, char *argv[])
{

	// set random seeds
	srand(time(NULL));
	// Initialize the river map and frog's starting position
	memset(map, 0, sizeof(map)); // set all the chars(1 byte each) of the map to '0'
	int i, j;

	// create the empty river
	for (i = 1; i < ROW; ++i)
	{
		for (j = 0; j < COLUMN - 1; ++j)
			map[i][j] = ' ';
	}

	// create the bottom bank
	for (j = 0; j < COLUMN - 1; ++j)
		map[ROW][j] = map[0][j] = '|';

	// create the top bank
	for (j = 0; j < COLUMN - 1; ++j)
		map[0][j] = map[0][j] = '|';

	frog = Node(ROW, (COLUMN - 1) / 2); // create the frog
	map[frog.x][frog.y] = '0';			// put the frog on the map

	// Print the map into screen
	system("clear");
	// printf("\033[?25l");

	map_print();
	game_status = 0;

	pthread_mutex_init(&lock, NULL);
	pthread_mutex_init(&screen_lock, NULL);

	/* Create pthreads for wood move and frog control. */
	pthread_t wood_threads[ROW]; // 0 for none, others for wood
	for (i = 1; i <= ROW - 1; i++)
		pthread_create(&wood_threads[i], NULL, logs_move, (void *)(long)i);

	/* Create pthreads for screen print and clean */
	pthread_t screen_threads[1]; // 0 for print, 1 fo clean
	pthread_create(&wood_threads[0], NULL, screen_print, NULL);
	pthread_create(&wood_threads[1], NULL, screen_clean, NULL);

	/* join all the threads */
	for (i = 1; i <= ROW - 1; i++)
	{
		pthread_join(wood_threads[i], NULL);
	}

	for (i = 0; i <= 1; i++)
	{
		pthread_join(screen_threads[i], NULL);
	}

	/* Display the output for user: win, lose or quit. */
	system("clear");

	switch (game_status)
	{
	case 1:
		printf("You win the game!!\n");
		break;
	case 2:
		printf("You lose the game!!\n");
		break;
	case 3:
		printf("You exit the game!!\n");
		break;
	default:
		printf("An unexpected error occurred.\n");
		printf("I am just a human.\n");
		printf("I cannot handle everything.\n");
		break;
	}

	pthread_exit(NULL);

	return 0;
}
