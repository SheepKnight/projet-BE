#ifndef _NAND_MODULE_H
#define _NAND_MODULE_H



//NAND PINS
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
#define NAND_ADDR_LATCH  (gpio_num_t)13  //Adress latch
#define NAND_CMD_LATCH  (gpio_num_t)14
#define NAND_READ_ENABLE  (gpio_num_t)34

#define NAND_CHIP_ENABLE  (gpio_num_t)33
#define NAND_CHIP_ENABLE_2  (gpio_num_t)36
#define NAND_READY  (gpio_num_t)37
#define NAND_READY_2  (gpio_num_t)35



//DEFINE USED FOR ARDUINO/READABILITY ONLY. DO NOT CHANGE.
#define HIGH 1
#define LOW 0
#define NOCARE 2
#define INPUT 0
#define OUTPUT 1
#define RISING 2
#define FALLING 3
#define WRITING 4
#define READING 5

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

typedef struct nand_interface{
	uint8_t CE;
	uint8_t CLE;
	uint8_t ALE;
	uint8_t WE;
	uint8_t RE;
	uint8_t WP;
	uint8_t DATA_ORIENTATION;
	uint8_t operation;
} interface;


void nand_write(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize);
void nand_read(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize);
void nand_erase(uint8_t lun, uint32_t block);
void nand_reset();
char * nand_read_id();
void nand_set_pins();

#endif