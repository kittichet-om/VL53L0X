/*
 * GccApplication2.c
 *
 * Created: 4/9/2020 1:20:44 PM
 * Author : ADMIN
 
*/
#if F_CPU
#define  F_CPU 16000000UL       //�ç������˹���Ҥ������� CPU �ͧ ATmega328p ������������������� project
#endif 
#if defined LONG_RANGE
setVcselPulsePeriod(VcselPeriodPreRange,18);    //���ǹ�ͧ�ç�����繡���������ŷ������Ǣ�ͧ�Ѻ�ػ�ó� 㹷�������˹���������仵�� Datasheet �ͧ�ػ�ó� VL53L0X��Ѻ
setVcselPulsePeriod(VcselPeriodFinalRange,14);
setSignalRateLimit(0.1);


#endif


#include <util/delay.h> //㹡�÷�������ػ�ó�����ö�ӧҹ������ͧ��èе�ͧ�ա�����¡�� Libraly �������Ǣͧ�Ѻ����Ѵ��Ңͧ�ػ�ó�����еç�������� set �Ңͧ�ػ�ó� 
#include <avr/io.h>     //������ SCL ��������ç�Ѻ�Ѻ �� PORTC 5 ���������� SDA ���ç�Ѻ �� PORTC4 ����ػ�ó��ǹ����ա����������觼�ҹ��������ѡɳ� I2C ���� 
#include <math.h>     //������������繡���觼�ҹ���������������� 2 ��� ��� �Ң����ŷ��� analog(SDA) �Ѻ ���ѭ�ҳ clock(SCL) ���㹷���������� Board (ATmega328p)
#include <stdlib.h>   //�繵�� master ��͵����觡����ѡ ����� slave address ����	�����Ţ���˹� �ͧ sensor 㹷���� set ���˹��·����˹觷����㹡�� ��¹��� 0x52 ��� 0x53
#include "i2c_master.h" //�繵��˹觢ͧ�����ҹ������ (����Ѻ Fast mode) ����ػ�ó׵�ǹ��������� clock ���������٧ (400 kHz)   
#define  VL53L0X_WRITE 0x52
#define  VL53L0X_READ  0x53
#include "VL53L0X.h"
#include <stdio.h>
#include "millis.h" //����Ѻ�ػ�ó��ǹ����Ҥ����˹��� ��������� �֧���繵�ͧ�ա�����¡ libraly ����Ѻ��û�Ѻ����� ˹��� ��������� ��੾��
#include <util/delay.h>
#define  PIN_I2C_SCL PC5
#define PIN_I2C_SDA PC4
#define PIN_UART_TX PD1
#include <avr/interrupt.h> //����Ѻ�ػ�ó��ǹ����ա������¹��Ң������Ѻ��觢ͧ�������ҡѺ�ػ��� �֧��ͧ�ա�����¡ system libraly ����ժ������ interrupt 

void init_usart(unsigned long baud){
	unsigned int ubrr;
	//Set baud rate, baud=Fosc/(16*(UBRR+1))                    //���ǹ�ͧ part �����繡�����ҧ Function �ͧ��� set up ���������鹢ͧ��ҡ���觼�ҹ�����������ʴ����͡�ҧ
	//Set baud rate, baud=Fosc/(8*(UBRR+1)) (2X mode)           //terminal window Ẻ�ӧҹ��� clock ��������觢�����Ẻ���������� i2c device
	ubrr=(unsigned int)(F_CPU/8/baud)-1;                        // �ͧ�Ѻ������͡��������ѡɳй��
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
	

void usart_putc(unsigned char c){  //㹡�÷����Ҩ�����ʴ����͡�繤�� ASCII �е�ͧ�ա�����ҧ�ѧ���蹷���红����������觤��� Ẻ USART 
	while(!(UCSR0A & (1<< UDRE0)));//��������������ҷ���� INput ����˹���红��������� ����� while �����繡�� check ʶҹТͧ serial ����բ���������������ѧ
	UDR0 = c;//�红����� �����¡ UDR0 ����繵�� input 㹡�÷�˹�ҷ���� data ���Ѻ�Ҩҡ��� ����� c ����˹������ ����������㹢�Ҵ�������дѺ 8 bits(unsigned char)
}
void usart_puts(char *s){    //���ҧ function 㹡���觢��������ҷ�����Ҩҡ character ������ٻ�ͧ address �֧��ͧ�� pointer �繵�Ǻ͡���˹� �ѧࡵ���ҡ������͡�ѹ
	while(*s){             //���Ѻ����÷���˹���� 㹷�����˹��繵�� s ��ж�Ҩ����¡���������ٻ�ͧ Value ��� &����ҧ˹�ҵ���ù�� ���㹿ѧ���蹹����繡���觼�ҹ������Ẻ ǹ�ٻ(while)
		usart_putc(*s);
		s++;
	}
}
void init(void){                                         //���ҧ Function ����ժ������ init ��͡�� function �ͧ��� set up ��ҷ������������Ǣ�ͧ�Ѻ���������â����� �¨���
	init_usart(115200);                           //���ǹ�ͧ USART ����˹�����Ǻ����觢������͡���ҧ���� �����¹�������������� Register �������� Port D �繵���纤����
	DDRD = (1<<PIN_UART_TX);                      //�ѧ����� ������͡�����ż�ҹ port c ��觵�� PORTC ���ա�����͡��� 2 ��Ǥ�� SDA �Ѻ SCL 
	PORTC = (1<<PIN_I2C_SCL) | (1<<PIN_I2C_SDA);
	i2c_init();  //���¡ Function �������鹡����ҹ�ͧ I2C �ҡ Libraly ����ժ������ i2c_master.h
	initMillis(); // ��ա� �ѧ���蹵��������鹡����ҹ�ͧ�ź��������ժ������ milli.h 
	sei(); //interrupt �ء����ǧ��
}
void debug_dec( uint32_t val ){ //���ҧ Function 㹡���ŧ��Ҩҡ��� string ������Ţ�ҹ 10 �����ҧ array�ͧ�Ѻ��ҷ����� 10 ���ҧ �¨��ա���ŧ���Ţ�ҹ 10 ����Ẻǹ�ٻ
	char buffer[10];
	char *p = buffer;          //㹡�äԴ���ѧ����ŧ��Ҩ����ٵôѧ������
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
	statInfo_t xTrastats;                     //����͡�˹���� parameter ŧ� Function���������ҧ��������� ���仨��繡�����¡�� Function���������ҧ��������㹵�� Function ��ѡ
	init();                                 // ���Ф�ҷ���ʴ��͡�Ҩҡ��鹡Ѻ��� Function ��ѡ (int main()) �¨����¡��ҷ������������� libraly ���������ػ�ó� ��Ẻ�ѡɳ�
                                                  // typedef ����ժ������ statInfo_t �¨л�С�ȵ�����������ժ������ xTrastats ���¡ Function ����ժ������ init,initVL53L0X
	usart_puts("\n\n=----------------\n");   //�·�� initVL53L0X �繿ѧ���� �����㹡�����͡mode�����ҹ �ػ�ó� ����� 1 ��͡�����͡��ҹ�ػ�ó� Ẻ 2V8 
	usart_puts("Hello world  VL53L0X\n");   //�ʴ���Ẻ usart string �͡����� Hello world VL5L0X ��С�˹���� timingBuget � datasheet �� 500*1000UL
	usart_putc('\n');
	initVL53L0X(1);
	setMeasurementTimingBudget(500 * 1000UL);
	
	
	
    /* Replace with your application code */
    while (1){                                        //����ػ�ó� sensor ��ҹ���Ẻ������ͧ ���� mode 㹡���ѴẺ single �������Ѵ��� loop ��˹�觤����ҹ��
		readRangeSingleMillimeters(&xTrastats);    //���ʴ��͡�� distance : decimal number mm ��� reset�������ͨ�� loop �������� return 0 ��
		usart_puts("\ndistance = ");      //ʶҹз��觺͡��Ҩ���÷ӧҹ��ͺ����  �����ػ�ó��ǹ����Ѵ������ 30-400 mm ������͡��ҹẺSinglemillimeter
		debug_dec(xTrastats.rawDistance);
		usart_puts("\mm \n");
		
		
	}
    return 0;
}


