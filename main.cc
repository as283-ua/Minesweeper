#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <SFML/System.hpp>
#include <time.h>
#include <vector>

//#include <conio.h>
//copied getch from conio so that it's not necessary to have the library
#include <termios.h>
#include <unistd.h>

#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_LEFT 68
#define KEY_RIGHT 67
#define KEY_SPACE 32
#define KEY_ENTER 10
#define KEY_A 97

int getch(){
    struct termios oldt, newt;
    int ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~ICANON;
    
    newt.c_lflag &= ~ECHO;
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}

const int MAXSIZE = 99;

struct T_box {
    bool isMine = false;
    bool isRevealed = false;
    bool isMarked = false;  //only useful when revealed = false
    bool isCleared  = false;    //true after doing cleararound()
    int value = -1;
};

struct T_coord {
    int x = 0;
    int y = 0;
};

void displayGrid(T_box grid[], int height, int width, T_coord &selector);

void displayGridDebug(T_box grid[], int height, int width);

int countRevealed(T_box grid[], int height, int width);

void initGrid(T_box grid[], int height, int width, int bombs, T_coord firstMove);

void setValueBoxes(T_box grid[], int height, int width);

int getNumMines(T_box grid[], int height, int width, int i, int j);

int getNumMarked(T_box grid[], int height, int width, int i, int j);

void mark(T_box grid[], int height, int width, int pos);

bool clearAround(T_box grid[], int height, int width, T_coord selector);

bool select(T_box grid[], int height, int width, T_coord selector);



int main(int argc, char const *argv[])
{
    int width, height, bombs;
    system("clear");
    
    std::cout << "Arrow keys to move 'cursor'\nSpace or Enter to click box\n'a' to place flag\n'x' to quit\n\n";

    do
    {
        std::cout << "Enter width: ";
        std::cin >> width;
    } while (width > 99 || width < 1);
    
    do
    {
        std::cout << "Enter height: ";
        std::cin >> height;
    } while (height > 99 || height < 1);

    do
    {
        std::cout << "Enter number of mines: ";
        std::cin >> bombs;
    } while (bombs > height*width || bombs < 1);

    std::cin.ignore();

    T_box grid[height*width];
    T_coord selector;
    bool lost = false;
    bool won = false;
    bool gameStarted = false;
    int selection = -1;

    while (selection != 'x' && !lost && !won)
    {
        system("clear");
        displayGrid(grid, height, width, selector);
        //displayGridDebug(grid, height, width);

        selection = getch();

        if (selection == 27)
        {
            getch();    //1 unnecessary char when press arrow
            switch (selection = getch())
            {
            case KEY_UP:
                if (selector.y != 0)
                {
                    selector.y--;
                }
                
                break;

            case KEY_DOWN:
                if (selector.y != height - 1)
                {
                    selector.y++;
                }
                
                break;

            case KEY_LEFT:
                if (selector.x != 0)
                {
                    selector.x--;
                }
                
                break;

            case KEY_RIGHT:
                if (selector.x != width - 1)
                {
                    selector.x++;
                }
                
                break;
            }
        } else if (selection == KEY_SPACE || selection == KEY_ENTER){
            if (!gameStarted)
            {
                gameStarted = true;
                initGrid(grid, height, width, bombs, selector);
                //usleep(200);
            }
            
            lost = select(grid, height, width, selector);
            
            if (countRevealed(grid, height, width) == height*width-bombs)
            {
                won = true;
            }
            
        } else if (selection == KEY_A)
        {
            mark(grid, height, width, selector.y*width + selector.x);
        }
        
        
    }

    system("clear");
    displayGrid(grid, height, width, selector);

    if (lost == true)
    {
        std::cout << "You lost\n";
    } else if (won == true)
    {
        std::cout << "You won\n";
    }
    
    
    
    return 0;
}



void displayGrid(T_box grid[], int height, int width, T_coord &selector){
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            T_box box = grid[i*width + j];

            if (i == selector.y && j == selector.x)
            {
                std::cout << "& ";
            } else {
                if (box.isRevealed)
                {
                    if (box.isMine)
                    {
                        std::cout << "x ";
                    } else {
                        if (box.value == 0)
                        {
                            std::cout << "  ";
                        } else {
                            std::cout << box.value << ' ';
                        }
                        
                    }
                } else {
                    if (box.isMarked)
                    {
                        std::cout << "\033[1;31m■ \033[0m";
                    } else {
                        std::cout << "■ ";
                    }
                    
                }
            }
            
        }
        std::cout << std::endl;
    }
    
}

void displayGridDebug(T_box grid[], int height, int width) {
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            T_box box = grid[i*width + j];

            if (box.isMine)
            {
                std::cout << "x ";
            } else {
                if (box.value == 0)
                {
                    std::cout << "  ";
                } else {
                    std::cout << box.value << ' ';
                }
                
            }

        }
        std::cout << std::endl;
    }
}

void initGrid(T_box grid[], int height, int width, int bombs, T_coord firstMove){
    srand(time(NULL));
    //srand(0); //debug
    std::vector<int> available_boxes;

    //fill vector
    int hw = height*width;
    for (size_t i = 0; i < hw; i++)
    {
        available_boxes.push_back(i);
    }    

    //don't put mines in first selected boxes
    if (bombs <= height*width-9)
    {
        int limit_up = 0, limit_down = 0, limit_left = 0, limit_right = 0;

        int targetBox = firstMove.y*width + firstMove.x;    //position of box in grid
        int otherPosition;

        if (firstMove.y == 0)
        {
            limit_up = 1;
        }
        
        if (firstMove.y == height-1)
        {
            limit_down = 1;
        }
        
        if (firstMove.x == width-1)
        {
            limit_right = 1;
        }
        
        if (firstMove.x == 0)
        {
            limit_left = 1;
        }

        for (size_t l = 0+limit_up; l < 3-limit_down; l++)
        {
            for (size_t k = 0+limit_left; k < 3-limit_right; k++)
            {
                otherPosition = targetBox + (l-1)*width + (k-1);    //position where mine can't be placed

                int i;

                for (i = 0; i < available_boxes.size(); i++)
                {
                    //std::cout << "available_boxes[" << i << "]: " << available_boxes[i] << std::endl;
                    if (otherPosition == available_boxes[i])
                    {
                        available_boxes.erase(available_boxes.begin() + i);
                        break;
                    }
                    
                }
                
                
                
            }
            
        }
    }
    
    
    int box;

    while (bombs > 0)
    {
        box = rand()%available_boxes.size();

        grid[available_boxes[box]].isMine = true;

        available_boxes.erase(available_boxes.begin() + box);
        bombs--;
    }

    setValueBoxes(grid, height, width);
}

int countRevealed(T_box grid[], int height, int width){     //win condition : get numrevealed == height*width-bombs
    int numRevealed = 0;
    for (size_t i = 0; i < height * width; i++)
    {
        if (grid[i].isRevealed)
        {
            numRevealed++;
        }
    }
    
    return numRevealed;
}

int getNumMines(T_box grid[], int height, int width, int i, int j){ //i = row number, j = column number -> i*width + j
    int marked = 0;
    int limit_up = 0, limit_down = 0, limit_left = 0, limit_right = 0;

    int targetBox = i*width + j;    //position of box in grid
    int otherPosition;

    if (i == 0)
    {
        limit_up = 1;
    }
    if (i == height-1)
    {
        limit_down = 1;
    }
    if (j == width-1)
    {
        limit_right = 1;
    }
    if (j == 0)
    {
        limit_left = 1;
    }
    
    for (size_t l = 0+limit_up; l < 3-limit_down; l++)
    {
        for (size_t k = 0+limit_left; k < 3-limit_right; k++)
        {
            otherPosition = targetBox + (l-1)*width + (k-1);

            if (otherPosition == targetBox)
            {
                continue;
            }
            
            if (grid[otherPosition].isMine)
            {
                marked++;
            }
            
        }
        
    }

    return marked;
}

//i for y axis, j for x axis
int getNumMarked(T_box grid[], int height, int width, int i, int j){
    int marked = 0;
    int limit_up = 0, limit_down = 0, limit_left = 0, limit_right = 0;

    int targetBox = i*width + j;    //position of box in grid
    int otherPosition;

    if (i == 0)
    {
        limit_up = 1;
    }
    if (i == height-1)
    {
        limit_down = 1;
    }
    if (j == width-1)
    {
        limit_right = 1;
    }
    if (j == 0)
    {
        limit_left = 1;
    }
    
    for (size_t l = 0+limit_up; l < 3-limit_down; l++)
    {
        for (size_t k = 0+limit_left; k < 3-limit_right; k++)
        {
            otherPosition = targetBox + (l-1)*width + (k-1);

            if (otherPosition == targetBox)
            {
                continue;
            }
            
            if (grid[otherPosition].isMarked)
            {
                marked++;
            }
            
        }
        
    }

    return marked;
}

//toggle for isMarked
void mark(T_box grid[], int height, int width, int pos){
    T_box &target = grid[pos];

    if (!target.isRevealed)
    {
        target.isMarked = !target.isMarked;
    }
}

void setValueBoxes(T_box grid[], int height, int width){
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            T_box &box = grid[i*width + j];
            
            if (!box.isMine)
            {
                box.value = getNumMines(grid, height, width, i, j);
            }
            
        }
        
    }
}

bool clearAround(T_box grid[], int height, int width, T_coord selector){
    bool lost = false;

    int limit_up = 0, limit_down = 0, limit_left = 0, limit_right = 0;

    int targetIndex = selector.y*width + selector.x;    //position of box in grid
    int otherPosition;

    if (selector.y == 0)
    {
        limit_up = 1;
    }
    if (selector.y == height-1)
    {
        limit_down = 1;
    }
    if (selector.x == width-1)
    {
        limit_right = 1;
    }
    if (selector.x == 0)
    {
        limit_left = 1;
    }
    for (size_t l = 0+limit_up; l < 3-limit_down; l++)
    {
        for (size_t k = 0+limit_left; k < 3-limit_right; k++)
        {
            otherPosition = targetIndex + (l-1)*width + (k-1);

            if (otherPosition == targetIndex)
            {
                continue;
            }

            T_box &otherBox = grid[otherPosition];

            T_coord selector;
            selector.x = otherPosition%width;
            selector.y = otherPosition/width;

            select(grid, height, width, selector);

        }
        
    }

    return lost;
}

bool select(T_box grid[], int height, int width, T_coord selector){
    bool lost = false;
    int targetIndex = selector.y*width + selector.x;    //position of box in grid
    T_box &targetBox = grid[targetIndex];
    if (!targetBox.isMarked)
    {
        if (!targetBox.isCleared && getNumMarked(grid, height, width, selector.y, selector.x) == targetBox.value)   //when box has already been selected and all mines are marked
        {
            targetBox.isRevealed = true;
            targetBox.isCleared = true;     //avoid loop where it keeps clearing around a box
            lost = clearAround(grid, height, width, selector);

        } else {
            targetBox.isRevealed = true;
            if (targetBox.isMine)
            {
                lost = true;    //lost
            }
            
        }
        
    }
    
    return lost;
}