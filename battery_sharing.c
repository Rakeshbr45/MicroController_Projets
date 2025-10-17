/*
 * File:   battery_sharing.c
 * Author: Rakesh B
 *
 * Created on October 14, 2025, 12:29 PM
 */

// PIC16F877A Configuration Bit Settings

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Disable
#pragma config CPD = OFF        // Data EEPROM Code Protection off
#pragma config WRT = OFF        // Flash Program Memory Write Protection off
#pragma config CP = OFF         // Flash Program Memory Code Protection off



#include <xc.h>
#define _XTAL_FREQ 20000000

// LCD control pins
#define RS RB0
#define RW RB1
#define EN RB2
#define LCD PORTD

//Battery Threshold
#define Full_Volt 138   //13.8v x 10 
#define Low_Volt 122    //12.2 x 10
// ---------------- LCD FUNCTIONS ----------------
void lcd_cmd(unsigned char cmd){
    LCD = cmd;
    RS = 0; 
    RW = 0; 
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_data(unsigned char data){
    LCD = data;
    RS = 1; 
    RW = 0; 
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_print_string(char *str){
    while(*str){
        lcd_data(*str++);
    }
}

void lcd_init(){
    lcd_cmd(0x38); // 8-bit, 2-line
    lcd_cmd(0x06); // Increment cursor
    lcd_cmd(0x0C); // Display on, cursor off
    lcd_cmd(0x01); // Clear screen
    __delay_ms(10);
}

//Battery Reading Function

unsigned int read_battery(unsigned char channel){
    
    ADCON0 = (channel << 2) | 0x01; //select ADC channel bit 2-5((channel - 0,1,2,3 (AN0 - AN03)  and bit 0 - ADON =1 turn ON ADC
    __delay_ms(10);
    GO_nDONE = 1;
    while(GO_nDONE);
    
    unsigned int adc_val = ((ADRESH << 8) | ADRESL);
    unsigned int volt  = (adc_val *50UL /1023) +100; 
    if(volt > 255) volt = 255;   // limit to 25.5v max
    //ad_val =   it hold the 10-bit ADC values(0-1023)
    //50ul (integer math ) - 50 unsigned long (multiplication done in 32-bit ))
    //1023 divide by max ADC value(1023) to normalize 
    //+100 is a offset an adjustment to match real battery voltage which adds 10.0v (+100 = 10v)
    //ADC reads 512 ? (512*50/1023)+100 = 125 ? represents 12.5V
    return  volt;
}

void main(void) {
    TRISC = 0XF0;     // For battery
    TRISB = 0X00;
    TRISD = 0X00;     //LCD PORT
    TRISA = 0XFF;     //AN0 - AN3 AS INPUT
    ADCON1 = 0X06;    //AN0 - AN3 ANALOG,REST DIGITAL
    ADCON0 = 0X00;    //ADC OFF INITIALLY
    
    lcd_init();
    lcd_print_string("Charge Link of 4");
    __delay_ms(1000);
    lcd_cmd(0x01);
    
    while(1){
        // Read battery voltages
        unsigned char bat0 = read_battery(0);
        unsigned char bat1 = read_battery(1);
        unsigned char bat2 = read_battery(2);
        unsigned char bat3 = read_battery(3);
        
        unsigned char charging_bat = 0;
        
        // Determine which battery to charge
        if(bat0 < Full_Volt && bat0 < bat1 && bat0 < bat2 && bat0 < bat3) 
            charging_bat = 1;
        else if(bat1 < Full_Volt && bat1 < bat0 && bat1 < bat2 && bat1 < bat3) 
            charging_bat = 2;
        else if(bat2 < Full_Volt && bat2 < bat0 && bat2 < bat1 && bat2 < bat3) 
            charging_bat = 3;
        else if(bat3 < Full_Volt && bat3 < bat0 && bat3 < bat1 && bat3 < bat2) 
            charging_bat = 4;
        
        // Switch relays and print charging status on line 1
        RC0 = RC1 = RC2 = RC3 = 0; // Turn off all relays
        lcd_cmd(0x80); // Line 1
        __delay_ms(20);
        
        if(charging_bat != 0){
            switch(charging_bat){
                case 1: RC0 = 1; lcd_print_string("Charging B1   "); break;
                case 2: RC1 = 1; lcd_print_string("Charging B2   "); break;
                case 3: RC2 = 1; lcd_print_string("Charging B3   "); break;
                case 4: RC3 = 1; lcd_print_string("Charging B4   "); break;
                __delay_ms(1000);
            }
        } else {
            lcd_print_string("All 4  Bat Full");
            __delay_ms(500);
        }
        
        // Print battery voltages on line 2
        
        lcd_cmd(0x01);
        lcd_cmd(0x80); // Line 2
        lcd_print_string("B1:");
        lcd_data((bat0/10)+'0');
        lcd_data('.');
        lcd_data((bat0%10)+'0');
        
        
        lcd_print_string(" B2:");
        lcd_data((bat1/10)+'0');
        lcd_data('.');
        lcd_data((bat1%10)+'0');
        
        lcd_cmd(0xC0);
        lcd_print_string("B3:");
        lcd_data((bat2/10)+'0');
        lcd_data('.');
        lcd_data((bat2%10)+'0');
        
        
        lcd_print_string(" B4:");
        lcd_data((bat3/10)+'0');
        lcd_data('.');
        lcd_data((bat3%10)+'0');
        
        __delay_ms(5000); // Update rate
        lcd_cmd(0x01);
    }
    return;
}
   
