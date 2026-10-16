/**
 * ============================================================================
 * TỰA GAME: RẮN SĂN MỒI TRONG NGÔI NHÀ (HOUSE SNAKE ARCADE EDITION)
 * Ngôn ngữ: C++ (Tương thích 100% với Code::Blocks / MinGW trên Windows)
 * Điều khiển: W, A, S, D (Di chuyển) | P (Tạm dừng) | R (Chơi lại) | ESC (Thoát)
 * Tính năng nổi bật:
 *  - Kết xuất đồ họa Console Double-Buffering mượt mà 60 FPS (Không nháy hình).
 *  - Bản đồ Ngôi Nhà gồm 5 khu vực: Phòng Khách, Phòng Bếp, Hành Lang,
 *    Phòng Ngủ, Phòng Giải Trí cùng các chướng ngại vật nội thất và cửa thông phòng.
 *  - Hệ thống nhiều trái cây xuất hiện cùng lúc trong các phòng.
 *  - Các quả chức năng (Power-ups):
 *      [G] Quả Bóng Ma (Cyan)    : Đi xuyên tường & nội thất trong 10 giây.
 *      [X] Quả Nhân Đôi (Tím)    : Nhân đôi (x2) điểm số nhận được trong 12 giây.
 *      [S] Quả Băng Giá (Xanh)   : Làm chậm tốc độ rắn để dễ qua cửa hẹp (8 giây).
 *      [B] Quả Thu Nhỏ (Lục)     : Cắt ngắn bớt 3 đốt thân rắn + thưởng 30 điểm.
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <windows.h>
#include <conio.h>

using namespace std;

// Kích thước toàn bộ màn hình Console
const int SCREEN_WIDTH = 96;
const int SCREEN_HEIGHT = 32;

// Kích thước khu vực ngôi nhà (bản đồ chơi)
const int MAP_WIDTH = 62;
const int MAP_HEIGHT = 30;

// Tọa độ góc trên bên trái của bản đồ trên màn hình
const int MAP_OFFSET_X = 2;
const int MAP_OFFSET_Y = 1;

// Bảng mã màu Console Windows
enum ConsoleColor {
    COLOR_BLACK = 0,
    COLOR_DARK_BLUE = 1,
    COLOR_DARK_GREEN = 2,
    COLOR_DARK_CYAN = 3,
    COLOR_DARK_RED = 4,
    COLOR_DARK_MAGENTA = 5,
    COLOR_DARK_YELLOW = 6,
    COLOR_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_BLUE = 9,
    COLOR_GREEN = 10,
    COLOR_CYAN = 11,
    COLOR_RED = 12,
    COLOR_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15
};

// Hướng di chuyển của rắn (Điều khiển bằng W, A, S, D)
enum Direction {
    DIR_UP = 0,
    DIR_DOWN = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3
};

// Các loại ô địa hình trong ngôi nhà
enum TileType {
    TILE_EMPTY = 0,       // Sàn nhà trống có thể đi lại
    TILE_OUTER_WALL = 1,  // Tường bao quanh ngôi nhà
    TILE_INNER_WALL = 2,  // Tường ngăn giữa các phòng
    TILE_FURNITURE = 3,   // Nội thất (Bàn ăn, ghế sofa, giường...)
    TILE_DOOR = 4         // Cửa thông giữa các phòng
};

// Các loại trái cây & quả chức năng
enum FruitType {
    FRUIT_APPLE = 0,      // [@] Táo Đỏ: +10 điểm, +1 đốt
    FRUIT_BANANA = 1,     // [$] Chuối Vàng: +25 điểm, +1 đốt
    FRUIT_GHOST = 2,      // [G] Quả Bóng Ma: Đi xuyên tường trong 10 giây
    FRUIT_DOUBLE = 3,     // [X] Quả Nhân Đôi: x2 điểm số trong 12 giây
    FRUIT_SLOW = 4,       // [S] Quả Băng Giá: Làm chậm tốc độ trong 8 giây
    FRUIT_SHRINK = 5      // [B] Quả Thu Nhỏ: Giảm 3 đốt thân rắn + 30 điểm
};

// Cấu trúc tọa độ 2D
struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// Cấu trúc vật phẩm / trái cây trên bản đồ
struct FruitItem {
    Point pos;            // Vị trí trong ngôi nhà
    FruitType type;       // Loại quả
    DWORD spawnTime;      // Thời điểm xuất hiện (để tính thời gian biến mất của quả chức năng)
    bool isSpecial;       // Là quả chức năng hay quả thường
    int durationMs;       // Thời gian tồn tại trên bản đồ (nếu là quả chức năng)
};

// Lớp quản lý hiển thị Console bằng kỹ thuật Double Buffering (Chống nháy 100%)
class ConsoleRenderer {
private:
    HANDLE hConsole;
    CHAR_INFO* buffer;
    COORD bufferSize;
    COORD bufferCoord;
    SMALL_RECT writeRegion;

public:
    ConsoleRenderer() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        bufferSize = { (SHORT)SCREEN_WIDTH, (SHORT)SCREEN_HEIGHT };
        bufferCoord = { 0, 0 };
        writeRegion = { 0, 0, (SHORT)(SCREEN_WIDTH - 1), (SHORT)(SCREEN_HEIGHT - 1) };
        buffer = new CHAR_INFO[SCREEN_WIDTH * SCREEN_HEIGHT];

        // Thiết lập kích thước cửa sổ Console và ẩn con trỏ nhấp nháy
        SetConsoleTitleA("Game Ran San Moi Trong Ngoi Nha - WASD Edition");
        SetConsoleScreenBufferSize(hConsole, bufferSize);
        SetConsoleWindowInfo(hConsole, TRUE, &writeRegion);

        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);

        clearBuffer();
    }

    ~ConsoleRenderer() {
        delete[] buffer;
    }

    // Xóa bộ đệm màn hình về màu đen
    void clearBuffer() {
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
            buffer[i].Char.AsciiChar = ' ';
            buffer[i].Attributes = COLOR_WHITE;
        }
    }

    // Vẽ 1 ký tự tại tọa độ (x, y) với màu chữ và màu nền
    void drawChar(int x, int y, char c, WORD fgColor, WORD bgColor = COLOR_BLACK) {
        if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
        int index = y * SCREEN_WIDTH + x;
        buffer[index].Char.AsciiChar = c;
        buffer[index].Attributes = fgColor | (bgColor << 4);
    }

    // Vẽ 1 chuỗi văn bản tại tọa độ (x, y)
    void drawText(int x, int y, const string& text, WORD fgColor, WORD bgColor = COLOR_BLACK) {
        for (size_t i = 0; i < text.length(); ++i) {
            drawChar(x + (int)i, y, text[i], fgColor, bgColor);
        }
    }

    // Vẽ khung hình chữ nhật trên giao diện
    void drawBox(int x, int y, int w, int h, WORD color, const string& title = "") {
        for (int i = x; i < x + w; ++i) {
            drawChar(i, y, '=', color);
            drawChar(i, y + h - 1, '=', color);
        }
        for (int j = y; j < y + h; ++j) {
            drawChar(x, j, '|', color);
            drawChar(x + w - 1, j, '|', color);
        }
        drawChar(x, y, '+', color);
        drawChar(x + w - 1, y, '+', color);
        drawChar(x, y + h - 1, '+', color);
        drawChar(x + w - 1, y + h - 1, '+', color);

        if (!title.empty() && (int)title.length() + 4 < w) {
            drawText(x + 2, y, " " + title + " ", COLOR_YELLOW);
        }
    }

    // Đẩy toàn bộ khung hình ra màn hình Console chỉ trong 1 lệnh hệ thống
    void present() {
        WriteConsoleOutputA(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
    }
};

class HouseSnakeGame {
private:
    ConsoleRenderer renderer;
    TileType houseMap[MAP_HEIGHT][MAP_WIDTH];
    deque<Point> snake;
    Direction currentDir;
    Direction nextDir;

    vector<FruitItem> fruits;
    int score;
    int highScore;
    int applesEaten;
    bool isRunning;
    bool isGameOver;
    bool isPaused;
    string gameOverReason;
    string statusMessage;
    DWORD statusMsgEndTime;

    // Thời điểm kết thúc các hiệu ứng chức năng (Power-ups)
    DWORD ghostEndTime;      // Hiệu ứng đi xuyên tường
    DWORD doubleEndTime;     // Hiệu ứng x2 điểm
    DWORD slowEndTime;       // Hiệu ứng làm chậm

    DWORD lastMoveTime;
    DWORD lastSpecialSpawnTime;

public:
    HouseSnakeGame() {
        srand((unsigned int)time(NULL));
        highScore = 0;
        isRunning = true;
        initGame();
    }

    // Khởi tạo hoặc đặt lại trạng thái ván chơi mới
    void initGame() {
        buildHouseArchitecture();

        snake.clear();
        // Đặt rắn bắt đầu ở khu vực Hành Lang Trung Tâm (an toàn và rộng rãi)
        int startX = MAP_WIDTH / 2;
        int startY = 14;
        snake.push_back({ startX, startY });
        snake.push_back({ startX - 1, startY });
        snake.push_back({ startX - 2, startY });
        snake.push_back({ startX - 3, startY });

        currentDir = DIR_RIGHT;
        nextDir = DIR_RIGHT;

        score = 0;
        applesEaten = 0;
        isGameOver = false;
        isPaused = false;
        gameOverReason = "";

        ghostEndTime = 0;
        doubleEndTime = 0;
        slowEndTime = 0;

        fruits.clear();
        // Khởi tạo 3 trái cây thường rải rác trong các phòng
        for (int i = 0; i < 3; ++i) {
            spawnFruit(false);
        }
        // Khởi tạo 1 quả chức năng ngay từ đầu để người chơi trải nghiệm
        spawnFruit(true);

        lastMoveTime = GetTickCount();
        lastSpecialSpawnTime = GetTickCount();
        setStatus("Chao mung! Dung W, A, S, D de di chuyen qua cac phong!", 4000);
    }

    // Hiển thị thông báo ngắn ở thanh trạng thái
    void setStatus(const string& msg, int durationMs = 3000) {
        statusMessage = msg;
        statusMsgEndTime = GetTickCount() + durationMs;
    }

    // Thiết kế kiến trúc ngôi nhà với các phòng, cửa ra vào và đồ nội thất
    void buildHouseArchitecture() {
        // 1. Làm sạch toàn bộ sàn nhà
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                if (y == 0 || y == MAP_HEIGHT - 1 || x == 0 || x == MAP_WIDTH - 1) {
                    houseMap[y][x] = TILE_OUTER_WALL;
                } else {
                    houseMap[y][x] = TILE_EMPTY;
                }
            }
        }

        // 2. Xây tường ngăn ngang tạo Hành Lang Trung Tâm (từ y = 11 đến y = 17)
        for (int x = 1; x < MAP_WIDTH - 1; ++x) {
            houseMap[11][x] = TILE_INNER_WALL;
            houseMap[17][x] = TILE_INNER_WALL;
        }

        // 3. Xây tường ngăn dọc giữa Phòng Khách (Trái) & Phòng Bếp (Phải) ở tầng trên
        for (int y = 1; y < 11; ++y) {
            houseMap[y][30] = TILE_INNER_WALL;
        }

        // 4. Xây tường ngăn dọc giữa Phòng Ngủ (Trái) & Phòng Giải Trí (Phải) ở tầng dưới
        for (int y = 17; y < MAP_HEIGHT - 1; ++y) {
            houseMap[y][30] = TILE_INNER_WALL;
        }

        // 5. Mở các Cửa Thông Phòng (TILE_DOOR) để rắn bò qua lại giữa các phòng
        // Cửa giữa Phòng Khách và Phòng Bếp
        for (int y = 4; y <= 7; ++y) houseMap[y][30] = TILE_DOOR;

        // Cửa từ Phòng Khách xuống Hành Lang Trung Tâm
        for (int x = 12; x <= 18; ++x) houseMap[11][x] = TILE_DOOR;

        // Cửa từ Phòng Bếp xuống Hành Lang Trung Tâm
        for (int x = 43; x <= 49; ++x) houseMap[11][x] = TILE_DOOR;

        // Cửa từ Hành Lang xuống Phòng Ngủ
        for (int x = 12; x <= 18; ++x) houseMap[17][x] = TILE_DOOR;

        // Cửa từ Hành Lang xuống Phòng Giải Trí
        for (int x = 43; x <= 49; ++x) houseMap[17][x] = TILE_DOOR;

        // Cửa giữa Phòng Ngủ và Phòng Giải Trí
        for (int y = 22; y <= 25; ++y) houseMap[y][30] = TILE_DOOR;

        // 6. Đặt Nội Thất (TILE_FURNITURE) trong từng phòng tạo thử thách thú vị
        // Nội thất Phòng Khách: Bộ ghế Sofa chữ L
        for (int x = 6; x <= 14; ++x) houseMap[4][x] = TILE_FURNITURE;
        for (int y = 4; y <= 7; ++y)  houseMap[y][6] = TILE_FURNITURE;

        // Nội thất Phòng Bếp: Đảo bếp & Bàn ăn ở giữa phòng
        for (int x = 42; x <= 50; ++x) {
            houseMap[5][x] = TILE_FURNITURE;
            houseMap[6][x] = TILE_FURNITURE;
        }

        // Nội thất Phòng Ngủ: Giường đôi và tủ đầu giường
        for (int y = 21; y <= 25; ++y) {
            for (int x = 6; x <= 11; ++x) {
                houseMap[y][x] = TILE_FURNITURE;
            }
        }

        // Nội thất Phòng Giải Trí: Hai стол Bida / Kệ tủ đối xứng
        for (int x = 38; x <= 43; ++x) houseMap[21][x] = TILE_FURNITURE;
        for (int x = 48; x <= 54; ++x) houseMap[25][x] = TILE_FURNITURE;
    }

    // Xác định tên căn phòng mà đầu rắn đang ở
    string getCurrentRoomName() const {
        if (snake.empty()) return "Khong xac dinh";
        Point head = snake.front();

        if (head.y < 11) {
            if (head.x < 30) return "PHONG KHACH (Living Room)";
            if (head.x > 30) return "PHONG BEP (Kitchen)";
            return "CUA THONG PHONG TREN";
        } else if (head.y > 17) {
            if (head.x < 30) return "PHONG NGU (Bedroom)";
            if (head.x > 30) return "PHONG GIAI TRI (Game Room)";
            return "CUA THONG PHONG DUOI";
        } else {
            return "HANH LANG TRUNG TAM";
        }
    }

    // Kiểm tra hiệu ứng chức năng có đang kích hoạt hay không
    bool isGhostActive() const {
        return GetTickCount() < ghostEndTime;
    }

    bool isDoubleScoreActive() const {
        return GetTickCount() < doubleEndTime;
    }

    bool isSlowActive() const {
        return GetTickCount() < slowEndTime;
    }

    // Kiểm tra một ô trong nhà có trống để đặt thức ăn hay không
    bool isPositionFreeForFruit(Point p) const {
        if (p.x <= 1 || p.x >= MAP_WIDTH - 2 || p.y <= 1 || p.y >= MAP_HEIGHT - 2)
            return false;

        if (houseMap[p.y][p.x] != TILE_EMPTY)
            return false;

        // Không đặt trùng lên thân rắn
        for (size_t i = 0; i < snake.size(); ++i) {
            if (snake[i] == p) return false;
        }

        // Không đặt trùng lên quả khác đang có
        for (size_t i = 0; i < fruits.size(); ++i) {
            if (fruits[i].pos == p) return false;
        }

        return true;
    }

    // Sinh ngẫu nhiên trái cây thường hoặc quả chức năng vào các phòng
    void spawnFruit(bool specialFruit) {
        Point p = { 0, 0 };
        int attempts = 0;
        while (attempts < 500) {
            p.x = 2 + rand() % (MAP_WIDTH - 4);
            p.y = 2 + rand() % (MAP_HEIGHT - 4);
            if (isPositionFreeForFruit(p)) break;
            attempts++;
        }

        if (attempts >= 500) return;

        FruitItem item;
        item.pos = p;
        item.spawnTime = GetTickCount();
        item.isSpecial = specialFruit;

        if (!specialFruit) {
            // 75% ra Táo Đỏ (+10đ), 25% ra Chuối Vàng (+25đ)
            item.type = (rand() % 100 < 75) ? FRUIT_APPLE : FRUIT_BANANA;
            item.durationMs = 0; // Tồn tại vĩnh viễn cho tới khi được ăn
        } else {
            // Chọn ngẫu nhiên 1 trong 4 quả chức năng
            int r = rand() % 100;
            if (r < 35) {
                item.type = FRUIT_GHOST;   // 35% Quả Bóng Ma (Đi xuyên tường)
            } else if (r < 65) {
                item.type = FRUIT_DOUBLE;  // 30% Quả Nhân Đôi Điểm
            } else if (r < 85) {
                item.type = FRUIT_SLOW;    // 20% Quả Băng Giá (Làm chậm)
            } else {
                item.type = FRUIT_SHRINK;  // 15% Quả Thu Nhỏ
            }
            item.durationMs = 14000; // Quả chức năng tồn tại 14 giây trên bản đồ
        }

        fruits.push_back(item);
    }

    // Xử lý phím bấm từ người chơi (Hỗ trợ cả W, A, S, D và phím mũi tên)
    void handleInput() {
        while (_kbhit()) {
            int key = _getch();

            // Xử lý phím mũi tên mở rộng (nếu người chơi bấm phím mũi tên)
            if (key == 0 || key == 224) {
                int extKey = _getch();
                if (!isPaused && !isGameOver) {
                    if (extKey == 72 && currentDir != DIR_DOWN) nextDir = DIR_UP;
                    else if (extKey == 80 && currentDir != DIR_UP) nextDir = DIR_DOWN;
                    else if (extKey == 75 && currentDir != DIR_RIGHT) nextDir = DIR_LEFT;
                    else if (extKey == 77 && currentDir != DIR_LEFT) nextDir = DIR_RIGHT;
                }
                continue;
            }

            // Chuyển về chữ thường để nhận cả chữ hoa (W/w, A/a, S/s, D/d)
            char ch = (char)tolower(key);

            if (key == 27 || ch == 'q') { // Phím ESC hoặc Q để thoát game
                isRunning = false;
                return;
            }

            if (ch == 'r') { // Chơi lại bất cứ lúc nào
                initGame();
                return;
            }

            if (ch == 'p' && !isGameOver) { // Tạm dừng trò chơi
                isPaused = !isPaused;
                if (isPaused) {
                    setStatus("Tro choi dang TAM DUNG. Nhan P de tiep tuc!", 60000);
                } else {
                    setStatus("Tiep tuc tro choi!", 2000);
                    lastMoveTime = GetTickCount();
                }
                return;
            }

            // Điều khiển chính bằng 4 phím W, A, S, D
            if (!isPaused && !isGameOver) {
                if (ch == 'w' && currentDir != DIR_DOWN) {
                    nextDir = DIR_UP;
                } else if (ch == 's' && currentDir != DIR_UP) {
                    nextDir = DIR_DOWN;
                } else if (ch == 'a' && currentDir != DIR_RIGHT) {
                    nextDir = DIR_LEFT;
                } else if (ch == 'd' && currentDir != DIR_LEFT) {
                    nextDir = DIR_RIGHT;
                }
            }
        }
    }

    // Tính toán chu kỳ di chuyển (ms) tùy theo điểm số, hướng đi và hiệu ứng Làm Chậm
    DWORD getStepDelayMs() const {
        // Tốc độ cơ bản tăng dần khi ăn nhiều táo (nhưng có giới hạn an toàn)
        int baseDelay = 115 - (applesEaten * 2);
        if (baseDelay < 55) baseDelay = 55;

        // Nếu đang có hiệu ứng Quả Băng Giá (Slow-Mo), rắn di chuyển chậm rãi hơn
        if (isSlowActive()) {
            baseDelay = (int)(baseDelay * 1.55);
        }

        // Trong Console, ô ký tự cao gấp ~1.6 lần chiều ngang nên đi dọc cần delay lớn hơn một chút để tốc độ nhìn đồng đều
        if (currentDir == DIR_UP || currentDir == DIR_DOWN) {
            baseDelay = (int)(baseDelay * 1.30);
        }

        return (DWORD)baseDelay;
    }

    // Quản lý vòng đời của các quả chức năng trên bản đồ
    void updateFruitsLifecycle() {
        DWORD now = GetTickCount();

        // Xóa các quả chức năng đã hết hạn trên sàn nhà
        for (int i = (int)fruits.size() - 1; i >= 0; --i) {
            if (fruits[i].isSpecial && (now - fruits[i].spawnTime >= (DWORD)fruits[i].durationMs)) {
                fruits.erase(fruits.begin() + i);
            }
        }

        // Đếm số quả chức năng hiện có trên bản đồ
        int specialCount = 0;
        for (size_t i = 0; i < fruits.size(); ++i) {
            if (fruits[i].isSpecial) specialCount++;
        }

        // Cứ mỗi 9 giây, nếu trên bản đồ có ít hơn 2 quả chức năng thì sinh thêm 1 quả
        if (specialCount < 2 && (now - lastSpecialSpawnTime >= 9000)) {
            spawnFruit(true);
            lastSpecialSpawnTime = now;
        }
    }

    // Cập nhật logic di chuyển, va chạm tường phòng và ăn quả
    void updateGameLogic() {
        if (isPaused || isGameOver) return;

        updateFruitsLifecycle();

        DWORD now = GetTickCount();
        if (now - lastMoveTime < getStepDelayMs()) {
            return;
        }
        lastMoveTime = now;

        currentDir = nextDir;
        Point newHead = snake.front();

        switch (currentDir) {
            case DIR_UP:    newHead.y--; break;
            case DIR_DOWN:  newHead.y++; break;
            case DIR_LEFT:  newHead.x--; break;
            case DIR_RIGHT: newHead.x++; break;
        }

        bool ghostMode = isGhostActive();

        // Xử lý khi chạm tường bao ngoài ngôi nhà
        if (newHead.x <= 0 || newHead.x >= MAP_WIDTH - 1 ||
            newHead.y <= 0 || newHead.y >= MAP_HEIGHT - 1) {
            if (ghostMode) {
                // Nếu đang có Quả Bóng Ma: Đi xuyên qua tường bao sang phía đối diện ngôi nhà!
                if (newHead.x <= 0) newHead.x = MAP_WIDTH - 2;
                else if (newHead.x >= MAP_WIDTH - 1) newHead.x = 1;
                if (newHead.y <= 0) newHead.y = MAP_HEIGHT - 2;
                else if (newHead.y >= MAP_HEIGHT - 1) newHead.y = 1;
            } else {
                triggerGameOver("Ran da dam vao tuong bao quanh ngoi nha!");
                return;
            }
        }

        // Xử lý khi chạm tường ngăn phòng hoặc nội thất trong nhà
        TileType targetTile = houseMap[newHead.y][newHead.x];
        if (targetTile == TILE_INNER_WALL || targetTile == TILE_FURNITURE) {
            if (!ghostMode) {
                if (targetTile == TILE_INNER_WALL)
                    triggerGameOver("Ran da dam vao buc tuong ngan phong!");
                else
                    triggerGameOver("Ran da va vao do noi that trong phong!");
                return;
            }
            // Nếu đang có hiệu ứng Quả Bóng Ma (ghostMode == true): Đi xuyên qua bình thường!
        }

        // Kiểm tra tự cắn vào thân mình (Trừ đốt đuôi cuối cùng nếu không ăn quả)
        for (size_t i = 0; i < snake.size() - 1; ++i) {
            if (snake[i] == newHead) {
                triggerGameOver("Ran da tu can vao than minh!");
                return;
            }
        }

        // Kiểm tra xem đầu rắn có ăn được quả nào không
        int eatenIndex = -1;
        for (size_t i = 0; i < fruits.size(); ++i) {
            if (fruits[i].pos == newHead) {
                eatenIndex = (int)i;
                break;
            }
        }

        // Thêm đầu mới cho rắn
        snake.push_front(newHead);

        if (eatenIndex != -1) {
            FruitItem eaten = fruits[eatenIndex];
            fruits.erase(fruits.begin() + eatenIndex);
            applyFruitEffect(eaten);
        } else {
            // Nếu không ăn quả thì rút đốt đuôi để giữ nguyên chiều dài
            snake.pop_back();
        }
    }

    // Áp dụng hiệu ứng khi ăn từng loại trái cây hoặc quả chức năng
    void applyFruitEffect(const FruitItem& item) {
        int multiplier = isDoubleScoreActive() ? 2 : 1;
        DWORD now = GetTickCount();

        switch (item.type) {
            case FRUIT_APPLE: {
                int pts = 10 * multiplier;
                score += pts;
                applesEaten++;
                setStatus("An Tao Do [@]: +" + to_string(pts) + " diem!", 2500);
                spawnFruit(false);
                break;
            }
            case FRUIT_BANANA: {
                int pts = 25 * multiplier;
                score += pts;
                applesEaten++;
                setStatus("An Chuoi Vang [$]: +" + to_string(pts) + " diem!", 2500);
                spawnFruit(false);
                break;
            }
            case FRUIT_GHOST: {
                int pts = 20 * multiplier;
                score += pts;
                // Cộng dồn hoặc đặt mới 10 giây Đi Xuyên Tường
                ghostEndTime = (ghostEndTime > now ? ghostEndTime : now) + 10000;
                snake.pop_back(); // Quả chức năng không làm rắn bị dài thêm
                setStatus("KICH HOAT [G] BONG MA: Di xuyen moi buc tuong & noi that (10s)!", 4000);
                break;
            }
            case FRUIT_DOUBLE: {
                int pts = 15 * multiplier;
                score += pts;
                doubleEndTime = (doubleEndTime > now ? doubleEndTime : now) + 12000;
                snake.pop_back();
                setStatus("KICH HOAT [X] NHAN DOI: x2 tat ca diem so nhận duoc (12s)!", 4000);
                break;
            }
            case FRUIT_SLOW: {
                int pts = 15 * multiplier;
                score += pts;
                slowEndTime = (slowEndTime > now ? slowEndTime : now) + 8000;
                snake.pop_back();
                setStatus("KICH HOAT [S] BANG GIA: Lam cham thoi gian de luon lach (8s)!", 4000);
                break;
            }
            case FRUIT_SHRINK: {
                int pts = 30 * multiplier;
                score += pts;
                snake.pop_back();
                // Cắt ngắn bớt tối đa 3 đốt thân rắn (giữ tối thiểu 3 đốt)
                int removed = 0;
                while (snake.size() > 3 && removed < 3) {
                    snake.pop_back();
                    removed++;
                }
                setStatus("KICH HOAT [B] THU NHO: Giam " + to_string(removed) + " dot than ran & +" + to_string(pts) + " diem!", 4000);
                break;
            }
        }

        if (score > highScore) {
            highScore = score;
        }
    }

    // Xử lý khi kết thúc ván chơi
    void triggerGameOver(const string& reason) {
        isGameOver = true;
        gameOverReason = reason;
        if (score > highScore) {
            highScore = score;
        }
    }

    // Vẽ toàn bộ ngôi nhà, các phòng, trái cây, rắn và bảng thông tin
    void render() {
        renderer.clearBuffer();

        bool ghostMode = isGhostActive();

        // 1. Vẽ nhãn tên các phòng chìm trên sàn nhà để người chơi dễ nhận biết
        renderer.drawText(MAP_OFFSET_X + 3,  MAP_OFFSET_Y + 2,  "PHONG KHACH", COLOR_DARK_GRAY);
        renderer.drawText(MAP_OFFSET_X + 34, MAP_OFFSET_Y + 2,  "PHONG BEP", COLOR_DARK_GRAY);
        renderer.drawText(MAP_OFFSET_X + 20, MAP_OFFSET_Y + 14, "HANH LANG TRUNG TAM", COLOR_DARK_GRAY);
        renderer.drawText(MAP_OFFSET_X + 3,  MAP_OFFSET_Y + 19, "PHONG NGU", COLOR_DARK_GRAY);
        renderer.drawText(MAP_OFFSET_X + 34, MAP_OFFSET_Y + 19, "PHONG GIAI TRI", COLOR_DARK_GRAY);

        // 2. Vẽ cấu trúc ngôi nhà (Tường bao, tường phòng, cửa, nội thất)
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                int sx = MAP_OFFSET_X + x;
                int sy = MAP_OFFSET_Y + y;

                switch (houseMap[y][x]) {
                    case TILE_OUTER_WALL:
                        renderer.drawChar(sx, sy, '#', COLOR_WHITE, COLOR_DARK_RED);
                        break;
                    case TILE_INNER_WALL:
                        // Khi bật chế độ Đi Xuyên Tường, tường trong nhà chuyển sang nét đứt màu xanh mờ
                        if (ghostMode) {
                            renderer.drawChar(sx, sy, ':', COLOR_DARK_CYAN, COLOR_BLACK);
                        } else {
                            renderer.drawChar(sx, sy, '#', COLOR_GRAY, COLOR_DARK_GRAY);
                        }
                        break;
                    case TILE_FURNITURE:
                        if (ghostMode) {
                            renderer.drawChar(sx, sy, '=', COLOR_DARK_CYAN, COLOR_BLACK);
                        } else {
                            renderer.drawChar(sx, sy, '=', COLOR_YELLOW, COLOR_DARK_YELLOW);
                        }
                        break;
                    case TILE_DOOR:
                        renderer.drawChar(sx, sy, '.', COLOR_DARK_GREEN, COLOR_BLACK);
                        break;
                    default:
                        break;
                }
            }
        }

        // 3. Vẽ các trái cây & quả chức năng trên sàn nhà
        DWORD now = GetTickCount();
        for (size_t i = 0; i < fruits.size(); ++i) {
            const FruitItem& f = fruits[i];
            int sx = MAP_OFFSET_X + f.pos.x;
            int sy = MAP_OFFSET_Y + f.pos.y;

            // Hiệu ứng nhấp nháy báo hiệu quả chức năng sắp biến mất (khi còn dưới 3 giây)
            if (f.isSpecial) {
                DWORD elapsed = now - f.spawnTime;
                if (elapsed > (DWORD)(f.durationMs - 3000)) {
                    if ((now / 150) % 2 == 0) continue;
                }
            }

            switch (f.type) {
                case FRUIT_APPLE:
                    renderer.drawChar(sx, sy, '@', COLOR_RED);
                    break;
                case FRUIT_BANANA:
                    renderer.drawChar(sx, sy, '$', COLOR_YELLOW);
                    break;
                case FRUIT_GHOST:
                    renderer.drawChar(sx, sy, 'G', COLOR_CYAN);
                    break;
                case FRUIT_DOUBLE:
                    renderer.drawChar(sx, sy, 'X', COLOR_MAGENTA);
                    break;
                case FRUIT_SLOW:
                    renderer.drawChar(sx, sy, 'S', COLOR_BLUE);
                    break;
                case FRUIT_SHRINK:
                    renderer.drawChar(sx, sy, 'B', COLOR_GREEN);
                    break;
            }
        }

        // 4. Vẽ Rắn (Đầu rắn và các đốt thân đổi màu khi có hiệu ứng Đi Xuyên Tường / Nhân Đôi)
        for (int i = (int)snake.size() - 1; i >= 0; --i) {
            int sx = MAP_OFFSET_X + snake[i].x;
            int sy = MAP_OFFSET_Y + snake[i].y;

            if (i == 0) {
                // Vẽ Đầu Rắn theo hướng di chuyển
                char headChar = 'O';
                if (currentDir == DIR_UP) headChar = '^';
                else if (currentDir == DIR_DOWN) headChar = 'v';
                else if (currentDir == DIR_LEFT) headChar = '<';
                else if (currentDir == DIR_RIGHT) headChar = '>';

                WORD headColor = COLOR_YELLOW;
                if (ghostMode) headColor = COLOR_WHITE;
                renderer.drawChar(sx, sy, headChar, headColor);
            } else {
                // Vẽ Thân Rắn
                char bodyChar = (i == (int)snake.size() - 1) ? 'o' : 'O';
                WORD bodyColor = COLOR_GREEN;

                if (ghostMode) {
                    bodyColor = (i % 2 == 0) ? COLOR_CYAN : COLOR_BLUE;
                } else if (isDoubleScoreActive()) {
                    bodyColor = (i % 2 == 0) ? COLOR_MAGENTA : COLOR_YELLOW;
                } else if (isSlowActive()) {
                    bodyColor = (i % 2 == 0) ? COLOR_BLUE : COLOR_GREEN;
                } else {
                    bodyColor = (i % 2 == 0) ? COLOR_GREEN : COLOR_DARK_GREEN;
                }

                renderer.drawChar(sx, sy, bodyChar, bodyColor);
            }
        }

        // 5. Vẽ Bảng Điều Khiển bên phải (HUD)
        renderSidebar(now);

        // 6. Vẽ Thanh Trạng Thái dưới đáy màn hình
        if (now < statusMsgEndTime && !statusMessage.empty()) {
            renderer.drawText(MAP_OFFSET_X, SCREEN_HEIGHT - 1, ">> " + statusMessage, COLOR_YELLOW);
        } else {
            renderer.drawText(MAP_OFFSET_X, SCREEN_HEIGHT - 1, ">> Meo: An qua [G] de di xuyen qua tuong cac phong trong ngoi nha!", COLOR_GRAY);
        }

        // 7. Vẽ Hộp Thoại Tạm Dừng hoặc Kết Thúc Game nếu có
        if (isPaused) {
            renderOverlayBox("TAM DUNG TRO CHOI", "Nhan phim [P] de tiep tuc hoac [R] de choi lai", COLOR_CYAN);
        } else if (isGameOver) {
            renderGameOverOverlay();
        }

        renderer.present();
    }

    // Vẽ bảng thông tin chi tiết ở cột bên phải màn hình
    void renderSidebar(DWORD now) {
        int panelX = MAP_OFFSET_X + MAP_WIDTH + 1;
        int panelW = SCREEN_WIDTH - panelX - 1;

        renderer.drawBox(panelX, 1, panelW, 30, COLOR_DARK_CYAN, "THONG TIN & CHUC NANG");

        // Điểm số & Kỷ lục
        renderer.drawText(panelX + 2, 3, "DIEM SO    : " + to_string(score), COLOR_YELLOW);
        renderer.drawText(panelX + 2, 4, "KY LUC     : " + to_string(highScore), COLOR_GREEN);
        renderer.drawText(panelX + 2, 5, "CHIEU DAI  : " + to_string(snake.size()) + " dot", COLOR_WHITE);

        // Vị trí phòng hiện tại
        renderer.drawText(panelX + 2, 7, "VI TRI HIEN TAI:", COLOR_GRAY);
        renderer.drawText(panelX + 2, 8, getCurrentRoomName(), COLOR_CYAN);

        // Trạng thái Hiệu ứng (Power-ups) đang kích hoạt
        renderer.drawText(panelX + 2, 10, "HIEU UNG DANG BAT:", COLOR_WHITE);
        int buffRow = 11;

        if (isGhostActive()) {
            int secLeft = (int)((ghostEndTime - now + 999) / 1000);
            renderer.drawText(panelX + 2, buffRow++, "[G] Xuyen Tuong : " + to_string(secLeft) + "s", COLOR_CYAN);
        }
        if (isDoubleScoreActive()) {
            int secLeft = (int)((doubleEndTime - now + 999) / 1000);
            renderer.drawText(panelX + 2, buffRow++, "[X] Nhan Doi x2 : " + to_string(secLeft) + "s", COLOR_MAGENTA);
        }
        if (isSlowActive()) {
            int secLeft = (int)((slowEndTime - now + 999) / 1000);
            renderer.drawText(panelX + 2, buffRow++, "[S] Lam Cham    : " + to_string(secLeft) + "s", COLOR_BLUE);
        }
        if (buffRow == 11) {
            renderer.drawText(panelX + 2, buffRow, "(Chua co hieu ung nao)", COLOR_DARK_GRAY);
        }

        // Chú giải các loại quả trong nhà
        renderer.drawText(panelX + 2, 15, "CAC LOAI QUA TRONG NHA:", COLOR_YELLOW);
        renderer.drawChar(panelX + 2, 16, '@', COLOR_RED);
        renderer.drawText(panelX + 4, 16, "Tao Do     : +10 diem", COLOR_GRAY);

        renderer.drawChar(panelX + 2, 17, '$', COLOR_YELLOW);
        renderer.drawText(panelX + 4, 17, "Chuoi Vang : +25 diem", COLOR_GRAY);

        renderer.drawChar(panelX + 2, 18, 'G', COLOR_CYAN);
        renderer.drawText(panelX + 4, 18, "Qua Bong Ma: Xuyen tuong", COLOR_GRAY);

        renderer.drawChar(panelX + 2, 19, 'X', COLOR_MAGENTA);
        renderer.drawText(panelX + 4, 19, "Qua Nhan 2 : x2 Diem so", COLOR_GRAY);

        renderer.drawChar(panelX + 2, 20, 'S', COLOR_BLUE);
        renderer.drawText(panelX + 4, 20, "Qua Bang   : Giam toc do", COLOR_GRAY);

        renderer.drawChar(panelX + 2, 21, 'B', COLOR_GREEN);
        renderer.drawText(panelX + 4, 21, "Qua Thu Nho: -3 dot ran", COLOR_GRAY);

        // Chú giải địa hình & Điều khiển
        renderer.drawText(panelX + 2, 23, "DIEU KHIEN GAME:", COLOR_YELLOW);
        renderer.drawText(panelX + 2, 24, "[W/A/S/D] : Di chuyen 4 huong", COLOR_WHITE);
        renderer.drawText(panelX + 2, 25, "[P]       : Tam dung game", COLOR_GRAY);
        renderer.drawText(panelX + 2, 26, "[R]       : Choi van moi", COLOR_GRAY);
        renderer.drawText(panelX + 2, 27, "[ESC / Q] : Thoat tro choi", COLOR_GRAY);
        renderer.drawText(panelX + 2, 29, "[...] Cua | [===] Noi that", COLOR_DARK_GREEN);
    }

    // Vẽ hộp thoại thông báo nổi giữa bản đồ
    void renderOverlayBox(const string& title, const string& subtitle, WORD borderColor) {
        int boxW = 48;
        int boxH = 7;
        int boxX = MAP_OFFSET_X + (MAP_WIDTH - boxW) / 2;
        int boxY = MAP_OFFSET_Y + (MAP_HEIGHT - boxH) / 2;

        for (int y = boxY; y < boxY + boxH; ++y) {
            for (int x = boxX; x < boxX + boxW; ++x) {
                renderer.drawChar(x, y, ' ', COLOR_WHITE, COLOR_BLACK);
            }
        }
        renderer.drawBox(boxX, boxY, boxW, boxH, borderColor, title);
        renderer.drawText(boxX + 3, boxY + 3, subtitle, COLOR_WHITE);
    }

    // Vẽ bảng tổng kết khi kết thúc ván chơi (Game Over)
    void renderGameOverOverlay() {
        int boxW = 52;
        int boxH = 9;
        int boxX = MAP_OFFSET_X + (MAP_WIDTH - boxW) / 2;
        int boxY = MAP_OFFSET_Y + (MAP_HEIGHT - boxH) / 2;

        for (int y = boxY; y < boxY + boxH; ++y) {
            for (int x = boxX; x < boxX + boxW; ++x) {
                renderer.drawChar(x, y, ' ', COLOR_WHITE, COLOR_BLACK);
            }
        }
        renderer.drawBox(boxX, boxY, boxW, boxH, COLOR_RED, "KET THUC VAN CHOI (GAME OVER)");
        renderer.drawText(boxX + 3, boxY + 2, gameOverReason, COLOR_YELLOW);
        renderer.drawText(boxX + 3, boxY + 4, "Tong diem dat duoc : " + to_string(score) + " diem", COLOR_WHITE);
        renderer.drawText(boxX + 3, boxY + 5, "Ky luc cao nhat    : " + to_string(highScore) + " diem", COLOR_GREEN);
        renderer.drawText(boxX + 3, boxY + 7, "Nhan [R] de Choi Lai  |  Nhan [ESC] de Thoat", COLOR_CYAN);
    }

    // Vòng lặp chính của trò chơi
    void run() {
        while (isRunning) {
            handleInput();
            updateGameLogic();
            render();
            Sleep(12); // Giữ tốc độ quét khung hình ổn định ~60-80 FPS, không tốn CPU
        }
    }
};

// Hàm main khởi chạy trò chơi
int main() {
    HouseSnakeGame game;
    game.run();
    return 0;
}