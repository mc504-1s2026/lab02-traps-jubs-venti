#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <arch/plic.h>

extern void trap_entry();
extern void timer_irq();
extern void serial_irq();

// SSTATUS_SIE eh o bit 1 do registrador sstatus, que habilita ou desabilita as interrupcoes de supervisor
#define SSTATUS_SIE (1ULL << 1)

void handle_irq()
{
	u64 scause = csr_read(CSR_SCAUSE);
    u64 exception_code = scause & ~(1ULL << 63);

    if (exception_code == 5) {
        // Interrupcao de temporizador do supervisor
        timer_irq();
    } else if (exception_code == 9) {
        // interrupcao do supervisor
        u32 irq = plic_hart_claim_irq(0);
        
        // ve se a interrupcao eh IRQ 10
        if (irq == 10) {
            serial_irq();
        }
        
        if (irq) {
            plic_hart_complete_irq(0, irq);
        }
    } else {
        printk(LOG_INFO, "[IRQ] Interrupcao nao tratada. Codigo: %llu\n", exception_code);
    }
}

void handle_exception()
{
	u64 scause = csr_read(CSR_SCAUSE);
    u64 exception_code = scause & ~(1ULL << 63);
    u64 stval = csr_read(CSR_STVAL);
    u64 sepc = csr_read(CSR_SEPC);
    
	printk(LOG_INFO, "KERNEL PANIC: Excecao nao tratada.\n");
    printk(LOG_INFO, "scause: %llu, stval: 0x%llx, sepc: 0x%llx\n", exception_code, stval, sepc);
    BUG();
}

void trap_setup()
{
	
    csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE);
    int is_interrupt = (scause >> 63) & 1;

    if (is_interrupt) {
        handle_irq();
    } else {
        handle_exception();
    }
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 old_sstatus = csr_read(CSR_SSTATUS);
    hart_irq_disable();
    return old_sstatus;
}

void hart_irq_restore(u64 flags)
{
	if (flags & SSTATUS_SIE) {
        hart_irq_enable();
    } else {
        hart_irq_disable();
    }
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, SSTATUS_SIE);
}
