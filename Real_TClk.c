/*
 * File:   Real_TClk.c
 * Author: Rakesh B
 *
 * Created on October 6, 2025, 7:07 PM
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
#define RS RB0
#define RW RB1
#define EN RB2

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
    __delay_ms(2);

}

void lcd_string(const char *str){
    while(*str)
        lcd_data(*str++);
}

void I2C_wait_idle(void){
    // Wait until all I2C conditions done and R/W is idle
    while ((SSPCON2 & 0x1F) || (SSPSTAT & 0x04)); // SEN, RSEN, PEN, RCEN, ACKEN or R/W active
    
}

void I2C_init(){
    SSPCON = 0X28; //0010 1000 ENABLE I2C ,MASTER MODE
    SSPADD = ((_XTAL_FREQ/4)/100000)-1; // BAUD RATE GENERATOR 100khz
    SSPSTAT = 0x80; //SMP =1  (slew rate disable  STANDARD SPEED)
    SSPIF =0;
    TRISC3 = 1;
    TRISC4 = 1;
}

void I2C_start(){
    I2C_wait_idle();
    SEN =1;        //SEND START CONDITION
    while(SEN);    //AUTO CLEAR WHEN DONE
}

void I2C_repeated_start(void){
    I2C_wait_idle();
    RSEN = 1;
    while (RSEN);
}

void I2C_stop(){
    I2C_wait_idle();
    PEN =1;       //STOP CONDITION
    while(PEN);
}


unsigned char  I2C_write(unsigned char data){
    I2C_wait_idle();
    SSPBUF = data ;   //load data into buffer
    while(!SSPIF);
    SSPIF = 0;        //ACK received
    if(SSPCON2bits.ACKSTAT){   //NACK received
        return 0;          //signal failure
    }
    return 1;              //success
}    

unsigned char I2C_read(unsigned char ack){
    I2C_wait_idle();
    RCEN = 1;           //enable receiver
    while (!BF);
    unsigned char data = SSPBUF;  //read it
    I2C_wait_idle(); 
    ACKDT = (ack)?0:1; // 0=ACK, 1=NACK
    ACKEN  = 1;      //send acknowledge
    while(ACKEN);    //wait for enable complete
    return data;
}

unsigned char BCD_to_DEC(unsigned char value){
    return((value>>4)*10 + (value & 0x0F));
}

//unsigned char DEC_to_BCD(unsigned char value){
//    return ((value/10) << 4) | (value % 10);
//}


void RTC_start() {
    unsigned char sec;
    I2C_start();
    I2C_write(0xD0);      // Write mode
    I2C_write(0x00);      // Seconds register
    I2C_repeated_start();
    I2C_write(0xD1);      // Read mode
    sec = I2C_read(0);    // Read seconds
    I2C_stop();

    // Clear CH (bit7) to start clock
    sec &= 0x7F;          

    I2C_start();
    I2C_write(0xD0);      // Write mode
    I2C_write(0x00);      // Seconds register
    I2C_write(sec);       // Write updated seconds
    I2C_stop();
}
        
void RTC_read(unsigned char *sec,unsigned char *min,unsigned char *hrs,
        unsigned char *date,unsigned char *month,unsigned char *year){
 
    I2C_start();
    I2C_write(0xD0);   //DS1307 0xD0 =address + write mode
    I2C_write(0x00);   //start from register 00h(seconds)
    
    I2C_repeated_start();
    I2C_write(0xD1);   //DS1307  0xD1 = address + read mode
    
    // Registers: 00 sec, 01 min, 02 hr, 03 day, 04 date, 05 month, 06 year
    unsigned char s  = I2C_read(1);  // ACK, more to come
    unsigned char m  = I2C_read(1);
    unsigned char h  = I2C_read(1);
    I2C_read(1);              // day, discard or store if needed
    unsigned char dt = I2C_read(1);
    unsigned char mo = I2C_read(1);
    unsigned char yr = I2C_read(0);  // NACK last
    I2C_stop();

    // Convert BCD to DEC
    *sec   = BCD_to_DEC(s & 0x7F);   // mask CH
    *min   = BCD_to_DEC(m);
    *hrs =   BCD_to_DEC(h & 0x3F);
    *date  = BCD_to_DEC(dt);
    *month = BCD_to_DEC(mo);
    *year  = BCD_to_DEC(yr);
}
void main(void) {
    
   // Make analog pins digital (important!)
    ADCON1 = 0x07;  // All PORTA/portr analog pins -> digital
    
    TRISB = 0X00; //CONTROL SIGNALS
    TRISD = 0X00; //DATA

    unsigned char sec,min,hrs,date,month,year; //variables to hold time & date
    
    lcd_init();
    I2C_init();
    RTC_start();
    
    lcd_cmd(0x01);
    lcd_string("DS1307 RTC Demo:");
    __delay_ms(2000);
    lcd_cmd(0x01);
   
    while(1){
        RTC_read(&sec,&min,&hrs,&date,&month,&year);
        
        lcd_cmd(0x80); // 1st row
        lcd_string("Time:");
        
        lcd_data((hrs/10) + '0');
        lcd_data((hrs%10) + '0');
        lcd_data(':');
        
        lcd_data((min/10) + '0');
        lcd_data((min%10) + '0');
        lcd_data(':');
        
        lcd_data((sec/10) + '0');
        lcd_data((sec%10) + '0');
        
        lcd_cmd(0xC0); //second row
        lcd_string("Date:");
        
        lcd_data((date/10) + '0');lcd_data((date%10) + '0');
        lcd_data('/');
        
        lcd_data((month/10) + '0');lcd_data((month%10) + '0');
        lcd_data('/');
        
        lcd_data((year/10) + '0');lcd_data((year%10) + '0');
        
        __delay_ms(1000);
    }
}
