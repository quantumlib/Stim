#ifndef _STIM_UTIL_BOT_ERROR_DECOM_H
#define _STIM_UTIL_BOT_ERROR_DECOM_H

#include <cstddef>

namespace stim {

void independent_to_disjoint_xyz_errors(double x, double y, double z, double *out_x, double *out_y, double *out_z);
bool try_disjoint_to_independent_xyz_errors_approx(
    double x, double y, double z, double *out_x, double *out_y, double *out_z, size_t max_steps = 50);
double depolarize1_probability_to_independent_per_channel_probability(double p);
double depolarize2_probability_to_independent_per_channel_probability(double p);
double independent_per_channel_probability_to_depolarize1_probability(double p);
double independent_per_channel_probability_to_depolarize2_probability(double p);

}  // namespace stim

#endif
