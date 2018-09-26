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
    config.enable_rx_LPF = false;                       // Enable RX Low Pass Filter
    config.rx_LPF_bandwidth = 10e6;                     // RX Analog Low Pass Filter Bandwidth
    config.enable_rx_cal = true;                        // Enable RX Calibration
    config.rx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth

    config.tx_centre_frequency = 868e6;                 // TX Center Freuency
    config.tx_antenna = LMS_PATH_TX2;                   // TX RF Path = 10MHz - 2GHz
    config.tx_gain = 0.4;                               // TX Normalised Gain - 0 to 1.0
    config.enable_tx_LPF = false;                        // Enable TX Low Pass Filter
    config.tx_LPF_bandwidth = 10e6;                      // TX Analog Low Pass Filter Bandwidth
    config.enable_tx_cal = true;                        // Enable TX Calibration
    config.tx_cal_bandwidth = 8e6;                      // Automatic Calibration Bandwidth

    config.sample_rate = 30.72e6;                       // Device Sample Rate 
    config.rf_oversample_ratio = 4;                     // ADC Oversample Ratio

    configure_tranciever(config);

    /* Enable Test Signal */
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NCODIV8, 0, 0) != 0){
        error();
    }

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
    tx_stream.fifoSize = 1360 * 2048;                   // Fifo Size in Samples
    tx_stream.throughputVsLatency = 0;                  // Optimize Throughput (1.0) or Latency (0)
    tx_stream.isTx = true;                              // TX/RX Channel
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;      // Data Format - 12-bit sample stored as int16_t
    LMS_SetupStream(device, &tx_stream);

    /* TX Data Buffer */
    const int num_tx_samples = 1360;
    const int tx_buffer_size = num_tx_samples * 2;
    int16_t tx_buffer[tx_buffer_size];

    /* TX Stream Metadata */
    lms_stream_meta_t tx_metadata;
    tx_metadata.flushPartialPacket = false;             // Dont force sending of incomplete packets
    tx_metadata.waitForTimestamp = true;                // Enable synchronization to HW timestamp

    
    /* Record Test Signal */
    LMS_StartStream(&rx_stream);
    if (LMS_RecvStream(&rx_stream, tx_buffer, num_tx_samples, &rx_metadata, 1000) != num_tx_samples){
        cout << "Could not recieve test signal!" << endl;
        error();
    }
    LMS_StopStream(&rx_stream);
    
    /* Disable Test Signal */
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NONE, 0, 0) != 0){
        error();
    }

    uint64_t rx_event = 0;
    ofstream outfile;
    outfile.open("event.bin", std::ofstream::binary);
    
    /* 1. Start Streams */
    LMS_StartStream(&tx_stream);
    LMS_StartStream(&rx_stream);


    /* 2. RX Event */
    if (LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000) != num_rx_samples){
        error();
    } else {
        rx_event = rx_metadata.timestamp;
        cout << "RX Event at " << rx_event << endl;
    }

    /* 3. Schedule TX */
    tx_metadata.timestamp = rx_event + 1360 * 1000;
    if (LMS_SendStream(&tx_stream, tx_buffer, num_tx_samples, &tx_metadata, 1000) != num_tx_samples){
        error();
    }
    LMS_StopStream(&tx_stream);
    cout << "TX Scheduled for " << tx_metadata.timestamp << endl;
    
    /* 4. Record TX Event */
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(1)){

        if (LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000) != num_rx_samples){
            error();
        }

        /* Start Prior to TX */
        if(rx_metadata.timestamp == rx_event + 1360 * 950){
            cout << "Recording from " << rx_metadata.timestamp << endl;
            for(int k=0; k <200; k++){
                
                LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000);
                outfile.write((char*)rx_buffer, sizeof(rx_buffer));
            }
        }
    }

    /* 5. Stop Streams */
    LMS_StopStream(&rx_stream);

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
