#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];
extern u64 uptime_seconds;
extern u64 alarm_target;
extern int alarm_active;

void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	hart_irq_enable(); 

    // iniciando o shell 
    char cmdbuf[128];         // buffer para armazenar o comando digitado pelo usuario
    int cmdlen = 0;           // tamanho atual do comando no buffer
    char rx_buf_temp[256];    // buffer temporario para armazenar os dados lidos da serial
    
    char lixo[256];
    serial_read(lixo);  
	printk(LOG_INFO, "> ");
    while (1) {
        // le os dados da serial para o buffer temporario
        size_t bytes_lidos = serial_read(rx_buf_temp);
        
        if (bytes_lidos == 0) {
            // se nao leu nada, entra em modo de espera  
            __asm__ volatile("wfi");
            continue;
        }

        // se leu algum dado, processa cada caractere
        for (size_t i = 0; i < bytes_lidos; i++) {
            char c = rx_buf_temp[i];
            
            if (cmdlen == 0 && (c == '\r' || c == '\n')) {
            // se o usuario apertou enter sem digitar nada, apenas imprima o prompt e ignore
            printk(LOG_INFO, "\n> ");
            continue;
            }

            // verifica se o caractere lido eh enter
            if (c == '\r' || c == '\n') {
                printk(LOG_INFO, "\r\n");
                cmdbuf[cmdlen] = '\0'; 

                // processa o comando digitado
                if (strncmp(cmdbuf, "uptime", 6) == 0) {
                    printk(LOG_INFO, "%llus\r\n", uptime_seconds);
                } 
                else if (strncmp(cmdbuf, "echo ", 5) == 0) {
                    printk(LOG_INFO, "%s\r\n", cmdbuf + 5); // imprime o que vem depois do "echo "
                } 
                else if (strncmp(cmdbuf, "alarm ", 6) == 0) {
                    // extrai o numero de segundos do comando alarm
                    u64 secs = 0;
                    for (int j = 6; cmdbuf[j] != '\0'; j++) {
                        if (cmdbuf[j] >= '0' && cmdbuf[j] <= '9') {
                            secs = secs * 10 + (cmdbuf[j] - '0');
                        }
                    }
                    // ativa o alarme para disparar apos secs segundos
                    alarm_target = uptime_seconds + secs;
                    alarm_active = 1;
                }
                //else if (cmdlen > 0) {
                //    //printk(LOG_INFO, "Comando desconhecido: %s\n", cmdbuf);
                //}

                // reseta o buffer do comando e imprime o prompt novamente
                cmdlen = 0;
                printk(LOG_INFO, "> ");
            } 
            else {
                // se nao for enter, adiciona o caractere ao buffer do comando
                if (cmdlen < 127) {
                    cmdbuf[cmdlen++] = c;
                }
            }
        }
    }
}