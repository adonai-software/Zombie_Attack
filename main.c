#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_OF_OBSTRUCTION 20
#define NUM_OF_FOOD 5
#define MAX_X 50
#define MAX_Y 50

/* TODO
	- Convert int x,y into struct Point [x]
	- Write unflatten function
*/

typedef enum {
	PLAYER,
	OBSTRUCTION,
	FOOD	
} Type;

typedef struct {
	int x, y;
} Point;

typedef struct {
	Point p;
	Type t;
} object;

// x,y grid is mapped unto a 2 demensional array
object *map[MAX_X * MAX_Y] = {NULL};
object p1;
object perimeter[(2*MAX_X)+(2*MAX_Y)];
object foods[NUM_OF_FOOD];
object obstructions[NUM_OF_OBSTRUCTION];

int flatten(int x, int y) {
	return y == 1 ? x : ((y-1)*MAX_X)+x;
}

Point unflatten(int z) {
	Point p;
	p.x = 1;
	p.y = 1;
	return p;
}

bool set_object(object *o, int x, int y) {
	int new_pos = flatten(x,y);
	int old_pos = flatten(o->p.x, o->p.y);

	if(map[new_pos] == NULL) {
		map[new_pos] = o;
		if(o->t == PLAYER)
			map[old_pos] = NULL;
		return true;
	} else if (o->t == PLAYER && map[new_pos]->t == FOOD) {
		map[new_pos] = o;
		map[old_pos] = NULL;
		return true;
		// player + 1 food
	}

	return false;
}

void draw_scene() {
	int num_food = 0;
	for(int i = 0;i < MAX_Y*MAX_Y;i++) {
		if(map[i] == NULL) {
			continue;
		} else {
			switch(map[i]->t) {
				case PLAYER:
					mvprintw(map[i]->p.y, map[i]->p.x, "t");
					break;
				case OBSTRUCTION:
					mvprintw(map[i]->p.y, map[i]->p.x, "O");
					break;
				case FOOD:
					mvprintw(map[i]->p.y, map[i]->p.x, "x");
					num_food++;
					break;
				default:
					break;
			}
		}
	}

	if(num_food <= 0) {
		clear(); // Clear the screen
		mvprintw(10, 10, "%s", "Winner!");
	} else {
		mvprintw(0,0,"%s %d", "food remaining = ", num_food);
	}

}

int main() {
    initscr();         // Initialize ncurses
    cbreak();          // Line buffering disabled
    noecho();          // Don't echo input characters
    keypad(stdscr, TRUE); // Enable special keys
    curs_set(0);       // Hide cursor

		srand(time(NULL));

    int ch;

	// Initialize player start
	p1.t = PLAYER;
	p1.p.x = 1;
	p1.p.y = 1;
	set_object(&p1, 1, 1);

	// Generate objects
	for(int i = 0; i < NUM_OF_OBSTRUCTION; i++) {
		obstructions[i].t = OBSTRUCTION;
		obstructions[i].p.x = rand() % MAX_X;
		obstructions[i].p.y = rand() % MAX_Y;
		set_object(&obstructions[i], obstructions[i].p.x, obstructions[i].p.y);
	}

	// Generate foods
	for(int i = 0; i < NUM_OF_FOOD; i++) {
		foods[i].t = FOOD;
		foods[i].p.x = rand() % MAX_X;
		foods[i].p.y = rand() % MAX_Y;
		set_object(&foods[i], foods[i].p.x, foods[i].p.y);
	}
	


    while ((ch = getch()) != 'q') { // Game loop, exit on 'q'
        clear(); // Clear the screen

        // Update player position based on input
        switch (ch) {
            case KEY_UP:    
		if(set_object(&p1, p1.p.x, p1.p.y-1))
			p1.p.y--; 
		break;
            case KEY_DOWN:  
		if(set_object(&p1, p1.p.x, p1.p.y+1))
			p1.p.y++; 
		break;
            case KEY_LEFT:  
		if(set_object(&p1, p1.p.x-1, p1.p.y))
			p1.p.x--; 
		break;
            case KEY_RIGHT: 
		if(set_object(&p1, p1.p.x+1, p1.p.y))
			p1.p.x++; 
		break;
        }

	draw_scene();

        refresh(); // Update the screen
    }

    endwin(); // Clean up ncurses
    return 0;
}
