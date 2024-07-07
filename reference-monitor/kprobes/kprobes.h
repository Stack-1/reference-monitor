#define NUM_KRETPROBES 2


struct probe_data {
        char *error_message;
};


int kretprobe_init(void);
void kretprobe_clean(void);
void enable_kprobes(void);
void disable_kprobes(void);