/**
 * Project : RFID based Security system
 */



#include <asf.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


//USART Init for RFID Reader
#define USART_SERIAL_RFID                &USARTC0
#define USART_SERIAL_BAUDRATE_RFID       9600
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

//USART Init for Serial Moniter
#define USART_SERIAL_Monitor			&USARTE0
#define USART_SERIAL_BAUDRATE_Monitor	38400


static usart_rs232_options_t usart_options_RFID =
{
	.baudrate = USART_SERIAL_BAUDRATE_RFID,
	.charlength = USART_SERIAL_CHAR_LENGTH,
	.paritytype = USART_SERIAL_PARITY,
	.stopbits = USART_SERIAL_STOP_BIT
};

static usart_rs232_options_t usart_options_Monitor =
{
	.baudrate = USART_SERIAL_BAUDRATE_Monitor,
	.charlength = USART_SERIAL_CHAR_LENGTH,
	.paritytype = USART_SERIAL_PARITY,
	.stopbits = USART_SERIAL_STOP_BIT
};

//Variable Definition
uint8_t card_no;
uint32_t rtc_timestamp;
struct calendar_date  get_date, date={00,10,19,9,11,2014,3};


// Function Prototypes
void LCD_Out_Dec(uint32_t,int);
int Receive(void);
int Check(void);
void set_current_RTC(void);
char *display_time(struct calendar_date,char *);
void USART_TransmitString(char *str);
void USART_TransmitNumber(unsigned long n);


uint8_t code[11];
char arr[25];
char names[5][20]=	{"Riken","Milan","Rajat","Ruchit"};
uint8_t Original_data[4][10]=	{{0x36,0x35,0x30,0x30,0x32,0x34,0x32,0x44,0x31,0x44},
								 {0x36,0x35,0x30,0x30,0x32,0x34,0x34,0x37,0x34,0x43},
								 {0x36,0x35,0x30,0x30,0x32,0x34,0x33,0x36,0x30,0x42},
								 {0x36,0x35,0x30,0x30,0x32,0x34,0x35,0x37,0x35,0x44}};
// 								 {0x36,0x35,0x30,0x30,0x33,0x33,0x42,0x39,0x32,0x45}};		//Un-authorized card



int main (void)
{
	board_init();	//Board definition and selection
	sysclk_init();	//System clock init
	
	usart_init_rs232(USART_SERIAL_RFID, &usart_options_RFID);	//UART init
	usart_init_rs232(USART_SERIAL_Monitor, &usart_options_Monitor);
	
	gfx_mono_init();	//LCD init
	PORTE.OUTSET=PIN4_bm;	//LCD Back light on
	
	//RTC Init
	sysclk_enable_module(SYSCLK_PORT_GEN, SYSCLK_RTC);
	while (RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm);
	if (rtc_vbat_system_check(false) != VBAT_STATUS_OK)
		rtc_init();
	PORTE.DIRCLR=PIN5_bm;
	
	while(1)
	{
		if(Receive())
		{
			card_no=Check();
			if(card_no)
			{
				PORTR.OUTCLR=PIN0_bm;
				gfx_mono_draw_string("Card Detected",0,0,&sysfont);
				gfx_mono_draw_string("Welcome",0,10,&sysfont);
				gfx_mono_draw_string(names[card_no-1],55,10,&sysfont);
				rtc_timestamp=rtc_get_time();
				calendar_timestamp_to_date_tz(rtc_timestamp,5,30,&get_date);
				gfx_mono_draw_string(display_time(get_date,arr),0,20,&sysfont);
				delay_s(1);
				gfx_mono_init();
				PORTR.OUTSET=PIN0_bm;
			}
			else
			{
				PORTR.OUTCLR=PIN1_bm;
				gfx_mono_draw_string("Invalid Card",0,0,&sysfont);
				delay_s(1);
				gfx_mono_init();
				PORTR.OUTSET=PIN1_bm;
			}
		}
	}
}


void LCD_Out_Dec(uint32_t n,int y)
{
	int a=log10(n);
	char ab[a+2];
	ab[a+1]='\0';
	for(;a>=0;a--)
	{
		ab[a]=(n%10)+'0';
		n/=10;
	}
	gfx_mono_draw_string(ab,0,y,&sysfont);
}


int Receive()
{
	while(usart_getchar(USART_SERIAL_RFID)!=0x0A);
	int i=0;
	while(i<10)
	{
		code[i++]=usart_getchar(USART_SERIAL_RFID);
	}
	return(usart_getchar(USART_SERIAL_RFID)==0x0D);
}


int Check()
{
	uint8_t flag=true;
	for(uint8_t i=0;i<4;i++)
	{
		flag=true;
		for(uint8_t j=0;j<10;j++)
		{
			if(code[j]!=Original_data[i][j])
			{
				flag=false;
				break;
			}
		}
		if(flag)
			return i+1;
	}
	return flag;
}


char *display_time(struct calendar_date st,char *arr)
{
	int temp;
	arr[0]=(st.hour/10)+'0';
	arr[1]=(st.hour%10)+'0';
	arr[2]=':';
	arr[3]=(st.minute/10)+'0';
	arr[4]=(st.minute%10)+'0';
	arr[5]=':';
	arr[6]=(st.second/10)+'0';
	arr[7]=(st.second%10)+'0';
	arr[8]=' ';
	temp=st.date+1;
	arr[9]=(temp/10)+'0';
	arr[10]=(temp%10)+'0';
	arr[11]='/';
	temp=st.month+1;
	arr[12]=(temp/10)+'0';
	arr[13]=(temp%10)+'0';
	arr[14]='/';
	temp=st.year;
	arr[18]=(temp%10)+'0';
	temp/=10;
	arr[17]=(temp%10)+'0';
	temp/=10;
	arr[16]=(temp%10)+'0';
	temp/=10;
	arr[15]=(temp%10)+'0';
	arr[19]='\0';
	return arr;
}


void set_current_RTC(void)
{
	rtc_timestamp=calendar_date_to_timestamp_tz(&date,5,30);
	rtc_set_time(rtc_timestamp);
	while(1);
}

void USART_TransmitString(char *str)
{
	while(*str)
		usart_putchar(USART_SERIAL_Monitor,(*str)++);
}

void USART_TransmitNumber(unsigned long n)
{
	if(n >= 10)
	{
		USART_TransmitNumber(n/10);
		n = n%10;
	}
	usart_putchar(USART_SERIAL_Monitor,n+'0');	
}