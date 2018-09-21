import sys
import struct
import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt

# Useage
if len(sys.argv) != 2:
    print("Usage: {} <samples.bin>".format(sys.argv[0]))
    sys.exit(1)
    
# Open Datafile
file = open(sys.argv[1], 'rb')
    
# Compute Number of Samples Present
file.read()
num_samples = int((file.tell()-24)/4)
file.seek(0)
print("File contains", num_samples, "samples.")
dur = (1/30.72)*num_samples
print("Duration: %.4f us" % dur)
print("")

# Read Metadata
meta = struct.unpack('QQQ', file.read(24))
print(datetime.utcfromtimestamp(meta[0]).strftime('%Y-%m-%d %H:%M:%S'))
print("File begins with sample", meta[1])
print("PPS sync occured at sample", meta[2])
print("Sync event offset =", meta[2] - meta[1])

# Create I and Q Arrays
I = np.zeros(num_samples, dtype=float)
Q = np.zeros(num_samples, dtype=float)
for n in range(0, num_samples):
    
    I[n] = struct.unpack('h', file.read(2))[0]
    Q[n] = struct.unpack('h', file.read(2))[0]

# Creat Complex Array
samples = (I + 1j*Q)
print(samples)

# Plot I&Q Channels
plt.plot(np.real(samples), label='I')
plt.plot(np.imag(samples), label='Q')
plt.grid(True)  
plt.legend(loc='upper right', frameon=True)
plt.show()