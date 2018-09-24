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

// g++ main.cpp tranciever_setup.cpp -std=c++11 -lLimeSuite -o pps-tx-rx.out

/* Entry Point */
int main(int argc, char** argv){

    /* Hardware Config */
    tranciever_configuration config;
    config.rx_centre_frequency = 868e6;                 // RX Center Freuency    
    config.rx_antenna = LMS_PATH_LNAW;                  // RX RF Path =ls 10MHz - 2GHz
    config.rx_gain = 0.7;                               // RX Normalised Gain - 0 to 1.0
    config.enable_rx_LPF = true;                        // Enable RX Low Pass Filter
    config.rx_LPF_bandwidth = 10e6;                      // RX Analog Low Pass Filter Bandwidth
    config.enable_rx_cal = true;                        // Enable RX Calibration
    config.rx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth

    config.tx_centre_frequency = 868e6;                 // TX Center Freuency
    config.tx_antenna = LMS_PATH_TX2;                   // TX RF Path = 10MHz - 2GHz
    config.tx_gain = 0.4;                               // TX Normalised Gain - 0 to 1.0
    config.enable_tx_LPF = true;                        // Enable TX Low Pass Filter
    config.tx_LPF_bandwidth = 10e6;                      // TX Analog Low Pass Filter Bandwidth
    config.enable_tx_cal = true;                        // Enable TX Calibration
    config.tx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth

    config.sample_rate = 30.72e6;                       // Device Sample Rate 
    config.rf_oversample_ratio = 4;                     // ADC Oversample Ratio

    configure_tranciever(config);

    
    /* RX Stream Config  */
    lms_stream_t rx_stream;
    rx_stream.channel = 0;                              // Channel Number
    rx_stream.fifoSize = 1360 * 2048;                   // Fifo Size in Samples
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

    
    /* TX Stream Config */
    lms_stream_t tx_stream;
    tx_stream.channel = 0;                              // Channel Number
    tx_stream.fifoSize = 1360 * 256;                    // Fifo Size in Samples
    tx_stream.throughputVsLatency = 1.0;                // Optimize Throughput (1.0) or Latency (0)
    tx_stream.isTx = true;                              // TX/RX Channel
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;      // Data Format - 12-bit sample stored as int16_t
    LMS_SetupStream(device, &tx_stream);

    /* TX Data Buffer */
    const int num_tx_samples = 1360 * 8;
    const int tx_buffer_size = num_tx_samples * 2;
    int16_t tx_buffer[tx_buffer_size];

    /* TX Stream Metadata */
    lms_stream_meta_t tx_metadata;
    tx_metadata.flushPartialPacket = false;              // Force sending of incomplete packets
    tx_metadata.waitForTimestamp = true;                // Enable synchronization to HW timestamp

    /* Generate TX Test Signal */
    const double tone_freq = 1e6;
    for (int i = 0; i <num_tx_samples; i++){
        const double pi = acos(-1);
        double w = 2*pi*i*tone_freq/config.sample_rate;
        tx_buffer[2*i] = (int16_t)(cos(w)*2048.0f);
        tx_buffer[2*i+1] = (int16_t)(sin(w)*2048.0f);
    }

    /* Sync Counters */
    uint64_t curr_buff_idx = 0;
    uint64_t pps_sync_idx = 0;
    uint64_t prev_pps_sync_idx = 0;
    uint64_t cap_start_idx = 0;
    uint64_t tx_sync_offset = 1360 * 4500;              // TX Offset from PPS in Samples (400ms)


    /* Output File */
    ofstream data_file;
    file_header file_metadata;
    string out_path = "data/";                          // Output Directory
    const int capture_duration = 250;                    // Output File Length in Buffers
    int16_t file_buffer[capture_duration * rx_buffer_size];

    /* Start Streams */
    LMS_StartStream(&tx_stream);
    LMS_StartStream(&rx_stream);

    /* Process Stream */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(10)){
        
        /* Recieve Buffer of 1360 Samples */
        if (LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000) != num_rx_samples){
            cout << "RX Stream Error" << endl;
            error();
        }

        /* Check PPS Sync Flag - MSB Set */
        if((rx_metadata.timestamp & 0x8000000000000000) == 0x8000000000000000){
            
            /* Extract PPS Sync Index - Clear MSB */
            prev_pps_sync_idx = pps_sync_idx;
            pps_sync_idx = rx_metadata.timestamp ^ 0x8000000000000000;
            curr_buff_idx += num_rx_samples;
            
            /* Ignore Repeated Timestamp */
            if (pps_sync_idx != prev_pps_sync_idx){
                
                /* Schedule TX Event */
                tx_metadata.timestamp = pps_sync_idx + tx_sync_offset;
                if (LMS_SendStream(&tx_stream, tx_buffer, num_tx_samples, &tx_metadata, 1000) != num_tx_samples){
                    cout << "TX Stream Error" << endl;
                    error();
                }

                /* Schedule Capture to Start One Buffer Before TX */
                //cap_start_idx = curr_buff_idx + tx_sync_offset - 1360;
                cap_start_idx = curr_buff_idx + 1360;

                /* Debug Info */
                lms_stream_status_t rx_status;
                LMS_GetStreamStatus(&rx_stream, &rx_status);
                cout << "\nPPS sync occured at sample " << pps_sync_idx << endl;
                cout << "Samples since last PPS = " << pps_sync_idx - prev_pps_sync_idx << endl;
                cout << "Offset = " << pps_sync_idx - curr_buff_idx << endl;
                cout << "TX scheduled for sample " << tx_metadata.timestamp << endl;
                cout << "Capture scheduled for sample " << cap_start_idx << endl;
                cout << "RX Data rate: " << rx_status.linkRate / 1e6 << " MB/s\n";
            }
        } else {
            curr_buff_idx = rx_metadata.timestamp;
        }

        /* Begin Capture of TX Event */
        if(curr_buff_idx == cap_start_idx){
            
            /* Populate Metadata */
            file_metadata.unix_stamp = std::time(NULL);
            file_metadata.pps_index = pps_sync_idx;  
            file_metadata.buffer_index = curr_buff_idx;
            
            cout << "Capturing at " << rx_metadata.timestamp << endl;

            /* Save Current Buffer */
            memcpy(file_buffer, rx_buffer, sizeof(rx_buffer));

            /* Loop for File Duration */
            for(int k=1; k<capture_duration; k++){

                /* Recieve Buffer of 1360 Samples */
                if (LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000) != num_rx_samples){
                    cout << "RX Stream Error" << endl;
                    error();
                }
    
                /* Save Current Buffer */
                memcpy(&file_buffer[rx_buffer_size * k], rx_buffer, sizeof(rx_buffer));
                curr_buff_idx += num_rx_samples;
            }

            /* Write to File */
            data_file.open(out_path + to_string(file_metadata.unix_stamp) + ".bin", std::ofstream::binary);
            data_file.write((char*)&file_metadata, sizeof(file_metadata));
            data_file.write((char*)file_buffer, sizeof(file_buffer));
            data_file.close();
        }
    }
  
    /* Stop Streaming */
    LMS_StopStream(&rx_stream);
    LMS_StopStream(&tx_stream);
    
    /* Destroy Stream */
    LMS_DestroyStream(device, &rx_stream);
    LMS_DestroyStream(device, &tx_stream);   

    /* Close File */
    data_file.close();
    
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
