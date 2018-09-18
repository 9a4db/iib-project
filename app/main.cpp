#include <chrono>
#include <iostream>
#include <stdio.h>
#include "string.h"
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
    config.oversample_ratio = 2;                    // ADC Oversample Ratio
    config.antenna = LMS_PATH_LNAW;                 // RF Path
    config.rx_gain = 0.6;                           // Normalised Gain - 0 to 1.0
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
    const int bufersize = 1020;
    int16_t curr_buffer[bufersize*2];

    /* Book Keeping Indicies */
    uint64_t curr_buff_idx = 0;
    uint64_t pps_sync_idx = 0;
    uint64_t prev_pps_sync_idx = 0;
    uint64_t sync_offset = 0;
    
    /* Open File */
    FILE * data_file;
    data_file = fopen("samples.bin", "wb");

    /* Start streaming */
    LMS_StartStream(&streamId);

    /* Process Stream for 20s */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(10)){
        
        lms_stream_meta_t meta_data;

        /* Read 1020 Samples into Buffer */
        if(LMS_RecvStream(&streamId, curr_buffer, bufersize, &meta_data, 1000) != bufersize){
            LMS_StopStream(&streamId);
            LMS_DestroyStream(device, &streamId);
            error();
        };

        /* Check PPS Sync Flag - MSB Set */
        if((meta_data.timestamp & 0x8000000000000000) == 0x8000000000000000){
            
            /* Extract PPS Sync Index  - Clear MSB */
            prev_pps_sync_idx = pps_sync_idx;
            pps_sync_idx = meta_data.timestamp ^ 0x8000000000000000;
            curr_buff_idx += bufersize;
            
            /* Check for Double Fire */
            if (pps_sync_idx != prev_pps_sync_idx){
                
                sync_offset = pps_sync_idx - curr_buff_idx;
    
                cout << "\nCurrent buffer contains samples " << curr_buff_idx << " to " << curr_buff_idx + bufersize - 1 << endl;
                cout << "PPS sync occured at sample " << pps_sync_idx << endl;
                cout << "Offset = " << sync_offset << endl;

                fwrite(curr_buffer , sizeof(int16_t), bufersize*2, data_file);               
            }
        } else {
            curr_buff_idx = meta_data.timestamp;
        }
    }

    /* Stop Streaming (Start again with LMS_StartStream()) */
    LMS_StopStream(&streamId);
    
    /* Destroy Stream */
    LMS_DestroyStream(device, &streamId);

    /* Close Device */
    LMS_Close(device);

    /* Close File */
    fclose(data_file);

    return 0;
}