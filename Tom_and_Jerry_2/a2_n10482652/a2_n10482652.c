#include <avr/io.h>
#include <util/delay.h>
#include <cpu_speed.h>
#include <graphics.h>
#include <lcd.h>
#include <macros.h>
#include <lcd_model.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>
#include <cab202_adc.h>
#include <usb_serial.h>


#define FREQ     (8000000.0)
#define PRESCALE (1024.0)
#define BIT(x) (1<< (x))
#define OVERFLOW_TOP (1023)
#define ADC_MAX (1023)
char buffer[20];

void draw_time(uint8_t x, uint8_t y, int value, colour_t colour) {
	snprintf(buffer, sizeof(buffer), "%02d", value);
	draw_string(x, y, buffer, colour);
}

void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
	snprintf(buffer, sizeof(buffer), "%d", value);
	draw_string(x, y, buffer, colour);
}

//Serial setup
void setup_usb_serial( void );
void usb_serial_send(char * message)
{
    usb_serial_write((uint8_t *) message, strlen(message));
}

void usb_serial_read_string( char * message)
{
    int c = 0;
    int buffer_count = 0;

    while (c != '\n')
    {
        c = usb_serial_getchar();
        message[buffer_count] = c;
        buffer_count++;
    }
}

//PWM
void set_duty_cycle(int duty_cycle)
{
    TC4H = duty_cycle >> 8;

    OCR4A = duty_cycle & 0xff;
}

//Global Variables
int level = 1;
int lives = 5;
int score = 0;
int minutes = 0;
int seconds = 0;
int milliseconds = 0;
int game;
double tolerance = 0.1;
int Jerry_Col[4];
int Tom_Col;
#define SQRT(x, y) sqrt(x*x + y*y)
int door_x;
int door_y;
int draw = 0;

//Moustrap variables
int mousetrap_time = 0;
int mousetrap_count = 0;
int mousetrapX[5] = {100, 100, 100, 100, 100};
int mousetrapY[5] = {100, 100, 100, 100, 100};

//Milk variables
int milk_time = 0;
int milk_count = 0;
int milk_x = 0;
int milk_y = 0;
int super = 0;

//Cheese variables
int cheese_count = 0;
int cheese_time = 0;
int cheeseX[5] = {100, 100, 100, 100, 100};
int cheeseY[5] = {100, 100, 100, 100, 100};
int cheese_consumed = 0;

//Fireworks variables
int fireworks_count = 0;
int fireworksX[20] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
int fireworksY[20] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
int hit = 0;
int firework_col[20];

//Values for bitmap locations
#define JERRY_X 0
#define JERRY_Y 11
#define TOM_X LCD_X-5
#define TOM_Y LCD_Y-9
int Jerry_x = 0;
int Jerry_y = 11;
int Tom_x = 78;
int Tom_y = 40;

//Bitmaps
char Tom = 'T';
char Jerry = 'j';
char JERRY = 'J';
char Cheese = 'C';
char Mousetrap = '@';
char Door = 'X';
char Milk = 'o';

//Coords for lines
int wall_x1[] = {18, 25, 45, 58};
int wall_y1[] = {15, 35, 10, 25};
int wall_x2[] = {13, 25, 60, 72};
int wall_y2[] = {25, 45, 10, 30};

int wall2_x1[] = {10, 25, 45, 58};
int wall2_y1[] = {20, 35, 30, 25};
int wall2_x2[] = {20, 10, 15, 72};
int wall2_y2[] = {10, 15, 30, 30};

int Jx, Jy;
int Tx, Ty;
int Xw[4], Yw[4];

//Directions for walls
int wall_time = 0;
char w3_dir = 'U';
char w1_dir = 'R';
char w2_dir = 'R';
char w4_dir = 'L';

//adc
int adc_wall;
int adc_characters;

//Initialise all inputs
void button_init()
{
    CLEAR_BIT(DDRF, 5); //Initialise SW3 for input
    CLEAR_BIT(DDRF, 6); //Initialise SW2 for input

    //Initialise joystick for input
    CLEAR_BIT(DDRD, 0); //right
    CLEAR_BIT(DDRD, 1); //up
    CLEAR_BIT(DDRB, 0); //centre
    CLEAR_BIT(DDRB, 1); //left
    CLEAR_BIT(DDRB, 7); //down

    //Initialise LED for output
    SET_BIT(DDRB, 2);
    SET_BIT(DDRB, 3);
}

void adc_startup()
{
    int right_adc = adc_read(1);
    int left_adc = adc_read(0);

    adc_wall = (right_adc * 6 / 1024) - 3;
    adc_characters = left_adc * 5 / 1024;
}
//Setup program and display the starting screen as specified in Task 1b
void start_screen()
{
    //Display Title
    draw_string(0, 0, "Callum McNeilage", FG_COLOUR);
    draw_string(0, 10, "n14082652", FG_COLOUR);
    draw_string(0, 20, "Tom and Jerry:", FG_COLOUR);
    draw_string(0, 30, "The Game", FG_COLOUR);

    //Display control prompt
    draw_string(15, 40, "(Press SW3)", FG_COLOUR);

    button_init();
    show_screen();
}

void statusbar()
{
    //draw info panel
    draw_string(0, 2, "__________________", FG_COLOUR);
    draw_string(0, 0, "L:", FG_COLOUR);
    draw_int(10, 0, level, FG_COLOUR);
    draw_string(15, 0, "H:", FG_COLOUR);
    draw_int(25, 0, lives, FG_COLOUR);
    draw_string(30, 0, "S:", FG_COLOUR);
    draw_int(40, 0, score, FG_COLOUR);
    draw_string(45, 0, "T:", FG_COLOUR);
    draw_time(55, 0, minutes, FG_COLOUR);
    draw_string(65, 0, ":", FG_COLOUR);
    draw_time(70, 0, seconds, FG_COLOUR);
}

void draw_walls()
{
    if (level == 1)
    {
        //draw walls
        for (int i = 0; i < 4; i++)
        {
            draw_line(wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], FG_COLOUR);
        }
    }
    else if (level == 2)
    {
        //draw walls
        for (int i = 0; i < 4; i++)
        {
            draw_line(wall2_x1[i], wall2_y1[i], wall2_x2[i], wall2_y2[i], FG_COLOUR);
        }
    }
}

void move_walls()
{
    if (wall_time == 2)
    {
        if (level == 1)
        {
            for (int i = 0; i < 4; i++)
            {
                if (i == 0)
                {
                    if (w1_dir == 'R')
                    {
                        wall_x1[i] = wall_x1[i] + adc_wall;
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_x2[i] = wall_x2[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_y2[i] == LCD_Y)
                        {
                            w1_dir = 'L';
                        }
                    }

                    if (w1_dir == 'L')
                    {
                        wall_x1[i] = wall_x1[i] - adc_wall;
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_x2[i] = wall_x2[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if(wall_y1[i] == 10)
                        {
                            w1_dir = 'R';
                        }
                    }
                }
                else if (i == 1)
                {
                    if (w1_dir == 'R')
                    {
                        wall_x1[i] = wall_x1[i] + adc_wall;
                        wall_x2[i] = wall_x2[i] + adc_wall;

                        if(wall_x1[i] == LCD_X)
                        {
                            w1_dir = 'L';
                        }
                    }

                    if (w1_dir == 'L' )
                    {
                        wall_x1[i] = wall_x1[i] - adc_wall;
                        wall_x2[i] = wall_x2[i] - adc_wall;

                        if(wall_x1[i] == 0)
                        {
                            w1_dir = 'R';
                        }
                    }
                }
                else if (i == 2)
                {
                    if (w3_dir == 'U')
                    {
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_y1[i] == LCD_Y)
                        {
                            w3_dir = 'D';
                        }
                    }
            
                    if (w3_dir == 'D')
                    {
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if (wall_y1[i] == 10)
                        {
                            w3_dir = 'U';
                        }
                    }
                }
                else if (i == 3)
                {
                    if (w4_dir == 'L')
                    {
                        wall_x1[i] = wall_x1[i] - adc_wall;
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_x2[i] = wall_x2[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_y2[i] == LCD_Y)
                        {
                            w4_dir = 'R';
                        }
                    }

                    if (w4_dir == 'R')
                    {
                        wall_x1[i] = wall_x1[i] + adc_wall;
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_x2[i] = wall_x2[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if(wall_x2[i] == LCD_X)
                        {
                            w4_dir = 'L';
                        }
                    }
                }
            }
        }
        else if (level == 2)
        {
            for (int i = 0; i < 4; i++)
            {
                if (i == 0)
                {
                    if (w1_dir == 'R')
                    {
                        wall_x1[i] = wall_x1[i] + adc_wall;
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_x2[i] = wall_x2[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_y2[i] == LCD_Y)
                        {
                            w1_dir = 'L';
                        }
                    }

                    if (w1_dir == 'L')
                    {
                        wall_x1[i] = wall_x1[i] - adc_wall;
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_x2[i] = wall_x2[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if(wall_y1[i] == 10)
                        {
                            w1_dir = 'R';
                        }
                    }
                }
                else if (i == 1)
                {
                    if (w1_dir == 'R')
                    {
                        wall_x1[i] = wall_x1[i] + adc_wall;
                        wall_x2[i] = wall_x2[i] + adc_wall;
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_x1[i] == LCD_X)
                        {
                            w1_dir = 'L';
                        }
                    }

                    if (w1_dir == 'L' )
                    {
                        wall_x1[i] = wall_x1[i] - adc_wall;
                        wall_x2[i] = wall_x2[i] - adc_wall;
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if(wall_x1[i] == 0)
                        {
                            w1_dir = 'R';
                        }
                    }
                }
                else if (i == 2)
                {
                    if (w3_dir == 'U')
                    {
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_y1[i] == LCD_Y)
                        {
                            w3_dir = 'D';
                        }
                    }
            
                    if (w3_dir == 'D')
                    {
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if (wall_y1[i] == 10)
                        {
                            w3_dir = 'U';
                        }
                    }
                }
                else if (i == 3)
                {
                    if (w4_dir == 'L')
                    {
                        wall_x1[i] = wall_x1[i] - adc_wall;
                        wall_y1[i] = wall_y1[i] + adc_wall;
                        wall_x2[i] = wall_x2[i] - adc_wall;
                        wall_y2[i] = wall_y2[i] + adc_wall;

                        if(wall_y2[i] == LCD_Y)
                        {
                            w4_dir = 'R';
                        }
                    }

                    if (w4_dir == 'R')
                    {
                        wall_x1[i] = wall_x1[i] + adc_wall;
                        wall_y1[i] = wall_y1[i] - adc_wall;
                        wall_x2[i] = wall_x2[i] + adc_wall;
                        wall_y2[i] = wall_y2[i] - adc_wall;

                        if(wall_x2[i] == LCD_X)
                        {
                            w4_dir = 'L';
                        }
                    }
                }
            }            
        }
        wall_time = 0;
    }
}

void Potion()
{
    if (level == 2)
    {
        if (milk_count == 0)
        {
            milk_x = Tom_x;
            milk_y = Tom_y;
        }
    
        if(milk_time == 50 && milk_count < 1)
        {
            draw_char(milk_x, milk_y, Milk, FG_COLOUR);
            milk_time = 0;
            milk_count++;
        }

        if (milk_count == 1)
        {
            draw_char(milk_x, milk_y, Milk, FG_COLOUR);
        }
    }
}

void MouseTrap()
{
    //Spawn mousetraps every 3 secs to max of 5
    for (int i = 0; i < 5; i++)
    {
        if (mousetrap_time == 30 && mousetrap_count < 6)
        {
            draw_char(Tom_x, Tom_y, Mousetrap, FG_COLOUR); //Spawn at Tom's location
            mousetrap_time = 0;
            for (int j = mousetrap_count; j < 5; j++)
            {
                mousetrapX[mousetrap_count] = Tom_x; //Add to array
                mousetrapY[mousetrap_count] = Tom_y;
            }
            mousetrap_count++;
        }

        if (mousetrap_count > 0) //Draw each screen
        {
            draw_char(mousetrapX[i], mousetrapY[i], Mousetrap, FG_COLOUR);
        }
    }
}

void cheese()
{
    //Randomise cheese position
    int cheese_x = 1 + rand() % (LCD_X - 2);
    int cheese_y = 3 + rand() % (LCD_Y - 10);

    //Spawn cheese every 2 secs to a max of 5
    for (int i = 0; i < 5; i++)
    {
        if (cheese_time == 20 && cheese_count < 6)
        {
            draw_char(cheese_x, cheese_y, Cheese, FG_COLOUR);
            cheese_time = 0;
            for (int j = cheese_count; j < 5; j++) //add cheese locations to array
            {
                if (cheese_y < 11)
                {
                    cheeseX[cheese_count] = cheese_x;
                    cheeseY[cheese_count] = cheese_y + 10;
                }
                else if ((cheese_x + 5) > LCD_X)
                {
                    cheeseX[cheese_count] = cheese_x - 10;
                    cheeseY[cheese_count] = cheese_y;
                }
                else
                {
                    cheeseX[cheese_count] = cheese_x;
                    cheeseY[cheese_count] = cheese_y;
                }
                
            }
                
            cheese_count++;
        }

        if (cheese_count > 0) //draw cheese on each screen
        {
            draw_char(cheeseX[i], cheeseY[i], Cheese, FG_COLOUR);
        }
    }
}

void Tom_hit()
{
    Tom_x = TOM_X;
    Tom_y = TOM_Y;
}

void draw_firework()
{
    int fireworks_x;
    int fireworks_y;

    fireworks_x = Jerry_x;
    fireworks_y = Jerry_y;
    draw_pixel(fireworks_x, fireworks_y, FG_COLOUR);

    fireworksX[fireworks_count] = fireworks_x;
    fireworksY[fireworks_count] = fireworks_y;
    fireworks_count++;
}

void move_firework()
{
    for (int i = 0; i < fireworks_count; i++)
    {
        if (fireworksX[i] < Tom_x && !firework_col[i])
        {
            fireworksX[i]++;
        }
        else if (fireworksX[i] > Tom_x && !firework_col[i])
        {
            fireworksX[i]--;
        }
        else if (fireworksY[i] < Tom_y && !firework_col[i])
        {
            fireworksY[i]++;
        }
        else if (fireworksY[i] > Tom_y && !firework_col[i])
        {
            fireworksY[i]--;
        }
        draw_pixel(fireworksX[i], fireworksY[i], FG_COLOUR);

        if (fireworksX[i] == Tom_x && fireworksY[i] == Tom_y)
        {
            fireworksX[i] = 100;
            fireworksY[i] = 100;
            fireworks_count--;
            hit = 1;
        }
        
        if (firework_col[i])
        {
            fireworksX[i] = 100;
            fireworksY[i] = 100;
            fireworks_count--;
        }
    }
    
    if (hit == 1)
    {
        Tom_hit();
    }
}

double findDist(int x1, int y1, int x2, int y2)
{
    return SQRT((x1 - x2), (y1 - y2));
}

int PointLinesOnLine(int xp, int yp, int x1, int y1, int x2, int y2, double tol)
{
    double dist1 = findDist(xp, yp, x1, y1);
    double dist2 = findDist(xp, yp, x2, y2);
    double dist3 = findDist(x1, y1, x2, y2);

    return abs(dist3 - (dist1 + dist2)) <= tol;
}

void Jerry_collision()
{
    if (level == 1)
    {
        for (int i = 0; i < 4; i++)
        {
            //int i = 0;
            if(BIT_IS_SET(PIND, 1))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x, Jerry_y, wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], tolerance);
            }
            else if(BIT_IS_SET(PINB, 7))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x, Jerry_y + 11, wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], tolerance);
            }
            else if(BIT_IS_SET(PINB, 1))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x - 2, Jerry_y, wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], tolerance);
            }
            else if(BIT_IS_SET(PIND, 0))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x + 5, Jerry_y, wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], tolerance);
            }   
        }
    }

    if (level == 2)
    {
        for (int i = 0; i < 4; i++)
        {
            //int i = 0;
            if(BIT_IS_SET(PIND, 1))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x, Jerry_y, wall2_x1[i], wall2_y1[i], wall2_x2[i], wall2_y2[i], tolerance);
            }
            else if(BIT_IS_SET(PINB, 7))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x, Jerry_y + 11, wall2_x1[i], wall2_y1[i], wall2_x2[i], wall2_y2[i], tolerance);
            }
            else if(BIT_IS_SET(PINB, 1))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x - 2, Jerry_y, wall2_x1[i], wall2_y1[i], wall2_x2[i], wall2_y2[i], tolerance);
            }
            else if(BIT_IS_SET(PIND, 0))
            {
                Jerry_Col[i] = PointLinesOnLine(Jerry_x + 5, Jerry_y, wall2_x1[i], wall2_y1[i], wall2_x2[i], wall2_y2[i], tolerance);
            }   
        }
    }
}

void firework_collide()
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < fireworks_count; j++)
        {
            firework_col[i] = PointLinesOnLine(fireworksX[j], fireworksY[j], wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], tolerance);
        }
    }
}

void Tom_collide()
{
    for (int i = 0; i < 4; i++)
    {
        Tom_Col = PointLinesOnLine(Tom_x, Tom_y, wall_x1[i], wall_y1[i], wall_x2[i], wall_y2[i], tolerance);
    }
}

void draw_player()
{
    //Draw Jerry
    if (super == 1)
    {
        draw_char(Jerry_x, Jerry_y, JERRY, FG_COLOUR);
    }
    else
    {
        draw_char(Jerry_x, Jerry_y, Jerry, FG_COLOUR);
    }
    
    //draw Tom
    draw_char(Tom_x, Tom_y, Tom, FG_COLOUR);
}

void move_Jerry()
{
    if((BIT_IS_SET(PIND, 1) && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
    {
        if (Jerry_y > 10) //within top bound
        {
            Jerry_y = Jerry_y - adc_characters; //move up
        }
    }
    else if((BIT_IS_SET(PINB, 7) && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
    {
        if (Jerry_y < 40) //within bottom bound
        {
            Jerry_y = Jerry_y + adc_characters; //move down
        }
    }
    else if((BIT_IS_SET(PINB, 1) && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
    {
        if (Jerry_x >= 0) //within left bound
        {
            Jerry_x = Jerry_x - adc_characters; //move left
        }
    }
    else if((BIT_IS_SET(PIND, 0) && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
    {
        if (Jerry_x < 78) //within right bound
        {
            Jerry_x = Jerry_x + adc_characters; //move right
        }
    }
    else if(BIT_IS_SET(PINB, 0))
    {
        while (BIT_IS_SET(PINB, 0))
        {
            //Do nothing
        }
        if (cheese_consumed >= 3)
        {
            //Shoot fireworks
            draw_firework();
        }
    }
    else
    {
        //Do nothing
    }
}

void move_Tom()
{
    int Move = rand() % 100;

    if(Move < 25) //move up if between 0 and 25
    {
        if (Tom_y > 10)
        {
            Tom_y = Tom_y - (int) adc_characters;

            /*if (Tom_y <= 10)
            {
                Tom_y = 11;
            }*/
        }
    }
    else if(Move < 50 && Move > 25) //move down if between 25 and 50
    {
        if (Tom_y < 40)
        {
            Tom_y = Tom_y + (int) adc_characters;

            /*if (Tom_y >= 40)
            {
                Tom_y = 39;
            }*/
        }
    }
    else if(Move < 75 && Move > 50) //move left if between 50 and 75
    {
        if (Tom_x >= 0)
        {
            Tom_x = Tom_x - adc_characters;

            /*if (Tom_x > 0)
            {
                Tom_x = 0;
            }*/
        }
    }
    else if(Move < 100 && Move > 75) //move right if between 75 and 100
    {
        if (Tom_x < 78)
        {
            Tom_x = Tom_x + adc_characters;

            /*if (Tom_x >= 78)
            {
                Tom_x = 77;
            }*/
        }
    }
    else
    {
        //Do nothing
    }
}

void Super_Jerry()
{
    int super_time = 0;
    
    while (super_time < 100)
    {
        super = 1;
    }
    
    super_time = 0;
}

void player_collide()
{
    if((Jerry_x == Tom_x && Jerry_y == Tom_y) && super == 0) //return true is bitmaps overlap
    {
        Jerry_x = JERRY_X;
        Jerry_y = JERRY_Y;
        Tom_x = TOM_X;
        Tom_y = TOM_Y;
        lives--;
    }
    else if ((Jerry_x == Tom_x && Jerry_y == Tom_y) && super == 1)
    {
        Jerry_x = JERRY_X;
        Jerry_y = JERRY_Y;
        Tom_x = TOM_X;
        Tom_y = TOM_Y;
        score++;
    }
}

void mousetrap_collide() //return true if bitmaps overlap
{
    for (int i = 0; i < mousetrap_count; i++)
    {
        if ((Jerry_x == mousetrapX[i] && Jerry_y == mousetrapY[i]) && super == 0)
        {
            lives--;
            mousetrapX[i] = 100;
            mousetrapY[i] = 100;
        }
    }
}

void cheese_collide() //return true if bitmaps overlap
{
    for (int i = 0; i < cheese_count; i++)
    {
        if (Jerry_x == cheeseX[i] && Jerry_y == cheeseY[i])
        {
            score++;
            cheese_consumed++;
            cheeseX[i] = 100;
            cheeseY[i] = 100;
            cheese_count--;
        }
    }
}

void potion_collide()
{
    if (Jerry_x == milk_x && Jerry_y == milk_y)
    {
        Super_Jerry();
    }
}

void draw_level()
{    
    statusbar();
    draw_walls();
    wall_time++;
}

void Game_over()
{
    //Display game over screen
    clear_screen();
    draw_string(0, 10, "Game Over!", FG_COLOUR);
    draw_string(0, 20, "Press SW3 to", FG_COLOUR);
    draw_string(0, 30, "restart", FG_COLOUR);
    show_screen();
    game = 0;

    //Reset level
    if (BIT_IS_SET(PINF, 5))
    {
        while (BIT_IS_SET(PINF, 5))
        {
            //Do nothing
        }
        draw_level();
        level = 1;
        cheese_count = 0;
        mousetrap_count = 0;
        mousetrap_time = 0;
        cheese_time = 0;
        cheese_consumed = 0;
        milk_count = 0;
        for (int i = 0; i < 5; i++)
        {
            cheeseX[i] = 100;
            cheeseY[i] = 100;
            mousetrapX[i] = 100;
            mousetrapY[i] = 100;
        }
    }
}

void draw_door()
{
    //Set door location
    if (draw == 0)
    {
        door_x = 1 + rand() % (LCD_X - 5);
        door_y = 10 + rand() % (LCD_Y - 10);
        draw = 1;
    }
    draw_char(round(door_x), round(door_y), Door, FG_COLOUR); //draw door
}

void door_collide() //display game over if bitmaps overlap
{
    if(Jerry_x == door_x && Jerry_y == door_y)
    {
        while (level < 3)
        {
            level++;
            cheese_count = 0;
            mousetrap_count = 0;
            mousetrap_time = 0;
            cheese_time = 0;
            cheese_consumed = 0;
            for (int i = 0; i < 5; i++)
            {
                cheeseX[i] = 100;
                cheeseY[i] = 100;
                mousetrapX[i] = 100;
                mousetrapY[i] = 100;
            }
        }
        if (level == 3)
        {
            level = 1;
        }
        Game_over();  
    }
}

void game_clock() //Counter for displaying time
{
    if (milliseconds == 10)
    {
        milliseconds = 0;
        seconds++;
    }
    if (seconds == 60)
    {
        seconds = 0;
        minutes++;
    }
}

void PWM()
{
    // Enable PWM on OC4A (C7), which is wired to LCD backlight. 

	// (a)	Set the TOP value for the timer overflow comparator to 1023, 
	//		yielding a cycle of 1024 ticks per overflow.
	TC4H = OVERFLOW_TOP >> 8;
	OCR4C = OVERFLOW_TOP & 0xff;

	// (b)	Use OC4A for PWM. Remember to set DDR for C7.
	TCCR4A = BIT(COM4A1) | BIT(PWM4A);
	SET_BIT(DDRC, 7);

	// (c)	Set pre-scale to "no pre-scale" 
	TCCR4B = BIT(CS42) | BIT(CS41) | BIT(CS40);

	// (c.1) If you must adjust TCCR4C, be surgical. If you clear
	//		bits COM4A1 and COM4A0 you will _disable_ PWM on OC4A, 
	//		because the COM4Ax and COM4Bx hardware pins appear in
	//		both TCCR4A and TCCR4C!

	/* In this example, TCCR4C is not needed */

	// (d)	Select Fast PWM
	TCCR4D = 0;
}

void Serial_walls()
{
    char tx_buffer[32];
    int c = usb_serial_getchar();
    if(c == 'J')
    {
        usb_serial_read_string(tx_buffer);
        usb_serial_send(tx_buffer);
        sscanf(tx_buffer, "%d %d", &Jx, &Jy);

        Jerry_x = Jx;
        Jerry_y = Jy;
    }

    if (c == 'T')
    {
        usb_serial_read_string(tx_buffer);
        usb_serial_send(tx_buffer);
        sscanf(tx_buffer, "%d %d", &Tx, &Ty);

        Tom_x = Tx;
        Tom_y = Ty;
    }

    if (c == '1')
    {
        usb_serial_read_string(tx_buffer);
        usb_serial_send(tx_buffer);
        sscanf(tx_buffer, "%d %d %d %d", &Xw[0], &Yw[0], &Xw[1], &Yw[1]);

        wall_x1[0] = Xw[0];
        wall_y1[0] = Yw[0];
        wall_x2[0] = Xw[1];
        wall_y2[0] = Yw[1];
    }

    if (c == '2')
    {
        usb_serial_read_string(tx_buffer);
        usb_serial_send(tx_buffer);
        sscanf(tx_buffer, "%d %d %d %d", &Xw[0], &Yw[0], &Xw[1], &Yw[1]);

        wall_x1[1] = Xw[0];
        wall_y1[1] = Yw[0];
        wall_x2[1] = Xw[1];
        wall_y2[1] = Yw[1];
    }

    if (c == '3')
    {
        usb_serial_read_string(tx_buffer);
        usb_serial_send(tx_buffer);
        sscanf(tx_buffer, "%d %d %d %d", &Xw[0], &Yw[0], &Xw[1], &Yw[1]);

        wall_x1[2] = Xw[0];
        wall_y1[2] = Yw[0];
        wall_x2[2] = Xw[1];
        wall_y2[2] = Yw[1];
    }

    if (c == '4')
    {
        usb_serial_read_string(tx_buffer);
        usb_serial_send(tx_buffer);
        sscanf(tx_buffer, "%d %d %d %d", &Xw[0], &Yw[0], &Xw[1], &Yw[1]);

        wall_x1[3] = Xw[0];
        wall_y1[3] = Yw[0];
        wall_x2[3] = Xw[1];
        wall_y2[3] = Yw[1];
    }
}

//Serial assign
void Serial_movement()
{
    if (usb_serial_available())
    {
        char tx_buffer[32];
        int c = usb_serial_getchar();

        if ((c == 'a' && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (Jerry_x >= 0) //within left bound
            {
                Jerry_x = Jerry_x - adc_characters; //move left
            }
        }

        if ((c == 'd' && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (Jerry_x < 78) //within right bound
            {
                Jerry_x = Jerry_x + adc_characters; //move right
            }
        }

        if ((c == 'w' && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (Jerry_y > 10) //within top bound
            {
                Jerry_y = Jerry_y - adc_characters; //move up
            }
        }

        if ((c == 's' && !(Jerry_Col[0] | Jerry_Col[1] | Jerry_Col[2] | Jerry_Col[3])))
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (Jerry_y < 40) //within bottom bound
            {
                Jerry_y = Jerry_y + adc_characters; //move down
            }
        }

        if (c == 'p')
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (game == 1)
            {
                game = 2;
            }
            else
            {
                game = 1;
            }
        }

        if (c == 'f')
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (cheese_consumed >= 3)
            {
                //Shoot fireworks
                draw_firework();
            }
        }

        if (c == 'l')
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved '%c'\r\n", c);
            usb_serial_send(tx_buffer);

            if (level == 2)
            {
                Game_over();
            }
            else
            {
                level++;
                cheese_count = 0;
                mousetrap_count = 0;
                mousetrap_time = 0;
                cheese_time = 0;
                cheese_consumed = 0;
                milk_count = 0;
                milk_x = 100;
                milk_y = 100;
                for (int i = 0; i < 5; i++)
                {
                    cheeseX[i] = 100;
                    cheeseY[i] = 100;
                    mousetrapX[i] = 100;
                    mousetrapY[i] = 100;
                }
            }

            wall_x1[0] = 18, wall_x1[1] = 25, wall_x1[2] = 45, wall_x1[3] = 58;
            wall_y1[0] = 15, wall_y1[1] = 35, wall_y1[2] = 10, wall_y1[3] = 25;
            wall_x2[0] = 13, wall_x2[1] = 25, wall_x2[2] = 60, wall_x2[3] = 72;
            wall_y2[0] = 25, wall_y2[1] = 45, wall_y2[2] = 10, wall_y2[3] = 30;        
        }

        if (c == 'i')
        {
            snprintf(tx_buffer, sizeof(tx_buffer), "recieved %c\r\n", c);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Timestamp: %02d:%02d\r\n", minutes, seconds);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Level: %d\r\n", level);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Lives: %d\r\n", lives);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Score: %d\r\n", score);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Number of fireworks: %d\r\n", fireworks_count);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Number of Mousetraps: %d\r\n", mousetrap_count);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Number of cheese: %d\r\n", cheese_count);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Number of cheese consumed: %d\r\n", cheese_consumed);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Super Mode: %d\r\n", super);
            usb_serial_send(tx_buffer);
            snprintf(tx_buffer, sizeof(tx_buffer), "Pause Mode: %d\r\n", game - 1);
            usb_serial_send(tx_buffer);
        }
    }
}

void process()
{
    adc_startup();

    if (super == 1)
    {
        for (int i = 0; i < 1024; i++)
        {
            set_duty_cycle(i);
        }
    }
    
    //Load first level
    if(BIT_IS_SET(PINF, 5) && game == 0)
    {
        while(BIT_IS_SET(PINF, 5))
        {
            //Block until release
        }
        game = 1;
    }
    
    if(BIT_IS_SET(PINF, 6))
    {
        while(BIT_IS_SET(PINF, 6))
        {
            //Do nothing
        }

        if (level == 2)
        {
            Game_over();
        }
        else
        {
            level++;
            cheese_count = 0;
            mousetrap_count = 0;
            mousetrap_time = 0;
            cheese_time = 0;
            cheese_consumed = 0;
            milk_count = 0;
            milk_x = 100;
            milk_y = 100;
            for (int i = 0; i < 5; i++)
            {
                cheeseX[i] = 100;
                cheeseY[i] = 100;
                mousetrapX[i] = 100;
                mousetrapY[i] = 100;
            }
        }

        Serial_walls();

        //wall_x1[0] = 18, wall_x1[1] = 25, wall_x1[2] = 45, wall_x1[3] = 58;
        //wall_y1[0] = 15, wall_y1[1] = 35, wall_y1[2] = 10, wall_y1[3] = 25;
        //wall_x2[0] = 13, wall_x2[1] = 25, wall_x2[2] = 60, wall_x2[3] = 72;
        //wall_y2[0] = 25, wall_y2[1] = 45, wall_y2[2] = 10, wall_y2[3] = 30;        
    }

    if (BIT_IS_SET(PINF, 5) && game == 1)
    {
        while(BIT_IS_SET(PINF, 5))
        {
            //Do nothing
        }
        game = 2;
    }

    if (BIT_IS_SET(PINF, 5) && game == 2)
    {
        while(BIT_IS_SET(PINF, 5))
        {
            //Do nothing
        }
        game = 1;
    }

    if (game == 1) //If playable game
    {
        clear_screen();
        game_clock();
        draw_level();
        Serial_movement();

        player_collide();
        firework_collide();
        Jerry_collision();
        mousetrap_collide();
        cheese_collide();
        potion_collide();
        door_collide();

        move_Jerry();
        move_Tom();
        move_walls();
        draw_player();

        MouseTrap();
        mousetrap_time++;
        cheese();
        cheese_time++;
        Potion();
        milk_time++;
        milliseconds++;

        if (cheese_consumed > 4)
        {
            draw_door();
        }

        if (lives == 0)
        {
            Game_over();
        }

        if (fireworks_count > 0)
        {
            move_firework();
        }
    }
    else if (game == 2) //If paused game
    {
        clear_screen();
        draw_level();
        move_Jerry();
        Serial_movement();
        draw_player();

        MouseTrap();
        cheese();
        Potion();

        player_collide();
        mousetrap_collide();
        cheese_collide();
    }
    show_screen();    
}

int main(void)
{
    set_clock_speed(CPU_8MHz);
    lcd_init(LCD_DEFAULT_CONTRAST);
    TCCR0A = 0;
    TCCR0B = 5;
    srand(TCNT0 * PRESCALE / FREQ);    
    adc_init();

    usb_init();

    PWM();

    //Start Game
    start_screen();

    for ( ;; ) 
    {
		process();
		_delay_ms(100);
	}
    //return 0;
}
