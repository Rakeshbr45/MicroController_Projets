/*
 * File:   RFID_PIC.c
 * Author: Rakesh B
 *
 * Created on October 5, 2025, 8:05 PM
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
#include  <string.h>
#define _XTAL_FREQ 20000000
#define baud_rate 9600
#define tag_length 12

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
}



void UART_int(){
   
    TRISC6 = 0; //R6 AS OUTPUT
    TRISC7 = 1;  //R7 AS INPUT
    
    SPBRG = ((_XTAL_FREQ/16)/baud_rate)-1;   //or it can be set to 129 refers 9600 baud rate for more observe datasheet    
    
    BRGH = 1;
    SYNC = 0;//SYNCHRONIZATION 
    SPEN = 1;
    TXEN = 1;
    CREN = 1;
}

//send character
void uart_send_char(char a){
    while(!TXIF);  //transistor interrupt flag
                   //TXIF = 1 ? Buffer is empty ? ready to send.
                   //TXIF = 0 ? Buffer is full ? must wait.
    TXREG = a;
}

//SEND string
void lcd_string(const char *str){
    while(*str)
    lcd_data(*str++);
}

void uart_send_string(const char *str){
    while(*str)
    uart_send_char(*str++);
}
//receive character
char uart_get(){
      if(OERR){    //overrun error reset
        CREN = 0;  //TO CLEAR RESET
        CREN = 1;  // to enable reset
    }
    while(!RCIF);
    return RCREG; //TO SEND    
}

void main(void) {
    TRISB = 0x00; //CONTROL SIGNALS
    TRISD = 0x00;
    
    char TAG[tag_length + 1];  //+1 for null terminator 
    const char valid_tag[]= "123412341234"; //authorized ID 
    int i =0;
    
    lcd_init();
    UART_int();
    lcd_cmd(0x01); // Clear display
    
    
    lcd_string("UART Initialize"); // r = moves cursor to the beginning of line
                                             //n = new line
    __delay_ms(2000);
    lcd_cmd(0x01); 
    lcd_string("Scan RF ID....\r\n");
    
    while(1){
        
        //read 12-byte RFID tag
        for(i=0;i<tag_length;i++){
            TAG[i] = uart_get();
        }
        TAG[i] = '\0';   // Null terminate
        
       // Display on LCD
        lcd_cmd(0x01); // Clear display
        lcd_string("Tag ID:");
        lcd_cmd(0xC0);
        for(i=0;i<tag_length;i++){ 
            lcd_data(TAG[i]); 
        }
        __delay_ms(2000);
        
        //compare with valid id 
        
        lcd_cmd(0x01);
        if(strcmp(TAG ,valid_tag) == 0){
            lcd_string("Access Granted");
            uart_send_string("\r\n Access Granted \r\n ");
        } else{
            lcd_string("Access Denied");
            uart_send_string("\r\n Access Denied");
        }
       
        __delay_ms(2000);
    lcd_cmd(0x01);
    lcd_string("Next SCAN ID...");
   }
}