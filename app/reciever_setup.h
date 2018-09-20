#ifndef RECIEVER_SETUP_H
#define RECIEVER_SETUP_H

#include "lime/LimeSuite.h"
using namespace std;

class reciever_configuration {
    public:
        float_type centre_frequency;
        size_t antenna;
        float_type sample_rate;
        int oversample_ratio;
        float_type rx_gain;
        float_type LPF_bandwidth;
        double cal_bandwidth;
};

class file_header {
    public:
        time_t unix_stamp;
        uint64_t buffer_index;
        uint64_t pps_index;
};

/* Device Structure */
extern lms_device_t* device;

/* Reciever Setup */
int configure_reciever(reciever_configuration rx_config);

/* Error Handler */
int error();

#endif