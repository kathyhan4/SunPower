import matplotlib.pyplot as plt
from scipy import fftpack
from scipy import signal
import numpy as np
from numpy import genfromtxt
from skimage import util
import matplotlib
from scipy.signal import butter, lfilter, freqz
import datetime
import csv


def butter_lowpass(cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    return b, a

def butter_lowpass_filter(data, cutoff, fs, order=5):
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = lfilter(b, a, data)
    return y

#Sampling rate, or number of measurements per second
fs = 400

#Imports the data into CSV
myimport = genfromtxt('SSX57810_VF.csv', delimiter=',')

#Pulls out the time domain data of the Z axis
x_raw = myimport[:,3]

# Filter requirements.
order = 6
cutoff = fs/2-1  # desired cutoff frequency of the filter, Hz

# Filter the data, and plot both the original and filtered signals.
x = butter_lowpass_filter(x_raw, cutoff, fs, order)


#Plots FFT with hanning window - need to really understand what's going on here (I currently understand somewhat, but not 100%)
M = 1024



#PSD
psd,freqs = matplotlib.mlab.psd(x, M,fs)

plt.figure(figsize=(5, 4))
plt.loglog(freqs, psd)
plt.xlim([0,200])
plt.ylim([0,1.2])
plt.title('PSD: power spectral density')
plt.xlabel('Frequency')
plt.ylabel('Power')
plt.tight_layout()
plt.show()

#Exports data to CSV
now = datetime.datetime.now()

with open('PSD Data ' + now.strftime("%Y-%m-%d %H %M") + '.csv', mode='w', newline='') as datafile:
    PSD_Writer = csv.writer(datafile, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    PSD_Writer.writerow(['Frequency (Hz)', 'Amplitude (G^2)'])
    for x in range(len(freqs)):
        PSD_Writer.writerow([freqs[x], psd[x]])



