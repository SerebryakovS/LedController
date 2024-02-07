
#ifndef RPI_HARDWARE_MAPPING_H
#define RPI_HARDWARE_MAPPING_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "gpio-bits.h"

struct BitToGPIOMap {
    int bit;       // Original bit number
    char port;     // New port
    int pin;       // New pin
};

struct gpio_t {
	char       name[5];
	uint32_t   base_off;
	uint32_t   idx; 
	uint32_t   reg_off;
	uint32_t*  reg_ptr;
	uint32_t*  dat_ptr;
	uint32_t   reg_idx;
	uint32_t   reg_clear_mask;
	uint32_t   data_clear_mask;
	uint32_t   old_val;
	uint32_t   val;
};

struct gpio_bank_t {
	struct gpio_t*  gpio[MAX_GPIO_BANK_SIZE];
	uint32_t  size;
};

struct HardwareMapping {
  const char *name;
  int max_parallel_chains;
  
  gpio_bits_t output_enable;
  gpio_bits_t clock;
  gpio_bits_t strobe;

  gpio_bits_t a, b, c, d, e;

  gpio_bits_t p0_r1, p0_g1, p0_b1;
  gpio_bits_t p0_r2, p0_g2, p0_b2;

  gpio_bits_t p1_r1, p1_g1, p1_b1;
  gpio_bits_t p1_r2, p1_g2, p1_b2;

  gpio_bits_t p2_r1, p2_g1, p2_b1;
  gpio_bits_t p2_r2, p2_g2, p2_b2;

  gpio_bits_t p3_r1, p3_g1, p3_b1;
  gpio_bits_t p3_r2, p3_g2, p3_b2;

  gpio_bits_t p4_r1, p4_g1, p4_b1;
  gpio_bits_t p4_r2, p4_g2, p4_b2;

  gpio_bits_t p5_r1, p5_g1, p5_b1;
  gpio_bits_t p5_r2, p5_g2, p5_b2;
};

extern struct HardwareMapping matrix_hardware_mappings[];

#ifdef  __cplusplus
}  // extern C
#endif

#endif

