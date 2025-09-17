#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
    TODO:
        1) Add About Section in Menus
        2) Check if game runs before linking
        3) Fix game interlinking bug within menus
        
*/

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
#endif


#ifdef _WIN32
    #define ZOMBIE_PATH ".\\Zombie_Attack\\game.exe" // for windows
    #define SNAKE_PATH ".\\Snake_Game\\snake.exe"
#else
    #define ZOMBIE_PATH "./Zombie_Attack/game" // for mac/linx systems
    #define SNAKE_PATH "./Snake_Game/snake"
#endif

int max_x, max_y;

// Main menu options
static const char* menu_items[] = {
    "Zombie Attack!",
    "Snake Game",
    "About", 
    "Quit"
};

static const int n_items = sizeof(menu_items) / sizeof(menu_items[0]);

typedef enum { 
    ZOMBIE_GAME, 
    SNAKE_GAME, 
    ABOUT_GAME, 
    QUIT_GAME 
} MenuOption;


void draw_menu_box(const char** items, int n_items, int highlight, int x_pos, int y_pos) {
    int max_len = 0;
    
    // Find the longest menu item
    for (int i = 0; i < n_items; i++) {
        int len = strlen(items[i]);
        if (len > max_len) max_len = len;
    }
    
    int box_width = max_len + 6;
    int box_height = n_items + 4;
    
    int start_x = x_pos - (box_width / 2);
    int start_y = y_pos - (box_height / 2);
    
    // Draw border
    for (int i = 0; i < box_width; i++) {
        mvprintw(start_y, start_x + i, "=");
        mvprintw(start_y + box_height - 1, start_x + i, "=");
    }
    
    for (int i = 1; i < box_height - 1; i++) {
        mvprintw(start_y + i, start_x, "|");
        mvprintw(start_y + i, start_x + box_width - 1, "|");
        
        for (int j = 1; j < box_width - 1; j++) {
            mvprintw(start_y + i, start_x + j, " ");
        }
    }
    
    
    mvprintw(start_y + 1, start_x + (box_width - 11) / 2, "GAME CENTER");
    
    // Draw menu
    for (int i = 0; i < n_items; i++) {
        int item_x = start_x + 3;
        int item_y = start_y + 3 + i;
        
        if (highlight == i) {
            attron(A_REVERSE);
            mvprintw(item_y, item_x, "> %s", items[i]);
            attroff(A_REVERSE);
        } else {
            mvprintw(item_y, item_x, "  %s", items[i]);
        }
    }
}


int run_game(const char* game_executable) {
    // TODO: Check if the game works before trying to run it
    
    endwin(); 

    system("clear || cls");
    
    int result = system(game_executable); // runs the game
    
    // Reinitialize ncurses after game ends
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    getmaxyx(stdscr, max_y, max_x);
    
    return 0;
}

void display_main_menu() {
    MenuOption selection = ZOMBIE_GAME;
    int quit = 0; 
    
    while (!quit) {
        clear();
        
        draw_menu_box(menu_items, n_items, selection, max_x / 2, max_y / 2);
        
        mvprintw(max_y - 3, 2, "Use ↑↓ arrows to navigate and press enter to select mode.");
        
        refresh();
        
        int ch = getch();
        switch (ch) {
            case KEY_UP:
                selection = (selection > 0) ? selection - 1 : n_items - 1;
                break;
            case KEY_DOWN:
                selection = (selection < n_items - 1) ? selection + 1 : 0;
                break;
            case 10:  // Enter key (line feed)
            case 13:  // Enter key (carriage return)
            case KEY_ENTER:
                switch (selection) {
                    case ZOMBIE_GAME:
                        run_game(ZOMBIE_PATH);
                        break;
                    case SNAKE_GAME:
                        run_game(SNAKE_PATH);
                        break;
                    case QUIT_GAME:
                        quit = 1;
                        break;
                }
                break;
            case 27: // esc key
                quit = 1;
                break;
            default:
                break;
        }
    }
}

int main() {
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // gets user screen info
    getmaxyx(stdscr, max_y, max_x);
    
    display_main_menu();
    
    clear();
    
    refresh();
    
    
#ifdef _WIN32 // pauses for one second
    Sleep(1000); // windows uses milliseconds
#else
    usleep(1000000); // mac/linus use microseconds
#endif
    
    
    endwin(); // housekeeping 
    return 0;
}