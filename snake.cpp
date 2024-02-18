#include <thread>
#include <curses.h>
#include <vector>
#include <atomic>
#include <csignal>
#include <cmath>
#include <bitset>
#include <cstddef>
#include <locale>
#include <algorithm>

static std::atomic<bool> run{true};
static std::atomic<int> winX{getmaxx(stdscr)};
static std::atomic<int> winY{getmaxy(stdscr)};

namespace view{
    namespace snake{
        const char* vHead = " ▉ ";
        const char* lHead = "■■  ";
        const char* rHead = " ■■";

        const char* tTail = " ╹ ";
        const char* bTail = " ╻ ";
        const char* lTail = "═━ ";
        const char* rTail = " ━═";

        const char* hBody = "═══";
        const char* vBody = " ║ ";

        const char* lbBody = "═╗ ";
        const char* rbBody= " ╔═";
        const char* ltBody = "═╝ ";
        const char* rtBody = " ╚═";

    }
    
    const char* apple = " Ö ";

    namespace menu{
        const char* title = "Select Level!";
    };
};   
namespace size{
    int wBlock = 3;
    int hBlock = 1;
    short height = 8;
    short width = 8;
}
namespace color{
    short apple = 1;
    short snake = 2;
    short block = 3;
    short accent = 4;
}

std::pair<int,int> operator-(const std::pair<int,int>& lhs, const std::pair<int,int>& rhs){
    return {lhs.first - rhs.first, lhs.second - rhs.second};
}

std::pair<int,int> operator+(const std::pair<int,int>& lhs, const std::pair<int,int>& rhs){
    return {lhs.first + rhs.first, lhs.second + rhs.second};
}


void handler(int signum){
    run.store(false, std::memory_order_relaxed);
}

void resizeEvent(int){
    winX.store(getmaxx(stdscr));
    winY.store(getmaxy(stdscr));
}

int TransformY(int x){
    return x * size::wBlock + winX.load(std::memory_order_relaxed)/2 - size::width/2;
}
int TransformX(int y){
    return y * size::hBlock + winY.load(std::memory_order_relaxed)/2 - size::height/2;
}

void Menu(std::byte& pos, bool hide){

    move(winY.load(std::memory_order_relaxed)/2 - 2,
        winX.load(std::memory_order_relaxed)/2 - strlen(view::menu::title)/2);
    printw(view::menu::title);
    
    const char* easy = "Medium";
    const char* medium = "Medium";
    const char* hard = "Medium";
    const char* curs = " <";
    
    move(winY.load(std::memory_order_relaxed)/2,
        winX.load(std::memory_order_relaxed)/2 - strlen(view::menu::title)/2);
    printw(easy);

    if(!hide && std::to_integer<int>(pos & std::byte{0b00000001})){
        printw(curs);
    }
    move(winY.load(std::memory_order_relaxed)/2 + 1,
        winX.load(std::memory_order_relaxed)/2 - strlen(view::menu::title)/2);
    printw(medium);
    if(!hide && std::to_integer<int>(pos & std::byte{0b00000010})){
        printw(curs);
    }

    move(winY.load(std::memory_order_relaxed)/2 + 2,
        winX.load(std::memory_order_relaxed)/2 - strlen(view::menu::title)/2);
    printw(hard);
    
    if(!hide && std::to_integer<int>(pos & std::byte{0b00000100})){
        printw(curs);
    }
}

void Initialize(std::vector<std::pair<int,int>>& snake){
    snake.push_back({0,6});
    snake.push_back({0,5});
    snake.push_back({0,4});
}

bool Move(
        std::vector<std::pair<int,int>>& snake, 
        std::pair<int,int>& apple, 
        const std::pair<int,int>& velocity){
            
    std::pair<int,int>& head = snake.front();
    
    for(std::vector<std::pair<int,int>>::iterator it = std::prev(snake.end()); it != snake.begin() ; std::advance(it, -1)){
        *it = *std::prev(it);
    };
    head.first += velocity.first;
    head.second += velocity.second;
    
    if(head.first < 0){
        head.first = size::height - 1;
    }else if(head.first > size::height - 1){
        head.first = 0;
    }

    if (head.second < 0){
        head.second = size::width - 1;
    }else if(head.second > size::width - 1){
        head.second = 0;
    }
    
    if (std::find_if(
        std::next(snake.begin()),
        snake.end(),
        [&head](std::pair<int,int>& part){
            return part.first == head.first  && part.second  == head.second ;
        }
    ) != snake.end()){
        return false;
    }

    if(head.first == apple.first && head.second == apple.second){
        apple = {std::numeric_limits<int>::max(), 0};
    }

    return true;
}

void Draw(
        const std::vector<std::pair<int,int>>& snake, 
        const std::pair<int,int>& apple,
        const std::pair<int,int>& velocity
        ){
    
    //Background
    for(int y = 0 ; y < size::width; ++y){
        for(int x = 0; x < size::height; ++x){

            mvprintw(
                TransformX(x),
                TransformY(y),
                " ° "
            );
            
        }
    }
    
    //snake
        //Head
    if (can_change_color())
        attron(COLOR_PAIR(color::snake));

    if(velocity.first != 0){
        mvprintw(
            TransformX(snake.front().first),
            TransformY(snake.front().second),
            view::snake::vHead
        );
    }else{
        if(velocity.second > 0){
            mvprintw(
                TransformX(snake.front().first),
                TransformY(snake.front().second),
                view::snake::lHead
            );
        }else{
            mvprintw(
                TransformX(snake.front().first),
                TransformY(snake.front().second),
                view::snake::rHead
            );
        }
    }

        //Body
    for(size_t i = snake.size() - 1; i > 0; --i){
        const std::pair<int,int>& cur = snake.at(i);
        const std::pair<int,int>& next = snake.at(i - 1);

        if (i == snake.size() - 1){
            if (next.first == cur.first){
                
                if (next.second < cur.second){
                    mvprintw(
                        TransformX(cur.first),
                        TransformY(cur.second),
                        view::snake::lTail
                    );
                }else{
                    mvprintw(
                        TransformX(cur.first),
                        TransformY(cur.second),
                        view::snake::rTail
                    );
                }
                
            }else{
                if (cur.first > next.first){
                    mvprintw(
                        TransformX(cur.first),
                        TransformY(cur.second),
                        view::snake::tTail
                    );
                }else{
                    mvprintw(
                        TransformX(cur.first),
                        TransformY(cur.second),
                        view::snake::bTail
                    );
                }
                
            }
            
        }else{
            const std::pair<int,int>& prev = snake.at(i + 1);
            std::pair<int,int> dif = snake.at(i + 1) - snake.at(i - 1);
  
            if(dif.first == 0){
                mvprintw(
                        TransformX(cur.first),
                        TransformY(cur.second),
                        view::snake::hBody
                    );
            }else if(dif.second == 0){
                mvprintw(
                        TransformX(cur.first),
                        TransformY(cur.second),
                        view::snake::vBody
                    );
            }else{
                if (prev.second == cur.second ){
                    if (dif.first == 1){
                        if (dif.second == 1 || dif.second < -1){
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::lbBody
                            );
                        }else{
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::rbBody
                            );
                        }
                    }else{
                        if (dif.second == 1 || dif.second < -1){
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::ltBody
                            );
                        }else{
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::rtBody
                            );
                        }
                    }   
                }else{
                    if (dif.first == -1 || dif.first > 1){
                        if (dif.second == -1 || dif.second > 1){
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::lbBody
                            );
                        }else{
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::rbBody
                            );
                        }
                    }else{
                        if (dif.second == -1 || dif.second > 1){
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::ltBody
                            );
                        }else{
                            mvprintw(
                                TransformX(cur.first),
                                TransformY(cur.second),
                                view::snake::rtBody
                            );
                        }
                    }
                }
            }
         
        }
        
       
    }
    attroff(COLOR_PAIR(color::snake));

    //Apple
    if (can_change_color()){
        attron(COLOR_PAIR(color::apple));
    }
    
    mvprintw(
        TransformX(apple.first),
        TransformY(apple.second),
        view::apple
    );

    if(can_change_color()){
        attroff(COLOR_PAIR(color::apple));
    }
    
}

inline void Increase(std::vector<std::pair<int,int>>& snake){
    snake.push_back(snake.back() + (snake.at(snake.size() - 2) - snake.back()));
}

void Rand(std::vector<std::pair<int,int>>& snake, std::pair<int,int>& apple){
    while(run.load(std::memory_order_relaxed)){
        apple.first = std::rand() % (size::height);
        apple.second = std::rand() % (size::width);

        if (std::find_if(
            snake.begin(),
            snake.end(),
            [&apple](std::pair<int,int>& part){
                return part.first == apple.first && part.second == apple.second;
            }
        ) == snake.end()){
            break;
        }
    }
    
}



int main(void){
    
    //Expand ASCII
    setlocale(LC_ALL, "");
    std::uint32_t fr = 0;

    //Handle Ctrl+C
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    struct sigaction sigResizeHandler;
    sigResizeHandler.sa_handler = resizeEvent;
    sigemptyset(&sigResizeHandler.sa_mask);
    sigResizeHandler.sa_flags = 0;

    sigaction(SIGWINCH, &sigResizeHandler, NULL);

    //Curses
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    keypad(stdscr, TRUE);

    if(can_change_color()){
        start_color();
        
        init_pair(color::apple,COLOR_RED, 0);
        init_pair(color::snake, COLOR_WHITE, 0);
        init_pair(color::block, COLOR_BLACK, 0);
        init_pair(color::accent, COLOR_GREEN, 0);

    }

    //Global Variables
    std::atomic<int> key;
    std::atomic<bool> pause{false};
    std::atomic<int> score{0};
    std::byte level{2};
    std::atomic<int> speed = 1000/5;

    winX.store(getmaxx(stdscr));
    winY.store(getmaxy(stdscr));
    while(run.load(std::memory_order_relaxed)){
        //Game Variables
        std::vector<std::pair<int,int>> snake;
        std::pair<int,int> apple;
        std::pair<int,int> velocity = {0,1};

        //Initialize snake and Apple
        Initialize(snake);
        Rand(snake, apple);
        

        //Menu
        while(run.load(std::memory_order_relaxed)){
            key.store(getch(), std::memory_order_relaxed);

            if (key.load(std::memory_order_relaxed) > 0){
                if (
                        key.load(std::memory_order_relaxed) == KEY_UP || 
                        key.load(std::memory_order_relaxed) == 119 && 
                        !std::to_integer<int>(level & std::byte{0b00000001})){
                    level >>= 1;
                    fr = 0;
                }else if(
                        key.load(std::memory_order_relaxed) == KEY_DOWN || 
                        key.load(std::memory_order_relaxed) == 115 && 
                        !std::to_integer<int>(level & std::byte{0b00000100})){
                    level <<= 1;
                    fr = 0;
                }else if(key.load(std::memory_order_relaxed) == 10){
                    break;
                }
            }
            if (fr % 3 == 0){
                clear();
                Menu(level, fr % 6 != 0);
                
                if(can_change_color()){
                    attron(COLOR_PAIR(color::accent));
                }
                mvprintw(
                    winY.load(std::memory_order_relaxed)/2 + 4, 
                    winX.load(std::memory_order_relaxed)/2 - 6, 
                    "Scores: %i", 
                    score.load(std::memory_order_relaxed));
                attroff(COLOR_PAIR(color::accent));

                refresh();
            }
            
            fr++;
            std::this_thread::sleep_for(std::chrono::milliseconds(speed.load(std::memory_order_relaxed)));
            
        }

        //Reset 
        score.store(0, std::memory_order_relaxed);
        pause.store(false, std::memory_order_relaxed);

        //Playing
        while(run.load(std::memory_order_relaxed)){
            key.store(getch(), std::memory_order_relaxed);

            if (pause.load(std::memory_order_relaxed)){
                if (key.load(std::memory_order_relaxed) == 114){
                    pause.store(false, std::memory_order_relaxed);
                }
            }else{
                system("clear");
                clear();
                
                
                if(key.load(std::memory_order_relaxed) > 0){
                    if (key.load(std::memory_order_relaxed) == KEY_LEFT || key.load(std::memory_order_relaxed) == 97){
                        if (velocity.second == 0){
                            velocity.first = 0;
                            velocity.second = -1;
                        }
                    }else if (key.load(std::memory_order_relaxed) == KEY_RIGHT || key.load(std::memory_order_relaxed) == 100){
                        if (velocity.second == 0){
                            velocity.first = 0;
                            velocity.second = 1;
                        }
                    }else if (key.load(std::memory_order_relaxed) == KEY_UP || key.load(std::memory_order_relaxed) == 119){
                        if (velocity.first == 0){
                            velocity.first = -1;
                            velocity.second = 0;
                        }
                    }else if (key.load(std::memory_order_relaxed) == KEY_DOWN || key.load(std::memory_order_relaxed) == 115){
                        if (velocity.first == 0){
                            velocity.first = 1;
                            velocity.second = 0;
                        }
                    }else if (key.load(std::memory_order_relaxed) == 114){
                        mvprintw(
                            winY.load(std::memory_order_relaxed)/2,
                            winX.load(std::memory_order_relaxed)/2 - 15,
                            "Paused! Press 'R' to continue...");
                        pause.store(true, std::memory_order_relaxed);
                        refresh();
                        continue;
                    }
                }
            
                if (!Move(snake, apple, velocity) 
                    || snake.size() >= size::width * size::height){
                    break;
                }

                if(apple.first == std::numeric_limits<int>::max()){
                    Increase(snake);
                    Rand(snake, apple);
                    
                    score.fetch_add(1, std::memory_order_relaxed);
                }

                Draw(snake, apple,velocity);
                refresh();

                std::this_thread::sleep_for(std::chrono::milliseconds(speed.load(std::memory_order_relaxed)));
            }
            
        }

        if (snake.size() >= size::height * size::width){
            fr = 0;
            std::uint8_t i = 0;

            
            
            while(run.load(std::memory_order_relaxed)){
                if (getch() > 0){
                    break;
                }
                
                fr++;

                if(fr % 500'000 == 0){
                    clear();
                    i++;

                    if(i == 4){
                        i = 0;
                    }
                    
                    mvprintw(
                        winY.load(std::memory_order_relaxed)/2, 
                        winX.load(std::memory_order_relaxed)/2 - 5
                        ,"Applause!");
                    mvprintw(
                        winY.load(std::memory_order_relaxed)/2 + 1, 
                        winX.load(std::memory_order_relaxed)/2 - 5,
                        "Press Any To Continue");

                    for(std::uint8_t j = 0; j <= i; ++j){
                        mvprintw(
                            winY.load(std::memory_order_relaxed)/2 + 1, 
                            winX.load(std::memory_order_relaxed)/2 +  16 + j, 
                            ".");
                    }
                }
                
                
            }
            

            
        }
        
    }

    endwin();

    return 0;
}