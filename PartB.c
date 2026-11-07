/*
 * ESE519_Lab4_Pong_Starter.c
 *
 * Created: 9/21/2021 21:21:21 AM
 * Author : J. Ye
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#include "ST7735.h"
#include "LCD_GFX.h"


// GAME SETTINGS


#define SCREEN_W 160
#define SCREEN_H 128

#define PADDLE_W 4
#define PADDLE_H 24

#define PLAYER_X 2
#define COMPUTER_X 153

#define BALL_RADIUS 3

#define WIN_SCORE 3

#define PLAYER_SPEED 2
#define AI_UPDATE_RATE 2

// LED / BUZZER PINS

#define GREEN_LED PC1
#define RED_LED   PC2
#define BUZZER    PD2

// MOTOR PIN

// PD3 = OC2B
#define MOTOR_PWM PD3

// GAME VARIABLES

int playerY = 52;
int computerY = 52;

int ballX = 80;
int ballY = 64;

int ballDX = 3;
int ballDY = 2;

int playerScore = 0;
int computerScore = 0;

uint8_t aiCounter = 0;

// ADC / JOYSTICK

void ADC_Init(void)
{
    // AVcc reference, ADC0 / PC0
    ADMUX = (1 << REFS0);

    // Enable ADC, prescaler = 128
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

// LED FLASHES

void FlashGreen(void)
{
    PORTC |= (1 << GREEN_LED);
    _delay_ms(100);
    PORTC &= ~(1 << GREEN_LED);
}

void FlashRed(void)
{
    PORTC |= (1 << RED_LED);
    _delay_ms(100);
    PORTC &= ~(1 << RED_LED);
}

// MOTOR PWM

void Motor_Init(void)
{
    // PD3 / OC2B as output
    DDRD |= (1 << DDD3);

    // Timer2 Fast PWM
    // OC2B non-inverting
    TCCR2A =
        (1 << COM2B1) |
        (1 << WGM21) |
        (1 << WGM20);

    // Prescaler = 64
    TCCR2B = (1 << CS22);

    // Motor starts OFF
    OCR2B = 0;
}

void Motor_SetSpeed(uint8_t speed)
{
    OCR2B = speed;
}

// Gradually speed up, then slow down
void Motor_WinAnimation(void)
{
    // Speed up
    for (uint16_t speed = 0; speed <= 255; speed += 5)
    {
        Motor_SetSpeed((uint8_t)speed);
        _delay_ms(25);
    }

    // Stay near full speed briefly
    Motor_SetSpeed(255);
    _delay_ms(300);

    // Slow down
    for (int16_t speed = 255; speed >= 0; speed -= 5)
    {
        Motor_SetSpeed((uint8_t)speed);
        _delay_ms(25);
    }

    Motor_SetSpeed(0);
}

// INITIALIZE

void Initialize(void)
{
    lcd_init();
    ADC_Init();
    Motor_Init();

    // LEDs
    DDRC |= (1 << DDC1) | (1 << DDC2);

    // Buzzer
    DDRD |= (1 << DDD2);

    // Start LEDs off
    PORTC &= ~((1 << GREEN_LED) | (1 << RED_LED));

    // Start buzzer off
    PORTD &= ~(1 << BUZZER);
}

// DRAW SCORE

void DrawScore(void)
{
    LCD_drawChar(
        60,
        5,
        '0' + playerScore,
        WHITE,
        BLACK
    );

    LCD_drawChar(
        95,
        5,
        '0' + computerScore,
        WHITE,
        BLACK
    );
}

// RESET BALL

void ResetBall(int direction)
{
    ballX = SCREEN_W / 2;
    ballY = SCREEN_H / 2;

    if (direction > 0)
    {
        ballDX = 3;
    }
    else
    {
        ballDX = -3;
    }

    ballDY = -ballDY;

    if (ballDY == 0)
    {
        ballDY = 2;
    }
}

// MAIN

int main(void)
{
    uint16_t joystick;

    Initialize();

    // Clear screen once
    LCD_setScreen(BLACK);

    DrawScore();

    // Draw player paddle
    LCD_drawBlock(
        PLAYER_X,
        playerY,
        PLAYER_X + PADDLE_W,
        playerY + PADDLE_H,
        WHITE
    );

    // Draw computer paddle
    LCD_drawBlock(
        COMPUTER_X,
        computerY,
        COMPUTER_X + PADDLE_W,
        computerY + PADDLE_H,
        WHITE
    );

    // Draw ball
    LCD_drawCircle(
        ballX,
        ballY,
        BALL_RADIUS,
        WHITE
    );

    while (1)
    {
        // ERASE OLD BALL

        LCD_drawCircle(
            ballX,
            ballY,
            BALL_RADIUS,
            BLACK
        );

        // READ JOYSTICK

        joystick = ADC_Read();

        // PLAYER MOVE UP

        if (joystick > 650)
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

        // PLAYER MOVE DOWN

        else if (joystick < 350)
        {
            if (playerY <= SCREEN_H - PADDLE_H - PLAYER_SPEED - 1)
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

        // COMPUTER AI

        aiCounter++;

        if (aiCounter >= AI_UPDATE_RATE)
        {
            aiCounter = 0;

            if (ballY < computerY + (PADDLE_H / 2))
            {
                if (computerY > 0)
                {
                    LCD_drawBlock(
                        COMPUTER_X,
                        computerY + PADDLE_H,
                        COMPUTER_X + PADDLE_W,
                        computerY + PADDLE_H,
                        BLACK
                    );

                    computerY--;

                    LCD_drawBlock(
                        COMPUTER_X,
                        computerY,
                        COMPUTER_X + PADDLE_W,
                        computerY,
                        WHITE
                    );
                }
            }

            else if (ballY > computerY + (PADDLE_H / 2))
            {
                if (computerY < SCREEN_H - PADDLE_H - 1)
                {
                    LCD_drawBlock(
                        COMPUTER_X,
                        computerY,
                        COMPUTER_X + PADDLE_W,
                        computerY,
                        BLACK
                    );

                    computerY++;

                    LCD_drawBlock(
                        COMPUTER_X,
                        computerY + PADDLE_H,
                        COMPUTER_X + PADDLE_W,
                        computerY + PADDLE_H,
                        WHITE
                    );
                }
            }
        }

        // MOVE BALL

        ballX += ballDX;
        ballY += ballDY;

        // TOP WALL

        if (ballY <= BALL_RADIUS)
        {
            ballY = BALL_RADIUS;
            ballDY = -ballDY;

            Beep(20);
        }

        // BOTTOM WALL

        if (ballY >= SCREEN_H - BALL_RADIUS - 1)
        {
            ballY = SCREEN_H - BALL_RADIUS - 1;
            ballDY = -ballDY;

            Beep(20);
        }

        // PLAYER PADDLE COLLISION

        if (
            ballDX < 0 &&
            ballX - BALL_RADIUS <= PLAYER_X + PADDLE_W &&
            ballX + BALL_RADIUS >= PLAYER_X &&
            ballY + BALL_RADIUS >= playerY &&
            ballY - BALL_RADIUS <= playerY + PADDLE_H
        )
        {
            ballX = PLAYER_X + PADDLE_W + BALL_RADIUS;
            ballDX = -ballDX;

            Beep(30);
        }

        // COMPUTER PADDLE COLLISION

        if (
            ballDX > 0 &&
            ballX + BALL_RADIUS >= COMPUTER_X &&
            ballX - BALL_RADIUS <= COMPUTER_X + PADDLE_W &&
            ballY + BALL_RADIUS >= computerY &&
            ballY - BALL_RADIUS <= computerY + PADDLE_H
        )
        {
            ballX = COMPUTER_X - BALL_RADIUS;
            ballDX = -ballDX;

            Beep(30);
        }

        // PLAYER MISSES
        // COMPUTER SCORES

        if (ballX < 0)
        {
            computerScore++;

            Beep(100);
            FlashRed();

            DrawScore();

            ResetBall(-1);
        }

        // COMPUTER MISSES
        // PLAYER SCORES

        if (ballX >= SCREEN_W)
        {
            playerScore++;

            Beep(100);
            FlashGreen();

            DrawScore();

            ResetBall(1);
        }

        // CHECK FOR WINNER

        if (
            playerScore >= WIN_SCORE ||
            computerScore >= WIN_SCORE
        )
        {
            // Win sound
            Beep(200);

            // Spin flag motor
            Motor_WinAnimation();

            // Pause briefly
            _delay_ms(500);

            // Reset scores
            playerScore = 0;
            computerScore = 0;

            DrawScore();

            // Reset ball
            ResetBall(1);
        }

        // DRAW BALL

        LCD_drawCircle(
            ballX,
            ballY,
            BALL_RADIUS,
            WHITE
        );

        // GAME SPEED

        _delay_ms(7);
    }

    return 0;
}
