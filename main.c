#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#include "ST7735.h"
#include "LCD_GFX.h"


// SETTINGS


#define SCREEN_W 160
#define SCREEN_H 128

#define PADDLE_W 4
#define PADDLE_H 24

#define PLAYER_X 2
#define COMPUTER_X 153

#define BALL_RADIUS 3
#define WIN_SCORE 3
#define PLAYER_SPEED 2

#define GREEN_LED PC1
#define RED_LED   PC2
#define BUZZER    PD2

#define WIRELESS_UP   PC3
#define WIRELESS_DOWN PC4


// PONG VARIABLES


int playerY = 52;
int computerY = 52;

int ballX = 80;
int ballY = 64;

int ballDX = 3;
int ballDY = 2;

int playerScore = 0;
int computerScore = 0;

uint8_t difficulty = 1;
uint8_t aiCounter = 0;
uint8_t aiRate = 1;
uint8_t aiSpeed = 1;



// ADC


void ADC_Init(void)
{
    ADMUX = (1 << REFS0);

    ADCSRA =
        (1 << ADEN) |
        (1 << ADPS2) |
        (1 << ADPS1) |
        (1 << ADPS0);

    DIDR0 |= (1 << ADC0D);
}

uint16_t ADC_Read(void)
{
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}



// BUZZER


void Beep(uint16_t cycles)
{
    for (uint16_t i = 0; i < cycles; i++)
    {
        PORTD |= (1 << BUZZER);
        _delay_us(200);

        PORTD &= ~(1 << BUZZER);
        _delay_us(200);
    }
}



// LED ANIMATIONS - LEVEL I


void PlayerPointAnimation(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        PORTC |= (1 << GREEN_LED);
        _delay_ms(70);

        PORTC &= ~(1 << GREEN_LED);
        _delay_ms(70);
    }
}

void ComputerPointAnimation(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        PORTC |= (1 << RED_LED);
        _delay_ms(70);

        PORTC &= ~(1 << RED_LED);
        _delay_ms(70);
    }
}

void GameWinAnimation(uint8_t playerWon)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        PORTC |= (1 << GREEN_LED);
        PORTC &= ~(1 << RED_LED);
        _delay_ms(80);

        PORTC &= ~(1 << GREEN_LED);
        PORTC |= (1 << RED_LED);
        _delay_ms(80);
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        PORTC |= (1 << GREEN_LED) | (1 << RED_LED);
        _delay_ms(100);

        PORTC &= ~((1 << GREEN_LED) | (1 << RED_LED));
        _delay_ms(100);
    }

    if (playerWon)
        PORTC |= (1 << GREEN_LED);
    else
        PORTC |= (1 << RED_LED);

    _delay_ms(350);

    PORTC &= ~((1 << GREEN_LED) | (1 << RED_LED));
}



// MOTOR


void Motor_Init(void)
{
    DDRD |= (1 << DDD3);

    TCCR2A =
        (1 << COM2B1) |
        (1 << WGM21) |
        (1 << WGM20);

    TCCR2B = (1 << CS22);

    OCR2B = 0;
}

void Motor_SetSpeed(uint8_t speed)
{
    OCR2B = speed;
}

void Motor_WinAnimation(void)
{
    for (uint16_t s = 0; s <= 255; s += 5)
    {
        Motor_SetSpeed((uint8_t)s);
        _delay_ms(20);
    }

    Motor_SetSpeed(255);
    _delay_ms(250);

    for (int16_t s = 255; s >= 0; s -= 5)
    {
        Motor_SetSpeed((uint8_t)s);
        _delay_ms(20);
    }

    Motor_SetSpeed(0);
}



// INITIALIZATION


void Initialize(void)
{
    lcd_init();
    ADC_Init();
    Motor_Init();

    // LEDs
    DDRC |= (1 << DDC1) | (1 << DDC2);

    // Wireless inputs
    DDRC &= ~((1 << DDC3) | (1 << DDC4));

    // No internal pullups
    PORTC &= ~((1 << PORTC3) | (1 << PORTC4));

    // Buzzer
    DDRD |= (1 << DDD2);

    PORTC &= ~((1 << GREEN_LED) | (1 << RED_LED));
    PORTD &= ~(1 << BUZZER);
}



// GAME SELECT MENU


uint8_t GameMenu(void)
{
    uint8_t game = 0;
    uint8_t oldUp = 0;
    uint8_t oldDown = 0;

    LCD_setScreen(BLACK);

    LCD_drawString(45, 20, "SELECT", WHITE, BLACK);

    while (1)
    {
        LCD_drawBlock(30, 50, 130, 75, BLACK);

        if (game == 0)
            LCD_drawString(60, 55, "PONG", GREEN, BLACK);
        else
            LCD_drawString(50, 55, "FLAPPY", YELLOW, BLACK);

        LCD_drawString(20, 90, "UP=CHANGE", WHITE, BLACK);
        LCD_drawString(20, 105, "DOWN=START", WHITE, BLACK);

        uint8_t up =
            (PINC & (1 << WIRELESS_UP)) != 0;

        uint8_t down =
            (PINC & (1 << WIRELESS_DOWN)) != 0;

        if (up && !oldUp)
        {
            game = !game;
            Beep(10);
        }

        if (down && !oldDown)
        {
            while (PINC & (1 << WIRELESS_DOWN))
            {
            }

            return game;
        }

        oldUp = up;
        oldDown = down;

        _delay_ms(20);
    }
}



// DIFFICULTY - LEVEL II


void SetDifficulty(void)
{
    if (difficulty == 0)
    {
        // EASY
        aiRate = 4;
        aiSpeed = 1;
    }
    else if (difficulty == 1)
    {
        // MEDIUM - much better than before
        aiRate = 1;
        aiSpeed = 1;
    }
    else
    {
        // HARD
        aiRate = 1;
        aiSpeed = 2;
    }
}

void DrawDifficulty(void)
{
    LCD_drawBlock(25, 50, 135, 70, BLACK);

    if (difficulty == 0)
        LCD_drawString(55, 55, "EASY", GREEN, BLACK);

    else if (difficulty == 1)
        LCD_drawString(48, 55, "MEDIUM", YELLOW, BLACK);

    else
        LCD_drawString(55, 55, "HARD", RED, BLACK);
}

void DifficultyMenu(void)
{
    uint8_t oldUp = 0;
    uint8_t oldDown = 0;

    LCD_setScreen(BLACK);

    LCD_drawString(33, 20, "DIFFICULTY", WHITE, BLACK);
    LCD_drawString(20, 90, "UP=CHANGE", WHITE, BLACK);
    LCD_drawString(20, 105, "DOWN=START", WHITE, BLACK);

    DrawDifficulty();

    while (1)
    {
        uint8_t up =
            (PINC & (1 << WIRELESS_UP)) != 0;

        uint8_t down =
            (PINC & (1 << WIRELESS_DOWN)) != 0;

        if (up && !oldUp)
        {
            difficulty++;

            if (difficulty > 2)
                difficulty = 0;

            DrawDifficulty();
            Beep(10);
        }

        if (down && !oldDown)
        {
            SetDifficulty();

            while (PINC & (1 << WIRELESS_DOWN))
            {
            }

            return;
        }

        oldUp = up;
        oldDown = down;

        _delay_ms(20);
    }
}



// PONG HELPERS


void DrawScore(void)
{
    LCD_drawChar(
        60, 5,
        '0' + playerScore,
        WHITE,
        BLACK
    );

    LCD_drawChar(
        95, 5,
        '0' + computerScore,
        WHITE,
        BLACK
    );
}

void ResetBall(int direction)
{
    ballX = 80;
    ballY = 64;

    ballDX = (direction > 0) ? 3 : -3;

    ballDY = -ballDY;

    if (ballDY == 0)
        ballDY = 2;
}



// BETTER COMPUTER AI


void MoveComputer(void)
{
    aiCounter++;

    if (aiCounter < aiRate)
        return;

    aiCounter = 0;

    int target = ballY;

    // HARD predicts ahead vertically
    if (difficulty == 2 && ballDX > 0)
    {
        target += ballDY * 6;

        if (target < 5)
            target = 5;

        if (target > SCREEN_H - 5)
            target = SCREEN_H - 5;
    }

    int center = computerY + (PADDLE_H / 2);

    if (target < center && computerY > 0)
    {
        uint8_t move = aiSpeed;

        if (computerY < move)
            move = computerY;

        LCD_drawBlock(
            COMPUTER_X,
            computerY + PADDLE_H - move + 1,
            COMPUTER_X + PADDLE_W,
            computerY + PADDLE_H,
            BLACK
        );

        computerY -= move;

        LCD_drawBlock(
            COMPUTER_X,
            computerY,
            COMPUTER_X + PADDLE_W,
            computerY + move - 1,
            WHITE
        );
    }

    else if (
        target > center &&
        computerY < SCREEN_H - PADDLE_H - 1
    )
    {
        uint8_t move = aiSpeed;

        int room =
            SCREEN_H -
            PADDLE_H -
            1 -
            computerY;

        if (move > room)
            move = room;

        LCD_drawBlock(
            COMPUTER_X,
            computerY,
            COMPUTER_X + PADDLE_W,
            computerY + move - 1,
            BLACK
        );

        computerY += move;

        LCD_drawBlock(
            COMPUTER_X,
            computerY + PADDLE_H - move + 1,
            COMPUTER_X + PADDLE_W,
            computerY + PADDLE_H,
            WHITE
        );
    }
}


// PONG


void PongGame(void)
{
    uint16_t joystick;

    playerY = 52;
    computerY = 52;

    playerScore = 0;
    computerScore = 0;

    ResetBall(1);

    LCD_setScreen(BLACK);

    DrawScore();

    LCD_drawBlock(
        PLAYER_X,
        playerY,
        PLAYER_X + PADDLE_W,
        playerY + PADDLE_H,
        WHITE
    );

    LCD_drawBlock(
        COMPUTER_X,
        computerY,
        COMPUTER_X + PADDLE_W,
        computerY + PADDLE_H,
        WHITE
    );

    while (1)
    {
        LCD_drawCircle(
            ballX,
            ballY,
            BALL_RADIUS,
            BLACK
        );

        joystick = ADC_Read();

        uint8_t up =
            (PINC & (1 << WIRELESS_UP)) != 0;

        uint8_t down =
            (PINC & (1 << WIRELESS_DOWN)) != 0;


        // PLAYER UP
        if (joystick > 650 || up)
        {
            if (playerY >= PLAYER_SPEED)
            {
                LCD_drawBlock(
                    PLAYER_X,
                    playerY + PADDLE_H - PLAYER_SPEED + 1,
                    PLAYER_X + PADDLE_W,
                    playerY + PADDLE_H,
                    BLACK
                );

                playerY -= PLAYER_SPEED;

                LCD_drawBlock(
                    PLAYER_X,
                    playerY,
                    PLAYER_X + PADDLE_W,
                    playerY + PLAYER_SPEED - 1,
                    WHITE
                );
            }
        }

        // PLAYER DOWN
        else if (joystick < 350 || down)
        {
            if (
                playerY <=
                SCREEN_H -
                PADDLE_H -
                PLAYER_SPEED - 1
            )
            {
                LCD_drawBlock(
                    PLAYER_X,
                    playerY,
                    PLAYER_X + PADDLE_W,
                    playerY + PLAYER_SPEED - 1,
                    BLACK
                );

                playerY += PLAYER_SPEED;

                LCD_drawBlock(
                    PLAYER_X,
                    playerY + PADDLE_H - PLAYER_SPEED + 1,
                    PLAYER_X + PADDLE_W,
                    playerY + PADDLE_H,
                    WHITE
                );
            }
        }

        // BETTER AI
        MoveComputer();

        // BALL
        ballX += ballDX;
        ballY += ballDY;

        // TOP/BOTTOM
        if (ballY <= BALL_RADIUS)
        {
            ballY = BALL_RADIUS;
            ballDY = -ballDY;
            Beep(20);
        }

        if (ballY >= SCREEN_H - BALL_RADIUS - 1)
        {
            ballY = SCREEN_H - BALL_RADIUS - 1;
            ballDY = -ballDY;
            Beep(20);
        }

        // PLAYER PADDLE
        if (
            ballDX < 0 &&
            ballX - BALL_RADIUS <= PLAYER_X + PADDLE_W &&
            ballY + BALL_RADIUS >= playerY &&
            ballY - BALL_RADIUS <= playerY + PADDLE_H
        )
        {
            ballX =
                PLAYER_X +
                PADDLE_W +
                BALL_RADIUS;

            ballDX = -ballDX;

            Beep(30);
        }

        // COMPUTER PADDLE
        if (
            ballDX > 0 &&
            ballX + BALL_RADIUS >= COMPUTER_X &&
            ballY + BALL_RADIUS >= computerY &&
            ballY - BALL_RADIUS <= computerY + PADDLE_H
        )
        {
            ballX =
                COMPUTER_X -
                BALL_RADIUS;

            ballDX = -ballDX;

            Beep(30);
        }

        // COMPUTER SCORES
        if (ballX < 0)
        {
            computerScore++;

            Beep(80);
            ComputerPointAnimation();

            DrawScore();

            ResetBall(-1);
        }

        // PLAYER SCORES
        if (ballX >= SCREEN_W)
        {
            playerScore++;

            Beep(80);
            PlayerPointAnimation();

            DrawScore();

            ResetBall(1);
        }

        // WIN
        if (
            playerScore >= WIN_SCORE ||
            computerScore >= WIN_SCORE
        )
        {
            uint8_t playerWon =
                playerScore >= WIN_SCORE;

            Beep(150);

            GameWinAnimation(playerWon);
            Motor_WinAnimation();

            playerScore = 0;
            computerScore = 0;

            DrawScore();
            ResetBall(1);
        }

        LCD_drawCircle(
            ballX,
            ballY,
            BALL_RADIUS,
            WHITE
        );

        _delay_ms(7);
    }
}



// SIMPLE FLAPPY BIRD - LEVEL III


void FlappyGame(void)
{
    int birdY = 60;
    int velocity = 0;

    int pipeX = 150;
    int gapY = 45;

    uint8_t score = 0;
    uint8_t oldUp = 0;

    const uint8_t birdX = 30;
    const uint8_t birdSize = 5;

    const uint8_t pipeW = 8;
    const uint8_t gapH = 40;

    LCD_setScreen(BLACK);

    LCD_drawString(
        5,
        5,
        "FLAPPY",
        WHITE,
        BLACK
    );

    while (1)
    {
        // ERASE OLD BIRD
        LCD_drawBlock(
            birdX,
            birdY,
            birdX + birdSize,
            birdY + birdSize,
            BLACK
        );

        // ERASE OLD PIPE
        LCD_drawBlock(
            pipeX,
            0,
            pipeX + pipeW,
            gapY,
            BLACK
        );

        LCD_drawBlock(
            pipeX,
            gapY + gapH,
            pipeX + pipeW,
            SCREEN_H - 1,
            BLACK
        );


        // UP BUTTON = FLAP
        uint8_t up =
            (PINC & (1 << WIRELESS_UP)) != 0;

        if (up && !oldUp)
        {
            velocity = -5;
            Beep(10);
        }

        oldUp = up;


        // GRAVITY
        velocity++;

        if (velocity > 4)
            velocity = 4;

        birdY += velocity;


        // MOVE PIPE
        pipeX -= 3;


        // NEW PIPE
        if (pipeX < -pipeW)
        {
            pipeX = SCREEN_W - 1;

            // Change gap position
            gapY += 18;

            if (gapY > 70)
                gapY = 25;

            score++;

            if (score > 9)
                score = 0;

            LCD_drawChar(
                80,
                5,
                '0' + score,
                WHITE,
                BLACK
            );
        }


        // COLLISION WITH TOP/BOTTOM
        if (
            birdY <= 0 ||
            birdY + birdSize >= SCREEN_H
        )
        {
            Beep(120);
            ComputerPointAnimation();
            _delay_ms(500);
            return;
        }


        // COLLISION WITH PIPE
        if (
            birdX + birdSize >= pipeX &&
            birdX <= pipeX + pipeW
        )
        {
            if (
                birdY <= gapY ||
                birdY + birdSize >= gapY + gapH
            )
            {
                Beep(120);
                ComputerPointAnimation();
                _delay_ms(500);
                return;
            }
        }


        // DRAW PIPE
        LCD_drawBlock(
            pipeX,
            0,
            pipeX + pipeW,
            gapY,
            GREEN
        );

        LCD_drawBlock(
            pipeX,
            gapY + gapH,
            pipeX + pipeW,
            SCREEN_H - 1,
            GREEN
        );


        // DRAW BIRD
        LCD_drawBlock(
            birdX,
            birdY,
            birdX + birdSize,
            birdY + birdSize,
            YELLOW
        );

        _delay_ms(30);
    }
}



// MAIN


int main(void)
{
    Initialize();

    while (1)
    {
        uint8_t game = GameMenu();

        if (game == 0)
        {
            DifficultyMenu();
            PongGame();
        }
        else
        {
            FlappyGame();
        }
    }

    return 0;
}
