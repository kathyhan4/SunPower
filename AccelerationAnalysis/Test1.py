import numpy as np
import matplotlib.pyplot as plt



import matplotlib
from scipy import fftpack
from scipy import signal
import numpy as np
from numpy import genfromtxt
from skimage import util




fSample = 64
length = 10000

t = np.arange(0,length,1/fSample)

xt = 1*np.sin(4*2*np.pi*t)

##xt = np.zeros((fSample*length))
##
##for g in range(32):
##    xt[g] = 3*np.sin(2*np.pi*t[g])
##for g in range(32):
##    xt[g+32] = 1*np.sin(2*np.pi*t[g])


plt.plot(t,xt)
plt.show()

a = fftpack.fft(xt)

#Creates the frequency axis of the FFT, taking the sampling rate into account
freqs = fftpack.fftfreq(len(xt), 1/fSample)

plt.plot(freqs,abs(a)/(fSample),'ro')
plt.show()




#PSD
psd,freqs = matplotlib.mlab.psd(xt, 1024,fSample)

plt.figure(figsize=(5, 4))
plt.loglog(freqs, psd)
plt.title('PSD: power spectral density')
plt.xlabel('Frequency')
plt.ylabel('Power')
plt.tight_layout()
plt.show()