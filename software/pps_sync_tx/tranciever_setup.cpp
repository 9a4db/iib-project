#include <iostream>
#include "lime/LimeSuite.h"
#include "tranciever_setup.h"

using namespace std;

/* LimeSDR Device Structure */
lms_device_t* device = NULL;

/* Prototypes */
int open_device(void);


/* Configure Reciever */
int configure_tranciever(tranciever_configuration tx_rx_config){
    
    /*  DEVICE SETUP  */
    
    /* Connect to LimeSDR */
    if (open_device() != 0)
        return -1;

    /* Initialize Device with Default Configuration */
    if (LMS_Init(device) != 0)
        error();

    /* Enable RX Channel */
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();

    /* Enable TX Channel */
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
        error();
    

    /*  LO SELECTION  */

    /* Set RX Centre Frequency */
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, tx_rx_config.rx_centre_frequency) != 0)
        error();

    /* Set TX Centre Frequency */
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, tx_rx_config.tx_centre_frequency) != 0)
        error();

    /* Print Selected Centre Frequencies */
    float_type freq_rx, freq_tx;
    if (LMS_GetLOFrequency(device, LMS_CH_RX, 0, &freq_rx) != 0)
        error();
    if (LMS_GetLOFrequency(device, LMS_CH_TX, 0, &freq_tx) != 0)
        error();
    cout << "\nRX Center frequency: " << freq_rx / 1e6 << " MHz\n";
    cout << "TX Center frequency: " << freq_tx / 1e6 << " MHz\n";

    
    /*  ANTENNA SELECTION  */
    
    /* Get Avaliable Antennae */
    lms_name_t rx_antenna_list[5];
    lms_name_t tx_antenna_list[5];    
    if (LMS_GetAntennaList(device, LMS_CH_RX, 0, rx_antenna_list) < 0)
        error();
    if (LMS_GetAntennaList(device, LMS_CH_TX, 0, tx_antenna_list) < 0)
        error();

    /* Select RX Antenna */
    if (LMS_SetAntenna(device, LMS_CH_RX, 0, tx_rx_config.rx_antenna) != 0)
        error();

    /* Select TX Antenna */
    if (LMS_SetAntenna(device, LMS_CH_TX, 0, tx_rx_config.tx_antenna) != 0)
        error();

    /* Print Currently Selected Antenna */
    int ant_idx_rx, ant_idx_tx;
    if ((ant_idx_rx = LMS_GetAntenna(device, LMS_CH_RX, 0)) < 0)
        error();
    if ((ant_idx_tx = LMS_GetAntenna(device, LMS_CH_TX, 0)) < 0)
        error();
    cout << "Selected RX path " << ant_idx_rx << ": " << rx_antenna_list[ant_idx_rx] << endl;
    cout << "Selected TX path " << ant_idx_tx << ": " << tx_antenna_list[ant_idx_tx] << endl;


    /*  SAMPLE RATE SELECTION  */

    /* Set Sample Rate & Preferred Oversampling in RF */
    if (LMS_SetSampleRate(device, tx_rx_config.sample_rate, tx_rx_config.rf_oversample_ratio) != 0)
        error();
    
    /* Print Resulting Sampling Rates (ADC & Host Interface) */
    float_type rate, rf_rate;
    if (LMS_GetSampleRate(device, LMS_CH_RX, 0, &rate, &rf_rate) != 0)
        error();
    cout << "Host interface sample rate: " << rate / 1e6 << " MHz\nRF ADC sample rate: " << rf_rate / 1e6 << "MHz\n";


    /*  ANALOG LOW PASS FILTER SELECTION  */

    /* RX LPF Setup */
    if(tx_rx_config.enable_rx_LPF){

        /* Set RX Analog LPF Bandwidth - 1.4001 to 130 MHz */
        if (LMS_SetLPFBW(device, LMS_CH_RX, 0, tx_rx_config.rx_LPF_bandwidth) != 0)
            error();
        cout << "RX LPF bandwitdh: " <<  tx_rx_config.rx_LPF_bandwidth / 1e6 << " MHz\n";

    } else {

        /* Disable RX LPF */
        LMS_SetLPF(device, LMS_CH_RX, 0, false);
        cout << "RX LPF disabled \n";
    }

    /* TX LPF Setup */
    if(tx_rx_config.enable_tx_LPF){

        /* Set TX Analog LPF Bandwidth - 5 to 130 MHz */
        if (LMS_SetLPFBW(device, LMS_CH_TX, 0, tx_rx_config.tx_LPF_bandwidth) != 0)
            error();
        cout << "TX LPF bandwitdh: " <<  tx_rx_config.tx_LPF_bandwidth / 1e6 << " MHz\n";

    } else {

        /* Disable TX LPF */
        LMS_SetLPF(device, LMS_CH_TX, 0, false);
        cout << "TX LPF disabled \n";
    }
    
    
    /*  RX GAIN SELECTION  */

    /* Set RX Gain - 0 to 1.0 */
    if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, tx_rx_config.rx_gain) != 0)
        error();

    /* Print Normalised RX Gain */
    float_type gain;
    if (LMS_GetNormalizedGain(device, LMS_CH_RX, 0, &gain) != 0)
        error();
    cout << "Normalized RX Gain: " << gain << endl;

    /* Print Resulting RX Gain in dB */
    unsigned int gaindB;
    if (LMS_GetGaindB(device, LMS_CH_RX, 0, &gaindB) != 0)
        error();
    cout << "RX Gain: " << gaindB << " dB" << endl;


    /*  TX GAIN SELECTION  */

    /* Set TX Gain - 0 to 1.0 */
    if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, tx_rx_config.tx_gain) != 0)
        error();

    /* Print Normalised TX Gain */
    if (LMS_GetNormalizedGain(device, LMS_CH_TX, 0, &gain) != 0)
        error();
    cout << "Normalized TX Gain: " << gain << endl;

     /* Print Resulting TX Gain in dB */
    if (LMS_GetGaindB(device, LMS_CH_TX, 0, &gaindB) != 0)
        error();
    cout << "TX Gain: " << gaindB << " dB" << endl;

    
    /*  CALIBRATION  */

    /* RX Calibration - 2.5 to 120 MHz */
    if(tx_rx_config.enable_rx_cal){

        if (LMS_Calibrate(device, LMS_CH_RX, 0, tx_rx_config.rx_cal_bandwidth, 0) != 0)
        error();
    }

    /* TX Calibration - 2.5 to 120 MHz */
    if(tx_rx_config.enable_tx_cal){

        if (LMS_Calibrate(device, LMS_CH_TX, 0, tx_rx_config.tx_cal_bandwidth, 0) != 0)
        error();
    }

    /* Return Success */
    return 0;
}


/* Open LimeSDR Device */
int open_device(void){

    /* Find Number of Devices Attached */
    int num_dev;
    if ((num_dev = LMS_GetDeviceList(NULL)) < 0)
        error();
    cout << "Devices found: " << num_dev << endl;
    if (num_dev < 1)
        return -1;

    /* Allocate & Populate Device List */
    lms_info_str_t* list = new lms_info_str_t[num_dev];   
    if (LMS_GetDeviceList(list) < 0)                
        error();

    /* Print out Device List */
    for (int i = 0; i < num_dev; i++)                     
        cout << i << ": " << list[i] << endl;
    cout << endl;

    /* Open the First Device */
    if (LMS_Open(&device, list[0], NULL))
        error();

    /* Delete List */
    delete [] list;
    return 0;
}


/* Error Handler */
int error(){
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}