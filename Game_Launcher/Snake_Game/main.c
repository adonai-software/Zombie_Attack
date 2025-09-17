#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

/*
        TODO
        1) Fix endgame -> startgame bug, game does not start immediately after playing one session
        2) Cursor line appearing during snake gameply, need to remove
        3) Code Scaliability (e.g difficulty, speed, color?, more menu options)
*/

// initializing data

#define FRAME_TIME 110000
#define MAX_SCORE 256

typedef struct {
    int x;
    int y;
} vec2;

int score = 0;
int screen_width = 40;
int screen_height = 20;

bool skip = false;
bool is_running = true;
bool won_game = false;

WINDOW *win; 

vec2 head = {20, 10};
vec2 body[MAX_SCORE + 1];
vec2 dir = {1, 0};
vec2 food;

// declaring all functions to avoid future errors
void display_menu(WINDOW* win);
vec2 generate_food();
void init_game();
void restart_game();
void read_inputs();
bool check_collision(vec2 a, vec2 b);
bool check_collision_snake(vec2 point);
void board_update();
void draw();
void game_over();

void display_menu(WINDOW* win) {
    int choice = 0;
    int pressed;
    
    while (true) {
        erase();
        
        mvprintw(8, 15, "SNAKE GAME");
        mvprintw(9, 15, "==========");
        
        if (choice == 0) {
            mvprintw(12, 15, "> Start Game"); // Arrow will show hovering 
        } else {
            mvprintw(12, 15, "  Start Game"); 
        }
        
        if (choice == 1) {
            mvprintw(13, 15, "> Quit Game"); // Arrow will show hovering 
        } else {
            mvprintw(13, 15, "  Quit Game");
        }
        
        refresh();
        
        pressed = wgetch(win);
        
        if (pressed == KEY_UP && choice > 0) {
            choice--;
        }
        if (pressed == KEY_DOWN && choice < 1) {
            choice++;
        }
        if (pressed == '\n') {
            if (choice == 0) {
                return;
            } else {
                endwin();
                exit(0);
            }
        }
    }
}

vec2 generate_food() {
    vec2 berry;
    berry.x = 1 + rand() % (screen_width - 2);
    berry.y = 1 + rand() % (screen_height - 2);
    
    if (check_collision(head, berry) || check_collision_snake(berry)) {
        return generate_food();  // recursive calling might need base case in the future
    }
    
    return berry;
}

void init_game() {
    win = initscr();
    srand(time(NULL)); 
    keypad(win, true);
    nodelay(win, true);
    noecho();
    curs_set(0);

    // Player starts in the middle
    head.x = screen_width / 2;
    head.y = screen_height / 2;
    
    // Generating food
    food = generate_food();
    
    // Initialize body
    for (int i = 0; i < MAX_SCORE; i++) {
        body[i].x = -1;
        body[i].y = -1;
    }
}

void restart_game() {
    head.x = screen_width / 2; // housekeeping commands
    head.y = screen_height / 2;
    dir.x = 1;
    dir.y = 0;
    is_running = true;
    score = 0;
    won_game = false;
    
    // Reset body
    for (int i = 0; i < MAX_SCORE; i++) {
        body[i].x = -1;
        body[i].y = -1;
    }
    
    food = generate_food();
}

void read_inputs() {
    int pressed = wgetch(win);
    
    if (pressed == KEY_LEFT) {
        if (dir.x == 1) {
            skip = true;
            return;
        }
        dir.x = -1;
        dir.y = 0;
    }
    if (pressed == KEY_RIGHT) {
        if (dir.x == -1) {
            skip = true;
            return;
        }
        dir.x = 1;
        dir.y = 0;
    }
    if (pressed == KEY_UP) {
        if (dir.y == 1) {
            skip = true;
            return;
        }
        dir.x = 0;
        dir.y = -1;
    }
    if (pressed == KEY_DOWN) {
        if (dir.y == -1) {
            skip = true;
            return;
        }
        dir.x = 0;
        dir.y = 1;
    } 
    if (pressed == 27) { // need to fix 
        display_menu(win);
        if (!is_running) {
            restart_game();
        }
    }
}

bool check_collision(vec2 a, vec2 b) { // function that determines if two different things are in the same position
    return (a.x == b.x && a.y == b.y);
}

bool check_collision_snake(vec2 point) { // checks to see if you hit yourself
    for(int i = 0; i < score; i++) {
        if(check_collision(point, body[i])) {
            return true;
        }
    }
    return false; 
}

void board_update() {
    // Move body segments
    for (int i = score; i > 0; i--) {
        body[i] = body[i-1];
    }

    // Move head to body
    if (score > 0) {
        body[0] = head;
    }
    
    // Based on user input, update head position
    head.x += dir.x;
    head.y += dir.y; 

    // Check collisions
    if(check_collision_snake(head) || head.x < 0 || head.y < 0 || head.x >= screen_width || head.y >= screen_height) {
        is_running = false;
        return;
    }

    // When user collects food 
    if(check_collision(head, food)) {
        if (score < MAX_SCORE) {
            score++;
        } else {
            // Game won user got max score
            is_running = false;
            won_game = true;
            return;
        }
        food = generate_food(); // makes food once user eats food
    }
}

void draw() {
    erase(); // Housekeeping 
    
    // Drawing the map
    for (int x = 0; x < screen_width; x++) {
        mvaddch(0, x, '#');
        mvaddch(screen_height - 1, x, '#');
    }
    for (int y = 0; y < screen_height; y++) {
        mvaddch(y, 0, '#');
        mvaddch(y, screen_width - 1, '#');
    }
    
    // Drawing the snake's body based on user score
    for (int i = 0; i < score; i++) {
        if (body[i].x >= 0 && body[i].y >= 0) {
            mvaddch(body[i].y, body[i].x, 'o');
        }
    }
    
    // head of the snake
    mvaddch(head.y, head.x, 'O');
    
    // Drawing food
    mvaddch(food.y, food.x, 'F');
    
    // Printing score for user
    mvprintw(screen_height + 1, 0, "Score: %d", score);
    
    refresh();
}

void game_over() {
    erase();
    if (!won_game || score < MAX_SCORE) {
        mvprintw(screen_height / 2 - 2, screen_width / 2 - 5, "GAME OVER!");
    }
    else if (won_game && score == MAX_SCORE) {
        mvprintw(screen_height / 2 - 2, screen_width / 2 - 5, "YOU BEAT SNAKE!");    
    }
    mvprintw(screen_height / 2 - 1, screen_width / 2 - 8, "Final Score: %d", score);
    mvprintw(screen_height / 2 + 1, screen_width / 2 - 15, "Press SPACE to play again or hit ESC to go to the menu");
    refresh();
    
    int pressed;
    while (!is_running) {
        pressed = wgetch(win);
        if (pressed == ' ') { // Space bar
            restart_game();
            break;
        }
        if (pressed == 27) { // ESC
            display_menu(win);
            if (is_running) {
                restart_game();
            }
            break;
        }
        usleep(50000);
    }
}

int main() {
    init_game();
    display_menu(win);

    while(true) {
        if (!is_running) {
            game_over();
            continue;
        }
        
        read_inputs(); 
        
        if (skip) {
            skip = false;
            continue;
        }

        board_update();
        draw();
        
        usleep(FRAME_TIME);
    }

    endwin();
    return 0;
}