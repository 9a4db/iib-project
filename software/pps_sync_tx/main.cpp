#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "string.h"
#include "lime/LimeSuite.h"
#include "tranciever_setup.h"

using namespace std;

// g++ main.cpp tranciever_setup.cpp -std=c++11 -lLimeSuite -o tx-pps.out

/* Entry Point */
int main(int argc, char** argv){

    /* Hardware Config Parameters */
    tranciever_configuration config;
    config.rx_centre_frequency = 107.8e6;           // RX Center Freuency    
    config.rx_antenna = LMS_PATH_LNAW;              // RX RF Path =ls 10MHz - 2GHz
    config.rx_gain = 0.7;                           // RX Normalised Gain - 0 to 1.0
    config.enable_rx_LPF = true;                    // Enable RX Low Pass Filter
    config.rx_LPF_bandwidth = 8e6;                  // RX Analog Low Pass Filter Bandwidth
    config.enable_rx_cal = true;                    // Enable RX Calibration
    config.rx_cal_bandwidth = 8e6;                  // Automatic Calibration Bandwidth

    config.tx_centre_frequency = 107.8e6;           // TX Center Freuency
    config.tx_antenna = LMS_PATH_TX2;               // TX RF Path = 10MHz - 2GHz
    config.tx_gain = 0.4;                           // TX Normalised Gain - 0 to 1.0
    config.enable_tx_LPF = true;                    // Enable TX Low Pass Filter
    config.tx_LPF_bandwidth = 8e6;                  // TX Analog Low Pass Filter Bandwidth
    config.enable_tx_cal = true;                    // Enable TX Calibration
    config.tx_cal_bandwidth = 8e6;                  // Automatic Calibration Bandwidth

    config.sample_rate = 30.72e6;                   // Device Sample Rate 
    config.rf_oversample_ratio = 4;                 // ADC Oversample Ratio

    /* Setup SDR */
    if (configure_tranciever(config) != 0)
        return -1;

    /* Close Device */
    LMS_Close(device);

    return 0;
}
