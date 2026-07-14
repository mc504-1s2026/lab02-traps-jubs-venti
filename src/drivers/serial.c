#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/plic.h>
#include <arch/spinlock.h>
#include <arch/csr.h>
#include <kernel/types.h>

#define UART_BASE_VIRT 0xFFFFFFC010000000ULL 
#define UART_RBR 0 
#define UART_THR 0 
#define UART_IER 1 
#define UART_FCR 2 
#define UART_LSR 5 

#define UART_LSR_DATA_READY 0x01
#define UART_LSR_THR_EMPTY  0x20
#define SIE_SEIE (1ULL << 9)

#define SERIAL_BUF_SIZE 256
char rx_buf[SERIAL_BUF_SIZE];
int rx_head = 0; 
int rx_tail = 0; 
struct spinlock serial_lock = {0};

void serial_init()
{
	// habilita a interrupcao de recepcao de dados,,, bit 0 do IER
    *(volatile u8*)(UART_BASE_VIRT + UART_IER) = 0x01;
    
    // habilita FIFO e limpa os buffers,,, bit 0 e 1 do FCR
    *(volatile u8*)(UART_BASE_VIRT + UART_FCR) = 0x01;

    // habilita interrupcoes externas no PLIC para a porta serial
    plic_irq_set_priority(10, 1);      
    plic_hart_set_threshold(0, 0);     
    plic_hart_enable_irq(0, 10);       
}

void serial_irq_enable()
{
	csr_set(CSR_SIE, SIE_SEIE);
}

void serial_irq_disable()
{
	// desabilita a interrupcao de recepcao de dados no UART
    csr_clear(CSR_SIE, SIE_SEIE);
}

void serial_irq()
{
	// trava o acesso ao buffer circular para evitar corrupcao de dados
    spin_lock(&serial_lock);

    //verifica se o UART tem dados disponiveis para leitura
    while (*(volatile u8*)(UART_BASE_VIRT + UART_LSR) & UART_LSR_DATA_READY) {
        
        // le caract do RBR 
        char c = *(volatile u8*)(UART_BASE_VIRT + UART_RBR);
        
        // calcula a posicao do proximo head no buffer circular
        int next_head = (rx_head + 1) % SERIAL_BUF_SIZE;
        
        // salva o caractere no buffer circular se houver espaco
        if (next_head != rx_tail) {
            rx_buf[rx_head] = c;
            rx_head = next_head;
        }
    }

    // libera o acesso ao buffer circular apos a leitura dos dados
    spin_unlock(&serial_lock);
}

size_t serial_read(char *buf)
{
	size_t count = 0;
    
    // trava o acesso ao buffer circular para evitar corrupcao de dados
    spin_lock(&serial_lock);
    
    // escreve os dados do buffer circular para o buffer fornecido pelo usuario
    while (rx_head != rx_tail) {
        buf[count++] = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % SERIAL_BUF_SIZE;
    }
    
    spin_unlock(&serial_lock);
    
    return count;
}

void serial_puts(char *str)
{
	//  itera sobre cada caractere da string ate encontrar o terminador nulo \0
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_putc(char c)
{
	// espera o buffer de transmissao do UART estar vazio antes de enviar o caractere
    while ((*(volatile u8*)(UART_BASE_VIRT + UART_LSR) & UART_LSR_THR_EMPTY) == 0) {
        // espera ocupada (busy wait) ate o buffer de transmissao estar vazio
    }
    
    // escreve o caractere no registrador de transmissao do UART para enviar o caractere
    *(volatile u8*)(UART_BASE_VIRT + UART_THR) = c;
}
