#ifndef __PERFCTR_H
#define __PERFCTR_H

/* fire an interrupt this often */
#define __PERFCTR_CYCLES 50000

extern unsigned long __perfctr;
void __perfctr_init (void);
void __perfctr_start (void);
void __perfctr_stop (void);
void __perfctr_TA0_isr (void);

#endif
