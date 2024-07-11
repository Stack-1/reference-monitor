#ifndef KRET_MODULE
#define KRET_MODULE
#define NUM_KRETPROBES 10


struct probe_data {
        char *error_message;
};


int kretprobe_init(void);
void kretprobe_clean(void);
void enable_kprobes(void);
void disable_kprobes(void);
#endif