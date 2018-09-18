#include <chrono>
#include <iostream>
#include "lime/LimeSuite.h"
#include "reciever_setup.h"

using namespace std;

// g++ main.cpp reciever_setup.cpp -std=c++11 -lLimeSuite -o a.out

/* Entry Point */
int main(int argc, char** argv){

    /* Hardware Config Parameters */
    reciever_configuration config;
    config.centre_frequency = 868e6;                // RF Center Freuency
    config.sample_rate = 30.72e6;                   // Sample Rate 
    config.oversample_ratio = 4;                    // ADC Oversample Ratio
    config.antenna = LMS_PATH_LNAW;                 // RF Path
    config.rx_gain = 0.7;                           // Normalised Gain - 0 to 1.0
    config.LPF_bandwidth = 8e6;                     // RX Analog Low Pass Filter Bandwidth
    config.cal_bandwidth = 8e6;                     // Automatic Calibration Bandwidth

    /* Setup SDR */
    if (configure_reciever(config) != 0)
        return -1;
    

    /* Stream Config Parameters */
    lms_stream_t streamId;
    streamId.channel = 0;                           // Channel Number
    streamId.fifoSize = 1020 * 1020;                // Fifo Size in Samples
    streamId.throughputVsLatency = 1.0;             // Optimize Throughput (1.0) or Latency (0)
    streamId.isTx = false;                          // TX/RX Channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_I16;   // Data Format

    /* Setup Stream */
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    /* Data Buffers - Interleaved IQIQIQ...*/
    const int bufersize = 1020;                     // Complex Samples per Buffer
    float buffer[bufersize * 2];                    // Buffer holds I+Q values of each sample IQIQIQIQ...

    /* Book Keeping Indicies */

    
    /* Start streaming */
    LMS_StartStream(&streamId);

    /* Process Stream for 20s */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(20)){
        
        int samplesRead;
        uint64_t pps_idx;
        uint64_t prev_idx;
        uint64_t delta;
        lms_stream_meta_t meta_data;
        
        samplesRead = LMS_RecvStream(&streamId, buffer, bufersize, &meta_data, 1000);
       
        /* Test for PPS Sync */
        if((meta_data.timestamp & 0x8000000000000000) == 0x8000000000000000){
            
            /* Update PPS Index */
            prev_idx = pps_idx;
            pps_idx = meta_data.timestamp + 0x8000000000000000;

            /* Compute Delta */
            if(pps_idx != prev_idx){
                delta = pps_idx - prev_idx;
                cout << "Delta:    " << delta << endl;
            }
        }    
    }

    /* Stop Streaming (Start again with LMS_StartStream()) */
    LMS_StopStream(&streamId);
    
    /* Destroy Stream */
    LMS_DestroyStream(device, &streamId);

    /* Close Device */
    LMS_Close(device);

    return 0;
}