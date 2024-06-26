
#include "hardware-mapping.h"

#define GPIO_BIT(b) ((uint32_t)1<<(b))
//#define GPIO_BIT(port, pin) (((port - 'A') << 5) | (pin))

struct HardwareMapping matrix_hardware_mappings[] = {
  {
    .name          = "regular", // was adafruit-hat

    .output_enable = GPIO_BIT( 0),
    .clock         = GPIO_BIT( 1),
    .strobe        = GPIO_BIT( 2),

    .a             = GPIO_BIT( 3),
    .b             = GPIO_BIT( 4),
    .c             = GPIO_BIT( 5),
    .d             = GPIO_BIT( 6),
	.e 			   = GPIO_BIT( 7),

    .p0_r1         = GPIO_BIT( 8),
    .p0_g1         = GPIO_BIT( 9),
    .p0_b1         = GPIO_BIT(10),
    .p0_r2         = GPIO_BIT(11),
    .p0_g2         = GPIO_BIT(12),
    .p0_b2         = GPIO_BIT(13),

    /*
    .output_enable = GPIO_BIT('G',6),
    .clock         = GPIO_BIT('C',2),
    .strobe        = GPIO_BIT('A',1),

    .a             = GPIO_BIT('C',0),
    .b             = GPIO_BIT('G',8),
    .c             = GPIO_BIT('C',1),
    .d             = GPIO_BIT('G',9),

    .p0_r1         = GPIO_BIT('G',11),
    .p0_g1         = GPIO_BIT('G',7),
    .p0_b1         = GPIO_BIT('A',0),
    .p0_r2         = GPIO_BIT('A',2),
    .p0_g2         = GPIO_BIT('A',6),
    .p0_b2         = GPIO_BIT('A',3),
    */
  },

  {0}
};