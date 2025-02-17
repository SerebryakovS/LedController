#include "comm-h3.h"
#include "gpio-allwinner.h"


#ifdef defined(H3)
static const off_t GPIO_REG_BASE=0x01C20000;
#elif defined(H618)
static const off_t GPIO_REG_BASE=0x0300B000;
#endif

static const size_t GPIO_REG_OFF=0x800;
//static const size_t GPIO_REG_LEN=0x1800;
static const size_t GPIO_REG_LEN=0x2E2400;

static uint32_t*   p_gpio=NULL;

uint32_t* gpio_system_init(void)
{
    if ( NULL == p_gpio ) {
        int fd = open("/dev/mem", O_RDWR|O_SYNC);

        if (fd < 0) {
            fprintf(stderr, "Could not open /dev/mem\n");
            //return -1;
            return 0;
        }

        unsigned char * ptr = (unsigned char*)mmap(NULL, GPIO_REG_LEN, PROT_READ|PROT_WRITE,
                                                   MAP_SHARED, fd, GPIO_REG_BASE);

        if ( MAP_FAILED == ptr ) {
            FATAL_ERRORF("fd = %d, mmap error: %d-%s\n", fd, errno, strerror(errno));
            //return -1;
            return 0;
        }
        DBG_MSG("ptr = 0x%08X", ptr);
#ifdef defined(H3)
        ptr += GPIO_REG_OFF;
#endif
        p_gpio = (uint32_t*) ptr;
        DBG_MSG("p_gpio = 0x%08X", p_gpio);
    }
    //return 0;
    return p_gpio; // return memory address
}

// int gpio_init(struct gpio_t* p, const char * name)
// {
//     char bank;
//     memset(p->name, 0, 5);
//     strncpy(p->name, name, 4);
//     sscanf(p->name, "P%c%d", &bank, &p->idx);
//     p->base_off = bank - 'A';
//     p->reg_off  = p->idx / 8;
//     p->reg_ptr  = p_gpio + (9 * p->base_off) + p->reg_off;
//     p->dat_ptr  = p_gpio + (9 * p->base_off) + 4;
//     p->reg_idx  = p->idx % 8;
//     p->reg_clear_mask =  ~( 0xF << ( p->reg_idx * 4 )) ;
//     p->data_clear_mask = ~( 1 << (p->idx));
//     p->val = 0xffffffff;
//     DBG_MSG("bank = %c, idx = %d, base_off= %d, reg_off = %d, reg_idx = %d, reg=0x%x, clear_mask = 0x%x, data_clear_mask = 0x%x", bank, p->idx,
//             p->base_off, p->reg_off, p->reg_idx, *(p->reg_ptr), p->reg_clear_mask, p->data_clear_mask);
//     return 0;
// }

int gpio_init(struct gpio_t* p, const char* name) {
    char bank;
    memset(p->name, 0, 5);
    strncpy(p->name, name, 4);
    sscanf(p->name, "P%c%d", &bank, &p->idx);

    if (bank != 'I') {
        FATAL_ERRORF("Unsupported GPIO bank: %c", bank);
        return -1;
    }

    p->reg_off = p->idx / 8;
    p->reg_ptr = p_gpio + (0x120 / 4) + p->reg_off;  // PI_CFG registers
    p->dat_ptr = p_gpio + (0x130 / 4);               // PI_DAT register

    p->reg_idx = p->idx % 8;
    p->reg_clear_mask = ~(0xF << (p->reg_idx * 4));
    p->data_clear_mask = ~(1 << p->idx);

    // === SET DRIVE STRENGTH TO LEVEL 3 (STRONGEST) ===
    uint32_t *drv_reg = p_gpio + (0x134 / 4) + (p->idx / 16);  // Select PI_DRV0 or PI_DRV1
    uint8_t shift = (p->idx % 16) * 2;  // Each pin has 2 bits in the register

    uint32_t drv_val = *drv_reg;
    drv_val &= ~(0x3 << shift);  // Clear current setting
    drv_val |= (3 << shift);     // Set Level 3 (strongest)
    *drv_reg = drv_val;
    __sync_synchronize();

    DBG_MSG("GPIO PI%d Drive Strength Set: 0x%08X", p->idx, *drv_reg);

    return 0;
}



// int gpio_set_func(struct gpio_t* p, uint32_t i)
// {
//     uint32_t v = *p->reg_ptr;
//     uint32_t sv = (i << ( p->reg_idx * 4  ));
//     DBG_MSG("gpio_set_func: name = %s, i = %d, v = 0x%08x, sv = 0x%08x, clear_mask = 0x%08x", p->name, i, v, sv, p->reg_clear_mask);
//     v &= p->reg_clear_mask;
//     v |= sv;
//     DBG_MSG("v = 0x%x, sv = 0x%x", v, sv);
//     *p->reg_ptr = v;
//     __sync_synchronize();
//
//     return 0;
// }

int gpio_set_func(struct gpio_t* p, uint32_t i) {
    uint32_t shift = (p->reg_idx * 4);
    uint32_t v     = *p->reg_ptr;
    uint32_t sv    = (i << shift);

    v &= ~(0xF << shift);
    v |= sv;
    *p->reg_ptr = v;
    __sync_synchronize();

    uint32_t read_back = *p->reg_ptr;
    if ((read_back & (0xF << shift)) != sv) {
        FATAL_ERRORF("Failed to set function for %s: expected 0x%08x, got 0x%08x", p->name, sv, read_back);
        return -1;
    }
    DBG_MSG("Function set correctly for %s: 0x%08x", p->name, read_back);
    return 0;
}

int gpio_set_input(struct gpio_t* p) {
    return gpio_set_func(p, 0);
}

int gpio_set_output(struct gpio_t* p) {
    gpio_set_func(p, 1);
    return 0;
}


int gpio_read_input(struct gpio_t* p, uint32_t* v) {
    uint32_t mask = 1 << p->idx;
    DBG_MSG("read dat = 0x%08x", *p->dat_ptr);
    *v = (*p->dat_ptr & mask) ? 1 : 0;
    __sync_synchronize();
    return 0;
}

int gpio_set_disable(struct gpio_t* p) {
    gpio_set_func(p, 7);
    return 0;
}

int gpio_bank_set_output(struct gpio_bank_t* pbank)
{
	uint32_t i = 0;
	for ( i = 0; i < pbank->size; i++ ) {
		gpio_set_output(pbank->gpio[i]);
	}
	return 0;
}
int gpio_bank_set_output_value(struct gpio_bank_t* pbank, const uint32_t v)
{
	if ( pbank->size == 0 ) {
		return 0;
	}
	if ( ! (v == 1 || v == 0) ) {
		return -1;
	}

	uint32_t dv = *((pbank->gpio[0])->dat_ptr);
	DBG_MSG("bank_output_value %d, read dat = 0x%08x", v, dv);
	__sync_synchronize();
	uint32_t mask = 0xffffffff;
	uint32_t sv   = 0;
	for ( uint32_t i = 0; i < pbank->size; i++ ) {
		DBG_MSG("bank_output_value %d, mask = 0x%08x, clear_mask = 0x%08x", i, mask, pbank->gpio[i]->data_clear_mask );
		sv |= v << pbank->gpio[i]->idx;
		mask &= pbank->gpio[i]->data_clear_mask;
	}
	dv &= mask;
	dv |= sv;
	DBG_MSG("bank_output_value %d, set dat = 0x%08x", v, dv);
	*(pbank->gpio[0]->dat_ptr) = dv;
	__sync_synchronize();

	return 0;
}
int gpio_bank_read(struct gpio_bank_t* pbank)
{
	uint32_t dv = *((pbank->gpio[0])->dat_ptr);
	__sync_synchronize();
	for ( uint32_t i = 0; i < pbank->size; i++ ) {
		uint32_t mask = ( 1 << pbank->gpio[i]->idx );
		pbank->gpio[i]->val = ( dv & mask ) ? 1 : 0;
	}
	return 0;
}

int gpio_set_output_value(struct gpio_t* p) {
    uint32_t mask = (1 << p->idx);
    *p->dat_ptr |= mask;
    __sync_synchronize();

    // uint32_t read_back = *p->dat_ptr;
    // if ((read_back & mask) != mask) {
    //     FATAL_ERRORF("Failed to set output high for %s: expected 0x%08x, got 0x%08x", p->name, mask, read_back);
    //     return -1;
    // }
    return 0;
}

int gpio_reset_output_value(struct gpio_t* p) {
    uint32_t mask = (1 << p->idx);
    *p->dat_ptr &= ~mask;
    __sync_synchronize();

    // uint32_t read_back = *p->dat_ptr;
    // if ((read_back & mask) != 0) {
    //     FATAL_ERRORF("Failed to set output low for %s: expected 0x00000000, got 0x%08x", p->name, read_back);
    //     return -1;
    // }
    return 0;
}

// Precomputed GPIO index mapping (adjust this based on your real mapping)
static const uint32_t bit_remap_table[14] = {
    13, 1, 10, 6, 15, 14, 0, 12, 3, 16, 4, 11, 2, 9
};

void gpio_set_port_value(struct gpio_t *p, uint32_t value) {
    uint32_t *dat_ptr = p->dat_ptr;
    uint32_t mask = 0;

    // Apply mapping directly using bitwise operations (no loop)
    mask = ((value & (1 << 0)) ? (1 << bit_remap_table[0]) : 0) |
    ((value & (1 << 1)) ? (1 << bit_remap_table[1]) : 0) |
    ((value & (1 << 2)) ? (1 << bit_remap_table[2]) : 0) |
    ((value & (1 << 3)) ? (1 << bit_remap_table[3]) : 0) |
    ((value & (1 << 4)) ? (1 << bit_remap_table[4]) : 0) |
    ((value & (1 << 5)) ? (1 << bit_remap_table[5]) : 0) |
    ((value & (1 << 6)) ? (1 << bit_remap_table[6]) : 0) |
    ((value & (1 << 7)) ? (1 << bit_remap_table[7]) : 0) |
    ((value & (1 << 8)) ? (1 << bit_remap_table[8]) : 0) |
    ((value & (1 << 9)) ? (1 << bit_remap_table[9]) : 0) |
    ((value & (1 << 10)) ? (1 << bit_remap_table[10]) : 0) |
    ((value & (1 << 11)) ? (1 << bit_remap_table[11]) : 0) |
    ((value & (1 << 12)) ? (1 << bit_remap_table[12]) : 0) |
    ((value & (1 << 13)) ? (1 << bit_remap_table[13]) : 0);

    // Debugging
    // DBG_MSG("SET Mask: 0x%08X", mask);

    // Apply the mask in one operation
    *dat_ptr |= mask;
    __sync_synchronize();
}

void gpio_clear_port_value(struct gpio_t *p, uint32_t value) {
    uint32_t *dat_ptr = p->dat_ptr;
    uint32_t mask = 0;

    // Apply mapping directly using bitwise operations (no loop)
    mask = ((value & (1 << 0)) ? (1 << bit_remap_table[0]) : 0) |
    ((value & (1 << 1)) ? (1 << bit_remap_table[1]) : 0) |
    ((value & (1 << 2)) ? (1 << bit_remap_table[2]) : 0) |
    ((value & (1 << 3)) ? (1 << bit_remap_table[3]) : 0) |
    ((value & (1 << 4)) ? (1 << bit_remap_table[4]) : 0) |
    ((value & (1 << 5)) ? (1 << bit_remap_table[5]) : 0) |
    ((value & (1 << 6)) ? (1 << bit_remap_table[6]) : 0) |
    ((value & (1 << 7)) ? (1 << bit_remap_table[7]) : 0) |
    ((value & (1 << 8)) ? (1 << bit_remap_table[8]) : 0) |
    ((value & (1 << 9)) ? (1 << bit_remap_table[9]) : 0) |
    ((value & (1 << 10)) ? (1 << bit_remap_table[10]) : 0) |
    ((value & (1 << 11)) ? (1 << bit_remap_table[11]) : 0) |
    ((value & (1 << 12)) ? (1 << bit_remap_table[12]) : 0) |
    ((value & (1 << 13)) ? (1 << bit_remap_table[13]) : 0);

    // Debugging
    // DBG_MSG("CLEAR Mask: 0x%08X", mask);

    // Apply the mask in one operation
    *dat_ptr &= ~mask;
    __sync_synchronize();
}



