import matplotlib.pyplot as plt
from scipy import fftpack
from scipy import signal
import numpy as np
from numpy import genfromtxt

#Sampling rate, or number of measurements per second
f_s = 400

#Imports the data into CSV
myimport = genfromtxt('SSX63344 second half.csv', delimiter=',')

#Pulls out the time domain data of the Z axis
x = myimport[:,3]

#Performs FFT on the Z axis
X = fftpack.fft(x)

#Creates the frequency axis of the FFT, taking the sampling rate into account
freqs = fftpack.fftfreq(len(x), 1/f_s)

#Plots the data
plt.plot(freqs, np.abs(X))
plt.xlabel("Frequency (Hz)")
plt.ylabel("Amplitude (G)")
plt.xlim([0,200])
plt.show()



##freqs, psd = signal.welch(x)
##
##plt.figure(figsize=(5, 4))
##plt.semilogx(freqs, psd)
##plt.title('PSD: power spectral density')
##plt.xlabel('Frequency')
##plt.ylabel('Power')
##plt.tight_layout()
##plt.show()