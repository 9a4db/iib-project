#include <ctime>
#include <chrono>
#include <thread>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include "string.h"
#include "lime/LimeSuite.h"
#include "tranciever_setup.h"
using namespace std;

// g++ main.cpp tranciever_setup.cpp -std=c++11 -lLimeSuite -o pps-tx.out

/* Entry Point */
int main(int argc, char** argv){
    
    /* Hardware Config */
    tranciever_configuration config;
    config.rx_centre_frequency = 868e6;                 // RX Center Freuency    
    config.rx_antenna = LMS_PATH_LNAW;                  // RX RF Path = 10MHz - 2GHz
    config.rx_gain = 0.7;                               // RX Normalised Gain - 0 to 1.0
    config.enable_rx_LPF = true;                        // Enable RX Low Pass Filter
    config.rx_LPF_bandwidth = 10e6;                     // RX Analog Low Pass Filter Bandwidth
    config.enable_rx_cal = true;                        // Enable RX Calibration
    config.rx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth
    
    config.tx_centre_frequency = 868e6;                 // TX Center Freuency
    config.tx_antenna = LMS_PATH_TX2;                   // TX RF Path = 10MHz - 2GHz
    config.tx_gain = 0.4;                               // TX Normalised Gain - 0 to 1.0
    config.enable_tx_LPF = true;                        // Enable TX Low Pass Filter
    config.tx_LPF_bandwidth = 10e6;                     // TX Analog Low Pass Filter Bandwidth
    config.enable_tx_cal = true;                        // Enable TX Calibration
    config.tx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth
    
    config.sample_rate = 30.72e6;                       // Device Sample Rate 
    config.rf_oversample_ratio = 4;                     // ADC Oversample Ratio
    
    configure_tranciever(config);

    /* Share TX & RX PLL */
    LMS_WriteParam(device, LMS7_MAC, 2);
    LMS_WriteParam(device, LMS7_PD_LOCH_T2RBUF, 0);
    LMS_WriteParam(device, LMS7_MAC, 1);
    LMS_WriteParam(device, LMS7_PD_VCO, 1);
    
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
     
    /* TX Stream Config */
    lms_stream_t tx_stream;
    tx_stream.channel = 0;                              // Channel Number
    tx_stream.fifoSize = 1360 * 4096;                   // Fifo Size in Samples
    tx_stream.throughputVsLatency = 0;                  // Optimize Throughput (1.0) or Latency (0)
    tx_stream.isTx = true;                              // TX/RX Channel
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;      // Data Format - 12-bit sample stored as int16_t
    LMS_SetupStream(device, &tx_stream);
    
    /* TX Data Buffer */
    const int num_tx_samples = 1360 * 8;
    const int tx_buffer_size = num_tx_samples * 2;
    int16_t tx_buffer[tx_buffer_size];
    
    /* TX Stream Metadata */
    lms_stream_meta_t tx_metadata;
    tx_metadata.flushPartialPacket = true;             // Force sending of incomplete packets
    tx_metadata.waitForTimestamp = true;               // Enable synchronization to HW timestamp
     
    /* Generate 1 MHz Test Signal */
     for (int i = 0; i <num_tx_samples; i++) {
        const double pi = acos(-1);
        double w = 2*pi*i*(1e6)/config.sample_rate;
        tx_buffer[2*i] = (int16_t)(cos(w)*1000);
        tx_buffer[2*i+1] = (int16_t)(sin(w)*1000);
    }

    /* Output File */
    ofstream outfile;
    file_header file_metadata;
    const string out_path = "data/";

    /* Output Buffer */
    const int file_length = 50;
    int16_t file_buffer[rx_buffer_size * file_length];

    /* Book Keeping Indicies */
    uint64_t curr_buff_idx = 0;
    uint64_t pps_sync_idx = 0;
    uint64_t prev_pps_sync_idx = 0;
    uint64_t tx_schedule_event = 10e6;
    uint64_t tx_start_event = 0;
    uint64_t tx_capture_event = 10e6;
    uint64_t tx_stop_event = 10e6;
    

    /* Start RX Stream */
    LMS_StartStream(&rx_stream);

    /* Process Stream for 15s */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(10)){

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

                tx_schedule_event = curr_buff_idx + 1360 * 500;         // Send TX samples in 500 buffers time
                tx_capture_event = curr_buff_idx + 1360 * (575 - 15);   // Begin recording ~15 buffers prior to TX
                tx_start_event = pps_sync_idx + 1360 * 575;             // TX scheduled for 782 000 samples after PPS
                tx_stop_event = curr_buff_idx + 1360 * (575 + 15);      // Close TX stream ~15 buffers after start of TX
   
                cout << "\nCurrent buffer = " << curr_buff_idx << endl;
                cout << "PPS event occured at " << pps_sync_idx << endl;
            }
        } else {
            curr_buff_idx = rx_metadata.timestamp;
        }

        /* Schedule TX Event */
        if(curr_buff_idx == tx_schedule_event){
            
            /* Start TX Stream */
            LMS_StartStream(&tx_stream);

            /* Send TX Buffer */
            tx_metadata.timestamp = tx_start_event;
            if (LMS_SendStream(&tx_stream, tx_buffer, num_tx_samples, &tx_metadata, 1000) != num_tx_samples){
                error();
            }
            cout << "Buffer sent, TX scheduled for " << tx_metadata.timestamp << endl;
            cout << "Offset = " << tx_start_event - tx_capture_event << endl;
        }

        /* Capture TX Event */
        if(curr_buff_idx == tx_capture_event){
            
            /* Save Current RX Buffer */
            memcpy(file_buffer, rx_buffer, sizeof(rx_buffer));

            cout << "Capturing at " << curr_buff_idx << endl;
            
            /* Generate File Metadata */
            file_metadata.unix_stamp = std::time(NULL);
            file_metadata.buffer_index = curr_buff_idx;
            file_metadata.pps_index = pps_sync_idx;

            /* Save Subsequent RX Buffers */
            for(int k=1; k<file_length; k++){
    
                LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000);
                memcpy(&file_buffer[k*rx_buffer_size], rx_buffer, sizeof(rx_buffer));
                curr_buff_idx += num_rx_samples;

                /* Stop TX Stream */
                if(curr_buff_idx == tx_stop_event){
                    LMS_StopStream(&tx_stream);
                    cout << "TX stopped at " << curr_buff_idx << endl;
                }  
            }
            
            /* Write to File */
            outfile.open(out_path + to_string(file_metadata.unix_stamp) + ".bin", std::ofstream::binary);
            outfile.write((char*)&file_metadata, sizeof(file_metadata));
            outfile.write((char*)file_buffer, sizeof(file_buffer));
            outfile.close();
        }
    }

    /* Stop RX Stream */
    LMS_StopStream(&rx_stream);
    
    /* Destroy Stream */
    LMS_DestroyStream(device, &rx_stream);
    LMS_DestroyStream(device, &tx_stream);   
    
    /* Disable TX/RX Channels */
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, false)!=0)
        error();
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, false)!=0)
        error();
    
    /* Write Waveform to File */
    ofstream wfm;
    wfm.open("wfm.bin", std::ofstream::binary);
    wfm.write((char*)tx_buffer, sizeof(tx_buffer));
    wfm.close();
    
    /* Close Device */
    if (LMS_Close(device)==0)
        cout << "Device closed" << endl;
    return 0;
}