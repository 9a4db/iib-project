#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "string.h"
#include "lime/LimeSuite.h"
#include "reciever_setup.h"

using namespace std;

// g++ main.cpp reciever_setup.cpp -std=c++11 -lLimeSuite -o pps-rx.out

/* Entry Point */
int main(int argc, char** argv){

    /* Hardware Config */
    reciever_configuration config;
    config.rx_centre_frequency = 868e6;                 // RX Center Freuency    
    config.rx_antenna = LMS_PATH_LNAW;                  // RX RF Path = 10MHz - 2GHz
    config.rx_gain = 0.7;                               // RX Normalised Gain - 0 to 1.0
    config.enable_rx_LPF = true;                        // Enable RX Low Pass Filter
    config.rx_LPF_bandwidth = 10e6;                     // RX Analog Low Pass Filter Bandwidth
    config.enable_rx_cal = true;                        // Enable RX Calibration
    config.rx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth
    
    config.sample_rate = 30.72e6;                       // Sample Rate 
    config.rf_oversample_ratio = 4;                     // ADC Oversample Ratio
    
    configure_reciever(config);
    
    /* Enable Test Signal */
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NCODIV8, 0, 0) != 0)
        error();

    /* RX Stream Config  */
    lms_stream_t rx_stream;
    rx_stream.channel = 0;                              // Channel Number
    rx_stream.fifoSize = 1360 * 4096;                   // Fifo Size in Samples
    rx_stream.throughputVsLatency = 1.0;                // Optimize Throughput (1.0) or Latency (0)
    rx_stream.isTx = false;                             // TX/RX Channel
    rx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;      // Data Format - 12-bit sample stored as int16_t
    LMS_SetupStream(device, &rx_stream);

    /* RX Data Buffer */
    const int num_rx_samples = 1360;
    const int rx_buffer_size = num_rx_samples * 2;
    int16_t rx_buffer[rx_buffer_size];
    
    /* RX Stream Metadata */
    lms_stream_meta_t rx_metadata;

    /* Book Keeping Indicies */
    uint64_t curr_buff_idx = 0;
    uint64_t pps_sync_idx = 0;
    uint64_t prev_pps_sync_idx = 0;
    
    /* Output File */
    ofstream data_file;
    file_header file_metadata;
    const string out_path = "data/";
    
    /* Output Buffer */
    const int file_length = 12 + 1;
    int16_t file_buffer[rx_buffer_size * file_length];


    /* Start streaming */
    LMS_StartStream(&rx_stream);

    /* Process Stream for 5s */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(15)){

        /* Read Samples into Buffer */
        if(LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000) != num_rx_samples){
            LMS_StopStream(&rx_stream);
            LMS_DestroyStream(device, &rx_stream);
            error();
        };

        /* Check PPS Sync Flag - MSB Set */
        if((rx_metadata.timestamp & 0x8000000000000000) == 0x8000000000000000){
            
            /* Extract PPS Sync Index - Clear MSB */
            prev_pps_sync_idx = pps_sync_idx;
            pps_sync_idx = rx_metadata.timestamp ^ 0x8000000000000000;
            curr_buff_idx += num_rx_samples;
            
            /* Ignore Repeated Timestamp */
            if (pps_sync_idx != prev_pps_sync_idx){
                
                /* UNIQUE PPS EVENT DETCETED */

                /* Generate Header */
                file_metadata.unix_stamp = std::time(NULL);
                file_metadata.buffer_index = curr_buff_idx;
                file_metadata.pps_index = pps_sync_idx;                

                /* Save Current Buffer */
                memcpy(file_buffer, rx_buffer, sizeof(rx_buffer));

                /* Save Subsequent 12 Buffers */
                for(int k=1; k<file_length; k++){

                    /* Read Samples into Buffer */
                    if(LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000) != num_rx_samples){
                        LMS_StopStream(&rx_stream);
                        LMS_DestroyStream(device, &rx_stream);
                        error();
                    };
        
                    memcpy(&file_buffer[rx_buffer_size * k], rx_buffer, sizeof(rx_buffer));
                    curr_buff_idx += num_rx_samples;
                }

                /* Write to File */
                data_file.open(out_path + to_string(file_metadata.unix_stamp) + ".bin", std::ofstream::binary);
                data_file.write((char*)&file_metadata, sizeof(file_metadata));
                data_file.write((char*)file_buffer, sizeof(file_buffer));
                data_file.close();

                /* Debug Output */
                cout << "\nTime: " << file_metadata.unix_stamp << endl;
                cout << "File begins with sample " << file_metadata.buffer_index << endl;
                cout << "PPS sync occured at sample " << file_metadata.pps_index << endl;
                cout << "Samples since last PPS = " << pps_sync_idx - prev_pps_sync_idx << endl; 
                cout << "Sync event offset = " << file_metadata.pps_index - file_metadata.buffer_index << endl;     
            }
        } else {
            curr_buff_idx = rx_metadata.timestamp;
        }
    }

    /* Stop Streaming */
    LMS_StopStream(&rx_stream);
    
    /* Destroy Stream */
    LMS_DestroyStream(device, &rx_stream);

    /* Disable RX Channel */
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, false)!=0)
        error();

    /* Close Device */
    LMS_Close(device);

    return 0;
}