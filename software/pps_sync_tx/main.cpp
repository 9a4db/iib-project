#include <ctime>
#include <chrono>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include "string.h"
#include "lime/LimeSuite.h"
#include "tranciever_setup.h"

using namespace std;

// g++ main.cpp tranciever_setup.cpp -std=c++11 -lLimeSuite -o tx-pps.out

/* Entry Point */
int main(int argc, char** argv){

    /* Hardware Config */
    tranciever_configuration config;
    config.rx_centre_frequency = 868e6;             // RX Center Freuency    
    config.rx_antenna = LMS_PATH_LNAW;              // RX RF Path =ls 10MHz - 2GHz
    config.rx_gain = 0.7;                           // RX Normalised Gain - 0 to 1.0
    config.enable_rx_LPF = true;                    // Enable RX Low Pass Filter
    config.rx_LPF_bandwidth = 8e6;                  // RX Analog Low Pass Filter Bandwidth
    config.enable_rx_cal = true;                    // Enable RX Calibration
    config.rx_cal_bandwidth = 8e6;                  // Automatic Calibration Bandwidth

    config.tx_centre_frequency = 868e6;             // TX Center Freuency
    config.tx_antenna = LMS_PATH_TX2;               // TX RF Path = 10MHz - 2GHz
    config.tx_gain = 0.4;                           // TX Normalised Gain - 0 to 1.0
    config.enable_tx_LPF = true;                    // Enable TX Low Pass Filter
    config.tx_LPF_bandwidth = 8e6;                  // TX Analog Low Pass Filter Bandwidth
    config.enable_tx_cal = true;                    // Enable TX Calibration
    config.tx_cal_bandwidth = 8e6;                  // Automatic Calibration Bandwidth

    config.sample_rate = 30.72e6;                   // Device Sample Rate 
    config.rf_oversample_ratio = 4;                 // ADC Oversample Ratio

    configure_tranciever(config);

    /* TX Stream Config */
    lms_stream_t tx_stream;
    tx_stream.channel = 0;                           // Channel Number
    tx_stream.fifoSize = 1024 * 256;                 // Fifo Size in Samples
    tx_stream.throughputVsLatency = 1.0;             // Optimize Throughput (1.0) or Latency (0)
    tx_stream.isTx = true;                           // TX/RX Channel
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_I16;   // Data Format - int16_t
    LMS_SetupStream(device, &tx_stream);

    /* TX Data Buffer */
    const int num_tx_samples = 1024 * 8;
    const int tx_buffer_size = num_tx_samples * 2;
    int16_t tx_buffer[tx_buffer_size];

    /* TX Stream Metadata */
    lms_stream_meta_t tx_metadata;
    tx_metadata.flushPartialPacket = false;          // Dont force sending of incomplete packets
    tx_metadata.waitForTimestamp = false;            // Disable synchronization to HW timestamp



    /* Generate Test Signal */
    const double tone_freq = 1e6;
    for (int i = 0; i <num_tx_samples; i++){
        const double pi = acos(-1);
        double w = 2*pi*i*tone_freq/config.sample_rate;
        tx_buffer[2*i] = (int16_t)(cos(w)*32767.0f);
        tx_buffer[2*i+1] = (int16_t)(sin(w)*32767.0f);
    }

    
    /* Start Streams */
    LMS_StartStream(&tx_stream);
    
    
    
    /* Run for 10s */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(10)){
        
        /* Transmit Samples */
        if (LMS_SendStream(&tx_stream, tx_buffer, num_tx_samples, &tx_metadata, 1000) != num_tx_samples)
            error();
        
        /* Print Stats Every Second */
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(1)){
            t2 = chrono::high_resolution_clock::now();
            lms_stream_status_t status;
            LMS_GetStreamStatus(&tx_stream, &status);
            cout << "\nTX data rate: " << status.linkRate / 1e6 << " MB/s\n";
            cout << "Dropped TX packets: " << status.droppedPackets << endl;
        }
    }


    
    /* Stop Streaming */
    LMS_StopStream(&tx_stream);
    
    /* Destroy Stream */
    LMS_DestroyStream(device, &tx_stream);    
    
    /* Disable TX/RX Channels */
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, false)!=0)
        error();
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, false)!=0)
        error();

    /* Close Device */
    if (LMS_Close(device)==0)
        cout << "Device closed" << endl;
    return 0;
}
