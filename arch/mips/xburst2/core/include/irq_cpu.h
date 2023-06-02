#ifndef IRQ_CPU_H
#define IRQ_CPU_H

void ingenic_irq_migration(int lock);
void ingenic_irq_cpumask_idle(int idle);
#endif /* IRQ_CPU_H */
