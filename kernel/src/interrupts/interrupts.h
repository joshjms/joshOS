#pragma once

#include <stdint.h>

struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void irq_register_handler(int irq, void (*handler)(struct registers *));

void interrupt_handler(struct registers *regs);

void exception_handler(struct registers *regs);
