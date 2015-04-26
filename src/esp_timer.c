
#include "lwip/opt.h" // extern sys_now_ms

static u32_t last_now;
static u32_t diff_m16;
u32_t sys_now_ms;

// old timer code from espressif:
// sys_now_ms = (NOW() / ((CPU_CLK_FREQ / 1000) >> (timer2_ms_flag? 8: 4)))
// which is (flag=1) sys_now() = NOW() / 312.5
// and things break after several loops of NOW() (every 3.8hours, flag=1)
// because sys_now_ms is going back instead of looping through its 2^32 range

void esp_ms_timer_init (void)
{
	last_now = NOW();
	diff_m16 = 0;
	sys_now_ms = 0;
}

int esp_ms_timer_update_and_check_changed (void)
{
	u32_t new_now = NOW(); // get raw timer2 register
	diff_m16 += (new_now - last_now) << 4;
	last_now = new_now;
	
	// ms_m16 is 16*(number of NOW() ticks for a milliseconds)
	u32_t ms_m16 = CPU_CLK_FREQ / 1000;
	if (timer2_ms_flag)
		// this is espressif magic
		// timer2_ms_flag seems to always be 1 but who knows ?
		ms_m16 >>= 4;
		
	if (diff_m16 < ms_m16)
		// unchanged
		return 0;
		
#if 1
	// increase milliseconds according to ticks
	// we are called often enough to make only one loop
	while (diff_m16 >= ms_m16)
	{
		sys_now_ms++;
		diff_m16 -= ms_m16;
	}
#else
	// XXX otherwise, divide and modulo should be faster
	divide and modulo
#endif

	// changed
	return 1;
}
