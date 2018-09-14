import SoapySDR
from SoapySDR import * #SOAPY_SDR_ constants
import numpy #use numpy for buffers

#enumerate devices
results = SoapySDR.Device.enumerate()
for result in results: print(result)

#create device instance
#args can be user defined or from the enumeration result
args = dict(driver="lime")
sdr = SoapySDR.Device(args)

#apply settings
sdr.setSampleRate(SOAPY_SDR_RX, 0, 30e6)
sdr.setFrequency(SOAPY_SDR_RX, 0, 868e6)

#setup a stream (complex floats)
rxStream = sdr.setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32)
sdr.activateStream(rxStream) #start streaming

#create a re-usable buffer for rx samples
buff = numpy.array([0]*1020, numpy.complex64)

pps = 0
prev = 0
for i in range(1000000):
    sr = sdr.readStream(rxStream, [buff], len(buff))
    ts = int(sr.timeNs)
    if(ts < 0):
        prev = pps
        pps = ts + 9223372036854775808 
        if(pps!=prev):
            print(pps-prev)

#shutdown the stream
sdr.deactivateStream(rxStream) #stop streaming
sdr.closeStream(rxStream)
