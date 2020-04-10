/*
 * GccApplication2.c
 *
 * Created: 4/9/2020 1:20:44 PM
 * Author : ADMIN
 
*/
#if F_CPU
#define  F_CPU 16000000UL       //ตรงนี้ผมกำหนดค่าความเร็ว CPU ของ ATmega328p ให้ทั้งหมดที่อยู่ในไฟล์ project
#endif 
#if defined LONG_RANGE
setVcselPulsePeriod(VcselPeriodPreRange,18);    //ในส่วนของตรงนี้จะเป็นการใส่ข้อมูลที่เกี่ยวข้องกับอุปกรณ์ ในที่นี้ผมกำหนดค่าให้เป็นไปตาม Datasheet ของอุปกรณ์ VL53L0Xครับ
setVcselPulsePeriod(VcselPeriodFinalRange,14);
setSignalRateLimit(0.1);


#endif


#include <util/delay.h> //ในการที่จะให้อุปกรณ์สามารถทำงานได้ตามต้องการจะต้องมีการเรียกใช้ Libraly ที่เกี่ยวของกับการวัดค่าของอุปกรณ์นั้นและตรงนี้ผมก็ได้ set ขาของอุปกรณ์ 
#include <avr/io.h>     //โดยให้ขา SCL เชื่อมให้ตรงกับกับ ขา PORTC 5 และเชื่อมขา SDA ให้ตรงกับ ขา PORTC4 โดยในอุปกรณ์ตัวนี้จะมีการสื่อสารส่งผ่านข้อมูลในลักษณะ I2C หรือ 
#include <math.h>     //แปลเป็นไทยได้ว่าเป็นการส่งผ่านข้อมูลโดยใช้แค่สายไฟ 2 เส้น คือ ขาข้อมูลทีเป็น analog(SDA) กับ ขาสัญญาณ clock(SCL) และในที่นี้จะให้ตัว Board (ATmega328p)
#include <stdlib.h>   //เป็นตัว master คือตัวสั่งการหลัก และใช้ slave address หรือ	หมายเลขตำแหน่ง ของ sensor ในที่นี้ set ตำแหน่งโดยที่ตำแหน่งที่ใช้ในการ เขียนคือ 0x52 และ 0x53
#include "i2c_master.h" //เป็นตำแหน่งของการอ่านข้อมูล (สำหรับ Fast mode) โดยในอุปกรณืตัวนี้จะใช้โหมด clock ความเร็วสูง (400 kHz)   
#define  VL53L0X_WRITE 0x52
#define  VL53L0X_READ  0x53
#include "VL53L0X.h"
#include <stdio.h>
#include "millis.h" //สำหรับอุปกรณ์ตัวนี้อ่าค่าเป็นหน่วย มิลลิเมตร จึงจำเป็นต้องมีการเรียก libraly สำหรับการปรับให้เป็น หน่วย มิลลิเมตร โดยเฉพาะ
#include <util/delay.h>
#define  PIN_I2C_SCL PC5
#define PIN_I2C_SDA PC4
#define PIN_UART_TX PD1
#include <avr/interrupt.h> //สำหรับอุปกรณ์ตัวนี้จะมีการเปลี่ยนค่าขึ้นอยู่กับสิ่งของที่เข้ามากับอุปกณ์ จึงต้องมีการเรียก system libraly ที่มีชื่อว่า interrupt 

void init_usart(unsigned long baud){
	unsigned int ubrr;
	//Set baud rate, baud=Fosc/(16*(UBRR+1))                    //ในส่วนของ part นี้จะเป็นการสร้าง Function ของการ set up ค่าเริ่มต้นของค่าการส่งผ่านข้อมูลแล้วแสดงผลออกทาง
	//Set baud rate, baud=Fosc/(8*(UBRR+1)) (2X mode)           //terminal window แบบทำงานตาม clock ที่ใช้การส่งข้อมูลแบบนี้เพราะว่า i2c device
	ubrr=(unsigned int)(F_CPU/8/baud)-1;                        // รองรับการส่งออกข้อมูลในลักษณะนี้
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	//Double the USART Transmission Speed
	//to reduce speed error
	UCSR0A = (1<<U2X0);
	// Enable transmitter and receiver.
	UCSR0B = (1<<TXEN0)|(1<<RXEN0);
	//Set frame to 8data, 2stop bit
	UCSR0C = (1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
}
	

void usart_putc(unsigned char c){  //ในการที่เราจะให้แสดงผลออกเป็นค่า ASCII จะต้องมีการสร้างฟังก์ชั่นที่เก็บข้อมูลแล้วส่งค่าไป แบบ USART 
	while(!(UCSR0A & (1<< UDRE0)));//รอให้ข้อมูลเข้ามาที่ตัว INput ที่ทำหน้าเก็บข้อมูลเข้าไป และใช้ while เพื่อเป็นการ check สถานะของ serial ว่ามีข้อมูลเข้ามาหรือยัง
	UDR0 = c;//เก็บข้อมูล โดยเรียก UDR0 ที่เป็นตัว input ในการทำหน้าที่เก็บ data โดยรับมาจากค่า ตัวแปร c ที่กำหนดขึ้นมา ซึ่งให้อยู๋ในขนาดข้อมูลระดับ 8 bits(unsigned char)
}
void usart_puts(char *s){    //สร้าง function ในการส่งข้อมูลแต่ค่าที่ได้มาจาก character อยู่ในรูปของ address จึงต้องใช้ pointer เป็นตัวบอกตำแหน่ง สังเกตุได้จากการใส่ดอกจัน
	while(*s){             //ไว้กับตัวแปรที่กำหนดไว้ ในที่นี้กำหนดเป็นตัว s และถ้าจะเรียกให้อยู่ในรูปของ Value ใส่ &ไว้ข้างหน้าตัวแปรนั้น ซึ่งในฟังก์ชั่นนี้จะเป็นการส่งผ่านข้อมูลแบบ วนลูป(while)
		usart_putc(*s);
		s++;
	}
}
void init(void){                                         //สร้าง Function ที่มีชื่อว่า init คือการ function ของการ set up ค่าทั้งหมดที่เกี่ยวข้องกับการสื่อสารข้อมูล โดยจะมี
	init_usart(115200);                           //ในส่วนของ USART ที่กำหนดให้ตัวบอร์ดส่งข้อมูลออกอย่างเดียว ซึ่งเขียนคำสั่งแล้วให้ตัว Register ที่อยู่ใน Port D เป็นตัวเก็บคำสั่ง
	DDRD = (1<<PIN_UART_TX);                      //ดังกล่าว และส่งออกข้อมูลผ่าน port c ซึ่งตัว PORTC จะมีการส่งออกค่า 2 ตัวคือ SDA กับ SCL 
	PORTC = (1<<PIN_I2C_SCL) | (1<<PIN_I2C_SDA);
	i2c_init();  //เรียก Function ตัวเริ่ต้นการใช้งานของ I2C จาก Libraly ที่มีชื่อว่า i2c_master.h
	initMillis(); // เรีกก ฟังก์ชั่นตัวเริ่มต้นการใช้งานของไลบราลี่ที่มีชื่อว่า milli.h 
	sei(); //interrupt ทุกๆค่าในวงจร
}
void debug_dec( uint32_t val ){ //สร้าง Function ในการแปลงค่าจากตัว string ให้เป็นเลขฐาน 10 โดยสร้าง arrayรองรับค่าทั้งหมด 10 อย่าง โดยจะมีการแปลงเป็นเลขฐาน 10 อยู่แบบวนลูป
	char buffer[10];
	char *p = buffer;          //ในการคิดการังที่แปลงค่าจะใช้สูตรดังที่เห็น
	while (val || p == buffer) {
		*(p++) = val % 10;
		val = val / 10;
	}
	while (p != buffer) {
		usart_putc( '0' + *(--p) );
	}
}
int main(void)  
{
	statInfo_t xTrastats;                     //เมื่อกำหนดค่า parameter ลงใน Functionที่เราสร้างขึ้นมาแล้ว ต่อไปจะเป็นการเรียกใช้ Functionที่เราสร้างขึ้นมาไว้ในตัว Function หลัก
	init();                                 // เพราะค่าที่แสดงออกมาจากขึ้นกับตัว Function หลัก (int main()) โดยจะเรียกค่าทั้งหมดที่อยู่ใน libraly ที่อยู่ในอุปกรณ์ เป็นแบบลักษณะ
                                                  // typedef ที่มีชื่อว่า statInfo_t โดยจะประกาศตัวแปรใหม่ที่มีชื่อว่า xTrastats เรียก Function ที่มีชื่อว่า init,initVL53L0X
	usart_puts("\n\n=----------------\n");   //โดยที่ initVL53L0X เป็นฟังก์ชั่น ที่ใช้ในการเลือกmodeการใช้งาน อุปกรณ์ โดยใส่ 1 คือการเลือกใช้งานอุปกรณ์ แบบ 2V8 
	usart_puts("Hello world  VL53L0X\n");   //แสดงผลแบบ usart string ออกค่าเป็น Hello world VL5L0X และกำหนดค่า timingBuget ใน datasheet เป็น 500*1000UL
	usart_putc('\n');
	initVL53L0X(1);
	setMeasurementTimingBudget(500 * 1000UL);
	
	
	
    /* Replace with your application code */
    while (1){                                        //ให้อุปกรณ์ sensor อ่านค่าแบบต่อเนื่อง โดยใช้ mode ในการวัดแบบ single คือให้วัดค่า loop ละหนึ่งค่าเท่านั้น
		readRangeSingleMillimeters(&xTrastats);    //โดยแสดงออกเป็น distance : decimal number mm และ resetค่าเมื่อจบใน loop นั้นโดยใส่ return 0 เป็น
		usart_puts("\ndistance = ");      //สถานะที่บ่งบอกว่าจบการทำงานในรอบนั้นๆ  และในอุปกรณ์ตัวนี้จะวัดได้ตั้งแต่ 30-400 mm ถ้าเลือกใช้งานแบบSinglemillimeter
		debug_dec(xTrastats.rawDistance);
		usart_puts("\mm \n");
		
		
	}
    return 0;
}


