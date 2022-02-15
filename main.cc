#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <chrono>

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


//credit goes to mcleary for this timer class. https://gist.github.com/mcleary/b0bf4fa88830ff7c882d
class Timer
{
public:
    void start()
    {
        m_StartTime = std::chrono::system_clock::now();
        m_bRunning = true;
    }
    
    void stop()
    {
        m_EndTime = std::chrono::system_clock::now();
        m_bRunning = false;
    }
    
    double elapsedMilliseconds()
    {
        std::chrono::time_point<std::chrono::system_clock> endTime;
        
        if(m_bRunning)
        {
            endTime = std::chrono::system_clock::now();
        }
        else
        {
            endTime = m_EndTime;
        }
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
    }
    
    double elapsedSeconds()
    {
        return elapsedMilliseconds() / 1000.0;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> m_StartTime;
    std::chrono::time_point<std::chrono::system_clock> m_EndTime;
    bool                                               m_bRunning = false;
};

void clear()
{
    std::cout << "\x1B[2J\x1B[H";
}

void displayGrid(T_box [], int, int, T_coord &, int);

void displayGridDebug(T_box [], int, int);

int countRevealed(T_box [], int, int);

void initGrid(T_box [], int, int, int, T_coord);

void setValueBoxes(T_box [], int, int);

int getNumMines(T_box [], int, int, int, int);

int getNumMarked(T_box [], int, int, int, int);

void mark(T_box [], int, int, int , int &);

bool clearAround(T_box [], int, int, T_coord);

bool select(T_box [], int, int, T_coord);



int main(int argc, char const *argv[])
{
    int width, height, bombs, bombs_left;
    clear();
    
    std::cout << "Arrow keys to move 'cursor'\nSpace or Enter to click box\n'a' to place flag\n'x' to quit\n\n";

    std::cout << "a. Easy\nb. Medium\nc. Difficult\nd. Custom\n";
    std::cout << "Choose a difficulty: ";

    char difficulty = 'a';
    do
    {
        std::cin >> difficulty;
        if (difficulty != 'a' && difficulty != 'd' &&difficulty != 'c' &&difficulty != 'd')
        {
            std::cout << "Incorrect option. Choose again: ";
        }
        
    } while (difficulty != 'a' && difficulty != 'b' &&difficulty != 'c' &&difficulty != 'd');
    
    switch (difficulty)
    {
    case 'a':
        width = 9; height = 9; bombs = 10;
        break;
    case 'b':
        width = 16; height = 16; bombs = 40;
        break;
    case 'c':
        width = 30; height = 16; bombs = 99;
        break;
    default:
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

        break;
    }
    
    std::cin.ignore();

    bombs_left = bombs;
    T_box grid[height*width];
    T_coord selector;
    bool lost = false;
    bool won = false;
    bool gameStarted = false;
    int selection = -1;

    Timer timer; timer.start();

    while (selection != 'x' && !lost && !won)
    {
        clear();
        displayGrid(grid, height, width, selector, bombs_left);
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
            }
            
            lost = select(grid, height, width, selector);
            
            if (countRevealed(grid, height, width) == height*width-bombs)
            {
                won = true;
            }
            
        } else if (selection == KEY_A)
        {
            mark(grid, height, width, selector.y*width + selector.x, bombs_left);
        }
        
        
    }

    clear();
    displayGrid(grid, height, width, selector, 0);

    int time = timer.elapsedSeconds();

    if (lost == true)
    {
        std::cout << "You lost\n";
    } else if (won == true)
    {
        std::cout << "Time: " << time << '\n';
        std::cout << "You won\n";
    }
    
    return 0;
}



void displayGrid(T_box grid[], int height, int width, T_coord &selector, int bombs_left){
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
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
    std::cout << "Mines: " << bombs_left << '\n';
    
}

void displayGridDebug(T_box grid[], int height, int width) {
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
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
    for (int i = 0; i < hw; i++)
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

        for (int l = 0+limit_up; l < 3-limit_down; l++)
        {
            for (int k = 0+limit_left; k < 3-limit_right; k++)
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
void mark(T_box grid[], int height, int width, int pos, int &bombs_left){
    T_box &target = grid[pos];

    if (!target.isRevealed)
    {
        target.isMarked = !target.isMarked;
        if (target.isMarked)
        {
            bombs_left--;
        } else {
            bombs_left++;
        }
        
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

            if (!otherBox.isMarked && otherBox.isMine)
            {
                return true;
            } else if (!otherBox.isMarked)
            {
                T_coord selector;
                selector.x = otherPosition%width;
                selector.y = otherPosition/width;

                lost = select(grid, height, width, selector);
            }
    
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
