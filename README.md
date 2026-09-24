# kfs-4

Interrupts for the KFS kernel (i386, C and NASM): an IDT, a signal API, a panic
that saves the stack and clears the registers, and a keyboard driven by IRQ 1.

## Build and run

    make          # kernel.bin + kfs.iso (needs gcc -m32, nasm, ld, grub-mkrescue, xorriso)
    make run      # boots the ISO in qemu-system-i386; KVM=1 make run for hardware acceleration
    make check    # multiboot header and ISO size

## Layout

| Path | Role |
|---|---|
| boot/isr.asm | One stub per vector (0..47 and 0x80), the common frame builder, the stub table |
| boot/halt.asm | `cpu_halt_clean`: clears every general register and stops the CPU |
| kernel/idt.c | Fills the 256 gates and runs `lidt` |
| kernel/isr.c | C dispatcher: exceptions, IRQ callbacks, syscalls |
| kernel/pic.c | Remaps the 8259 PICs to vectors 32..47, masks, end of interrupt |
| kernel/timer.c | PIT at 100 Hz on IRQ 0, tick counter |
| kernel/signal.c | Signal callbacks, immediate delivery, scheduled delivery |
| kernel/panic.c | Stack snapshot, register dump, halt |
| kernel/syscall.c | `int 0x80` with eax = number (bonus) |
| drivers/keyboard.c | IRQ 1 handler, character queue, qwerty/azerty, `get_line` (bonus) |

## How an interrupt travels

1. The CPU looks up the vector in the IDT and jumps to `isrN` in boot/isr.asm.
2. The stub pushes a fake error code if the CPU did not, then the vector number.
3. `isr_common` saves the registers with `pusha` and calls `interrupt_handler`
   with a pointer to that frame (`t_registers` in include/isr.h).
4. Exceptions raise the matching signal, then traps resume and faults panic.
   IRQs run the driver callback, then acknowledge the PIC.
5. `popa` and `iret` restore everything, including the interrupt flag.

## Signals

    signal_register(sig, handler)      install a callback
    signal_raise(sig)                  deliver now
    signal_schedule(sig, delay_ticks)  deliver from the main loop once the timer says so
    signal_dispatch_pending()          called by the main loop between two 'hlt'

Ctrl+C schedules SIGINT from the keyboard IRQ; the shell's handler drops the line.

## Panic

`panic(message)` disables interrupts, copies the top of the stack into a static
buffer (`panic_save_stack`), prints the message and the copy, then calls
`cpu_halt_clean`. `panic_exception` adds the register dump taken from the frame.

## Shell commands to try

`idt`, `uptime`, `signal`, `int3`, `div0`, `ud2`, `syscall`, `read`, `layout azerty`,
`panic`, `halt`. Alt+1..4 switches between the four virtual screens.
