
#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "arducam/arducam.h"
#include "lib/st7735.h"
#include "lib/fonts.h"
#include "hardware/adc.h"

uint8_t image_buf[324*324];
uint8_t displayBuf[80*160*2];
uint8_t header[2] = {0x55,0xAA};

#define FLAG_VALUE 123
#define BUZZER_CTR_PIN 27


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
	while(!(hourSetting && minSetting && secSetting) && false){
		sleep_ms(100);
		if(counter < 2){
			gpio_put(PIN_LED, !gpio_get(PIN_LED));
			inputch = getchar_timeout_us(0);
			if(inputch == 'H'|| inputch == 'h'){
				currentContent = 0;
				printf("Current Content is hour \n");
			}else if(inputch == 'M'||inputch == 'm'){
				currentContent = 1;
				printf("Current Content is min \n");
			}else if(inputch == 's'||inputch == 'S'){
				currentContent = 2;
				printf("Current Content is sec \n");
			}else{
				if(inputch <= '9' && inputch >= '0'){
					inputchars[counter] = inputch;
					counter++;
					printf("Char %c captured, counter = %d \n",inputch, counter);
				}
			}
		}else{
			printf("Captured two chars input with currentContent %d\n", currentContent);
			if(currentContent == 0 && (!hourSetting)){
				hour = (int)(inputchars[0]-'0')*10 + (int)(inputchars[1]-'0');
				if(hour >= 0 && hour <= 24){
					printf("hour setting finished with %d\n", hour);
					hourSetting = true;
				}else{
					printf("Invalid hour input of %d, please input again! \n", hour);
					
				}
			}else if(currentContent == 1 && (!minSetting)){
				minute = (int)(inputchars[0]-'0')*10 + (int)(inputchars[1]-'0');
				if(minute >= 0 && minute <= 60){
					printf("minute setting finished with %d\n", minute);
					minSetting = true;
				}else{
					printf("Invalid min input of %d, please input again! \n", minute);
					
				}
				
			}else if(currentContent == 2 && (!secSetting)){
				sec = (int)(inputchars[0]-'0')*10 + (int)(inputchars[1]-'0');
				if(sec >= 0 && sec <= 60){
					printf("sec setting finished with %d\n", sec);
					secSetting = true;
				}else{
					printf("Invalid sec input of %d, please input again! \n", sec);
					
				}
				
			}else{
				printf("Sorry! Please indicate the meaning of your input.\n");
			}

			// reset the counter
			counter = 0;
		}
	}
	printf("The time has been initialized with %d: %d: %d \n", hour, minute, sec);

	bool ambientCTL = false;
	while (true) {
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
		
		if(avarageLightLevel >= 10000000 && roosterCounter <= 10){
			// turn on the buzzer without blocking the main loop
			gpio_put(BUZZER_CTR_PIN, 1);
			// Count the total length of the buzzer
			roosterCounter++;
			if(roosterCounter == 10){
				gpio_put(BUZZER_CTR_PIN, 0);
			}

			// TODO detect the button input, if the input detected, change the counter to 11 immidiately
			
			

		}else{
			gpio_put(BUZZER_CTR_PIN, 0);
		}

		if(avarageLightLevel <= 10000000){
			ambientCTL = true;
		}else{
			ambientCTL = false;
		}

		// TODO alarm clock time setting/execution logic

		// ST7735_DrawImage(0, 0, 80, 160, displayBuf);
		
		uint16_t raw = adc_read();
		printf("Raw Num: %d\n", raw);
		const float conversion_factor = 3.5f/(1<<12);
		float result = raw * conversion_factor;
		float temp = 27-(result-0.706)/0.003021;
		printf("Temp = %f C\n", temp);

		itoa((int)temp, tempchars, 10);
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

		sleep_ms(1000);

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
		if(ambientCTL){
			ST7735_FillScreen(ST7735_COLOR565(0xaa,0xaa ,0xaa));
			ST7735_WriteString(0,0,"Temp",Font_16x26, 0, ST7735_COLOR565(0xaa,0xaa ,0xaa));
			ST7735_WriteString(0,27,tempchars,Font_16x26, 0, ST7735_COLOR565(0xaa, 0xaa, 0xaa));
			ST7735_WriteString(0,54, hourchars,Font_16x26, 0, ST7735_COLOR565(0xaa, 0xaa, 0xaa));
			ST7735_WriteString(0,81, minchars,Font_16x26, 0, ST7735_COLOR565(0xaa, 0xaa, 0xaa));
			ST7735_WriteString(0,108, secchars,Font_16x26, 0, ST7735_COLOR565(0xaa, 0xaa, 0xaa));
		}else{
			ST7735_FillScreen(0xffff);
			ST7735_WriteString(0,0,"Temp",Font_16x26, 0, 0xffff);
			ST7735_WriteString(0,27,tempchars,Font_16x26, 0, 0xffff);
			ST7735_WriteString(0,54, hourchars,Font_16x26, 0, 0xffff);
			ST7735_WriteString(0,81, minchars,Font_16x26, 0, 0xffff);
			ST7735_WriteString(0,108, secchars,Font_16x26, 0, 0xffff);
		}
		

	}
}

#include "hardware/vreg.h"

int main() {
  int loops=20;
  stdio_init_all();
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
