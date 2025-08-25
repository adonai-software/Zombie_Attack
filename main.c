#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_OF_OBSTRUCTION 50
#define NUM_OF_MONSTER 10
#define NUM_OF_FOOD 5
#define NUM_OF_POWER 2
// #define max_x 50
// #define max_y 50

/* TODO
        - Convert int x,y into struct Point [x]
        - Write unflatten function [x]
        - Add perimeter [x]
        - Make monsters move
*/

int max_x, max_y;

typedef enum { PLAYER, OBSTRUCTION, FOOD, PERIMETER, MONSTER, POWER } Type;

typedef struct {
  int x, y;
} Point;

typedef struct {
  Point p;
  Type t;
} Object;

// x,y grid is mapped unto a 2 demensional array
Object **map = {NULL};
pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;
Object p1;
Object *perimeter;
Object foods[NUM_OF_FOOD];
Object obstructions[NUM_OF_OBSTRUCTION];
Object monsters[NUM_OF_MONSTER];
Object powers[NUM_OF_POWER];

int smallest_z = 0;
int largest_z = 0;
int num_food = 0;
int num_lives = 2;
bool power_enabled = false;
pthread_mutex_t power_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t power_cond = PTHREAD_COND_INITIALIZER;

int flatten(int x, int y) { return y == 1 ? x : ((y - 1) * max_x) + x; }

Point unflatten(int z) {
  Point p;
  p.x = 1;
  p.y = 1;

  if (z < max_x) {
    p.x = z;
    p.y = 1;
  } else {
    // find the row
    do {
      p.y++;
    } while (p.y * max_x < z);

    // find column
    while (((p.y - 1) * max_x) + p.x != z) p.x++;

    // if we go beyond the rows
    if (p.y > max_y) p.y--;
  }

  return p;
}

bool set_object(Object *o, int x, int y) {
  int new_pos = flatten(x, y);
  int old_pos = flatten(o->p.x, o->p.y);

  if (new_pos > largest_z || new_pos < smallest_z) return false;

  if (map[new_pos] == NULL) {
    map[new_pos] = o;
    if (o->t == PLAYER || o->t == MONSTER) map[old_pos] = NULL;
    return true;
  } else if (o->t == PLAYER && map[new_pos]->t == FOOD) {
    map[new_pos] = o;
    map[old_pos] = NULL;
    num_food--;
    return true;
    // player + 1 food
  } else if (o->t == PLAYER && map[new_pos]->t == POWER) {
    map[new_pos] = o;
    map[old_pos] = NULL;
    pthread_mutex_lock(&power_mutex);
    power_enabled = true;
    pthread_mutex_unlock(&power_mutex);
    pthread_cond_signal(&power_cond);
    return true;
  } else if ((o->t == MONSTER && map[new_pos]->t == PLAYER) ||
             (o->t == PLAYER && map[new_pos]->t == MONSTER)) {
    num_lives--;
    clear();  // Clear the screen
    mvprintw(10, 10, "%s", "Ouch!");
    usleep(1500000);
    return false;
  }

  return false;
}

void draw_scene() {
  for (int i = 0; i < max_x * max_y; i++) {
    if (map[i] == NULL) {
      continue;
    } else {
      switch (map[i]->t) {
        case PLAYER:
          mvprintw(map[i]->p.y, map[i]->p.x, "t");
          break;
        case OBSTRUCTION:
          mvprintw(map[i]->p.y, map[i]->p.x, "O");
          break;
        case FOOD:
          mvprintw(map[i]->p.y, map[i]->p.x, "*");
          break;
        case PERIMETER:
          mvprintw(map[i]->p.y, map[i]->p.x, "X");
          break;
        case MONSTER:
          if (!power_enabled) mvprintw(map[i]->p.y, map[i]->p.x, "M");
          else mvprintw(map[i]->p.y, map[i]->p.x, "m");
          break;
        case POWER:
          mvprintw(map[i]->p.y, map[i]->p.x, "p");
          break;
        default:
          break;
      }
    }
  }

  if (num_food <= 0) {
    clear();  // Clear the screen
    mvprintw(10, 10, "%s", "Winner!");
  }

  if (num_lives < 1) {
    clear();  // Clear the screen
    mvprintw(10, 10, "%s", "You Lose!");
  }
}

void *moving_monster(void *arg) {
  while (1) {
    pthread_mutex_lock(&map_mutex);
    int new_x_pos, new_y_pos;
    for (int i = 0; i < NUM_OF_MONSTER; i++) {
      // Random movement
      // int new_x_pos = rand()%2==0 ? monsters[i].p.x+1 : monsters[i].p.x-1;
      // int new_y_pos = rand()%2==0 ? monsters[i].p.y+1 : monsters[i].p.y-1;
      // Inteligent movement

      
      if(!power_enabled) {
        new_x_pos =
            monsters[i].p.x > p1.p.x ? monsters[i].p.x - 1 : monsters[i].p.x + 1;
        new_y_pos =
            monsters[i].p.y > p1.p.y ? monsters[i].p.y - 1 : monsters[i].p.y + 1;
      } else {
        new_x_pos =
            monsters[i].p.x > p1.p.x ? monsters[i].p.x + 1 : monsters[i].p.x - 1;
        new_y_pos =
            monsters[i].p.y > p1.p.y ? monsters[i].p.y + 1 : monsters[i].p.y - 1;

      }

      if (set_object(&monsters[i], new_x_pos, new_y_pos)) {
        monsters[i].p.x = new_x_pos;
        monsters[i].p.y = new_y_pos;
      }
    }
    pthread_mutex_unlock(&map_mutex);

    usleep(200000);
  }

  return NULL;
}

void *power_timer(void *arg) {
  while(1) {
    pthread_mutex_lock(&power_mutex);
    pthread_cond_wait(&power_cond, &power_mutex);
    usleep(5000000); // 5 seconds
    power_enabled = false;
    pthread_mutex_unlock(&power_mutex);
  }
}

void init_game() {

  // Initialize player start
  p1.t = PLAYER;
  p1.p.x = 2;
  p1.p.y = 2;
  set_object(&p1, p1.p.x, p1.p.y);

  int num_of_per = 0;
  // Top perimeter
  for (int i = 0; i < max_x; i++) {
    perimeter[num_of_per].t = PERIMETER;
    perimeter[num_of_per].p.y = 1;
    perimeter[num_of_per].p.x = i;
    set_object(&perimeter[num_of_per], perimeter[num_of_per].p.x,
               perimeter[num_of_per].p.y);
    num_of_per++;
  }

  // Left perimeter
  for (int i = 0; i < max_y; i++) {
    perimeter[num_of_per].t = PERIMETER;
    perimeter[num_of_per].p.x = 0;
    perimeter[num_of_per].p.y = i;
    set_object(&perimeter[num_of_per], perimeter[num_of_per].p.x,
               perimeter[num_of_per].p.y);
    num_of_per++;
  }

  // Right perimeter
  for (int i = 0; i < max_y; i++) {
    perimeter[num_of_per].t = PERIMETER;
    perimeter[num_of_per].p.x = max_x - 1;
    perimeter[num_of_per].p.y = i;
    set_object(&perimeter[num_of_per], perimeter[num_of_per].p.x,
               perimeter[num_of_per].p.y);
    num_of_per++;
  }

  // Bottom perimeter
  for (int i = 0; i < max_x; i++) {
    perimeter[num_of_per].t = PERIMETER;
    perimeter[num_of_per].p.x = i;
    perimeter[num_of_per].p.y = max_y - 1;
    set_object(&perimeter[num_of_per], perimeter[num_of_per].p.x,
               perimeter[num_of_per].p.y);
    num_of_per++;
  }

  // Generate foods
  for (int i = 0; i < NUM_OF_FOOD; i++) {
    foods[i].t = FOOD;
    foods[i].p.x = rand() % max_x;
    foods[i].p.y = rand() % max_y;
    if (set_object(&foods[i], foods[i].p.x, foods[i].p.y)) num_food++;
  }

  // Generate powers
  for (int i = 0; i < NUM_OF_POWER; i++) {
    powers[i].t = POWER;
    powers[i].p.x = rand() % max_x;
    powers[i].p.y = rand() % max_y;
    set_object(&powers[i], powers[i].p.x, powers[i].p.y);
  }

  // Generate monsters
  for (int i = 0; i < NUM_OF_MONSTER; i++) {
    monsters[i].t = MONSTER;
    monsters[i].p.x = rand() % max_x;
    monsters[i].p.y = rand() % max_y;
    set_object(&monsters[i], monsters[i].p.x, monsters[i].p.y);
  }

  // Generate obstructions
  for (int i = 0; i < NUM_OF_OBSTRUCTION; i++) {
    obstructions[i].t = OBSTRUCTION;
    obstructions[i].p.x = rand() % max_x;
    obstructions[i].p.y = rand() % max_y;
    set_object(&obstructions[i], obstructions[i].p.x, obstructions[i].p.y);
  }
}

int main() {
  initscr();             // Initialize ncurses
  cbreak();              // Line buffering disabled
  noecho();              // Don't echo input characters
  keypad(stdscr, TRUE);  // Enable special keys
  curs_set(0);           // Hide cursor

  nodelay(stdscr, TRUE);  // makes getch non-blocking
  srand(time(NULL));
  getmaxyx(stdscr, max_y, max_x);
  map = calloc(max_x * max_y, sizeof(Object *));
  perimeter = malloc(((2 * max_x) + (2 * max_y)) * sizeof(Object));
  largest_z = flatten(max_x, max_y);
  pthread_t mm, pt;
  int t1 = 1;
  int t2 = 2;

  init_game();
  pthread_create(&mm, NULL, moving_monster, &t1);
  pthread_create(&pt, NULL, power_timer, &t2);

  bool exit_game = false;
  while (!exit_game) {  // Game loop, exit on 'q'
    int ch = getch();
    clear();  // Clear the screen

    pthread_mutex_lock(&map_mutex);
    // Update player position based on input
    switch (ch) {
      case KEY_UP:
        if (set_object(&p1, p1.p.x, p1.p.y - 1)) p1.p.y--;
        break;
      case KEY_DOWN:
        if (set_object(&p1, p1.p.x, p1.p.y + 1)) p1.p.y++;
        break;
      case KEY_LEFT:
        if (set_object(&p1, p1.p.x - 1, p1.p.y)) p1.p.x--;
        break;
      case KEY_RIGHT:
        if (set_object(&p1, p1.p.x + 1, p1.p.y)) p1.p.x++;
        break;
      case 'q':
        exit_game = true;
      default:
        break;
    }
    pthread_mutex_unlock(&map_mutex);

    mvprintw(0, 0, "Position (%d,%d) max_x=%d,max_y=%d", p1.p.x, p1.p.y, max_x,
             max_y);
    mvprintw(0, max_x - 20, "%s %d", "food remaining = ", num_food);
    mvprintw(0, max_x - 50, "%s %d", "lives remaining = ", num_lives);
    draw_scene();
    refresh();  // Update the screen
    usleep(5000);
  }

  pthread_join(pt, NULL);
  pthread_join(mm, NULL);

  endwin();  // Clean up ncurses
  return 0;
}
