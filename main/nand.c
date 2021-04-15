#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define NAND_BUS_0  (gpio_num_t)3 //Bus Pins
#define NAND_BUS_1  (gpio_num_t)4
#define NAND_BUS_2  (gpio_num_t)5
#define NAND_BUS_3  (gpio_num_t)6
#define NAND_BUS_4  (gpio_num_t)7
#define NAND_BUS_5  (gpio_num_t)8
#define NAND_BUS_6  (gpio_num_t)9
#define NAND_BUS_7  (gpio_num_t)10

#define NAND_WRITE_PROTECT  (gpio_num_t)11  
#define NAND_WRITE_ENABLE  (gpio_num_t)12
#define NAND_ADDR_LATCH  (gpio_num_t)16  //Adress latch
#define NAND_CMD_LATCH  (gpio_num_t)17
#define NAND_READ_ENABLE  (gpio_num_t)34

#define NAND_CHIP_ENABLE  (gpio_num_t)33
#define NAND_CHIP_ENABLE_2  (gpio_num_t)36
#define NAND_READY  (gpio_num_t)37
#define NAND_READY_2  (gpio_num_t)35


#include "nand.h"

#include "esp_log.h"

gpio_num_t NAND_BITS[8] = {NAND_BUS_0, NAND_BUS_1, NAND_BUS_2, NAND_BUS_3, NAND_BUS_4, NAND_BUS_5, NAND_BUS_6, NAND_BUS_7};


#define HIGH 1
#define LOW 0
#define NOCARE 2
#define INPUT 0
#define OUTPUT 1
#define RISING 2
#define FALLING 3
#define WRITING 4
#define READING 5

//Standby
interface itf_standby = {HIGH, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE};
//Bus Idle
interface itf_idle = {LOW, NOCARE, NOCARE, HIGH, HIGH, NOCARE, NOCARE, NOCARE};
//Command input
interface itf_command = {LOW, HIGH, LOW, RISING, HIGH, HIGH, OUTPUT, WRITING};
//Address input
interface itf_address = {LOW, LOW, HIGH, RISING, HIGH, HIGH, OUTPUT, WRITING};
//Data input
interface itf_data_in = {LOW, LOW, LOW, RISING, HIGH, HIGH, OUTPUT, WRITING};
//Data output from nand
interface itf_data_out = {LOW, LOW, LOW, HIGH, FALLING, NOCARE, INPUT, READING};
//Write Protect
interface itf_write_protect = {NOCARE, NOCARE, NOCARE, NOCARE, NOCARE, LOW, NOCARE, NOCARE};



uint8_t is_ready(uint8_t lun){
	//uint8_t state = gpio_get_level(lun == 0 ? NAND_READY : NAND_READY_2);
	vTaskDelay(3 / portTICK_PERIOD_MS);
	//return state;
	return 1;
}

void write_gpio(gpio_num_t pin, uint8_t state){
	vTaskDelay(3 / portTICK_PERIOD_MS);
	if(state == NOCARE){return;}//Keep current state
	gpio_set_level(pin, state);
}

void set_bus_direction(uint8_t dir){
	static uint8_t last_val = 255;
	if(dir == NOCARE){return;}//Keep current state
	if(last_val == dir){return;}
	for(uint8_t i = 0; i < 8; i++){
		//gpio_reset_pin(NAND_BITS[i]);
		gpio_set_direction(NAND_BITS[i], dir == OUTPUT ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
	}
}


void write_bus(uint8_t val){
	for(int i = 0; i < 8; i++){
		gpio_set_level(NAND_BITS[i], (val & (0b00000001<<i)) ? 1:0 );
	}
}

char read_bus(){
	char out = 0b00000000;
	
	for(int i = 0; i < 8; i++){
		out |= ((gpio_get_level(NAND_BITS[i])==1?1:0)<<i);
	}
	
	return out;
}


char set_pins_state(interface itf, char parameter, int lun){
	
	char output = ' ';
	set_bus_direction(itf.DATA_ORIENTATION);
	
	write_gpio(lun == 0 ? NAND_CHIP_ENABLE : NAND_CHIP_ENABLE_2, itf.CE);
	if(lun == 0){write_gpio(NAND_CHIP_ENABLE_2, HIGH);}else{write_gpio(NAND_CHIP_ENABLE, HIGH);} //Turning off the other LUN.
	
	write_gpio(NAND_WRITE_PROTECT, itf.WP);
	write_gpio(NAND_ADDR_LATCH, itf.ALE);
	write_gpio(NAND_CMD_LATCH, itf.CLE);
	
	if(itf.operation == WRITING){
		write_gpio(NAND_READ_ENABLE, itf.RE);
		if(itf.WE == FALLING){write_gpio(NAND_WRITE_ENABLE, HIGH);}else{write_gpio(NAND_WRITE_ENABLE, LOW);};
		write_bus(parameter);
		if(itf.WE == FALLING){write_gpio(NAND_WRITE_ENABLE, LOW);}else{write_gpio(NAND_WRITE_ENABLE, HIGH);};
		
	}
	if(itf.operation == READING){
		write_gpio(NAND_WRITE_ENABLE, itf.WE);
		if(itf.RE == FALLING){write_gpio(NAND_READ_ENABLE, LOW);}else{write_gpio(NAND_READ_ENABLE, HIGH);};
		output = read_bus();
		if(itf.RE == FALLING){write_gpio(NAND_READ_ENABLE, HIGH);}else{write_gpio(NAND_READ_ENABLE, LOW);};
		
	}
	
	return output;
	
}

void nand_reset(){
	set_pins_state(itf_command, 0xFF, 0);
	set_pins_state(itf_command, 0xFF, 1);
	
}

char * nand_read_id(char * values){
	set_pins_state(itf_command, 0x90, 0);
	set_pins_state(itf_address, 0x20, 0);
	for(int i = 0; i < 4; i++){
		values[i] = set_pins_state(itf_data_out, 0x00, 0);
	}
	return values;
}

char * nand_read_id_long(char * values){
	set_pins_state(itf_command, 0x90, 0);
	set_pins_state(itf_address, 0x00, 0);
	for(int i = 0; i < 4; i++){
		values[i] = set_pins_state(itf_data_out, 0x00, 0);
	}
	return values;
}


void nand_set_pins(){
	gpio_set_direction(NAND_ADDR_LATCH, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_CHIP_ENABLE, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_CHIP_ENABLE_2, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_CMD_LATCH, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_READ_ENABLE, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_WRITE_ENABLE, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_WRITE_PROTECT, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_WRITE_PROTECT, GPIO_MODE_OUTPUT);
	gpio_set_direction(NAND_READY, GPIO_MODE_INPUT);
	gpio_set_direction(NAND_READY_2, GPIO_MODE_INPUT);
}

void nand_read(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize){
	ESP_LOGI("NAND", "READING %d + %d on LUN %d, size %d", lba, offset, lun, bufsize);
	
	set_pins_state(itf_command, 0x00, lun);
	uint8_t column1 = offset % 255;
	uint8_t column2 = (offset>>8) % 255;
	uint8_t page = lba % 255;
	uint8_t block1 = (lba>>8) % 255;
	uint8_t block2 = ((lba>>16) % 255)& 0b00001111; //Some Addresses are invalid
	set_pins_state(itf_address, column1, lun);
	set_pins_state(itf_address, column2, lun);
	set_pins_state(itf_address, page, lun);
	set_pins_state(itf_address, block1, lun);
	set_pins_state(itf_address, block2, lun);
	set_pins_state(itf_command, 0x30, lun);
	while(!is_ready(lun)){;}
	for(int i = 0; i < bufsize; i++){
		buffer[i] = set_pins_state(itf_data_out, 0x00, lun);
	}
	ESP_LOGI("NAND", "%.*s", bufsize, buffer);
	
}

void nand_write(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize){
	//See page 97 from the documentation.
	set_pins_state(itf_command, 0x80, lun);
	
	uint8_t column1 = offset % 256;
	uint8_t column2 = (offset>>8) % 256;
	uint8_t page = lba % 256;
	uint8_t block1 = (lba>>8) % 256;
	uint8_t block2 = ((lba>>16) % 256)& 0b00001111; //Some Addresses are invalid
	ESP_LOGI("NAND", "WRITING AT %d + %d on LUN %d, size %d", lba, offset, lun, bufsize);
	ESP_LOGI("NAND", "Address : %2x, %2x, %2x, %2x, %2x", column1, column2, page, block1, block2);
	ESP_LOGI("NAND", "<%.*s>", bufsize,  buffer);
	
	set_pins_state(itf_address, column1, lun);
	set_pins_state(itf_address, column2, lun);
	set_pins_state(itf_address, page, lun);
	set_pins_state(itf_address, block1, lun);
	set_pins_state(itf_address, block2, lun);
	
	vTaskDelay(10 / portTICK_PERIOD_MS);
	
	for(int i = 0; i < bufsize; i++){
		set_pins_state(itf_data_in, buffer[i], lun);
	}	
	
	set_pins_state(itf_command, 0x10, lun);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	while(!is_ready(lun)){;}
	set_pins_state(itf_command, 0x70, lun);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	uint8_t output = set_pins_state(itf_data_out, 0x00, lun);
	ESP_LOGI("NAND", "Writing returned : "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(output));
	
	
}

void nand_erase(uint8_t lun, uint32_t block){
	set_pins_state(itf_command, 0x80, lun);
	set_pins_state(itf_address, (uint8_t)(block % 255), lun);
	set_pins_state(itf_address, (uint8_t)(block>>8 % 255), lun);
	set_pins_state(itf_address, (uint8_t)(block>>16 % 255), lun);
	set_pins_state(itf_command, 0xd0, lun);
}