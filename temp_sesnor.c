/*
 * File:   temp_sesnor.c
 * Author: Rakesh B
 *
 * Created on October 5, 2025, 12:04 PM
 */


// PIC16F877A Configuration Bit Settings

// 'C' source line config statements

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


#include <xc.h>
#define _XTAL_FREQ 20000000

// LCD connections
#define RS RC0
#define RW RC1
#define EN RC2

#define LCD PORTD

// LCD Functions
void lcd_cmd(char cmd) {
    RS = 0;
    RW = 0;
    LCD = cmd;
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_data(char data) {
    RS = 1;
    RW = 0;
    LCD = data;
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_init() {
    lcd_cmd(0x38); // 8-bit, 2-line
    lcd_cmd(0x0C); // display ON
    lcd_cmd(0x06); // entry mode
    lcd_cmd(0x01); // clear display
}

void lcd_string(const char *str){
    while(*str)
        lcd_data(*str++);
}

void lcd_print_num(unsigned int num) {
    char digits[5];
    int i = 0;

    if (num == 0) {
        lcd_data('0');
        return;
    }

    while (num > 0) {
        digits[i++] = (num % 10) + '0';   //number are stored in reverse order ,& here each numbers are in char form
        num /= 10;                        //removes the last digit 
    }

    for (int j = i - 1; j >= 0; j--) {    //loop runs to convert the reverse order char into normal result
        lcd_data(digits[j]);
    }
}

// ADC Functions
void adc_init() {
    ADCON0 = 0x41; // ADC ON, Channel 0
    ADCON1 = 0x80; // Right justified, Vref = Vdd
}

unsigned int adc_read() {
    GO_nDONE = 1;
    while (GO_nDONE);
    return ((ADRESH << 8) + ADRESL);
}

void main() {
    TRISD = 0x00; // LCD output
    TRISC = 0x00; // Control output
    TRISA0 = 1;   // RA0 input

    lcd_init();
    adc_init();

    while (1) {
        unsigned int adc_val = adc_read();
        
       // ? Calculate Voltage and Temperature
        
        float temp = adc_val * 0.488;            
        int t_int = (int)temp;                    // Integer part

        // ? Display on LCD
        lcd_cmd(0x01);                // Clear LCD
        lcd_string("Temp: ");
        lcd_print_num(t_int);
        lcd_data(0xDF);               // Degree symbol
        lcd_data('C');

        lcd_cmd(0xC0);                // Move to 2nd line
        lcd_string("Volt: ");

        // Convert voltage to show 1 decimal place
        
        float voltage;
        int whole, decimal;

        voltage = (adc_val * 5.0) / 1023.0;   // Convert ADC to voltage

        whole = (int)voltage;                   // Take integer part ? e.g. 2
        decimal = (voltage * 100) - (whole * 100); // Get two digits after decimal ? e.g. 56

        lcd_print_num(whole);                   // Print integer part
        lcd_data('.');                          // Print decimal point
        lcd_print_num(decimal);                 // Print decimal digits
        lcd_data('V');                          // Print unit


        __delay_ms(1000);
    }
}
