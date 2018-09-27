#ifndef RECIEVER_SETUP_H
#define RECIEVER_SETUP_H

#include "lime/LimeSuite.h"
using namespace std;

class reciever_configuration {
    public:
        float_type rx_centre_frequency;
        size_t rx_antenna;        
        float_type rx_gain;
        bool enable_rx_LPF;
        float_type rx_LPF_bandwidth;
        bool enable_rx_cal;
        double rx_cal_bandwidth;
        float_type sample_rate;
        int rf_oversample_ratio;
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