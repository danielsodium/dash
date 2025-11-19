#ifndef _STATUS_H_
#define _STATUS_H_


typedef struct {
    int initalized;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CPUStats;

typedef struct {
    char name[8];
    char value[8];
    int bars;
    int percents[8];
} Box;

char* print_battery(char* s);
char* print_mem(char* s);
char* print_cpu(char* s);
/*
char* print_bright(char* s);
char* print_vol(char* s);



void battery_status(Monitor* m, char* output);
void cpu_status(Monitor* m, char* out, CPUStats* prev);
void mem_status(Monitor* m, char* output);
void bright_status(Monitor*m, char* output);
void vol_status(Monitor*m, char* output);

// UNUSED
void wifi_status(char* output);
void bt_status(char* output);
*/

#endif
