#include "alert.h"

const char *alert_level_to_string(AlertLevel level)
{
    switch (level) {
        case ALERT_NORMAL:
            return "NORMAL";

        case ALERT_WARNING:
            return "WARNING";

        case ALERT_CRITICAL:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}

AlertLevel alert_evaluate_percentage(
    double value,
    double warning_threshold,
    double critical_threshold
)
{
    if (value >= critical_threshold) {
        return ALERT_CRITICAL;
    }

    if (value >= warning_threshold) {
        return ALERT_WARNING;
    }

    return ALERT_NORMAL;
}

AlertLevel alert_get_higher_level(
    AlertLevel first,
    AlertLevel second
)
{
    if (first >= second) {
        return first;
    }

    return second;
}
