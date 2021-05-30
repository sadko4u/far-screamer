# far-screamer

Far Screamer is a command-line tool for applying IR convolution to the input file.

The user should provide the name of input file, the name of output file and the
name of the impulse response (IR) file. After that, the tool performs convolution between
the input file and the IR file and stores the result to the output file.

Additional settings allow to tune settings for the convolution processor:
  * Equalization of the processed (wet) signal with high-pass and low-pass filters.
  * Trimming the IR file contents and adding fades at the start and end to the IR file.
  * Adding pre-delay to the wet (processed) signal.
  * Adjusting Dry/Wet balance between processed (dry) and unprocessed (wet) signal.
  * Adjusting Mid/Side balance for the stereo output signal.
  * Performing audio processing chain mapping for non-trivial input and output files.

The tool allows to override some batch parameters by specifying them as command-line arguments.
The full list can be obtained by issuing ```far-screamer --help``` command and is the following:

```
  -dg, --dry-gain        Dry gain (in dB) - the amount of unprocessed signal
  -fi, --fade-in         Fade in of the IR file (in milliseconds)
  -fo, --fade-out        Fade out of the IR file (in milliseconds)
  -hc, --head-cut        Head cut of the IR file (in milliseconds)
  -hp, --hi-pass         High-pass filter parameters (--help for details)
  -if, --in-file         Input file
  -ir, --ir-file         Impulse response file
  -lp, --low-pass        Low-pass filter parameters (--help for details)
  -m, --mapping          IR convolution mapping in format: out:in:ir[:gain]
  -mb, --mid-balance     The amount of Middle part (in dB) in stereo signal
  -n, --normalize        Set normalization mode
  -ng, --norm-gain       Set normalization peak gain (in dB)
  -of, --out-file        Output file
  -pd, --predelay        The amount of pre-delay added to the signal (in ms)
  -sb, --side-balance    The amount of Side part (in dB) in stereo signal
  -sr, --srate           Sample rate of output file
  -tc, --tail-cut        Tail cut of the IR file (in milliseconds)
  -tl, --trim-length     Trim length of output file to match the input file
  -wg, --wet-gain        Wet gain (in dB) - the amount of processed signal


```

Equalizing the wet signal
===

The options ```-hp``` and ```-lp``` allow to set-up low-pass and high-pass filter parameters which
will be applied to the wet (processed) signal. The argument format may be obtained by specifying ```--help```
option after the corresponding parameter. The following output will be given:

```
Filter parameters format: <type>:<slope>:<freq>[:<q>]
  type  - filter type, one of: RLC_BT, RLC_MT, BWC_BT, BWC_MT, LRX_BT, LRX_MT, APO_DR
  slope - filter slope (1 .. 4)
  freq  - filter cut-off frequency (10 .. 24000)
  q     - quality factor of filter (0 .. 100)
```

As we see from description, the value for ```-hp``` and ```-lp``` parameters should consist of 3 or 4
colon-separated members.

The first member **type** specifies the type of the filter which consists of the prefix and
the postfix joined with underscore. The prefix describes the analog filter type:
  * RLC - Very smooth filters based on similar cascades of RLC contours.
  * BWC - Butterworth-Chebyshev-type-1 based filters.
  * LRX - Linkwitz-Riley based filters.
  * APO - Digital biquad filters derived from canonic analog biquad prototypes digitalized through Biilinear transform.
    These are [textbook filters](https://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html) which are implemented
    as in the [EqualizerAPO](https://equalizerapo.com/) software.

The postfix describes the filter transform type from the analog prototype to the digital form:
  * BT - Bilinear Z-transform is used for pole/zero mapping.
  * MT - Matched Z-transform is used for pole/zero mapping.
  * DR - Direct design is used to serve the digital filter coefficients directly in the digital domain,
    without performing transforms.

The **slope** parameter allows to control the slope of the filter.

The **freq** parameter sets the cut-off frequency of the filter (in Hz).

The **q** parameter allows to adjust the quality factor of the filter.

Also note that filter settings are fully compatible with the [LSP Parametric Equalizer](https://lsp-plug.in/?page=manuals&section=para_equalizer_x16_stereo) plugin series.

Here's the example of setting low-pass filter:

```
far-screamer -lp BWC_BT:2:4000:1.41
```

Trimming the IR file
===

Before the IR file will be applied to the output, the tool allows to cut some part of it's head and tail
and add linear fades to the head and the tail of the IR file after trimming. All values are pased in milliseconds
to the ```-hc```, ```-tc``` parameters for cutting head and tail respectively, and to the ```-fi``` and
```-fo``` parameters for adding fade-in and fade-out respectively.

These parameters work in the same manner to the [Impulse Reverb](https://lsp-plug.in/?page=manuals&section=impulse_reverb_stereo) plugin series
of the [LSP Plugins](https://lsp-plug.in/) bundle.

Adding pre-delay
===

By adding the additional delay to the wet (processed) signal we can enhance the spatial location of the recorded
audio source by moving it to the first plan. The parameter ```-pd``` allows to add the desired amount of milliseconds
of pre-delay to the wet signal.

Adjusting Dry/Wet balance
===

The tool allows to mix the unprocessed (dry) signal with the processed (wet) signal in the output file.

For that purpose parameters ```-dg``` and ```-wg``` are provided which are counted in dB.

By default, dry gain is set to -Inf dB and wet gain is set to 0 dB. Values less than -200 are considered to
be -Inf dB.

Adjusting Mid/Side balance for the stereo output signal
===

Any stereo signal can be split into middle component and side component. The middle component contains information
about how the left and right channels are same one to another. The side component, as an opposite, contains information
about how the left and right channels differ one to another. By raising gain of the side part or lowering gain of the
middle part the track becomes sounding wider. By the other side, raising middle part or lowering the side part results in
the narrower-sounding track.

Parameters ```-mb``` and ```-sb``` allow to set the amplification gain (in dB) of the middle and side components respectively. Values less than -200 dB are considered to be -Inf dB.

Performing audio processing chain mapping
===

There may be some non-trivial cases of applying convolution to the output file. For example, a purpose of
convolving 3-channel or 5-channel input file with a stereo impulse response may be possible.

This case is not trivial and can not be automatically handled by the software. By the other side, there is a way
to defining custom convolution mapping between input channel of the audio, input channel of the IR file and output
channel of the output file with the ```-m``` option.

The ```-m``` option allows to pass a 3 or 4 colon-separated values in the following format: ```out:in:ir[:gain]```.

Here:
  * **out** is a number of the channel for the output audio file.
  * **in** is a number of the channel for the input audio file.
  * **ir** is a number of the channel for the IR file.
  * **gain** is a gain applied to the result of convolution (in dB), by default 0 dB.

Command-line tool allows to specify multiple ```-m``` options. 

Note also that the ```-m``` option also specifies which dry (unprocessed) channel will be passed into
the corresponding output channel of the audio file. If there are multiple source channels of the input file
are mapped to the single output channel, then all these channels will be mixed together and passed to the
corresponding output channel. If there is a single input channel mapped multiple times to the same output channel,
it will be used only once in the dry mix.

Here is an example of mapping stereo IR file and stereo input file to the stereo output file with flipping
left and right channels:

```
far-screamer -m 1:0:0 -m 0:1:1

```

Requirements
======

The following packages need to be installed for building:

* gcc >= 4.9
* GNU make >= 4.0
* libsndfile
* libiconv

Building
======

To build the tool, perform the following commands:

```bash
make config # Configure the build
make fetch # Fetch dependencies from Git repository
make
sudo make install
```

To get more build options, run:

```bash
make help
```

To uninstall library, simply issue:

```bash
make uninstall
```

To clean all binary files, run:

```bash
make clean
```

To clean the whole project tree including configuration files, run:

```bash
make prune
```

To fetch all possible dependencies and make the source code tree portable between
different architectures and platforms, run:

```bash
make tree
```

To build source code archive with all possible dependencies, run:

```bash
make distsrc
```



