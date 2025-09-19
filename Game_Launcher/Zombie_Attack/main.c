#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
// need to add json header file 
#include <cjson/cJSON.h> // note this is different from regualr json, make sure to install cjson to work

// need to add json file that updates these macros
int NUM_OF_OBSTRUCTION = 50; 
int NUM_OF_MONSTER = 10;
int NUM_OF_FOOD = 5;
int NUM_OF_POWER = 2;

// need function to read JSON file
void load_JSON_Difficulty(const char* filename, const char* difficulty) {
  FILE* file = fopen(filename, "r");
    if (!file) {
      printf("JSON file did not open\n"); // testing to see if JSON file opens
      exit(1);
    }

    // Reading JSON file
    fseek(file, 0, SEEK_END); // traverses through the entire JSON file and marks pointer to the end
    long length = ftell(file); // takes the current pointer location to determine file length
    fseek(file, 0, SEEK_SET); // sets the pointer back to the beginning of file

    // Making dynamically allocated space
    char* data_values = malloc(length+1); // remember to deallocate memory
      if(!data_values) {
        printf("Dynamic space not made\n"); // testing to see if memory allocation works or not
        fclose(file); // housekeeping 
        exit(1);
      }

    // reading JSON file to newly made memory
    fread(data_values, 1, length, file);
    fclose(file); // housekeeping
    data_values[length] = '\0'; // adds null terminator needed for CJSON pre-defined functions

    // Parsing data values
    cJSON* head = cJSON_Parse(data_values); // marks head of object tree
      if(!head) {
        printf("JSON parsing didn't work\n"); // making sure object tree exists
        free(data_values);
        exit(1); 
      }

    // extracting certain object from tree
    cJSON* mode = cJSON_GetObjectItemCaseSensitive(head, difficulty);
      if(!mode) {
        printf("Difficulty not found\n"); //checking to see if mode is found in JSON file
        cJSON_Delete(head);
        free(data_values);
        exit(1);
      }
    // updating global macros (TODO MAKE SURE TO MAKE VARIABLE TYPE) TODO Check if JSON file is correct format
    NUM_OF_OBSTRUCTION = cJSON_GetObjectItemCaseSensitive(mode, "NUM_OF_OBSTRUCTION")->valueint;
    NUM_OF_MONSTER = cJSON_GetObjectItemCaseSensitive(mode, "NUM_OF_MONSTER")->valueint;
    NUM_OF_FOOD = cJSON_GetObjectItemCaseSensitive(mode, "NUM_OF_FOOD")->valueint;
    NUM_OF_POWER = cJSON_GetObjectItemCaseSensitive(mode, "NUM_OF_POWER")->valueint;

    cJSON_Delete(head); // housekeeping
    free(data_values);
}

// end of JSON file

/* TODO
        - Convert int x,y into struct Point [x]
        - Write unflatten function [x]
        - Add perimeter [x]
        - Make monsters move [x]
        - Add Pause and banner middle [x]
        - start menu [\]
                                        - Start [x]
                                        - Settings [x] Configurable?
                                        - High Scores [x]
                                        - About [x]
                                        - Quit [x]
                                - Game Banner [x]
                                - quit gracefully
*/

static const char* menu_items[] = {"Start", "Settings", "High Scores", "About",
                                   "Quit", "Difficulty"}; // Added Difficulty

static const int n_items = sizeof(menu_items) / sizeof(menu_items[0]);

typedef enum { START, SETTINGS, HIGH_SCORES, ABOUT, QUIT, DIFFICULTY } Menu; // Added Difficulty

// new added difficulty UI
static const char* mode_levels[] = {"Easy", "Medium", "Hard", "Insane"};
static const int n_modes = sizeof(mode_levels) / sizeof(mode_levels[0]);

typedef enum {EASY, MEDIUM, HARD, INSANE} Mode;

// end of new added difficulty UI

int max_x, max_y;

typedef enum { PLAYER, OBSTRUCTION, FOOD, PERIMETER, MONSTER, POWER } Type;

typedef struct {
  int x, y;
} Point;

typedef struct {
  Point p;
  Type t;
} Object;

// Points x,y is mapped unto a 1-dimentional array
// Use flatten(x,y) to get mapping
Object** map = {NULL};
pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;
Object p1;
Object* perimeter;
Object* foods = NULL; // from Object foods[NUM_OF_FOOD];
Object* obstructions = NULL; // from Object obstructions[NUM_OF_OBSTRUCTION];
Object* monsters = NULL; // from Object monsters[NUM_OF_MONSTER];
Object* powers = NULL; // from Object powers[NUM_OF_POWER];
pthread_t mm, pt;

bool game_pause = false;
bool exit_game = false;

int center_z = 0;
int smallest_z = 0;
int largest_z = 0;
int num_food = 0;
int num_lives = 2;
bool power_enabled = false;
pthread_mutex_t power_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t power_cond = PTHREAD_COND_INITIALIZER;

int flatten(int x, int y) { return y == 1 ? x : ((y - 1) * max_x) + x; }

// BUG - draws dimensions depending on biggest string
// possible string bigger than available area
// - word wrapping?
// - words are left aligned, center align?
void draw_rec(char** items, int n_items, int highlight, int x_pos, int y_pos) {
  int max_len = 0, x_offset = 0, y_offset = 0;

  max_len = strlen(items[0]);
  for (int i = 1; i < n_items; i++) {
    max_len = strlen(items[i]) > max_len ? strlen(items[i]) : max_len;
  }

  x_offset = max_len + 2;
  y_offset = n_items + 2;

  int c_x = x_pos;
  int c_y = y_pos;
  // mvprintw(c_y, c_x, "."); For Debugging Center

  int ltc_x = c_x - x_offset;
  int ltc_y = c_y - y_offset;

  int lbc_x = c_x - x_offset;
  int lbc_y = c_y + y_offset;

  int rtc_x = c_x + x_offset;
  int rtc_y = c_y - y_offset;

  int rbc_x = c_x + x_offset;
  int rbc_y = c_y + y_offset;

  for (int i = ltc_x; i < rtc_x; i++) mvprintw(ltc_y, i, "*");
  for (int i = lbc_x; i < rbc_x; i++) mvprintw(lbc_y, i, "*");
  for (int i = ltc_y; i < lbc_y; i++) mvprintw(i, ltc_x, "*");
  for (int i = rtc_y; i < rbc_y; i++) mvprintw(i, rtc_x, "*");

  if (n_items > 1) {
    int item_x = ltc_x;
    int item_y = ltc_y + 2;
    for (int i = 0; i < n_items; i++) {
      int x_margin = ((rtc_x - ltc_x) - strlen(items[i])) / 2;
      if (highlight == i) attron(A_REVERSE);
      mvprintw(item_y, item_x + x_margin, "%s", items[i]);
      if (highlight == i) attroff(A_REVERSE);
      item_y += 2;
    }
  } else {
    int x_margin = ((rtc_x - ltc_x) - strlen(items[0])) / 2;
    mvprintw(ltc_y + ((lbc_y - ltc_y) / 2), ltc_x + x_margin, "%s", items[0]);
  }
}

// Wrapper to draw in center
void draw_rec_center(char** items, int n_items, int highlight) {
  draw_rec(items, n_items, highlight, max_x / 2, max_y / 2);
}

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

void deter_monsters() {
    pthread_mutex_lock(&power_mutex);  // Start timer
    power_enabled = true;
    pthread_mutex_unlock(&power_mutex);
    pthread_cond_signal(&power_cond);
}

bool set_object(Object* o, int x, int y) {
  int new_pos = flatten(x, y);
  int old_pos = flatten(o->p.x, o->p.y);

  if (new_pos > largest_z || new_pos < smallest_z) return false;

  if (map[new_pos] == NULL) {  // Object placement
    map[new_pos] = o;
    if (o->t == PLAYER || o->t == MONSTER)
      map[old_pos] = NULL;  // Moving objects
    return true;
  } else if (o->t == PLAYER &&
             map[new_pos]->t == FOOD) {  // Player gaining food
    map[new_pos] = o;
    map[old_pos] = NULL;
    num_food--;
    return true;
  } else if (o->t == PLAYER &&
             map[new_pos]->t == POWER) {  // Player getting power up
    map[new_pos] = o;
    map[old_pos] = NULL;
		deter_monsters();
    return true;
  } else if ((o->t == MONSTER && map[new_pos]->t == PLAYER) ||
             (o->t == PLAYER && map[new_pos]->t == MONSTER)) {
    num_lives--;
    clear();  // Clear the screen
    char** str_a[1];
    char str[] = "Ouch!";
    str_a[0] = str;
    draw_rec_center(str_a, 1, -1);
		deter_monsters(); // Gives time for player to move - improve
    usleep(1000000);
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
          if (!power_enabled)
            mvprintw(map[i]->p.y, map[i]->p.x, "M");
          else
            mvprintw(map[i]->p.y, map[i]->p.x, "m");
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
    char** str_a[1];
    char str[] = "You Win!";
    str_a[0] = str;
    draw_rec_center(str_a, 1, -1);
  }

  if (num_lives < 1) {
    clear();  // Clear the screen
    char** str_a[1];
    char str[] = "You Lose!";
    str_a[0] = str;
    draw_rec_center(str_a, 1, -1);
  }
}

void* moving_monster(void* arg) {
  if (!game_pause && !exit_game) {
    while (1) {
      pthread_mutex_lock(&map_mutex);
      int new_x_pos, new_y_pos;
      for (int i = 0; i < NUM_OF_MONSTER; i++) {
        // Random movement
        // int new_x_pos = rand()%2==0 ? monsters[i].p.x+1 : monsters[i].p.x-1;
        // int new_y_pos = rand()%2==0 ? monsters[i].p.y+1 : monsters[i].p.y-1;
        // Inteligent movement

        if (!power_enabled) {
          new_x_pos = monsters[i].p.x > p1.p.x ? monsters[i].p.x - 1
                                               : monsters[i].p.x + 1;
          new_y_pos = monsters[i].p.y > p1.p.y ? monsters[i].p.y - 1
                                               : monsters[i].p.y + 1;
        } else {
          new_x_pos = monsters[i].p.x > p1.p.x ? monsters[i].p.x + 1
                                               : monsters[i].p.x - 1;
          new_y_pos = monsters[i].p.y > p1.p.y ? monsters[i].p.y + 1
                                               : monsters[i].p.y - 1;
        }

        if (set_object(&monsters[i], new_x_pos, new_y_pos)) {
          monsters[i].p.x = new_x_pos;
          monsters[i].p.y = new_y_pos;
        }
      }
      pthread_mutex_unlock(&map_mutex);

      usleep(200000);
    }
  } else {
    usleep(1000000);
  }

  return NULL;
}

void* power_timer(void* arg) {
  if (!game_pause && !exit_game) {
    while (1) {
      pthread_mutex_lock(&power_mutex);
      pthread_cond_wait(&power_cond, &power_mutex);
      usleep(5000000);  // 5 seconds
      power_enabled = false;
      pthread_mutex_unlock(&power_mutex);
    }
  } else {
    usleep(1000000);  // 5 seconds
  }
}
// adding allocation for verying difficulty
  void allocate_game_arrays() {
  foods = malloc(NUM_OF_FOOD * sizeof(Object));
  obstructions = malloc(NUM_OF_OBSTRUCTION * sizeof(Object));
  monsters = malloc(NUM_OF_MONSTER * sizeof(Object));
  powers = malloc(NUM_OF_POWER * sizeof(Object));
  
  if (!foods || !obstructions || !monsters || !powers) {
    printf("New game arrays not allocated dynamically.\n"); //checking to see if it made allocated space
    exit(1);
  }
}
// end of adding new allocation

// cleaning up memory from previous function

void deallocate_game_arrays() {
  free(foods);
  free(obstructions);
  free(monsters);
  free(powers);

  foods = NULL; // housekeeping
  obstructions = NULL;
  monsters = NULL;
  powers = NULL;
}

// end of new cleanup

void init_game() {
// cleaning up previos memory allocations just in case
if (foods != NULL || obstructions != NULL || monsters != NULL || powers != NULL) {
  deallocate_game_arrays();
}

// new adding memory allocation for diffierent difficulties
allocate_game_arrays(); // will run default settings, and update if new difficulty is included

  // Initialize objects
  map = calloc(max_x * max_y, sizeof(Object*));
  perimeter = malloc(((2 * max_x) + (2 * max_y)) * sizeof(Object));

  // Initialize threads
  int t1 = 1;
  int t2 = 2;
  pthread_create(&mm, NULL, moving_monster, &t1);
  pthread_create(&pt, NULL, power_timer, &t2);

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

void pause_game() {
  nodelay(stdscr, FALSE);  // block
  game_pause = true;
  pthread_mutex_lock(&map_mutex);

  char* items[1];
  char* pause_str = "Pause";
  items[0] = pause_str;

  draw_rec_center(items, 1, -1);

  while (game_pause) {
    int ch = getch();
    switch (ch) {
      case ' ':
        nodelay(stdscr, TRUE);  // unblock
        game_pause = false;
        pthread_mutex_unlock(&map_mutex);
        break;
      default:
        break;
    }
    clear();  // Clear the screen
    draw_scene();
    refresh();  // Update the screen
    usleep(20000);
  }
}

/*
 * @brief main game function
 * @description
 *	@params none
 *	@return none
 */
void start_game() {
  nodelay(stdscr, TRUE);  // makes getch non-blocking
  while (!exit_game) {    // Game loop, exit on 'q'
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
      case ' ':  // Pause/Resume game
        pthread_mutex_unlock(&map_mutex);
        pause_game();
        break;
      case 'q':
        exit_game = true;
        pthread_mutex_unlock(&map_mutex);
        break;
      default:
        break;
    }

    if (!game_pause && !exit_game) {
      pthread_mutex_unlock(&map_mutex);
      mvprintw(0, 0, "Position (%d,%d) max_x=%d,max_y=%d", p1.p.x, p1.p.y,
               max_x, max_y);
      mvprintw(0, max_x - 20, "%s %d", "food remaining = ", num_food);
      mvprintw(0, max_x - 50, "%s %d", "lives remaining = ", num_lives);
      draw_scene();
      refresh();  // Update the screen
      usleep(5000);
    }
  }

  // BUG - game not ending gracefully, freezes
  pthread_join(pt, NULL);
  pthread_join(mm, NULL);

  deallocate_game_arrays();
}

void display_high_scores() {
  clear();
  const char* items[] = {"High Scores",
                         "==============================",
                         "Cong Do = 9999999999999999",
                         "Christie = 7777777777777",
                         "John Doe = 40000424",
                         "Susan Doe = 6666666666"};

  int n_items = sizeof(items) / sizeof(items[0]);
  draw_rec_center(items, n_items, -1);
}

void display_settings() {
  clear();
  char s_mons[50];
  char s_food[50];
  char s_power[50];
  snprintf(s_mons, sizeof(s_mons), "Number of Monster = %d\n", NUM_OF_MONSTER);
  snprintf(s_food, sizeof(s_food), "Number of Food = %d\n", NUM_OF_FOOD);
  snprintf(s_power, sizeof(s_power), "Number of Power Up = %d\n", NUM_OF_POWER);

  const char* items[] = {"Settings", "==============================", s_mons,
                         s_food, s_power};

  int n_items = sizeof(items) / sizeof(items[0]);
  draw_rec_center(items, n_items, -1);
}

void display_about() {
  clear();
  const char* items[] = {"About",
                         "==============================",
                         "Capture all foods (*) without losing all lives.",
                         "Arrow Keys = Player Movement",
                         "[Space Bar] = Pause/Resume Game",
                         "t = Player",
                         "* = Food",
                         "M = Monster",
                         "P = Power Up"};

  int n_items = sizeof(items) / sizeof(items[0]);
  draw_rec_center(items, n_items, -1);
}

/*
 * @brief displays startup menu and controls selection
 * @description
 *	@params none
 *	@return none
 */

 // todo Difficulty UI
void display_difficulty() {
  Mode selection = EASY;
  bool selected = false;
  while (!selected) {
    clear();
    draw_rec_center(mode_levels, n_modes, selection);
    int ch = getch();
    switch (ch) {
      case KEY_DOWN:
        selection = selection < n_modes - 1 ? selection + 1 : selection;
        ;
        break;
      case KEY_UP:
        selection = selection > 0 ? selection - 1 : selection;
        break;
      case KEY_ENTER:
      case '\n':
        switch (selection) {
          case EASY:
            // todo select easy
            load_JSON_Difficulty("macros.json", "easy");
            break;
          case MEDIUM:
            // todo select medium
            load_JSON_Difficulty("macros.json", "medium");
            break;
          case HARD:
            // todo select hard
            load_JSON_Difficulty("macros.json", "hard");
            break;
          case INSANE:
            // todo select insane
            load_JSON_Difficulty("macros.json", "insane");
            break;
        }
        selected = true; // backs out if player selects a difficulty
        break;
      defalt:
        break;
    }
    refresh();
  }
}

// end of Difficulty UI (works visually)

void display_menu() {
  Menu selection = START;
  bool selected = false;
  while (!selected) {
    clear();
    draw_rec_center(menu_items, n_items, selection);
    int ch = getch();
    switch (ch) {
      case KEY_DOWN:
        selection = selection < n_items - 1 ? selection + 1 : selection;
        ;
        break;
      case KEY_UP:
        selection = selection > 0 ? selection - 1 : selection;
        break;
      case KEY_ENTER:
      case '\n':
        switch (selection) {
          case START:
            init_game();
            start_game();
            break;
          case SETTINGS:
            display_settings();
            getch();
            break;
          case HIGH_SCORES:
            display_high_scores();
            getch();
            break;
          case ABOUT:
            display_about();
            getch();
            break;
          case QUIT:
            selected = true;
            break;
          case DIFFICULTY:
            display_difficulty();
            break;
        }
        break;
      defalt:
        break;
    }
  }
}

void display_banner() {
  char* game_title = "Zombie Attack!";
  char** str_arr[] = {game_title};
  draw_rec_center(str_arr, 1, -1);
  int c = getch();
}


int main() {
  initscr();             // Initialize ncurses
  cbreak();              // Line buffering disabled
  noecho();              // Don't echo input characters
  keypad(stdscr, TRUE);  // Enable special keys
  curs_set(0);           // Hide cursor
  srand(time(NULL));
  getmaxyx(stdscr, max_y, max_x);
  center_z = flatten(max_x / 2, max_y / 2);
  largest_z = flatten(max_x, max_y);

  display_banner();
  display_menu();

  endwin();  // Clean up ncurses
  return 0;
}