/*
 * File:   mini_calsi.c
 * Author: Rakesh B
 *
 * Created on September 26, 2025, 10:53 PM
 */


// PIC16F877A Configuration Bit Settings
// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Disable
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection off
#pragma config WRT = OFF        // Flash Program Memory Write Protection off
#pragma config CP = OFF         // Flash Program Memory Code Protection off

#include <xc.h>
#define _XTAL_FREQ 20000000     // 20 MHz crystal

// LCD control pins
#define RS RD0
#define RW RD1
#define EN RD2

// Keypad pins
#define C1 RB0
#define C2 RB1
#define C3 RB2
#define C4 RB3
#define R1 RB4
#define R2 RB5
#define R3 RB6
#define R4 RB7

// -------- LCD Functions --------
void lcd_cmd(unsigned char cmd){
    PORTC = cmd;
    RS = 0;
    RW = 0;
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_data(unsigned char data){
    PORTC = data;
    RS = 1;
    RW = 0;
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_string(const char *str){
    while(*str){
        lcd_data(*str++);
    }
}

void lcd_initialize(){
    lcd_cmd(0x38); // 8-bit, 2-line, 5x7
    lcd_cmd(0x06); // Increment cursor
    lcd_cmd(0x0C); // Display ON, cursor OFF
    lcd_cmd(0x01); // Clear screen
    __delay_ms(2);
}

// -------- Keypad Scan Function --------
char keypad(){
    // Column 1
    C1=1; C2=0; C3=0; C4=0;
    if(R1==1){ while(R1==1); return '7'; }
    if(R2==1){ while(R2==1); return '4'; }
    if(R3==1){ while(R3==1); return '1'; }
    if(R4==1){ while(R4==1); return 'C'; }

    // Column 2
    C1=0; C2=1; C3=0; C4=0;
    if(R1==1){ while(R1==1); return '8'; }
    if(R2==1){ while(R2==1); return '5'; }
    if(R3==1){ while(R3==1); return '2'; }
    if(R4==1){ while(R4==1); return '0'; }

    // Column 3
    C1=0; C2=0; C3=1; C4=0;
    if(R1==1){ while(R1==1); return '9'; }
    if(R2==1){ while(R2==1); return '6'; }
    if(R3==1){ while(R3==1); return '3'; }
    if(R4==1){ while(R4==1); return '='; }

    // Column 4
    C1=0; C2=0; C3=0; C4=1;
    if(R1==1){ while(R1==1); return '/'; }
    if(R2==1){ while(R2==1); return '*'; }
    if(R3==1){ while(R3==1); return '-'; }
    if(R4==1){ while(R4==1); return '+'; }

    return 0; // no key pressed
}

// -------- Main Program --------
void main(void){
    TRISC = 0x00;   // LCD data port
    TRISD = 0x00;   // LCD control port
    TRISB = 0xF0;   // RB7-RB4 input (rows), RB3-RB0 output (cols)
    PORTB = 0x00;   // clear keypad port

    lcd_initialize();
    lcd_cmd(0x80);
    lcd_string("Calculator");

    lcd_cmd(0xC0);
    lcd_string("Ready...");

    char key;
    while(1){
        key = keypad();
        if(key){
            lcd_data(key);   // display key on LCD
            __delay_ms(200);
        }
    }
}
