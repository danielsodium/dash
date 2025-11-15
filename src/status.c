#include "status.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <libnm/NetworkManager.h>

float calculate_cpu_usage(CPUStats *prev, CPUStats *curr) {
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long curr_idle = curr->idle + curr->iowait;

    unsigned long long prev_total = prev->user + prev->nice + prev->system + prev->idle +
                                    prev->iowait + prev->irq + prev->softirq + prev->steal;
    unsigned long long curr_total = curr->user + curr->nice + curr->system + curr->idle +
                                    curr->iowait + curr->irq + curr->softirq + curr->steal;

    unsigned long long delta_total = curr_total - prev_total;
    unsigned long long delta_idle = curr_idle - prev_idle;

    if (delta_total == 0) return 0.0;

    return 100.0f * (float)(delta_total - delta_idle) / delta_total;
}


// Formats KB
static void mem_format(unsigned long val, char* output) {
    // gb
    if (val > 1048576ul) {
        sprintf(output, "%.1fG", (float) val / (1024.0f * 1024.0f));
        return;
    }
    // mb
    else if (val > 1024ul) {
        sprintf(output, "%.1fM", (float) val / 1024.0f);
        return;
    }
    // kb
    sprintf(output, "%luK", val);
}

static char* print_box(Box* b, char* s) {
    int i, j;
    int increments;

    s += sprintf(s, "%s%*s%s\n", 
                 b->name, 
                 STR_WIDTH - strlen(b->name) - strlen(b->value), "", 
                 b->value);

    // BAR
    increments = 100 / (STR_WIDTH - 2);
    for (i = 0; i < b->bars; i++) {
        s = strcat(s, "[") + 1;

        for (j = 0; j < STR_WIDTH - 2; j++) {
            if (j * increments < b->percents[i])
                s = strcat(s,"\xE2\x96\x88") + 3;             
            else 
                s = strcat(s, " ") + 1;
        }

        s = strcat(s, "]\n") + 2;
    }
    *s = '\0';
    return s;
}
 
char* print_battery(char* s) {
    FILE* fp;
    Box b;
    int power, charge;
    unsigned long current, voltage;

    // Get system data
    fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    fscanf(fp, "%d", &charge);
    fclose(fp);
    fp = fopen("/sys/class/power_supply/BAT0/current_now", "r");
    fscanf(fp, "%lu", &current);
    fclose(fp);
    fp = fopen("/sys/class/power_supply/BAT0/voltage_now", "r");
    fscanf(fp, "%lu", &voltage);
    fclose(fp);
    power = ((current / 1e6) * (voltage / 1e6) - 5) * 2;

    // ORGANIZATION
    strcpy(b.name, "BAT");
    sprintf(b.value, "%d%%", charge);
    b.bars = 2;
    b.percents[0] = power;
    b.percents[1] = charge;

    return print_box(&b, s);
}


char* print_mem(char* s) {
    Box b;
    FILE* fp;
    unsigned long mem_total = 0, mem_available = 0, mem_used;
    unsigned long buf;
    char label[64];

    fp = fopen("/proc/meminfo", "r");
    while (fscanf(fp, "%63s %lu %*s", label, &buf) == 2) {
        if (strcmp(label, "MemTotal:") == 0)
            mem_total = buf;
        else if (strcmp(label, "MemAvailable:") == 0)
            mem_available = buf;
        if (mem_total && mem_available)
            break;
    }
    fclose(fp);
    mem_used = (mem_total - mem_available);

    strcpy(b.name, "RAM");
    mem_format(mem_used, b.value);
    b.bars = 1;
    b.percents[0] = 100 * mem_used / mem_total;

    return print_box(&b, s);
}

char* print_cpu(char* s) {
    static CPUStats prev = { .initalized = 0 };
    Box b;
    FILE* fp;
    CPUStats curr;
    int percent, temp;

    fp = fopen("/sys/class/thermal/thermal_zone3/temp", "r");
    fscanf(fp, "%d", &temp);
    fclose(fp);

    fp = fopen("/proc/stat", "r");

    char cpu_label[5];
    fscanf(fp, "%s %llu %llu %llu %llu %llu %llu %llu %llu",
                     cpu_label,
                     &curr.user,
                     &curr.nice,
                     &curr.system,
                     &curr.idle,
                     &curr.iowait,
                     &curr.irq,
                     &curr.softirq,
                     &curr.steal);

    fclose(fp);

    percent = prev.initalized ? calculate_cpu_usage(&prev, &curr) : 0;
    prev.initalized = 1;
    prev.user = curr.user;
    prev.nice = curr.nice;
    prev.system = curr.system;
    prev.idle = curr.idle;
    prev.iowait = curr.iowait;
    prev.irq = curr.irq;
    prev.softirq = curr.softirq;
    prev.steal = curr.steal;

    // Formatting temp to 30 - 80? -> 0 - 100
    strcpy(b.name, "CPU");
    sprintf(b.value, "%d%%", percent);
    b.bars = 2;
    b.percents[0] = percent;
    b.percents[1] = ((temp / 1000) - 30) * 2;

    return print_box(&b, s);
}


/*
void bright_status(Monitor* m, char* c) {
    FILE* fp;
    int current, max;
    int len;

    fp = fopen("/sys/class/backlight/intel_backlight/brightness", "r");
    fscanf(fp, "%d", &current);
    fclose(fp);
    
    fp = fopen("/sys/class/backlight/intel_backlight/max_brightness", "r");
    fscanf(fp, "%d", &max);
    fclose(fp);
    strcpy(m->name, "BRT");

    m->percent = 100 * ((float)current / (float)max);
    sprintf(m->value, "%d%%", m->percent);
    m->percent *= 5;

    monitor_update(m, c);
}

void vol_status(Monitor* m, char* c) {
    FILE* fp;
    float vol;

    fp = popen("wpctl get-volume @DEFAULT_AUDIO_SINK@", "r");
    fscanf(fp, "%*s %f", &vol);
    pclose(fp);

    vol *= 100;
    sprintf(m->value,"%.0f%%", vol);
    
    strcpy(m->name, "VOL");
    m->percent = (int) vol;

    monitor_update(m, c);
}

void wifi_status(char* output) {
    NMClient *client;
    const GPtrArray *devices;
    GError *error = NULL;
    char buffer[128];

    client = nm_client_new(NULL, &error);
    if (!client) {
        perror("Failed to connect to NetworkManager\n");
        g_error_free(error);
        return;
    }

    devices = nm_client_get_devices(client);
    for (guint i = 0; i < devices->len; i++) {
        NMDevice *device = g_ptr_array_index(devices, i);

        if (NM_IS_DEVICE_WIFI(device)) {
            NMAccessPoint *ap = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device));
            if (ap) {
                GBytes *ssid_bytes = nm_access_point_get_ssid(ap);
                if (ssid_bytes) {
                    gsize len;
                    const guint8 *ssid_data = g_bytes_get_data(ssid_bytes, &len);
                    memcpy(buffer, ssid_data, len);
                    buffer[len] = '\0';
                    buffer[7] = '\0';
                    strcpy(output, "WiFi ");
                    strcat(output, buffer);
                    g_object_unref(client);
                    return;
                }
            }
        }
    }

    strcpy(output, "WiFi None");
    g_object_unref(client);
    return;
}
*/
