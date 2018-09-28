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

// g++ main.cpp tranciever_setup.cpp -std=c++11 -lLimeSuite -o test.out

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
    tx_stream.fifoSize = 1360 * 1024;                   // Fifo Size in Samples
    tx_stream.throughputVsLatency = 0;                  // Optimize Throughput (1.0) or Latency (0)
    tx_stream.isTx = true;                              // TX/RX Channel
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;      // Data Format - 12-bit sample stored as int16_t
    LMS_SetupStream(device, &tx_stream);
    
    /* TX Data Buffer */
    const int num_tx_samples = 1360*8;
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
    outfile.open("output.bin", std::ofstream::binary);

    /* Output Buffer */
    const int file_length = 360;
    int16_t file_buffer[rx_buffer_size * file_length];

    uint64_t rx_event = 0;
    uint64_t tx_event = 0;
    
    /* 1. Start Streams */
    LMS_StartStream(&tx_stream);
    LMS_StartStream(&rx_stream);

    /* 1.1 Let Streams Stabalise */
    for(int j=0; j<100; j++){
        LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000);
    }

    /* 1.2 Zero Stats */
    lms_stream_status_t rx_status, tx_status;
    LMS_GetStreamStatus(&rx_stream, &rx_status);
    uint32_t tmp = rx_status.droppedPackets;

    /* 2. RX Event */
    LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000);
    rx_event = rx_metadata.timestamp;
    memcpy(file_buffer, rx_buffer, sizeof(rx_buffer));
    cout << "RX Event at " << rx_event << endl;

    /* 3. Schedule TX */
    tx_event = rx_event + 1360 * 75;
    tx_metadata.timestamp = tx_event;
    if (LMS_SendStream(&tx_stream, tx_buffer, num_tx_samples, &tx_metadata, 1000) != num_tx_samples){
        error();
    }    
    cout << "TX Scheduled for " << tx_metadata.timestamp << endl;
    cout << "Delta = " << tx_metadata.timestamp - rx_event << endl;
    
    /* 4. Record RX Event */
    for(int k=1; k<file_length; k++){
      
        LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000);
        memcpy(&file_buffer[k*rx_buffer_size], rx_buffer, sizeof(rx_buffer));
    
        /* Re-transmitt following TX event */
        if(rx_metadata.timestamp == tx_event + 16 * 1360){

            tx_event = rx_metadata.timestamp + 1360 * 75;
            tx_metadata.timestamp = tx_event;
            if (LMS_SendStream(&tx_stream, tx_buffer, num_tx_samples, &tx_metadata, 1000) != num_tx_samples){
                error();
            }    
            cout << "TX Scheduled for " << tx_metadata.timestamp << endl;
            cout << "Delta = " << tx_metadata.timestamp - rx_event << endl;
        }    
    }
        
    /* 5. Print Stats */
    LMS_GetStreamStatus(&rx_stream, &rx_status);
    LMS_GetStreamStatus(&tx_stream, &tx_status);
    cout << "Final RX stamp: " << rx_metadata.timestamp << endl;
    cout << "RX dropped: " << rx_status.droppedPackets << endl; 
    cout << "TX dropped: " << tx_status.droppedPackets << endl; 

    /* 6. Stop Streams */
    LMS_StopStream(&rx_stream);
    LMS_StopStream(&tx_stream);
    
    /* 7. Write to File */
    outfile.write((char*)file_buffer, sizeof(file_buffer));
    outfile.close();     
    
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