#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
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
    streamId.dataFmt = lms_stream_t::LMS_FMT_I16;   // Data Format - int16_t

    /* Setup Stream */
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    /* Data Buffer - Interleaved IQIQIQ...*/
    const int num_samples = 1020;
    const int buffer_size = num_samples*2;
    int16_t data_buffer[buffer_size];

    /* Book Keeping Indicies */
    uint64_t curr_buff_idx = 0;
    uint64_t pps_sync_idx = 0;
    uint64_t prev_pps_sync_idx = 0;
    
    /* Output File */
    ofstream data_file;
    file_header file_metadata;
    string out_path = "data/";
    const int file_length = 12 + 1;                 // 12 Buffers at 30.72 MS/s = 400 us 

    /* Start streaming */
    LMS_StartStream(&streamId);

    /* Process Stream for 1s */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(5)){
        
        /* Stream Metadata */
        lms_stream_meta_t meta_data;

        /* Read Samples into Buffer */
        if(LMS_RecvStream(&streamId, data_buffer, num_samples, &meta_data, 1000) != num_samples){
            LMS_StopStream(&streamId);
            LMS_DestroyStream(device, &streamId);
            error();
        };

        /* Check PPS Sync Flag - MSB Set */
        if((meta_data.timestamp & 0x8000000000000000) == 0x8000000000000000){
            
            /* Extract PPS Sync Index - Clear MSB */
            prev_pps_sync_idx = pps_sync_idx;
            pps_sync_idx = meta_data.timestamp ^ 0x8000000000000000;
            curr_buff_idx += num_samples;
            
            /* Ignore Repeated Timestamp */
            if (pps_sync_idx != prev_pps_sync_idx){
                
                /* UNIQUE PPS EVENT DETCETED */

                /* Generate Header */
                file_metadata.unix_stamp = std::time(NULL);
                file_metadata.buffer_index = curr_buff_idx;
                file_metadata.pps_index = pps_sync_idx;
                
                /* Open Unique File & Write Header */
                data_file.open(out_path + to_string(file_metadata.unix_stamp) + ".bin", std::ofstream::binary);
                data_file.write((char*)&file_metadata, sizeof(file_metadata));

                /* Write Current Buffer to File */
                data_file.write((char*)data_buffer, sizeof(data_buffer));

                /* Stream to File Following PPS Event */
                for(int k=1; k<file_length; k++){

                    /* Read Samples into Buffer */
                    if(LMS_RecvStream(&streamId, data_buffer, num_samples, &meta_data, 1000) != num_samples){
                        LMS_StopStream(&streamId);
                        LMS_DestroyStream(device, &streamId);
                        error();
                    };
                    
                    /* Write Samples to File */
                    data_file.write((char*)data_buffer, sizeof(data_buffer));
                }

                /* Close File */
                data_file.close();

                /* Debug Output */
                cout << "\nTime: " << file_metadata.unix_stamp << endl;
                cout << "File begins with sample " << file_metadata.buffer_index << endl;
                cout << "PPS sync occured at sample " << file_metadata.pps_index << endl;
                cout << "Offset = " << file_metadata.pps_index - file_metadata.buffer_index << endl;     
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

    return 0;
}