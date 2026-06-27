#include "stim/util_bot/error_decomp.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

using namespace stim;

void stim::independent_to_disjoint_xyz_errors(
    double x, double y, double z, double *out_x, double *out_y, double *out_z) {
    if (x < 0 || y < 0 || z < 0 || x > 1 || y > 1 || z > 1) {
        throw std::invalid_argument("x < 0 || y < 0 || z < 0 || x > 1 || y > 1 || z > 1");
    }
    double ab = x * y;
    double ac = x * z;
    double bc = y * z;
    double a_i = 1.0 - x;
    double b_i = 1.0 - y;
    double c_i = 1.0 - z;
    double ab_i = a_i * b_i;
    double ac_i = a_i * c_i;
    double bc_i = b_i * c_i;
    *out_x = x * bc_i + a_i * bc;
    *out_y = y * ac_i + b_i * ac;
    *out_z = z * ab_i + c_i * ab;
}

bool stim::try_disjoint_to_independent_xyz_errors_approx(
    double x, double y, double z, double *out_x, double *out_y, double *out_z, size_t max_steps) {
    if (x < 0 || y < 0 || z < 0 || x + y + z > 1) {
        throw std::invalid_argument("x < 0 || y < 0 || z < 0 || x + y + z > 1");
    }

    // Re-arrange the problem so identity is the most likely case.
    double i = std::max(0.0, 1.0 - x - y - z);
    if (i < x) {
        auto result = try_disjoint_to_independent_xyz_errors_approx(i, z, y, out_x, out_y, out_z, max_steps);
        *out_x = 1 - *out_x;
        return result;
    }
    if (i < y) {
        auto result = try_disjoint_to_independent_xyz_errors_approx(z, i, x, out_x, out_y, out_z, max_steps);
        *out_y = 1 - *out_y;
        return result;
    }
    if (i < z) {
        auto result = try_disjoint_to_independent_xyz_errors_approx(y, x, i, out_x, out_y, out_z, max_steps);
        *out_z = 1 - *out_z;
        return result;
    }

    // Solve analytically if an exact solution exists.
    if (x + z < 0.5 && x + y < 0.5 && y + z < 0.5) {
        double s_xz = sqrt(1 - 2 * x - 2 * z);
        double s_xy = sqrt(1 - 2 * x - 2 * y);
        double s_yz = sqrt(1 - 2 * y - 2 * z);
        double a = 0.5 - 0.5 * s_xz * s_xy / s_yz;
        double b = 0.5 - 0.5 * s_xy * s_yz / s_xz;
        double c = 0.5 - 0.5 * s_xz * s_yz / s_xy;
        if (a >= 0 && b >= 0 && c >= 0) {
            *out_x = a;
            *out_y = b;
            *out_z = c;
            return true;
        }
    }

    // If no exact solution exists, resort to approximations.
    double a = x;
    double b = y;
    double c = z;
    for (size_t step = 0; step < max_steps; step++) {
        // Compute current error.
        double ab = a * b;
        double ac = a * c;
        double bc = b * c;
        double a_i = 1.0 - a;
        double b_i = 1.0 - b;
        double c_i = 1.0 - c;
        double ab_i = a_i * b_i;
        double ac_i = a_i * c_i;
        double bc_i = b_i * c_i;
        double x2 = a * bc_i + a_i * bc;
        double y2 = b * ac_i + b_i * ac;
        double z2 = c * ab_i + c_i * ab;
        double dx = x2 - x;
        double dy = y2 - y;
        double dz = z2 - z;
        double err = fabs(dx) + fabs(dy) + fabs(dz);
        if (err < 1e-14) {
            // Good enough.
            *out_x = a;
            *out_y = b;
            *out_z = c;
            return true;
        }

        // Make a Newton-Raphson step towards the solution.
        double da = bc_i - bc;
        double db = ac_i - ac;
        double dc = ab_i - ac;
        a -= dx / da;
        b -= dy / db;
        c -= dz / dc;
        a = std::max(a, 0.0);
        b = std::max(b, 0.0);
        c = std::max(c, 0.0);
    }
    *out_x = a;
    *out_y = b;
    *out_z = c;
    return false;
}

double stim::depolarize1_probability_to_independent_per_channel_probability(double p) {
    if (p > 0.75) {
        throw std::invalid_argument(
            "depolarize1_probability_to_independent_per_channel_probability with p>0.75; p=" + std::to_string(p));
    }
    return 0.5 - 0.5 * sqrt(1 - (4 * p) / 3);
}

double stim::depolarize2_probability_to_independent_per_channel_probability(double p) {
    if (p > 0.9375) {
        throw std::invalid_argument(
            "depolarize2_probability_to_independent_per_channel_probability with p>15.0/16.0; p=" + std::to_string(p));
    }
    return 0.5 - 0.5 * pow(1 - (16 * p) / 15, 0.125);
}

double stim::independent_per_channel_probability_to_depolarize1_probability(double p) {
    double q = 1.0 - 2.0 * p;
    q *= q;
    return 3.0 / 4.0 * (1.0 - q);
}

double stim::independent_per_channel_probability_to_depolarize2_probability(double p) {
    double q = 1.0 - 2.0 * p;
    q *= q;
    q *= q;
    q *= q;
    return 15.0 / 16.0 * (1.0 - q);
}
