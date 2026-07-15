#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>

#define TIMER_FREQ 10000000
#define SIE_STIE (1ULL << 5)

u64 uptime_seconds = 0;
u64 alarm_target = 0;
int alarm_active = 0;
u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, SIE_STIE);
    timer_set_alarm(1); /* Inicia o ciclo agendando a 1a interrupcao para 1s */
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
    u64 tick_in_future = now + (secs * TIMER_FREQ);
    csr_write(CSR_STIMECMP, tick_in_future);
}

void timer_irq()
{
	uptime_seconds++;

    if (alarm_active && uptime_seconds >= alarm_target) {
        printk(LOG_INFO, "\nalarm\r\n"); 
    alarm_active = 0;
    }

    // ativa novamente o timer para disparar novamente no proximo seg 
    timer_set_alarm(1);
}
