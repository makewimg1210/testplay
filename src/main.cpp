#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct RawTerminal {
    termios oldTermios{};
    int oldFlags = 0;

    RawTerminal() {
        tcgetattr(STDIN_FILENO, &oldTermios);
        termios raw = oldTermios;
        raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        oldFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldFlags | O_NONBLOCK);
    }

    ~RawTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        fcntl(STDIN_FILENO, F_SETFL, oldFlags);
    }
};

struct Platform {
    int x;
    int y;
    int width;
};

struct Enemy {
    float x;
    int y;
    float left;
    float right;
    float speed;
    int direction;
};

struct Coin {
    int x;
    int y;
    bool collected = false;
};

constexpr int kViewWidth = 72;
constexpr int kViewHeight = 22;
constexpr int kWorldWidth = 240;
constexpr int kGroundY = 18;
constexpr float kGravity = 58.0f;
constexpr float kMoveSpeed = 20.0f;
constexpr float kJumpPower = 23.0f;

}  // namespace

int main() {
    RawTerminal terminal;

    float playerX = 4.0f;
    float playerY = static_cast<float>(kGroundY - 1);
    float vx = 0.0f;
    float vy = 0.0f;
    bool onGround = true;
    int score = 0;

    std::vector<Platform> platforms = {
        {0, kGroundY, kWorldWidth},
        {22, 14, 10},
        {46, 12, 12},
        {78, 9, 10},
        {104, 13, 12},
        {134, 11, 11},
        {165, 8, 14},
        {198, 12, 10},
    };

    std::vector<Enemy> enemies = {
        {60.0f, kGroundY - 1, 54.0f, 86.0f, 8.0f, 1},
        {144.0f, 10, 135.0f, 175.0f, 10.0f, -1},
    };

    std::vector<Coin> coins = {
        {25, 13, false}, {50, 11, false}, {82, 8, false}, {108, 12, false},
        {139, 10, false}, {170, 7, false}, {201, 11, false}, {225, 17, false},
    };

    auto previous = std::chrono::steady_clock::now();
    bool running = true;

    while (running) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - previous).count();
        previous = now;
        dt = std::min(dt, 0.05f);

        bool left = false;
        bool right = false;
        bool jump = false;

        while (true) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(STDIN_FILENO, &set);
            timeval timeout{0, 0};
            if (select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) <= 0) {
                break;
            }
            char c = '\0';
            if (read(STDIN_FILENO, &c, 1) <= 0) {
                break;
            }
            if (c == 'a' || c == 'A') {
                left = true;
            } else if (c == 'd' || c == 'D') {
                right = true;
            } else if (c == 'w' || c == 'W' || c == ' ') {
                jump = true;
            } else if (c == 'q' || c == 'Q') {
                running = false;
            }
        }

        vx = 0.0f;
        if (left) {
            vx = -kMoveSpeed;
        }
        if (right) {
            vx = kMoveSpeed;
        }

        if (jump && onGround) {
            vy = -kJumpPower;
            onGround = false;
        }

        vy += kGravity * dt;

        playerX += vx * dt;
        playerX = std::clamp(playerX, 0.0f, static_cast<float>(kWorldWidth - 1));

        playerY += vy * dt;
        onGround = false;

        for (const auto& platform : platforms) {
            int px = static_cast<int>(std::round(playerX));
            int py = static_cast<int>(std::round(playerY));
            if (px >= platform.x && px < platform.x + platform.width && py >= platform.y - 1 && py <= platform.y) {
                playerY = static_cast<float>(platform.y - 1);
                vy = 0.0f;
                onGround = true;
            }
        }

        if (playerY > static_cast<float>(kViewHeight + 5)) {
            playerX = 4.0f;
            playerY = static_cast<float>(kGroundY - 1);
            vy = 0.0f;
            score = 0;
            for (auto& coin : coins) {
                coin.collected = false;
            }
        }

        for (auto& enemy : enemies) {
            enemy.x += enemy.speed * enemy.direction * dt;
            if (enemy.x < enemy.left) {
                enemy.x = enemy.left;
                enemy.direction = 1;
            } else if (enemy.x > enemy.right) {
                enemy.x = enemy.right;
                enemy.direction = -1;
            }

            if (static_cast<int>(std::round(enemy.x)) == static_cast<int>(std::round(playerX)) && enemy.y == static_cast<int>(std::round(playerY))) {
                playerX = 4.0f;
                playerY = static_cast<float>(kGroundY - 1);
                vy = 0.0f;
                score = 0;
                for (auto& coin : coins) {
                    coin.collected = false;
                }
            }
        }

        for (auto& coin : coins) {
            if (!coin.collected && coin.x == static_cast<int>(std::round(playerX)) && coin.y == static_cast<int>(std::round(playerY))) {
                coin.collected = true;
                score += 10;
            }
        }

        int cameraLeft = std::clamp(static_cast<int>(std::round(playerX)) - kViewWidth / 3, 0, kWorldWidth - kViewWidth);

        std::vector<std::string> buffer(kViewHeight, std::string(kViewWidth, ' '));
        for (const auto& platform : platforms) {
            for (int x = platform.x; x < platform.x + platform.width; ++x) {
                if (x >= cameraLeft && x < cameraLeft + kViewWidth && platform.y >= 0 && platform.y < kViewHeight) {
                    buffer[platform.y][x - cameraLeft] = '#';
                }
            }
        }

        for (const auto& coin : coins) {
            if (!coin.collected && coin.x >= cameraLeft && coin.x < cameraLeft + kViewWidth && coin.y >= 0 && coin.y < kViewHeight) {
                buffer[coin.y][coin.x - cameraLeft] = '$';
            }
        }

        for (const auto& enemy : enemies) {
            int ex = static_cast<int>(std::round(enemy.x));
            if (ex >= cameraLeft && ex < cameraLeft + kViewWidth && enemy.y >= 0 && enemy.y < kViewHeight) {
                buffer[enemy.y][ex - cameraLeft] = 'X';
            }
        }

        int playerDrawX = static_cast<int>(std::round(playerX));
        int playerDrawY = static_cast<int>(std::round(playerY));
        if (playerDrawX >= cameraLeft && playerDrawX < cameraLeft + kViewWidth && playerDrawY >= 0 && playerDrawY < kViewHeight) {
            buffer[playerDrawY][playerDrawX - cameraLeft] = '@';
        }

        std::cout << "\x1b[2J\x1b[H";
        std::cout << "2D横スクロールアクション(ターミナル版) | A/D移動 W/Spaceジャンプ Q終了 | SCORE: " << score << "\n";
        for (const auto& line : buffer) {
            std::cout << line << '\n';
        }
        std::cout << "\nゴールは右端。敵(X)を避け、コイン($)を集めてください。\n";

        if (playerX >= kWorldWidth - 2) {
            std::cout << "\nCLEAR! スコア: " << score << " でゴールしました。Qで終了。\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
