# Matt's 4th Year Project

## GPSDO
This contains the design files and firmware for a GPS disciplined oscillator. It combines the frequency reference from a Ublox MAX-M8Q GPS module with a low jitter timing reference to create a very stable and accurate user defined frequency reference. There are two identical buffered clock outputs as well as a 1Hz PPS. The PPS output is used as a timing reference for synchronizing the output streams of multiple independant SDRs. There is also a command line application to view the status of the GPS and PLL live.

## Lime-SDR
This folder contains various resources such as the modified image running on the Lime Mini's MAX10 FPGA, the design files for a small PCB to break the 0.1" header on the Lime Mini out to an SMA connector, and a setup guide.

## Software
This folder contains some prototype applications written in C++ that make use of the LimeSuite LMS API.

### pps_rx_sync
This program produces an output file each second that contains a header followed by 13260 IQ samples in int16_t format. The header specifies the index of the first sample in the buffer and the index of the sample corresponding to the PPS trigger event as well as a unix timestamp for the file.
