# Glaze

GLAZE (glsl shoegaze) is a multi-mode audio effect module that combines various signal processing techniques with shader-based effects.
![GLAZE screenshot]()

Each mode uses three inputs (U1, U2, U3) to control different dsp parameters or shader uniforms. GLAZE is a GLIB subscriber and fetches its shaders from GLIB. By default, GLAZE uses dsp when no shaders are loaded. Currently, only RVB (reverb), FZZ (fuzz), and FLD (fold) support shader-based processing, but I'm planning to extend the support to all modes eventually.  

The demo shaders can be found here. The available shader uniforms are: ```
uniform float audioInL;
uniform float audioInR;
uniform float u1;
uniform float u2;
uniform float u3;
uniform int mode;
```

## Modes and Parameters

### REV (Reverb)
- **U1**: Decay (0-100%)
  - Controls the length of the reverb tail
- **U2**: Diffusion (0-100%)
  - Affects the density of the reverb
- **U3**: currently unused

### DLY (Delay)
- **U1**: Time (0-100%)
  - Controls delay time
- **U2**: Feedback (0-100%)
  - Amount of signal fed back into the delay
- **U3**: Modulation (0-100%)
  - Adds pitch variation to the delays

### FZZ (Fuzz)
- **U1**: Drive (0-100%)
  - Amount of distortion
- **U2**: Shape (0-100%)
  - Morphs between different waveshaping curves
  - 0-50%: soft to hard clipping
  - 50-100%: hard clipping to foldback
- **U3**: Tone (0-100%)
  - Controls the brightness of the distortion
    - Lower values are darker, higher values are brighter

### GLD (Glide)
- **U1**: Frequency (0-100%)
  - Controls the pitch shift range
    - Center position keeps original pitch
- **U2**: Speed (0-100%)
  - Rate of pitch change
    - Lower values create smooth transitions
- **U3**: Waveform (0-100%)
  - Morphs between different modulation shapes
    - 0-50%: sine to triangle
    - 50-100%: triangle to square

### GRN (Granular)
- **U1**: Density (0-100%)
  - Controls how many grains are active
- **U2**: Size (0-100%)
  - Controls the length of each grain
- **U3**: Pitch (0-100%)
  - Amount of pitch variation between grains

### FLD (Fold)
- **U1**: Folds (0-100%)
  - Number of wavefolding stages
- **U2**: Symmetry (0-100%)
  - Controls the folding curve shape
    - 0-50%: sine to triangle folding
    - 50-100%: triangle to parabolic folding
- **U3**: Bias (0-100%)
  - Offset of the folding center

### WRP (Warp)
- **U1**: Amount (0-100%)
  - Intensity of time manipulation
- **U2**: Shape (0-100%)
  - Type of time warping
    - 0-50%: sine warping (smooth)
    - 50-100%: exponential warping (aggressive)
- **U3**: Skew (0-100%)
  - Phase offset of the warping

### SPC (Spectral)
- **U1**: Spread (0-100%)
  - Spectral dispersion amount
- **U2**: Shift (0-100%)
  - Frequency shifting range
- **U3**: Smear (0-100%)
  - Spectral blur amount

Note: currently not working... 

## General Controls
- **Mix**: Blends between dry (0%) and wet (100%) signal
- **Mode Button**: Cycles through the different effect modes
- **Layer/Blend**: Reserved for shader functionalities

## Technical Details

### Signal Processing Overview
- Sample rate: 44.1kHz (default) or 48kHz
- Buffer size: 4096 samples
- All audio is normalized to [-1, 1] range internally
- DC offset protection on inputs and outputs
- Soft clipping (tanh) used for saturation

### Mode Implementations

#### REV (Reverb)
- Early reflections: 4 parallel delay lines with increasing times
- Feedback matrix for diffusion
- Decay implemented through recursive buffer processing
- Uses two separate buffers for true stereo processing

#### DLY (Delay)
- Circular buffer implementation
- Linear interpolation for sub-sample delay times
- Modulation using sine wave LFO
- Separate delay lines for left and right channels

#### FZZ (Fuzz)
- Multi-stage waveshaping
- Stage 1 (0-50%): Blend between tanh(x) and clamp(x)
- Stage 2 (50-100%): Blend between clamp(x) and x/(1+|x|)
- Single-pole lowpass filter for tone control

#### GLD (Glide)
- Phase accumulator for continuous frequency tracking
- Ring modulation with morphable waveform
- Waveform interpolation:
  - 0-50%: sine to triangle using linear interpolation
  - 50-100%: triangle to square using linear interpolation

#### GRN (Grain)
- Maximum 32 simultaneous grains
- Hann window function for grain envelopes
- Dynamic grain allocation system
- Density-controlled triggering using phase accumulator
- Stereo positioning using equal power panning

#### FLD (Fold)
- Multi-stage wavefolding using trigonometric functions
- Bias applied pre-folding for asymmetric harmonics
- Morphable folding curves:
  - 0-50%: sine to triangle folding using weighted sum
  - 50-100%: triangle to parabolic using weighted sum

#### WRP (Warp)
- Phase distortion synthesis technique
- Non-linear phase mapping using interpolated buffer reads
- Two warping algorithms:
  - 0-50%: Sinusoidal phase manipulation
  - 50-100%: Exponential phase manipulation

#### SPC (Spectral) [Experimental]
- Buffer size: 16 samples (reduced for stability)
- Blackman window function
- Real-time spectral processing without FFT
- Frequency shifting using phase manipulation
- Resonance enhancement through feedback

