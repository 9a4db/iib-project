#ifndef TRANCIEVER_SETUP_H
#define TRANCIEVER_SETUP_H

#include "lime/LimeSuite.h"
using namespace std;

class tranciever_configuration {
    public:
        
        /* RX Parameters */
        float_type rx_centre_frequency;
        size_t rx_antenna;
        float_type rx_gain;
        bool enable_rx_LPF;
        float_type rx_LPF_bandwidth;
        bool enable_rx_cal;
        double rx_cal_bandwidth;

        /* TX Parameters */        
        float_type tx_centre_frequency;
        size_t tx_antenna;
        float_type tx_gain;
        bool enable_tx_LPF;
        float_type tx_LPF_bandwidth;
        bool enable_tx_cal;
        double tx_cal_bandwidth;

        /* Common Parameters */
        float_type sample_rate;
        int rf_oversample_ratio;
};

/* Output File Header */
class file_header {
    public:
        time_t unix_stamp;
        uint64_t buffer_index;
        uint64_t pps_index;
};

/* Device Structure */
extern lms_device_t* device;

/* Device Setup */
int configure_tranciever(tranciever_configuration tx_rx_config);

/* Error Handler */
int error();

#endif
