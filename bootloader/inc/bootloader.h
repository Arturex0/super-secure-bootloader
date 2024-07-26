// Copyright 2024 The MITRE Corporation. ALL RIGHTS RESERVED
// Approved for public release. Distribution unlimited 23-02181-25.

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

//secrets buffer
#define READ_BUFFER_SIZE 1024

#define IV_LEN 16

// FLASH Constants
#define FLASH_PAGESIZE 1024
#define FLASH_WRITESIZE 4

// Protocol Constants
#define OK ((unsigned char)0x00)
#define ERROR ((unsigned char)0x01)
#define UPDATE ((unsigned char)'U')
#define BOOT ((unsigned char)'B')
#define FRAME ((unsigned char)'F')
#define BASS ((unsigned char)'T')

// Return messages
#define VERIFY_SUCCESS 0
#define VERIFY_ERR 1

#define FW_LOADED 0
#define FW_ERROR 1

//long program_flash(void* page_addr, unsigned char * data, unsigned int data_len);

#endif

