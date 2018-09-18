import sys
import struct
import numpy as np
import matplotlib.pyplot as plt

# Useage
if len(sys.argv) != 2:
    print("Usage: {} <samples.bin>".format(sys.argv[0]))
    sys.exit(1)
    
# Open Datafile
file = open(sys.argv[1], 'rb')
    
# Detect Number of Samples
file.read()
num_samples = int(file.tell()/4)
file.seek(0)
print("Found", num_samples, "samples.")

# Create I and Q Arrays
I = np.zeros(num_samples, dtype=float)
Q = np.zeros(num_samples, dtype=float)
for n in range(0, num_samples):
    
    I[n] = struct.unpack('h', file.read(2))[0]
    Q[n] = struct.unpack('h', file.read(2))[0]

plt.plot(I)
plt.plot(Q)
plt.show()