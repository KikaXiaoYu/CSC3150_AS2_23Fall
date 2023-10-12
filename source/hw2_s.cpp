#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>

#define ROW 10
#define COLUMN 50

struct Node
{
    int x, y;
    Node(int _x, int _y) : x(_x), y(_y){};
    Node(){};
} g_frog;

static char map[ROW + 10][COLUMN];

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0.
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

#define LOG_PRINT_LEN_MAX 15
pthread_t g_tid[ROW];
pthread_mutex_t t_mutex;

#define GAME_STATUS_INIT 0
#define GAME_STATUS_EXIT 1
#define GAME_STATUS_WIN 2
#define GAME_STATUS_LOSE 3

#define LOG_MODE_SPACE ' '
#define LOG_MODE_PIPE '|'
#define LOG_MODE_EQU '='
#define LOG_MODE_ZERO '0'

int g_game_status;
static void *logs_move(void *t);

int main(int argc, char *argv[])
{
    int i, j, row_id;

    memset(map, 0, sizeof(map));
    for (i = 1; i < ROW; i++)
    {
        for (j = 0; j < COLUMN - 1; j++)
        {
            map[i][j] = LOG_MODE_SPACE;
        }
    }

    for (j = 0; j < COLUMN - 1; j++)
    {
        map[ROW][j] = map[0][j] = LOG_MODE_PIPE;
    }

    for (j = 0; j < COLUMN - 1; j++)
    {
        map[0][j] = map[0][j] = LOG_MODE_PIPE;
    }

    g_frog = Node(ROW, (COLUMN - 1) / 2);
    map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;

    // Print the map into screen
    printf("\033[H\033[2J");
    for (i = 0; i <= ROW; i++)
    {
        puts(map[i]);
    }

    pthread_mutex_init(&t_mutex, NULL);
    /*	Create pthreads for wood move and g_frog control.  */
    g_game_status = GAME_STATUS_INIT;
    for (row_id = 0; row_id < ROW - 1; row_id++)
    {
        pthread_create(&g_tid[row_id], NULL, logs_move, (void *)(unsigned long)row_id);
    }

    // wait all thread exit
    for (row_id = 0; row_id < ROW - 1; row_id++)
    {
        pthread_join(g_tid[row_id], NULL);
    }

    if (g_game_status == GAME_STATUS_WIN)
    {
        printf("\nYou win the game!!\n");
    }
    else if (g_game_status == GAME_STATUS_LOSE)
    {
        printf("\nYou lose the game.\n");
    }
    else if (g_game_status == GAME_STATUS_EXIT)
    {
        printf("\nYou exit the game.\n");
    }

    return 0;
}

static void logs_move_init_map(int row, int row_index)
{
    int i, colmun;

    for (i = 0; i < LOG_PRINT_LEN_MAX; i++)
    {
        colmun = (row_index + i) % COLUMN;
        map[row][colmun] = LOG_MODE_EQU;
    }

    for (i = 0; i < COLUMN - LOG_PRINT_LEN_MAX; i++)
    {
        colmun = (row_index + LOG_PRINT_LEN_MAX + i) % COLUMN;
        map[row][colmun] = LOG_MODE_SPACE;
    }

    return;
}

static void logs_map_update_map(int row, int going_num)
{
    char up_map[COLUMN] = {0};
    int colmun;

    for (colmun = 0; colmun < COLUMN; colmun++)
    {
        up_map[(colmun + going_num) % COLUMN] = map[row][colmun];
    }

    for (colmun = 0; colmun < COLUMN; colmun++)
    {
        map[row][colmun] = up_map[colmun];
    }

    if (g_frog.x < ROW && g_frog.x == row)
    {
        g_frog.y = (g_frog.y + going_num) % COLUMN;
    }

    return;
}

static int logs_check_kb_data_quit(int kb_data)
{
    if (kb_data == 'Q' || kb_data == 'q')
    {
        return 1;
    }

    return 0;
}

static int logs_check_kb_data_UP(int kb_data)
{
    if (kb_data == 'W' || kb_data == 'w')
    {
        return 1;
    }

    return 0;
}

static int logs_check_kb_data_DOWN(int kb_data)
{
    if (kb_data == 'S' || kb_data == 's')
    {
        return 1;
    }

    return 0;
}

static int logs_check_kb_data_LEFT(int kb_data)
{
    if (kb_data == 'A' || kb_data == 'a')
    {
        return 1;
    }

    return 0;
}

static int logs_check_kb_data_RIGHT(int kb_data)
{
    if (kb_data == 'D' || kb_data == 'd')
    {
        return 1;
    }

    return 0;
}

static void logs_do_UP(void)
{
    if (map[g_frog.x - 1][g_frog.y] == LOG_MODE_SPACE)
    {
        if (g_frog.x == ROW)
        {
            map[g_frog.x][g_frog.y] = LOG_MODE_PIPE;
        }
        else
        {
            map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
        }
        g_frog.x -= 1;
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        g_game_status = GAME_STATUS_LOSE;
    }
    else if (map[g_frog.x - 1][g_frog.y] == LOG_MODE_PIPE)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
        g_frog.x -= 1;
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        g_game_status = GAME_STATUS_WIN;
    }
    else if (map[g_frog.x - 1][g_frog.y] == LOG_MODE_EQU)
    {
        if (g_frog.x == ROW)
        {
            map[g_frog.x][g_frog.y] = LOG_MODE_PIPE;
        }
        else
        {
            map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
        }
        g_frog.x -= 1;
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
    }

    return;
}

static void logs_do_DOWN(void)
{
    if (g_frog.x == ROW)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_PIPE;
        g_frog.x += 1;
        g_game_status = GAME_STATUS_LOSE;
        return;
    }

    if (map[g_frog.x + 1][g_frog.y] == LOG_MODE_PIPE)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
        g_frog.x += 1;
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
    }
    else if (map[g_frog.x + 1][g_frog.y] == LOG_MODE_EQU)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
        g_frog.x += 1;
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
    }
    else if (map[g_frog.x + 1][g_frog.y] == LOG_MODE_SPACE)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
        g_frog.x += 1;
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        g_game_status = GAME_STATUS_LOSE;
    }

    return;
}

static void logs_do_LEFT(void)
{
    if (g_frog.x == ROW)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_PIPE;
        g_frog.y = g_frog.y - 1;
        if (g_frog.y < 0)
        {
            g_game_status = GAME_STATUS_LOSE;
        }
        else
        {
            map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        }

        return;
    }

    map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
    g_frog.y = g_frog.y - 1;
    if (g_frog.y < 0)
    {
        g_game_status = GAME_STATUS_LOSE;
    }
    else if (map[g_frog.x][g_frog.y] == LOG_MODE_SPACE)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        g_game_status = GAME_STATUS_LOSE;
    }
    else
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
    }

    return;
}

static void logs_do_RIGHT(void)
{
    if (g_frog.x == ROW)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_PIPE;
        g_frog.y = g_frog.y + 1;
        if (g_frog.y > COLUMN)
        {
            g_game_status = GAME_STATUS_LOSE;
        }
        else
        {
            map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        }

        return;
    }

    map[g_frog.x][g_frog.y] = LOG_MODE_EQU;
    g_frog.y = g_frog.y + 1;
    if (g_frog.y > COLUMN - 1)
    {
        g_game_status = GAME_STATUS_LOSE;
    }
    else if (map[g_frog.x][g_frog.y] == LOG_MODE_SPACE)
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
        g_game_status = GAME_STATUS_LOSE;
    }
    else
    {
        map[g_frog.x][g_frog.y] = LOG_MODE_ZERO;
    }

    return;
}

static void *logs_move(void *t)
{
    int log_row_arg = (int)(unsigned long)t;
    int row = log_row_arg + 1;
    int going_num;
    int i, j, colmun;
    int kb_is_hit, kb_data;

    if ((row % 2) == 0)
    {
        going_num = 1;
    }
    else
    {
        going_num = COLUMN - 1;
    }

    int sleeper[ROW];
    for (i = 0; i < ROW; i++)
    {
        sleeper[i] = 80000 + (rand() % 30) * 1000; // sleep different logs for different frequencies
    }

    pthread_mutex_lock(&t_mutex);
    logs_move_init_map(row, log_row_arg);
    pthread_mutex_unlock(&t_mutex);

    while (1)
    {
        pthread_mutex_lock(&t_mutex);
        if (g_game_status != GAME_STATUS_INIT)
        {
            pthread_mutex_unlock(&t_mutex);
            break;
        }

        /*  Move the logs  */
        logs_map_update_map(row, going_num);

        /*  Check keyboard hits, to change g_frog's position or quit the game. */
        kb_is_hit = kbhit();
        if (kb_is_hit)
        {
            kb_data = getchar();
            if (logs_check_kb_data_quit(kb_data))
            {
                g_game_status = GAME_STATUS_EXIT;
                break;
            }

            if (logs_check_kb_data_UP(kb_data))
            {
                logs_do_UP();
            }
            else if (logs_check_kb_data_DOWN(kb_data))
            {
                logs_do_DOWN();
            }
            else if (logs_check_kb_data_LEFT(kb_data))
            {
                logs_do_LEFT();
            }
            else if (logs_check_kb_data_RIGHT(kb_data))
            {
                logs_do_RIGHT();
            }
        }

        /*  Check game's g_game_status  */
        if (g_frog.y == 0 || g_frog.y == COLUMN - 1)
        {
            g_game_status = GAME_STATUS_LOSE;
        }

        /*  Print the map on the screen  */
        printf("\033[H");
        for (i = 0; i <= ROW; i++)
        {
            for (j = 0; j < COLUMN; j++)
            {
                printf("%c", map[i][j]);
            }
            printf("\n");
        }

        pthread_mutex_unlock(&t_mutex);

        usleep(sleeper[row]);
    }

    return NULL;
}
