/**
 * @file radio.c
 * @author raphael wirtz
 * @brief Radio protocol
 * @version 1
 * @date 2024-05-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "radio_protocol.h"

#include "stdio.h"

#include "pico_config.h"

#include "pico/unique_id.h"

pico_unique_board_id_t pico_unique_board_id;



// pico_get_unique_board_id(&pico_unique_board_id);















/**
 * @brief Send payload separator character.
 */
static void send_payload_separator(void) {
  // todo
}

