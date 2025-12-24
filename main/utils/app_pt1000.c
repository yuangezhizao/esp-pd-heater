#include "app_pt1000.h"

#include <math.h>

double calculate_temperature_cvd(double resistance) {
    const double R0 = 1000.0;
    const double A = 3.9083e-3;
    const double B = -5.775e-7;
    const double C = -4.183e-12;

    if (!isfinite(resistance) || resistance <= 0.0) {
        return NAN;
    }

    // For T >= 0°C, CVD reduces to a quadratic and can be solved directly:
    // R = R0 * (1 + A*T + B*T^2)
    if (resistance >= R0) {
        const double a = B;
        const double b = A;
        const double c = 1.0 - (resistance / R0);

        const double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0.0) return NAN;
        const double sqrt_disc = sqrt(discriminant);

        // Choose the root that yields a valid temperature range.
        const double t1 = (-b + sqrt_disc) / (2.0 * a);
        const double t2 = (-b - sqrt_disc) / (2.0 * a);
        double t = (t1 >= 0.0 && t1 <= 850.0) ? t1 : t2;
        if (t < 0.0) t = 0.0;
        if (t > 850.0) t = 850.0;
        return t;
    }

    // For T < 0°C, use Newton-Raphson on full CVD equation:
    // R = R0 * (1 + A*T + B*T^2 + C*(T-100)*T^3)
    double t = -20.0; // reasonable starting point for ambient-range measurements
    for (int i = 0; i < 12; i++) {
        const double t2 = t * t;
        const double t3 = t2 * t;
        const double t4 = t2 * t2;
        const double r_est = R0 * (1.0 + A * t + B * t2 + C * (t4 - 100.0 * t3));
        const double f = r_est - resistance;
        const double df = R0 * (A + 2.0 * B * t + C * (4.0 * t3 - 300.0 * t2));
        if (fabs(df) < 1e-12) break;
        t -= f / df;
        if (t < -200.0) t = -200.0;
        if (t > 0.0) t = 0.0;
        if (fabs(f) < 1e-6) break;
    }
    return t;
}
