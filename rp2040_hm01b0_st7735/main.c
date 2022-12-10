
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// include <tusb.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "arducam/arducam.h"
#include "lib/st7735.h"
#include "lib/fonts.h"
#include "hardware/adc.h"
#include "tusb.h"


uint8_t image_buf[324*324];
uint8_t displayBuf[80*160*2];
uint8_t header[2] = {0x55,0xAA};

#define FLAG_VALUE 123
#define BUZZER_CTR_PIN 27
#define WIFI_ENABLE_PIN 26
#define TIMEOUT_S 2


void core1_entry() {

	multicore_fifo_push_blocking(FLAG_VALUE);

	uint32_t g = multicore_fifo_pop_blocking();

	if (g != FLAG_VALUE)
		printf("Hmm, that's not right on core 1!\n");
	else
		printf("It's all gone well on core 1!\n");

	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);

	gpio_init(BUZZER_CTR_PIN);
	gpio_set_dir(BUZZER_CTR_PIN, GPIO_OUT);	

	gpio_init(WIFI_ENABLE_PIN);
	gpio_set_dir(WIFI_ENABLE_PIN, GPIO_OUT);	
	gpio_put(WIFI_ENABLE_PIN,1);

	ST7735_Init();
	//ST7735_DrawImage(0, 0, 80, 160, arducam_logo);
	ST7735_FillScreen(ST7735_WHITE);
	

	struct arducam_config config;
	config.sccb = i2c0;
	config.sccb_mode = I2C_MODE_16_8;
	config.sensor_address = 0x24;
	config.pin_sioc = PIN_CAM_SIOC;
	config.pin_siod = PIN_CAM_SIOD;
	config.pin_resetb = PIN_CAM_RESETB;
	config.pin_xclk = PIN_CAM_XCLK;
	config.pin_vsync = PIN_CAM_VSYNC;
	config.pin_y2_pio_base = PIN_CAM_Y2_PIO_BASE;

	config.pio = pio0;
	config.pio_sm = 0;

	config.dma_channel = 0;
	config.image_buf = image_buf;
	config.image_buf_size = sizeof(image_buf);

	arducam_init(&config);

	uart_init(uart0, 115200);
	// gpio_set_function(0, GPIO_FUNC_UART);
	// gpio_set_function(1, GPIO_FUNC_UART);

	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

	volatile char tempchars[5];
	volatile char hourchars[5];
	volatile char minchars[5];
	volatile char secchars[5];
	volatile char inputchars[2];

	int hour = 23;
	int minute = 55;
	int sec = 56;
	int counter = 0;

	bool hourSetting = false;
	bool minSetting = false;
	bool secSetting = false;
	int currentContent = -1;// 0 for hour, 1 for min, 2 for sec

	volatile char inputch;

	int avarageLightLevel = 0;
	int total = 0;

	bool roosterEnabled = true;
	int roosterCounter = 0;


	// starting with the timesetting
	printf("Start time Initialization.\n");

	while(true){

		int rd;
		int k;
		int count;
		char url[] = "https://worldtimeapi.org/api/timezone/America/New_York";
		char buf[257];
		char tempSt[500];
		char hourNum[3];
		char minuteNum[3];
		char secondNum[3];
		bool validTime = true;
		// char *singleResult[256];

		printf("%s\n", url); // fflush(stdout);
		uart_puts(uart0, "https://worldtimeapi.org/api/timezone/America/New_York"); 
		uart_putc_raw(uart0, '\n');
		printf("Reading starts******************************\n");
		buf[256] = '\0';
		tempSt[499] = '\0';
		hourNum[2] = '\0';
		minuteNum[2] = '\0';
		secondNum[2] = '\0';
		count = 0;
		for(k = 0; k<256;k++){
			buf[k] = '0';
		}

		while (rd = uart_is_readable_within_us(uart0, TIMEOUT_S*1000000) > 0)
		{
		if (rd>256)  rd=256;
		uart_read_blocking(uart0, buf, rd);
		// fwrite(buf, rd, 1, stdout); 
		// fwrite(buf, rd, 1, singleResult);
		// fflush(stdout);
		tempSt[count] = buf[0];
		count++;
		// printf((char)buf[0]);
		printf("%c", buf[0]);
		
		// fflush(singleResult);
		}
		printf("\n");
		printf("Reading stops*********************************\n");
		printf("Counter: %d\n", count);
		printf("Content: %s\n", tempSt);
		printf("Time Index: %s\n", strstr(tempSt, "datetime"));
		if(strstr(tempSt, "datetime") && count >= 353){
			strncpy(hourNum, strstr(tempSt, "datetime")+22, 2);
			strncpy(minuteNum, strstr(tempSt, "datetime")+25, 2);
			strncpy(secondNum, strstr(tempSt, "datetime")+28, 2);
			printf("The time is %s:%s:%s\n",hourNum,minuteNum,secondNum);

			validTime = isdigit(hourNum[0])&&isdigit(minuteNum[0])&&isdigit(secondNum[0])&&isdigit(hourNum[1])&&isdigit(minuteNum[1])&&isdigit(secondNum[1]);
			printf("time validation: %d\n", validTime);
			if(validTime){
				hour = (int)(hourNum[0]-'0')*10+(int)(hourNum[1]-'0');
				minute = (int)(minuteNum[0]-'0')*10+(int)(minuteNum[1]-'0');
				sec = ((int)(secondNum[0]-'0')*10+(int)(secondNum[1]-'0')+3)%60;
				printf("The time has been initialized with %d:%d:%d \n", hour, minute, sec);
				break;
			}
		}
		count = 0;
		
	}

	int cycleCounter = 0;
	bool ambientCTL = false;
	bool previousAmbient = false;
	int backgroundColor;
	clock_t start, finish;
	int duration;
	while (true) {
		start = clock();
		gpio_put(PIN_LED, !gpio_get(PIN_LED));
		
		arducam_capture_frame(&config);

		uint16_t index = 0;
		for (int y = 0; y < 160; y++) {
			for (int x = 0; x < 80; x++) {
				uint8_t c = image_buf[(2+320-2*y)*324+(2+40+2*x)];
				uint16_t imageRGB   = ST7735_COLOR565(c, c, c);
				displayBuf[index++] = (uint8_t)(imageRGB >> 8) & 0xFF;
				displayBuf[index++] = (uint8_t)(imageRGB)&0xFF;
				}
		}
		total = 0;
		// sleep_ms(1000);
		for(int i = 0; i < 80*160*2; i++){
			total = total + displayBuf[i];
		
		}
		avarageLightLevel = total/80*160*2;
		printf("The avarage light level is: %d \n", avarageLightLevel);
		
		if(avarageLightLevel >= 10000000 && roosterCounter <= 40){
			// turn on the buzzer without blocking the main loop
			gpio_put(BUZZER_CTR_PIN, 1);
			// Count the total length of the buzzer
			roosterCounter++;

		}else{
			gpio_put(BUZZER_CTR_PIN, 0);
		}

		if(avarageLightLevel <= 10000000){
			ambientCTL = true;
			backgroundColor = ST7735_COLOR565(0x55,0x55,0x55);
		}else{
			ambientCTL = false;
			backgroundColor = 0xffff;
		}

		// TODO alarm clock time setting/execution logic

		// ST7735_DrawImage(0, 0, 80, 160, displayBuf);
		
		uint16_t raw = adc_read();
		printf("Raw Num: %d\n", raw);
		const float conversion_factor = 3.5f/(1<<12);
		float result = raw * conversion_factor;
		float temp = 27-(result-0.706)/0.003021;
		printf("Temp = %f C\n", temp);

		// itoa((int)temp, tempchars, 30);
		tempchars[0] = (char)((int)temp/10 + '0');
		tempchars[1] = (char)((int)temp%10 + '0');
		tempchars[2] = 'o';
		tempchars[3] = 'C';
		tempchars[4] = '\0';
		printf(tempchars);
		printf("\n");

		// update sec
		if((sec + 1) % 60 == 0){
			sec = 0;
			minute++;
		}else{
			sec++;
		}

		// update minute
		if(minute == 60){
			minute = 0;
			hour++;
		}

		// update hour
		if(hour== 24){
			hour = 0;
		}

		// sleep_ms(920);

		// update the roosterCounter
		if(hour == 0 && minute == 0 && sec == 0){
			roosterCounter = 0;
		}

		// hour int->string
		hourchars[0] = (char)(hour/10+'0');
		hourchars[1] = (char)(hour%10+'0');
		hourchars[2] = 'h';
		hourchars[3] = ':';
		hourchars[4] = '\0';
		// minute int->string
		minchars[0] = (char)(minute/10+'0');
		minchars[1] = (char)(minute%10+'0');
		minchars[2] = 'm';
		minchars[3] = ':';
		minchars[4] = '\0';

		// sec int->string
		secchars[0] = (char)(sec/10+'0');
		secchars[1] = (char)(sec%10+'0');
		secchars[2] = 's';
		secchars[3] = ' ';
		secchars[4] = '\0';
		
		if(!(ambientCTL==previousAmbient)){
			ST7735_FillScreen(backgroundColor);
		}
		ST7735_WriteString(0,0,"Temp",Font_16x26, 0, backgroundColor);
		ST7735_WriteString(0,27,tempchars,Font_16x26, 0, backgroundColor);
		ST7735_WriteString(0,54, hourchars,Font_16x26, 0, backgroundColor);
		ST7735_WriteString(0,81, minchars,Font_16x26, 0, backgroundColor);
		ST7735_WriteString(0,108, secchars,Font_16x26, 0, backgroundColor);

		previousAmbient = ambientCTL;
		cycleCounter++;
		printf("cycle num: %d\n", cycleCounter);
		finish = clock();

		duration = ((int)(finish-start*1000)/CLOCKS_PER_SEC*11);// 9
		printf("The display takes %dms \n", duration);

		if(duration < 1000){
			sleep_ms(1000-duration);
		}else{
			sleep_ms(duration);
		}
		// sleep_ms(900);

	}
}

#include "hardware/vreg.h"

int main() {
  int loops=20;
  stdio_init_all();
  stdio_filter_driver(&stdio_usb);
  while (!tud_cdc_connected()) { sleep_ms(100); if (--loops==0) break;  }

  printf("tud_cdc_connected(%d)\n", tud_cdc_connected()?1:0);

  vreg_set_voltage(VREG_VOLTAGE_1_30);
  sleep_ms(1000);
  set_sys_clock_khz(250000, true);

  multicore_launch_core1(core1_entry);

  uint32_t g = multicore_fifo_pop_blocking();

  if (g != FLAG_VALUE)
    printf("Hmm, that's not right on core 0!\n");
  else {
    multicore_fifo_push_blocking(FLAG_VALUE);
    printf("It's all gone well on core 0!\n");
  }

  while (1)
    tight_loop_contents();
}
