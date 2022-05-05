# 3HR VCVRack Modules


![Repeat3hr](https://github.com/Geekachuqt/3HR/blob/main/docs/Repeat3hr.png)

### REPEAT3HR

REPEAT3HR is a creative, digital-style mono delay that is designed for time manipulation. It uses volume interpolation to smoothen out clicks introduced by changes in the delay time.

Every parameter is fully CV-controllable, and works by adding the incoming signal ontop of the knob value. The Crush parameter is the exception to the rule, and here it subtracts from the knob value instead, mirroring the normal usage of the knob.

Finally, it also outputs a trig pulse every time the delayline resets. This allows you to, per example, trigger envelopes in sync with the output of the module, furthering the fun you can have with manipulating the delay time.

![Xtenuvat3hr](https://github.com/Geekachuqt/3HR/blob/main/docs/Xtenuvert3hr.png)

### XTENUAT3HR

XTENUAT3HR is a CV tool designed to crossfade two signals and attenuvert them. This can create some really interesting and fun patterns from just two base signals.

The Attenuverter doesn't just apply a -1 multiplier to the signal, but rather fully inverts it - turning +10V to 0 and vice versa. It is fully polyphonic, supporting both polyphonic audio inputs and CV inputs.

![FM3HR](https://github.com/Geekachuqt/3HR/blob/main/docs/FM3HR.png)

### FM3HR

FM3HR is an oscillator that pushes a continuous waveform oscillator through three sequential FM algorithms (additive -> linear -> exponential) and a smoother, and outputs each stage separately. Each of the FM stages can be CV controlled, and are arranged in a cascading manner so that each stage acts as both the carrier and the modulator for the next. Each FM stage can also be used with external modulators. The smoother at the end of the chain applies a slew algorithm which reduces the high frequency content of the generated waveform. It acts similarly to a low-pass filter, with slight differences. It is fully polyphonic, supporting polyphonic V/Oct, CV, and external FM source polyphony.

![NVLOP3HR](https://github.com/Geekachuqt/3HR/blob/main/docs/NVLOP3HR.png)

### NVLOP3HR

NVLOP3HR is a dual multifunction envelope generator with curavture control and separate loop switches. In normal mode, it acts as a classic envelope generator sporting linear envelopes that are then exponentialized through the curvature control. The fun stuff lies in the multiplicative outs combined with the loop switches.

In loop mode, the envelopes will loop **only while a gate is sustained**. This allows you to easily create some really interesting rhythmical envelopes. It also loves to be patched into inself, by having one envelope modulate the sustain of the other, per example. Combined with the multiplicative outs, this can generate some really long, musical, psuedo-random CV sequences. Upon release, a looping envelope will immediately proceed to its release stage, and then end.

### FMFILT3HR

![FMFILT3HR](https://github.com/Geekachuqt/3HR/blob/main/docs/FMFILT3HR.png)

FMFILT3HR is a lowpass filter designed for FM manipulation. It features two FM ports with different algorithms (Additive and multiplicative) which can be controlled manually or with CV. The FM modulation affects the Cutoff frequency of the filter. At its core, the filter is basically two 18dB butterworth filters in sequence with different resonance values, resulting in just enough dirt to give the input a unique characteristic at high resonance values without having the filter explode in your face.

### GLTCH3HR

![GLCTH3HR](https://github.com/Geekachuqt/3HR/blob/main/docs/gltch3hr.png)

GLTCH3HR is a threshold-controlled looper with controllable start and end-times, designed for stutter and artifact generation. Upon receiving a control signal that exceeds the threshold parameter, it will begin recording 0.5 seconds of the input signal. After an amount of time that equals to the startpoint plus the length of the sample, it will begin playing back the specified section of the recorded input. Once it's playing, the start and end times are freely modifiable, allowing you to extend or shorten the section being played at will, and even reversing the playback. A controllable volume interpolation algorithm is also included to slightly lessen the clicks introduced by the abrupt changes in waveform induced by the jump at the loop point. The module works with both CV and audio signals.

Note that the module does **not** pass through audio, and will only output the looped section. As such, it should be used as a "send" style effect.
