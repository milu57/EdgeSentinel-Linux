#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

typedef struct
{
    unsigned long long days;
    unsigned int hours;
    unsigned int minutes;
    unsigned int seconds;
} SystemUptime;

typedef struct
{
    double one_minute;
    double five_minutes;
    double fifteen_minutes;
} LoadAverage;

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} CurrentTime;

int get_system_uptime(
    unsigned long long *uptime_seconds
);

void convert_uptime(
    unsigned long long total_seconds,
    SystemUptime *uptime
);

int get_load_average(LoadAverage *load);

int get_current_time(CurrentTime *current_time);

#endif
