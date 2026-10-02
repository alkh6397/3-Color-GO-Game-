#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <string>

using namespace std;


// ============================================================
// 기본 설정
// ============================================================

const int BOARD_SIZE = 19;
const int CELL_SIZE = 55;
const int MARGIN = 70;

const int WINDOW_SIZE = 1150;


// ============================================================
// 플레이어
// ============================================================

enum Player
{
    BLACK = 1,
    WHITE = 2,
    RED = 3
};

Player currentPlayer = BLACK;


// ============================================================
// 바둑판
//
// 0 = 빈칸
// 1 = 흑
// 2 = 백
// 3 = 적
// ============================================================

int board[BOARD_SIZE][BOARD_SIZE] = {};


// 각 플레이어가 잡은 돌의 개수
int captured[4] = {};


// 연속 패스 횟수
int consecutivePasses = 0;


// 게임 종료 여부
bool gameOver = false;


// ============================================================
// 방향
// ============================================================

const int dx[4] = { 1, -1, 0, 0 };
const int dy[4] = { 0, 0, 1, -1 };


// ============================================================
// 플레이어 변경
// ============================================================

void nextPlayer()
{
    if (currentPlayer == BLACK)
        currentPlayer = WHITE;

    else if (currentPlayer == WHITE)
        currentPlayer = RED;

    else
        currentPlayer = BLACK;
}


// ============================================================
// 플레이어 이름
// ============================================================

string getPlayerName(Player player)
{
    if (player == BLACK)
        return "BLACK";

    if (player == WHITE)
        return "WHITE";

    return "RED";
}


// ============================================================
// 두 좌표가 바둑판 안에 있는지 확인
// ============================================================

bool isInside(int x, int y)
{
    return x >= 0 &&
        x < BOARD_SIZE &&
        y >= 0 &&
        y < BOARD_SIZE;
}


// ============================================================
// 그룹의 돌들을 찾는다.
//
// (startX, startY)에서 시작해서
// 같은 색으로 연결된 모든 돌을 찾는다.
// ============================================================

vector<pair<int, int>> getGroup(int startX, int startY)
{
    vector<pair<int, int>> group;

    int color = board[startY][startX];

    if (color == 0)
        return group;


    bool visited[BOARD_SIZE][BOARD_SIZE] = {};

    queue<pair<int, int>> q;

    q.push({ startX, startY });
    visited[startY][startX] = true;


    while (!q.empty())
    {
        auto [x, y] = q.front();
        q.pop();

        group.push_back({ x, y });


        for (int dir = 0; dir < 4; dir++)
        {
            int nx = x + dx[dir];
            int ny = y + dy[dir];

            if (!isInside(nx, ny))
                continue;

            if (visited[ny][nx])
                continue;

            if (board[ny][nx] != color)
                continue;

            visited[ny][nx] = true;

            q.push({ nx, ny });
        }
    }

    return group;
}


// ============================================================
// 그룹의 활로 개수
// ============================================================

int getLiberties(vector<pair<int, int>> group)
{
    bool counted[BOARD_SIZE][BOARD_SIZE] = {};

    int liberties = 0;


    for (auto [x, y] : group)
    {
        for (int dir = 0; dir < 4; dir++)
        {
            int nx = x + dx[dir];
            int ny = y + dy[dir];

            if (!isInside(nx, ny))
                continue;

            if (board[ny][nx] != 0)
                continue;

            if (counted[ny][nx])
                continue;

            counted[ny][nx] = true;

            liberties++;
        }
    }

    return liberties;
}


// ============================================================
// 그룹 제거
// ============================================================

void removeGroup(vector<pair<int, int>> group)
{
    for (auto [x, y] : group)
    {
        board[y][x] = 0;
    }
}


// ============================================================
// 특정 색의 돌을 포획
//
// x, y에 돌을 놓은 직후 주변의 상대 그룹을 확인한다.
// 활로가 0이면 제거한다.
//
// 반환값:
// 이번 착수에서 잡은 돌 개수
// ============================================================

int captureEnemyGroups(int x, int y, Player player)
{
    int capturedCount = 0;

    bool checked[BOARD_SIZE][BOARD_SIZE] = {};


    for (int dir = 0; dir < 4; dir++)
    {
        int nx = x + dx[dir];
        int ny = y + dy[dir];

        if (!isInside(nx, ny))
            continue;

        // 빈칸이거나 자기 돌이면 무시
        if (board[ny][nx] == 0)
            continue;

        if (board[ny][nx] == player)
            continue;

        if (checked[ny][nx])
            continue;


        vector<pair<int, int>> group =
            getGroup(nx, ny);


        // 그룹을 검사했음을 기록
        for (auto [gx, gy] : group)
        {
            checked[gy][gx] = true;
        }


        // 활로가 없다면 포획
        if (getLiberties(group) == 0)
        {
            capturedCount += group.size();

            removeGroup(group);
        }
    }


    return capturedCount;
}


// ============================================================
// 현재 보드 상태 복사
//
// 패 판정을 위해 사용
// ============================================================

void copyBoard(
    int destination[BOARD_SIZE][BOARD_SIZE],
    int source[BOARD_SIZE][BOARD_SIZE]
)
{
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            destination[y][x] = source[y][x];
        }
    }
}


// ============================================================
// 두 보드가 동일한지 확인
// ============================================================

bool sameBoard(
    int a[BOARD_SIZE][BOARD_SIZE],
    int b[BOARD_SIZE][BOARD_SIZE]
)
{
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (a[y][x] != b[y][x])
                return false;
        }
    }

    return true;
}


// ============================================================
// 이전 보드 상태
//
// 1차 버전에서는 "직전 상태와 동일한 상태"만 금지한다.
// ============================================================

int previousBoard[BOARD_SIZE][BOARD_SIZE] = {};


// ============================================================
// 착수
//
// 성공하면 true
// 실패하면 false
// ============================================================

bool placeStone(int x, int y)
{
    // 게임 종료
    if (gameOver)
        return false;


    // 이미 돌이 있다면 불가능
    if (board[y][x] != 0)
        return false;


    // 현재 보드 백업
    int beforeBoard[BOARD_SIZE][BOARD_SIZE];

    copyBoard(beforeBoard, board);


    // 돌 놓기
    board[y][x] = currentPlayer;


    // 상대 돌 포획
    int capturedCount =
        captureEnemyGroups(x, y, currentPlayer);


    // ========================================================
    // 자살수 검사
    // ========================================================

    vector<pair<int, int>> myGroup =
        getGroup(x, y);


    if (getLiberties(myGroup) == 0)
    {
        // 자살수이므로 원상복구
        copyBoard(board, beforeBoard);

        return false;
    }


    // ========================================================
    // 패 검사
    //
    // 착수 결과가 직전 보드 상태와 동일하면 금지
    // ========================================================

    if (sameBoard(board, previousBoard))
    {
        copyBoard(board, beforeBoard);

        return false;
    }


    // ========================================================
    // 성공
    // ========================================================

    captured[currentPlayer] += capturedCount;


    // 이번 상태를 다음 패 판정의 기준으로 저장
    copyBoard(previousBoard, beforeBoard);


    // 패스 카운터 초기화
    consecutivePasses = 0;


    // 다음 플레이어
    nextPlayer();


    return true;
}


// ============================================================
// 패스
// ============================================================

void passTurn()
{
    if (gameOver)
        return;


    consecutivePasses++;


    // 세 명 모두 연속으로 패스
    if (consecutivePasses >= 3)
    {
        gameOver = true;

        cout << "Game Over!\n";

        return;
    }


    nextPlayer();
}


// ============================================================
// 집 계산
//
// 빈 영역을 탐색해서
// 주변에 어떤 색의 돌이 있는지 확인한다.
//
// 한 가지 색만 접하면 해당 색의 집.
// 여러 색과 접하면 중립.
// ============================================================

int territory[4] = {};

void calculateTerritory()
{
    // 기존 점수 초기화
    territory[BLACK] = 0;
    territory[WHITE] = 0;
    territory[RED] = 0;


    bool visited[BOARD_SIZE][BOARD_SIZE] = {};


    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            // 이미 방문했거나 돌이 있으면 무시
            if (visited[y][x])
                continue;

            if (board[y][x] != 0)
                continue;


            // 하나의 빈 영역 찾기
            vector<pair<int, int>> region;

            bool touchesBlack = false;
            bool touchesWhite = false;
            bool touchesRed = false;


            queue<pair<int, int>> q;

            q.push({ x, y });
            visited[y][x] = true;


            while (!q.empty())
            {
                auto [cx, cy] = q.front();
                q.pop();


                region.push_back({ cx, cy });


                for (int dir = 0; dir < 4; dir++)
                {
                    int nx = cx + dx[dir];
                    int ny = cy + dy[dir];


                    if (!isInside(nx, ny))
                        continue;


                    int color = board[ny][nx];


                    // 빈칸
                    if (color == 0)
                    {
                        if (!visited[ny][nx])
                        {
                            visited[ny][nx] = true;

                            q.push({ nx, ny });
                        }
                    }

                    // 흑
                    else if (color == BLACK)
                    {
                        touchesBlack = true;
                    }

                    // 백
                    else if (color == WHITE)
                    {
                        touchesWhite = true;
                    }

                    // 적
                    else if (color == RED)
                    {
                        touchesRed = true;
                    }
                }
            }


            // 인접한 색의 개수
            int colors = 0;

            if (touchesBlack)
                colors++;

            if (touchesWhite)
                colors++;

            if (touchesRed)
                colors++;


            // 한 색에게만 둘러싸였으면 그 색의 집
            if (colors == 1)
            {
                if (touchesBlack)
                    territory[BLACK] += region.size();

                else if (touchesWhite)
                    territory[WHITE] += region.size();

                else if (touchesRed)
                    territory[RED] += region.size();
            }

            // colors >= 2이면 중립
        }
    }
}


// ============================================================
// 최종 점수
//
// 점수 = 집 + 잡은 돌
// ============================================================

int getScore(Player player)
{
    return territory[player] + captured[player];
}


// ============================================================
// 최종 결과 출력
// ============================================================

void printResult()
{
    calculateTerritory();


    cout << "\n============================\n";
    cout << "FINAL RESULT\n";
    cout << "============================\n";


    cout << "BLACK\n";
    cout << "Territory : "
        << territory[BLACK] << "\n";
    cout << "Captured  : "
        << captured[BLACK] << "\n";
    cout << "Score     : "
        << getScore(BLACK) << "\n\n";


    cout << "WHITE\n";
    cout << "Territory : "
        << territory[WHITE] << "\n";
    cout << "Captured  : "
        << captured[WHITE] << "\n";
    cout << "Score     : "
        << getScore(WHITE) << "\n\n";


    cout << "RED\n";
    cout << "Territory : "
        << territory[RED] << "\n";
    cout << "Captured  : "
        << captured[RED] << "\n";
    cout << "Score     : "
        << getScore(RED) << "\n";


    int blackScore = getScore(BLACK);
    int whiteScore = getScore(WHITE);
    int redScore = getScore(RED);


    int highest =
        max({ blackScore, whiteScore, redScore });


    cout << "\nWinner: ";

    if (blackScore == highest)
        cout << "BLACK ";

    if (whiteScore == highest)
        cout << "WHITE ";

    if (redScore == highest)
        cout << "RED ";

    cout << "\n";
}


// ============================================================
// 바둑판 그리기
// ============================================================

void drawBoard(sf::RenderWindow& window)
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        // 가로선
        sf::RectangleShape horizontal(
            sf::Vector2f(
                CELL_SIZE * (BOARD_SIZE - 1),
                2
            )
        );

        horizontal.setPosition({
            (float)MARGIN,
            (float)(MARGIN + i * CELL_SIZE)
            });

        horizontal.setFillColor(
            sf::Color::Black
        );

        window.draw(horizontal);


        // 세로선
        sf::RectangleShape vertical(
            sf::Vector2f(
                2,
                CELL_SIZE * (BOARD_SIZE - 1)
            )
        );

        vertical.setPosition({
            (float)(MARGIN + i * CELL_SIZE),
            (float)MARGIN
            });

        vertical.setFillColor(
            sf::Color::Black
        );

        window.draw(vertical);
    }
}


// ============================================================
// 돌 그리기
// ============================================================

void drawStones(sf::RenderWindow& window)
{
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (board[y][x] == 0)
                continue;


            sf::CircleShape stone(28);


            // 색상
            if (board[y][x] == BLACK)
            {
                stone.setFillColor(
                    sf::Color::Black
                );
            }

            else if (board[y][x] == WHITE)
            {
                stone.setFillColor(
                    sf::Color::White
                );

                // 백돌 테두리
                stone.setOutlineThickness(2);

                stone.setOutlineColor(
                    sf::Color::Black
                );
            }

            else if (board[y][x] == RED)
            {
                stone.setFillColor(
                    sf::Color::Red
                );
            }


            // 중심을 교차점에 맞추기
            stone.setPosition({
                (float)(MARGIN + x * CELL_SIZE - 28),
                (float)(MARGIN + y * CELL_SIZE - 28)
                });


            window.draw(stone);
        }
    }
}


// ============================================================
// 마우스 좌표 → 바둑판 좌표
// ============================================================

bool getBoardPosition(
    sf::Vector2i mouse,
    int& x,
    int& y
)
{
    float boardX =
        mouse.x - MARGIN;

    float boardY =
        mouse.y - MARGIN;


    x = (int)round(
        boardX / CELL_SIZE
    );

    y = (int)round(
        boardY / CELL_SIZE
    );


    if (!isInside(x, y))
        return false;


    // 실제 교차점과 너무 멀리 클릭했다면 무시
    float actualX =
        MARGIN + x * CELL_SIZE;

    float actualY =
        MARGIN + y * CELL_SIZE;


    float distance =
        sqrt(
            pow(mouse.x - actualX, 2) +
            pow(mouse.y - actualY, 2)
        );


    if (distance > 20)
        return false;


    return true;
}


// ============================================================
// 메인
// ============================================================

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({
            WINDOW_SIZE,
            WINDOW_SIZE
            }),
        "3 Player Go"
    );


    // 초기 패 상태
    copyBoard(
        previousBoard,
        board
    );


    while (window.isOpen())
    {
        while (auto event = window.pollEvent())
        {
            // 창 닫기
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }


            // 마우스
            if (event->is<
                sf::Event::MouseButtonPressed>())
            {
                auto mouseEvent =
                    event->getIf<
                    sf::Event::MouseButtonPressed
                    >();


                if (mouseEvent->button ==
                    sf::Mouse::Button::Left)
                {
                    int x;
                    int y;


                    if (getBoardPosition(
                        mouseEvent->position,
                        x,
                        y))
                    {
                        if (!gameOver)
                        {
                            bool success =
                                placeStone(x, y);


                            if (success)
                            {
                                cout
                                    << getPlayerName(
                                        currentPlayer
                                    )
                                    << "'s turn\n";
                            }
                            else
                            {
                                cout
                                    << "Invalid move!\n";
                            }
                        }
                    }
                }
            }


            // 키보드
            if (event->is<
                sf::Event::KeyPressed>())
            {
                auto keyEvent =
                    event->getIf<
                    sf::Event::KeyPressed
                    >();


                // SPACE = 패스
                if (keyEvent->code ==
                    sf::Keyboard::Key::Space)
                {
                    if (!gameOver)
                    {
                        passTurn();

                        cout
                            << "PASS\n";


                        if (gameOver)
                        {
                            printResult();
                        }
                    }
                }
            }
        }


        // ====================================================
        // 화면
        // ====================================================

        window.clear(
            sf::Color(220, 190, 130)
        );


        drawBoard(window);
        drawStones(window);


        window.display();
    }


    return 0;
}