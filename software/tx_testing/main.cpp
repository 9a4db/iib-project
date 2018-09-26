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

    config.sample_rate = 10e6;                          // Device Sample Rate 
    config.rf_oversample_ratio = 8;                     // ADC Oversample Ratio

    configure_tranciever(config);

    /* Share TX & RX PLL */
    LMS_WriteParam(device, LMS7_MAC, 2);
    LMS_WriteParam(device, LMS7_PD_LOCH_T2RBUF, 0);
    LMS_WriteParam(device, LMS7_MAC, 1);
    LMS_WriteParam(device, LMS7_PD_VCO, 1);


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
    tx_stream.fifoSize = 256*1024;                      // Fifo Size in Samples
    tx_stream.throughputVsLatency = 0;                  // Optimize Throughput (1.0) or Latency (0)
    tx_stream.isTx = true;                              // TX/RX Channel
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_F32;      // Data Format - 12-bit sample stored as int16_t
    LMS_SetupStream(device, &tx_stream);

    /* TX Stream Metadata */
    lms_stream_meta_t tx_metadata;
    tx_metadata.flushPartialPacket = false;             // Dont force sending of incomplete packets
    tx_metadata.waitForTimestamp = false;                // Enable synchronization to HW timestamp
    tx_metadata.timestamp = 0;
    

    /* TX Data Buffer */
    const int tx_size = 1024*8;
    float tx_buffer[2*tx_size];     
    for (int i = 0; i <tx_size; i++) {          
        const double pi = acos(-1);
        double w = 2*pi*i*(1e6)/(10e6);
        tx_buffer[2*i] = cos(w);
        tx_buffer[2*i+1] = sin(w);
    }

    /* Output File */
    ofstream outfile;
    outfile.open("output.bin", std::ofstream::binary);
    
    /* Start Streams */
    LMS_StartStream(&rx_stream);
    LMS_StartStream(&tx_stream);  
    
    auto t1 = chrono::high_resolution_clock::now();
    while (chrono::high_resolution_clock::now() - t1 < chrono::milliseconds(300)){

        /* RX/TX Samples */
        LMS_RecvStream(&rx_stream, rx_buffer, num_rx_samples, &rx_metadata, 1000);
        LMS_SendStream(&tx_stream, tx_buffer, tx_size, &tx_metadata, 1000);
        
        /* Save First 100 Packets */       
        if(rx_metadata.timestamp < 1360*400){
            outfile.write((char*)rx_buffer, sizeof(rx_buffer));
        }
    }

    /* Stop Streams */
    LMS_StopStream(&rx_stream);
    LMS_StopStream(&tx_stream);  

    /* Destroy Stream */
    LMS_DestroyStream(device, &rx_stream);
    LMS_DestroyStream(device, &tx_stream);

    outfile.close(); 
    
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
