import matplotlib.pyplot as plt
from scipy import fftpack
from scipy import signal
import numpy as np
from numpy import genfromtxt
from skimage import util
import matplotlib
from scipy.signal import butter, lfilter, freqz



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
myimport = genfromtxt('SSX04081.csv', delimiter=',')

#Pulls out the time domain data of the Z axis
x_raw = myimport[:,3]

# Filter requirements.
order = 6
cutoff = fs/2-1  # desired cutoff frequency of the filter, Hz

# Filter the data, and plot both the original and filtered signals.
x = butter_lowpass_filter(x_raw, cutoff, fs, order)


#Performs FFT on the Z axis
X = fftpack.fft(x)
X = X[1:(len(X)-1)]

#Creates the frequency axis of the FFT, taking the sampling rate into account
freqs = fftpack.fftfreq(len(x), 1/fs)
freqs = freqs[1:(len(freqs)-1)]

#Plots the data
plt.plot(freqs, np.abs(X))
plt.xlabel("Frequency (Hz)")
plt.ylabel("Amplitude (G)")
plt.xlim([0,200])
plt.show()



#Plots FFT with hanning window - need to really understand what's going on here (I currently understand somewhat, but not 100%)
M = 1024
L = len(x)

slices = util.view_as_windows(x, window_shape=(M,), step=100)
win = np.hanning(M + 1)[:-1]
slices = slices * win
slices = slices.T
print('Shape of `slices`:', slices.shape)

spectrum = np.fft.fft(slices, axis=0)[:M // 2 + 1:-1]
spectrum = np.abs(spectrum)

f, ax = plt.subplots(figsize=(4.8, 2.4))

S = np.abs(spectrum)
S = 10 * np.log10(S / np.max(S))

ax.imshow(S, origin='lower', cmap='viridis',extent=(0, L/60/fs, 0, fs / 2 ))
ax.axis('tight')
ax.set_ylabel('Frequency [Hz]')
ax.set_xlabel('Time [min]');
plt.title("Spectrogram")
plt.show()



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

##freqs, psd = signal.welch(x)
##
##plt.figure(figsize=(5, 4))
##plt.semilogx(freqs, psd)
##plt.title('PSD: power spectral density')
##plt.xlabel('Frequency')
##plt.ylabel('Power')
##plt.tight_layout()
##plt.show()