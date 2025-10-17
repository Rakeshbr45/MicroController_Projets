/*
 * File:   Digital_Clock.c
 * Author: Rakesh B
 *
 * Created on October 9, 2025, 12:05 PM
 */

// CONFIGURATION BITS
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#include <xc.h>
#define _XTAL_FREQ 20000000

// ---------- LCD CONNECTIONS ----------
#define RS RB0
#define RW RB1
#define EN RB2
#define LCD PORTD

// ---------- BUTTONS ----------
#define SET_BTN 1   // RA0
#define INC_BTN 2   // RA1
#define NEXT_BTN 3  // RA2
#define ALARM_BTN 4   // RA3
#define BUZZER RC0

// ---------- GLOBAL VARIABLES ----------
unsigned char sec, min, hr;
unsigned char alarm_hr = 12, alarm_min = 54;   // Default alarm time
unsigned char mode = 0; // 0 = Normal, 1 = Set Time, 2 = Set Alarm
unsigned char alarm_triggered = 0;


// ---------- BUTTON READ (Active Low, Debounced) ----------
// BUTTON READ (Active Low, Debounced)
unsigned char read_button(unsigned char button_pin) {
    if(button_pin == 1) { // SET_BTN -> RA0
        if(!(PORTA & 0x01)) {       // check RA0
            __delay_ms(50);          // debounce
            if(!(PORTA & 0x01)) {    // still pressed
                while(!(PORTA & 0x01)); // wait release
                __delay_ms(50);
                return 1;
            }
        }
    }
    else if(button_pin == 2) { // INC_BTN -> RA1
        if(!(PORTA & 0x02)) {       
            __delay_ms(50);          
            if(!(PORTA & 0x02)) {    
                while(!(PORTA & 0x02));
                __delay_ms(50);
                return 1;
            }
        }
    }
    else if(button_pin == 3) { // NEXT_BTN -> RA2
        if(!(PORTA & 0x04)) {       
            __delay_ms(50);          
            if(!(PORTA & 0x04)) {    
                while(!(PORTA & 0x04));
                __delay_ms(50);
                return 1;
            }
        }
    }
    else if(button_pin == 4) { // ALARM_BTN -> RA3
    if(!(PORTA & 0x08)) {       
        __delay_ms(50);          
        if(!(PORTA & 0x08)) {    
            while(!(PORTA & 0x08)); 
            __delay_ms(50);
            return 1;
        }
    }
}
    return 0; // not pressed
}

void lcd_cmd(char cmd) {
    RS = 0; RW = 0;
    LCD = cmd;
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_data(char data) {
    RS = 1; RW = 0;
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
    __delay_ms(2);
}

void lcd_string(const char *str) {
    while (*str) 
        lcd_data(*str++);
}

void I2C_wait_idle(void) {
    while ((SSPCON2 & 0x1F) || (SSPSTAT & 0x04));
}

void I2C_init(void) {
    SSPCON = 0x28; // enable I2C Master mode
    SSPADD = ((_XTAL_FREQ/4)/100000) - 1; // 100kHz
    SSPSTAT = 0x80;
}

void I2C_start(void) {
    I2C_wait_idle(); 
    SEN = 1;
    while (SEN);
}

void I2C_stop(void) {
    I2C_wait_idle(); 
    PEN = 1; 
    while (PEN);
}

void I2C_repeated_start(void) {
    I2C_wait_idle(); 
    RSEN = 1; 
    while (RSEN);
}

unsigned char I2C_write(unsigned char data) {
    I2C_wait_idle();
    SSPBUF = data;
    while (!SSPIF); 
    SSPIF = 0;
    return !SSPCON2bits.ACKSTAT;
}

unsigned char I2C_read(unsigned char ack) {
    I2C_wait_idle();
    RCEN = 1;
    while (!BF);
    unsigned char data = SSPBUF;
    I2C_wait_idle();
    ACKDT = (ack) ? 0 : 1;
    ACKEN = 1;
    while (ACKEN);
    return data;
}

unsigned char BCD_to_DEC(unsigned char value) {
    return ((value >> 4) * 10) + (value & 0x0F);
}

unsigned char DEC_to_BCD(unsigned char value) {
    return ((value / 10) << 4) | (value % 10);
}

void RTC_write_time(unsigned char h, unsigned char m, unsigned char s) {
    I2C_start();
    I2C_write(0xD0);
    I2C_write(0x00);
    I2C_write(DEC_to_BCD(s));
    I2C_write(DEC_to_BCD(m));
    I2C_write(DEC_to_BCD(h));
    I2C_stop();
}

void RTC_read_time(void) {
    unsigned char s, m, h;
    I2C_start();
    I2C_write(0xD0);
    I2C_write(0x00);
    I2C_repeated_start();
    
    I2C_write(0xD1);
    s = I2C_read(1);
    m = I2C_read(1);
    h = I2C_read(0);
    I2C_stop();

    sec = BCD_to_DEC(s & 0x7F);
    min = BCD_to_DEC(m);
    hr  = BCD_to_DEC(h & 0x3F);
}

void show_time_on_lcd() {
    lcd_cmd(0x80);
    lcd_string("Time: ");
    lcd_data((hr/10)+'0');
    lcd_data((hr%10)+'0');
    lcd_data(':');
    lcd_data((min/10)+'0');
    lcd_data((min%10)+'0');
    lcd_data(':');
    lcd_data((sec/10)+'0');
    lcd_data((sec%10)+'0');

    lcd_cmd(0xC0);
    lcd_string("Alarm:");
    lcd_data((alarm_hr/10)+'0');
    lcd_data((alarm_hr%10)+'0');
    lcd_data(':');
    lcd_data((alarm_min/10)+'0');
    lcd_data((alarm_min%10)+'0');
}

// ---------- Alarm Trigger ----------
void check_alarm(void) {
    if(hr == alarm_hr && min == alarm_min && sec == 0 && alarm_triggered == 0) {
        alarm_triggered = 1;
        lcd_cmd(0x01);
        lcd_string("ALARM RINGING!");
        BUZZER = 1;
        __delay_ms(3000);
        BUZZER = 0;
        lcd_cmd(0x01);
    }
    if(min != alarm_min) alarm_triggered = 0;
}


// Utility: Print 2-digit number or blank if blinking
void lcd_print_blink(unsigned char value, unsigned char blink) {
    if(blink) {
        lcd_data(' ');
        lcd_data(' ');
    } else {
        lcd_data((value/10)+'0');
        lcd_data((value%10)+'0');
    }
}


void adjust_time(void) {
    unsigned char set_hr = hr;
    unsigned char set_min = min;
    unsigned char field = 0; // 0 = hour, 1 = minute
    unsigned char blink = 0;

    lcd_cmd(0x01);
    lcd_string("Set Time Mode");
    __delay_ms(1000);

    while(1) {
        lcd_cmd(0xC0);

        // Print hour
        if(field == 0) 
            lcd_print_blink(set_hr, blink);
        else lcd_print_blink(set_hr, 0);

        lcd_data(':');

        // Print minute
        if(field == 1) 
            lcd_print_blink(set_min, blink);
        else lcd_print_blink(set_min, 0);

        blink = !blink;
        __delay_ms(500);

        if(read_button(INC_BTN)) {
            if(field == 0) set_hr = (set_hr + 1) % 24;
            else set_min = (set_min + 1) % 60;
        }
        if(read_button(NEXT_BTN)) field = !field;
        if(read_button(SET_BTN)) {
            RTC_write_time(set_hr,set_min,0);
            mode = 0;
            lcd_cmd(0x01);
            break;
        }
    }
}

void adjust_alarm(void) {
    unsigned char set_hr = alarm_hr;
    unsigned char set_min = alarm_min;
    unsigned char field = 0; // 0 = hour, 1 = minute
    unsigned char blink = 0;

    lcd_cmd(0x01);
    lcd_string("Set Alarm Mode");
    __delay_ms(1000);

    while(1) {
        lcd_cmd(0xC0);

        // Print hour
        if(field == 0) 
            lcd_print_blink(set_hr, blink);
        else lcd_print_blink(set_hr, 0);

        lcd_data(':');

        // Print minute
        if(field == 1) 
            lcd_print_blink(set_min, blink);
        else lcd_print_blink(set_min, 0);

        blink = !blink;
        __delay_ms(500);

        if(read_button(INC_BTN)) {
            if(field == 0) set_hr = (set_hr + 1) % 24;
            else set_min = (set_min + 1) % 60;
        }

        if(read_button(NEXT_BTN)) field = !field;

        if(read_button(SET_BTN)) {   // confirmation to save alarm
            alarm_hr = set_hr;
            alarm_min = set_min;
            mode = 0;
            lcd_cmd(0x01);
            break;
        }
    }
}


void main(void) {
    ADCON1 = 0x06; // disable ADC
    CMCON = 0x07;  // disable comparator

    TRISB = 0x00;  // LCD output
    TRISD = 0x00;
    TRISA = 0xFF;  // buttons input
    TRISC0 = 0;    // buzzer output

    lcd_init();
    I2C_init();

    lcd_cmd(0x80);
    lcd_string("Digital Clock");
    lcd_cmd(0xC0);
    lcd_string("With Alarm");
    __delay_ms(2000);
    lcd_cmd(0x01);

    while(1) {
    if(read_button(SET_BTN)) {
        __delay_ms(100);
        mode = 1;   // Set Time mode
    }

    if(read_button(ALARM_BTN)) {
        __delay_ms(100);
        mode = 2;   // Set Alarm mode
    }

    if(mode == 1) adjust_time();
    else if(mode == 2) adjust_alarm();
    else {
        RTC_read_time();
        show_time_on_lcd();
        check_alarm();
        __delay_ms(900);
    }
  }
}
