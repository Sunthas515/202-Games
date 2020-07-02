#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <cab202_graphics.h>
#include <cab202_timers.h>
// Insert other functions here, or include header files

// Set this to true when simulation is over
bool game_over = false;
bool pause = false;

//Set active character
char player = 'J';

//Set up counters for game objects
int cheese_count = 0;
int cheese_consumed = 0;
int room_catches = 0;
int mousetrap_counter = 0;
int weapon_counter = 0;

// Jerry state
double Jerry_x, Jerry_y, Jerry_dx, Jerry_dy;
int lives;
#define JERRY 'J'
#define MAX_LIVES 5

// Tom state
double Tom_x, Tom_y, Tom_dx, Tom_dy;
#define TOM 'T'

// Cheese state
double cheese_x, cheese_y;
int tom_score;
int jerry_score;
#define CHEESE '^'
time_t two = 0;
time_t delay_cheese = 0;

// Door state
double door_x, door_y;
#define DOOR 'X'

//Mousetrap state
#define MOUSETRAP 'M'
time_t three = 0;
time_t delay_mousetrap = 0;

// Screen dimensions
int W, H;

//Tracker for levels
int levels = 1;
char **arguments;

//Arrays for drawing
double wall_array[4000];
int cheeseX[5];
int cheeseY[5];
int mousetrapX[5];
int mousetrapY[5];

//Generate Game clock
time_t seconds = 0;
time_t minutes = 0;
time_t delay_count = 0;

void clock()
{
    delay_count++;
    if (delay_count == 100)
    {
        seconds++;
        delay_count = 0;
        if (seconds == 60)
        {
            seconds = 0;
            minutes++;
        }
    }
}

/**
 *	Draw a walls within the room using '*' symbols.
 */

void load_vars(FILE *stream)
{
    int j = 0;
    char command;
    // Declare variables
    double wall_x1, wall_y1, wall_x2, wall_y2;
    while (!feof(stream))
    {

        int wall_count = fscanf(stream, " %c%lf%lf%lf%lf", &command, &wall_x1, &wall_y1, &wall_x2, &wall_y2);

        //Set variables for walls
        wall_x1 = W * wall_x1;
        wall_y1 = H * wall_y1 + 3;
        wall_x2 = W * wall_x2;
        wall_y2 = H * wall_y2 + 3;

        if (command == 'J')
        {
            Jerry_x = wall_x1 + 1;
            Jerry_y = wall_y1 + 2;
        }
        else if (command == 'T')
        {
            Tom_x = wall_x1 - 5;
            Tom_y = wall_y1 - 5;
        }
        else if (command == 'W')
        {
            for (int i = j; i < 4000; i+=4)
            {
                wall_array[i] = wall_x1;
                wall_array[i+1] = wall_y1;
                wall_array[i+2] = wall_x2;
                wall_array[i+3] = wall_y2;
            }
        }
        j += 4;
    }
}

void draw_walls()
{
    for (int i = 0; i < 4000; i+=4)
    {
        draw_line(wall_array[i], wall_array[i+1], wall_array[i+2], wall_array[i+3], '*');
    }
}

/**
 *	Draw the status information.
 */
void draw_status()
{
    if (player == 'J')
    {
        draw_formatted(1, 1, "Student Number: n10482652 \t Score:%2d \t Lives:%2d \t Active Player:%c \t Time elapsed: %02d:%02d", jerry_score, lives, player, minutes, seconds);
    }
    else if (player == 'T')
    {
        draw_formatted(1, 1, "Student Number: n10482652 \t Score:%2d \t Lives:%2d \t Active Player:%c \t Time elapsed: %02d:%02d", tom_score, lives, player, minutes, seconds);
    }
    draw_formatted(1, 2, "");
    draw_formatted(1, 3, "No. cheese:%2d \t No. Mousetraps:%2d \t No. of Weapons:%2d", cheese_count, mousetrap_counter, weapon_counter);
    draw_line(0, 4, W - 1, 4, '-');
}

/**
 *  Detects collision between objects that each occupy a single pixel.
 *
 *  Parameters:
 *  (x0, y0)   the position of one object.
 *  (*x1, *y1)    the position of the other object.
 *
 *  Returns true if and only if the rounded locations are the same.
 */
bool collided(double x0, double y0, double x1, double y1)
{
    return round(x0) == round(x1) && round(y0) == round(y1);
}

/**
 *	Updates the position of the jerry based on a key code.
 *
 *	Parameter: ch, the key code to process.
 */
void update_player_Jerry(int ch)
{
    if (player == 'J') //Moves player - Jerry
    {
        if (ch == 'a' && Jerry_x > 1 && !(scrape_char(Jerry_x - 1, Jerry_y) == '*'))
        {
            Jerry_x--;
        }
        else if (ch == 'd' && Jerry_x < W - 2 && !(scrape_char(Jerry_x + 2, Jerry_y) == '*'))
        {
            Jerry_x++;
        }
        else if (ch == 's' && Jerry_y < H - 2 && !(scrape_char(Jerry_x, Jerry_y + 1) == '*'))
        {
            Jerry_y++;        
        }
        else if (ch == 'w' && Jerry_y > 5 && !(scrape_char(Jerry_x, Jerry_y - 1) == '*'))
        {
            Jerry_y--;        
        }
    }
}

void update_player_Tom(int ch)
{
    if (player == 'T') //Moves player - Tom
    {
        if (ch == 'a' && Tom_x > 1 && !(scrape_char(Tom_x - 1, Tom_y) == '*'))
        {
            Tom_x--;       
        }
        else if (ch == 'd' && Tom_x < W - 2 && !(scrape_char(Tom_x + 1, Tom_y) == '*'))
        {
            Tom_x++;      
        }
        else if (ch == 's' && Tom_y < H - 2 && !(scrape_char(Tom_x, Tom_y + 1) == '*'))
        {
            Tom_y++;   
        }
        else if (ch == 'w' && Tom_y > 5 && !(scrape_char(Tom_x, Tom_y - 1) == '*'))
        {
            Tom_y--;
        }
    }
}

/**
 *	Sets up the cheese.
 */
void setup_cheese()
{
    delay_cheese++;
    if (delay_cheese == 100)
    {
        two++;
        delay_cheese = 0;
        if (two == 2)
        {
            cheese_x = 1 + rand() % (W - 2);
            cheese_y = 3 + rand() % (H - 5);
            
            cheeseX[cheese_count] = cheese_x;
            cheeseY[cheese_count] = cheese_y;
            two = 0;
            cheese_count++;
        }
    }
}

/**
 *	Draws the cheese.
 */
int draw_cheese(int x, int y)
{
    draw_char(round(x), round(y), CHEESE);
    return 0;
}

/**
 *	Sets up door.
 */
void setup_door()
{
    door_x = 1 + rand() % (W - 2);
    door_y = 3 + rand() % (H - 5);
}

/**
 *	Draws door.
 */
void draw_door()
{
    draw_char(round(door_x), round(door_y), DOOR);
}

/**
 *	Sets up mousetrap.
*/
void setup_mousetrap()
{
    delay_mousetrap++;
        if (delay_mousetrap == 100)
        {
            three++;
            delay_mousetrap = 0;
            if (three == 3)
            {
                mousetrapX[mousetrap_counter] = Tom_x;
                mousetrapY[mousetrap_counter] = Tom_y + 1;
                three = 0;
                mousetrap_counter++;
            }
        }
}

/**
 *	Draws mousetrap.
 */
int draw_mousetrap(int x,int y)
{
    draw_char(round(x), round(y), MOUSETRAP);
    return 0;
}

/**
 *  Updates the state of the cheese, checking for collision with the jerry and
 *  if necessary incrementing the score and re-spawning the cheese.
 */
void update_cheese(int key)
{
    for (int i = 0; i < 5; i++)
    {
        if (collided(Jerry_x, Jerry_y, cheeseX[i], cheeseY[i]))
        {
            jerry_score++;
            cheese_consumed++;
            cheeseX[i] = cheeseX[i+1];
            cheeseY[i] = cheeseY[i+1];
            cheese_count--;
            if (cheese_consumed >= 5)
            {
                setup_door();
            }
        }
    }
}

void update_mousetrap(int key)
{
    for (int i = 0; i < 5; i++)
    {
        if (collided(Jerry_x, Jerry_y, mousetrapX[i], mousetrapY[i]))
        {
            tom_score++;
            room_catches++;
            mousetrap_counter--;
            mousetrapX[i] = mousetrapX[i+1];
            mousetrapY[i] = mousetrapY[i+1];
            if (room_catches >= 5)
            {
                setup_door();
            }
        }
    }
}

/**
 *	Sets up jerry, placing it initially in the centre of the screen.
 */
void setup_jerry()
{
    Jerry_x = 1 + rand() % (W - 2);
    Jerry_y = 3 + rand() % (H - 5);

    double jerry_dir = rand() * M_PI * 2 / RAND_MAX;
    const double step = 0.1;

    Jerry_dx = step * cos(jerry_dir);
    Jerry_dy = step * sin(jerry_dir);
    lives = MAX_LIVES;
}

/**
 *	Draws jerry.
 */
void draw_jerry()
{
    draw_char(round(Jerry_x), round(Jerry_y), JERRY);
}

/**
 *	Draws tom.
 */
void draw_tom()
{
    draw_char(round(Tom_x), round(Tom_y), TOM);
}

/**
 *	Sets up tom at a random location and direction.
 */
void setup_tom()
{
    Tom_x = 1 + rand() % (W - 2);
    Tom_y = 3 + rand() % (H - 5);

    double tom_dir = rand() * M_PI * 2 / RAND_MAX;
    const double step = 0.1;

    Tom_dx = step * cos(tom_dir);
    Tom_dy = step * sin(tom_dir);
}

/**
 * Game Over logic
 */
void do_collided()
{
    clear_screen();

    const char *message[] = {
        "Game Over!",
        "Press [q] to Quit"};

    const int rows = 2;

    for (int i = 0; i < rows; i++)
    {
        // Draw message in middle of screen.
        int len = strlen(message[i]);
        int x = (W - len) / 2;
        int y = (H - rows) / 2 + i;
        draw_formatted(x, y, message[i]);
    }

    show_screen();
    wait_char();

    game_over = true;
}

/**
 *	Moves tom a single step (if possible) with reflection
 *	from the border.
 */
void seek_Jerry()
{
    // Assume that tom has not already collided with the borders.
    // Predict the next screen position of the tom.

    int new_x = round(Tom_x + Tom_dx);
    int new_y = round(Tom_y + Tom_dy);

    bool bounced = false;

    if (new_x == 0 || new_x == screen_width() - 1 || scrape_char(new_x, new_y) == '*')
    {
        // Bounce of left or right wall: reverse horizontal direction
        Tom_dx = -Tom_dx;
        bounced = true;
    }

    if (new_y == 4 || new_y == screen_height() - 1 || scrape_char(new_x, new_y) == '*')
    {
        // Bounce off top or bottom wall: reverse vertical direction
        Tom_dy = -Tom_dy;
        bounced = true;
    }

    if (!bounced)
    {
        // no bounce: move instead.
        Tom_x += Tom_dx;
        Tom_y += Tom_dy;
    }
}

/**
 *	Moves jerry a single step (if possible) with reflection
 *	from the border.
 */
void run_from_Tom()
{
    // Assume that jerry has not already collided with the borders.
    // Predict the next screen position of the jerry.

    int new_x = round(Jerry_x + Jerry_dx);
    int new_y = round(Jerry_y + Jerry_dy);

    bool bounced = false;

    if (new_x == 0 || new_x == screen_width() - 1 || scrape_char(new_x, new_y) == '*')
    {
        // Bounce of left or right wall: reverse horizontal direction
        Jerry_dx = -Jerry_dx;
        bounced = true;
    }

    if (new_y == 4 || new_y == screen_height() - 1 || scrape_char(new_x, new_y) == '*')
    {
        // Bounce off top or bottom wall: reverse vertical direction
        Jerry_dy = -Jerry_dy;
        bounced = true;
    }

    if (!bounced)
    {
        // no bounce: move instead.
        Jerry_x += Jerry_dx;
        Jerry_y += Jerry_dy;
    }
}

/**
 *	Moves tom (if it their turn), and checks for collision
 *	with jerry.
 */
void update_ai(int key)
{
    if (player == 'J')
    {
        seek_Jerry();
    }
    else if (player == 'T')
    {
        run_from_Tom();
    }
}

void collide() //Collider logic between Tom and Jerry
{
    if (collided(Jerry_x, Jerry_y, Tom_x, Tom_y))
    {
        if (player == 'J')
        {
            setup_tom();
            setup_jerry();
            lives--;

            if (lives <= 0)
            {
                do_collided();
            }
        }
        else if (player == 'T')
        {
            setup_jerry();
            setup_tom();
            tom_score += 5;
            room_catches += 5;
            if (room_catches >= 5)
            {
                setup_door();
            }
        }
    }
}

/**
 *	Draws the display.
 */
void draw_all()
{
    if (cheese_count < 5) //setup cheese location
    {
        setup_cheese();
    }

    if (mousetrap_counter < 5) //setup mousetrap location
    {
        setup_mousetrap();
    }
    for (int i = 0; i < 5; i++) //draw cheese and mousetrap
    {
        draw_cheese(cheeseX[i], cheeseY[i]);
        draw_mousetrap(mousetrapX[i], mousetrapY[i]);
    }

    draw_status();
    draw_jerry();
    draw_tom();
    if (cheese_consumed >= 5 || room_catches >= 5)
    {
        draw_door();
    }
}

void loop()
{
    // loop logic
    int key = get_char();
    //quit game
    if (key == 'q')
    {
        game_over = true;
        return;
    }
    //change characters
    if (key == 'z')
    {
        if (player == 'J')
        {
            player = 'T';
        }
        else if (player == 'T')
        {
            player = 'J';
        }
        return;
    }
    //pause game
    if (key == 'p')
    {
        if (!pause)
        {
            pause = true;
        }
        else
        {
            pause = false;
        }
        return;
    }
    update_player_Jerry(key);
    update_player_Tom(key);
    update_cheese(key);
    update_mousetrap(key);
    collide();
    if (!pause)
    {
        update_ai(key);
    }
}
void setup(void)
{
    // setup logic

    FILE *stream = fopen(arguments[levels], "r");
    if (stream != NULL)
    {
        load_vars(stream);
        fclose(stream);
    }
    srand(get_current_time());
    W = screen_width();
    H = screen_height();
    setup_jerry();
    setup_tom();
}
/**
 * Win condition
 */
void win()
{
    if (collided(Jerry_x, Jerry_y, door_x, door_y) || (collided(Tom_x, Tom_y, door_x, door_y)))
    {
        do_collided();
    }
}
int main(int argc, char *argv[])
{
    const int DELAY = 10;
    int i = 1;
    arguments = argv;
    setup_screen();
    setup();

    while (!game_over)
    {
        if (!pause)
        {
            clock();
        }
        clear_screen();
        loop();
        draw_all();
        draw_walls();
        win();
        timer_pause(DELAY);
        show_screen();
    }
    return 0;
}