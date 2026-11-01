<h1 align="center">SMC26 DSP for Bela</h1>

<p align="center">
  Build sensor-driven music instruments on the <a href="https://learn.bela.io/">Bela</a> Gem Stereo, no coding experience needed.<br>
  Everything you write goes in one file: <code>render.cpp</code>.
</p>

---

This is the Bela version of the course project [SMC26---DSP](https://github.com/MarLaslo/SMC26---DSP) for the Daisy Seed.
Both have the same sounds, effects, sensors and example instruments, each written the way that is natural on its board.
This one uses Bela's words and functions, so you can read the [Bela documentation](https://learn.bela.io/) and examples alongside it.

## What's in here

| Folder / file | What it is |
| --- | --- |
| [`render.cpp`](render.cpp) | **Your instrument.** The only file you edit. |
| [`examples/`](examples/) | Finished instruments to copy into `render.cpp` and learn from |
| [`dsp/`](dsp/) | Ready-made sound building blocks: synths, effects, filters, envelopes, sequencer |
| [`sensors/`](sensors/) | One small class per sensor: knobs, buttons, touch, accelerometer, … |
| [`samples/`](samples/) | Sound files for the sampler. Put your own WAV files here |
| `libs/DaisySP` | The parts of the [DaisySP](https://github.com/daisyaudio/DaisySP) library that `dsp/` uses where Bela has no library of its own |
| [`Makefile`](Makefile) | Builds and runs the project on the Bela from your computer's terminal |

You don't need to open `dsp/`, `sensors/` or `libs/` to build an instrument. They are there if you're curious.

> The examples end in `.cpp.txt` instead of `.cpp`. Bela builds every `.cpp` file in a project, and each example is a complete
> instrument of its own, so they are stored as text. Open one, copy all of it, and paste it into `render.cpp`.

## Getting started

### 1. Connect the Bela

Plug the Bela into your computer with a USB cable and wait about a minute for it to start.
Then open the Bela IDE in your browser: [http://192.168.7.2](http://192.168.7.2) (on Windows: [http://192.168.6.2](http://192.168.6.2), or try [http://bela.local](http://bela.local)).
If the page opens, the board is ready.

### 2. Get the code

```sh
git clone https://github.com/MarLaslo/SMC--DSP-Bela.git
```

**No git?** Download the project as a ZIP instead: [SMC--DSP-Bela-main.zip](https://github.com/MarLaslo/SMC--DSP-Bela/archive/refs/heads/main.zip)
(or on GitHub: **Code → Download ZIP**).

### 3. Build and run

The code is built on the Bela itself, not on your computer. There are two ways to send it there. Use either one.

**A. In the Bela IDE** (any computer, nothing to install)

1. Upload the whole project folder (or its ZIP) into a new project in the IDE.
2. Open `render.cpp` in the IDE, edit it there, and click **Build & Run** (the ▶ button).
3. The **Stop** button (■) stops it.

**B. From the terminal** (macOS and Linux: needs `make`, `ssh` and `rsync`, which are already installed)

Edit `render.cpp` on your computer, in any editor (VS Code is recommended). Then, inside the project folder:

```sh
make run
```

This copies the project to the Bela, builds it there and starts it. **Ctrl-C** stops it.
The first build takes a minute or two; after that only what changed is rebuilt.

| Command | What it does |
| --- | --- |
| `make run` | Copy, build and run (Ctrl-C stops) |
| `make build` | Copy and build only |
| `make stop` | Stop whatever is running on the Bela |
| `make cpu` | Show how busy the CPU is (see [Troubleshooting](#troubleshooting)) |

The first time, `ssh` asks if you trust the board: type `yes`. The Bela has no password.

### 4. See the sensor values (optional)

In `render.cpp`, change `bool gPrintValues = false;` to `true`, then build and run again.
The Bela now prints every sensor's value about 10 times a second: in the terminal with `make run`, or in the IDE's console.

## How render.cpp is structured

Every `render.cpp` (and every example) has the same four parts, top to bottom:

| Part | What goes there |
| --- | --- |
| **1. Sensors** | Declare your sensors, and list them in `PrintValues()` |
| **2. Sound** | Declare your `dsp::` objects: oscillators, synths, effects, … |
| **3. render()** | Called over and over, very fast. At the top: read the sensors and pass their values to your `dsp::` objects. Below: the loop that makes the sound, one sample at a time |
| **4. setup()** | Runs once at the start: gives each sensor its pin and sets up the `dsp::` objects |

At the very end is `cleanup()`, which runs when the program stops. Usually there is nothing to do there.

The idea: **at the top of `render()`, read the sensors and pass their values to your `dsp::` objects; in the loop below, turn them into sound.**

To start from a blank file, copy [`examples/main_template.cpp.txt`](examples/main_template.cpp.txt) into `render.cpp`.

### Example: a knob that controls the pitch of a tone

This is [`examples/beginner/02_one_knob.cpp.txt`](examples/beginner/02_one_knob.cpp.txt), without the comments:

```cpp
#include <Bela.h>
#include "sensors/sensors.h"
#include "dsp/dsp.h"

// 1. Sensors
Potentiometer pitchKnob;

bool gPrintValues = false;

void PrintValues(BelaContext *context)
{
	PrintValue("pitchKnob", dsp::FullRange(pitchKnob.Read(context)));
	PrintLineEnd();
}

// 2. Sound
dsp::Oscillator osc;

// 3. render()
void render(BelaContext *context, void *userData)
{
	float knob = dsp::FullRange(pitchKnob.Read(context));
	float pitch = dsp::MapExponential(knob, 110.f, 880.f);
	osc.SetFreq(pitch);

	if(gPrintValues && TimeToPrint(context))
		PrintValues(context);

	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		float sample = osc.Process();
		audioWrite(context, n, 0, sample);
		audioWrite(context, n, 1, sample);
	}
}

// 4. setup()
bool setup(BelaContext *context, void *userData)
{
	dsp::FlushDenormals();

	pitchKnob.Init(0); // analog input A0

	osc.Init(context->audioSampleRate);
	osc.SetWaveform(dsp::Oscillator::WAVE_SINE);
	osc.NoteOn();
	return true;
}

void cleanup(BelaContext *context, void *userData)
{
}
```

> **Keep `render()` quick.** It runs in real time, thousands of times a second. Never use `printf()`, `new`, files or waiting in it,
> or the sound will click and drop out. Create everything in part 2 and set it up in `setup()`. To print, use `PrintValues()`.

> Leave `dsp::FlushDenormals();` at the top of `setup()`. Without it, sounds that slowly fade out (delay echoes, long releases)
> can suddenly make the CPU very busy.

## Examples

To try one, open it, copy the whole file into `render.cpp`, then build and run. Each example lists its wiring at the top of the file.
Save your own `render.cpp` somewhere first if you want to keep it.

**Start here, in order:**

| Example | What you learn |
| --- | --- |
| [`beginner/01_hello_sound`](examples/beginner/01_hello_sound.cpp.txt) | Make a tone |
| [`beginner/02_one_knob`](examples/beginner/02_one_knob.cpp.txt) | Change the tone with a knob |
| [`beginner/03_one_button`](examples/beginner/03_one_button.cpp.txt) | Play notes with buttons |

**Building blocks:**

| Example | What it shows |
| --- | --- |
| [`main_template`](examples/main_template.cpp.txt) | The empty starting point |
| [`sensor_printing`](examples/sensor_printing.cpp.txt) | Printing sensor values |
| [`effects_chain`](examples/effects_chain.cpp.txt) | Running a sound through several effects |
| [`voice_manager`](examples/voice_manager.cpp.txt) | Playing several notes at once |
| [`lfo_vibrato`](examples/lfo_vibrato.cpp.txt) | An LFO moving the pitch of a tone, with all five LFO waveforms |

**Demos**: every building block of one kind in one file, played with 2 buttons (D0 = next, D1 = play, bypass or action) and the knobs: A0, A1, and in `synthesis_demo` all five (A0–A4 = the voice's settings, the volume is fixed). The name of what's playing is printed in the terminal:

| Example | What you hear |
| --- | --- |
| [`demos/synthesis_demo`](examples/demos/synthesis_demo.cpp.txt) | Every synthesis voice: noises, subtractive, additive, FM, granular, drums, physical models, sampler |
| [`demos/filters_demo`](examples/demos/filters_demo.cpp.txt) | Every filter and mode, on a saw drone |
| [`demos/effects_demo`](examples/demos/effects_demo.cpp.txt) | Every effect, on a melody that plays by itself |
| [`demos/modulation_demo`](examples/demos/modulation_demo.cpp.txt) | AD envelope, LFO, sample & hold, sequencer, voice manager and limiter |

**Bela extras**, no extra hardware needed:

| Example | What it shows |
| --- | --- |
| [`scope`](examples/scope.cpp.txt) | See a waveform, before and after a filter, on Bela's oscilloscope ([192.168.7.2/scope](http://192.168.7.2/scope)) |
| [`gui_sliders`](examples/gui_sliders.cpp.txt) | Play a synth with sliders in the browser ([192.168.7.2/gui](http://192.168.7.2/gui)): try a sound before anything is wired |

**Instruments** ([`examples/instruments/`](examples/instruments/)):
`additive`, `drum_sequence`, `finger_drums`, `fm_filter_envelope`, `fm_simple`, `granular_drone`,
`modal_sequence`, `sample_hold_robot`, `sampler`, `strummed_strings`, `subtractive_wobble`, `wind_ocean`.
They all use the course board: 5 knobs on A0–A4 and 2 buttons on D0 and D1.

## Sound building blocks (`dsp::`)

All of these are included at the top of `render.cpp` already (`#include "dsp/dsp.h"`). Each one is a small class with the same shape:

```cpp
dsp::Oscillator osc;                  // declare it (part 2)
osc.Init(context->audioSampleRate);   // set it up once, in setup()
osc.SetFreq(440.f);                   // change settings, at the top of render()
float sample = osc.Process();         // get the next sample, in the loop in render()
```

Effects and filters take the sound in: `sample = delay.Process(sample);`.
A few don't need the sample rate and are set up with a plain `Init()`: `Noise`, `Overdrive`, `WaveFolder`, `SampleRateReducer` and `Limiter`.

`Sampler` also needs a sound: `sampler.Load("samples/drum_loop.wav");` in `setup()` (never in `render()`: it reads the whole file into memory),
or record one live from the audio input with `StartRecording()`, `Record(input)` and `StopRecording()`.
With the IDE, upload the `samples/` folder along with the rest of the project.
Open a class's header file in `dsp/` to see all its settings. Every header starts with a short explanation and a usage example.

### Synthesis: [`dsp/Synthesis/`](dsp/Synthesis/)

Start and stop notes with `NoteOn()` / `NoteOff()`. The noise sources have no notes: they play all the time.

| Class | Sound |
| --- | --- |
| `Oscillator` | A single waveform with a volume envelope. The simplest voice: start here |
| `Subtractive` | A bright oscillator carved down by a resonant filter |
| `Additive` | A stack of sine waves added together |
| `FM`, `FMSimple` | Frequency modulation: bells, electric pianos, metallic tones |
| `KarplusString`, `StringVoice` | Physical model of a plucked string |
| `ModalVoice` | Physical model of a struck object: bells, bars, bowls |
| `Granular` | Clouds of tiny sound grains, for drones and textures |
| `Sampler` | Plays a WAV file, or a sound recorded from the audio input: faster, slower, backwards, or a slice in a loop |
| `SynthBassDrum`, `SynthSnareDrum`, `HiHat` | Drum sounds |
| `Noise`, `ColoredNoise`, `SteppedNoise` | Noise: for wind, percussion, random modulation |

### Effects: [`dsp/Effects/`](dsp/Effects/)

`Overdrive`, `WaveFolder`, `SampleRateReducer` (lo-fi), `Tremolo`, `Delay`, `Chorus`, `Flanger`, `Phaser`, `PitchShifter`, `Reverb`.

`Reverb` works in mono or stereo, and is big: declare it in part 2 (never inside a function).

### Filters: [`dsp/Filters/`](dsp/Filters/)

| Class | Use |
| --- | --- |
| `StateVariableFilter` | All-round filter: lowpass, highpass, bandpass, notch, peak |
| `LadderFilter` | Classic Moog-style resonant filter |
| `OnePoleFilter` | Gentle lowpass/highpass, also good for smoothing a jumpy sensor |
| `AllPassFilter` | Bandpass or notch around one frequency |

### Envelopes, modulation and control: [`dsp/Other/`](dsp/Other/)

| Class | Use |
| --- | --- |
| `Adsr` | Attack-Decay-Sustain-Release envelope |
| `AdEnvelope` | Attack-Decay envelope, for hits and plucks |
| `Lfo` | A slow wobble for vibrato, tremolo or a moving filter |
| `SampleHold` | Turns a signal (or random values) into steps - the "random robot" bleeps of analog synths |
| `Sequencer` | A loop of notes that plays itself at a set tempo |
| `VoiceManager` | Turns any synth into a polyphonic one (several notes at once) |
| `Limiter` | Keeps the output from getting too loud and distorting |

### Helpers: [`dsp/Utility.h`](dsp/Utility.h)

Sensors give a number from `0` to `1`. These turn it into something useful:

| Function | What it does |
| --- | --- |
| `dsp::FullRange(value)` | Stretches a 3.3 V sensor's reading to the whole `0`–`1` (see [Sensors](#sensors)) |
| `dsp::MapLinear(value, min, max)` | `0`–`1` → `min`–`max`, evenly |
| `dsp::MapExponential(value, min, max)` | Same, but feels natural for pitch and time (like a piano keyboard) |
| `dsp::MapLog(value, min, max)` | The reverse of `MapExponential`: a value between `min` and `max` back to `0`–`1` |
| `dsp::MapSemitones(value, base_freq, semitone_range, scale)` | Snap a knob to musical notes, optionally only the notes of a scale (`dsp::SCALE_MAJOR`, `dsp::SCALE_MINOR`) |
| `dsp::MapPitchBend(value, base_freq, semitone_range)` | Bend a note up and down |
| `dsp::NoteToFreq("A4")` | A note name → its frequency in Hz |
| `dsp::ApplyGain(sample, db)`, `dsp::DbToLinear(db)`, `dsp::LinearToDb(x)` | Volume in decibels |
| `dsp::Clamp(value, min, max)` | Keep a value within a range |

## Sensors

Every sensor is already included (`#include "sensors/sensors.h"`). For each sensor:

1. declare it in part 1, e.g. `Potentiometer knob1;`
2. give it a pin in `setup()`, e.g. `knob1.Init(0);` for A0
3. read it at the top of `render()`, e.g. `float value = dsp::FullRange(knob1.Read(context));`

**Pins on the course board:** analog A0–A4 (channels `0`–`4`), digital D0 and D1 (pins `0` and `1`).
Any pin of the right type works: the Bela has 8 analog inputs and 16 digital pins.

### Analog sensors

All return a value from `0.0` to `1.0` with `Read(context)`.

| Sensor | Class | Init (in `setup()`) | Extras |
| --- | --- | --- | --- |
| Potentiometer (knob) | `Potentiometer` | `knob.Init(0)` | |
| Piezo disc | `Piezo` | `piezo.Init(1)` | |
| Stretch cord | `StretchSensor` | `stretch.Init(2)` | |
| Force sensor (FSR) | `ForceSensor` | `force.Init(3)` | `IsPressed(threshold)` |
| Joystick (KY-023) | `Joystick` | `joy.Init(context, xChannel, yChannel, swPin)` | `X()`, `Y()` from `-1` to `1`; call `Update(context)` at the top of `render()` |

**Wrap 3.3 V sensors in `dsp::FullRange()`.** Bela's analog inputs measure up to 4.096 V, so a sensor powered from 3.3 V
only reaches about `0.8`. `dsp::FullRange(knob.Read(context))` stretches it back to the whole `0`–`1`.
Not needed for the joystick (it measures from its centre) or the piezo.

Stretch and force sensors need a voltage divider: `3.3V → sensor → pin → 10 kΩ → GND`.

### Digital

| Sensor | Class | Init (in `setup()`) | Use |
| --- | --- | --- | --- |
| Button | `Button` | `button.Init(context, 0)` | Call `Update(context)` at the top of `render()` |
| Ultrasonic HC-SR04 | `Ultrasonic` | `sonar.Init(context, 2, 3)` (Trig, Echo) | `Read(context)` → cm, or `-1` |

**Buttons need a pull-down resistor:** the button between the pin and 3.3 V, and `10 kΩ` from the pin to GND.
The Bela has no built-in resistor you can use instead. Wired the other way round (10 kΩ to 3.3 V, button to GND)? Use `button.Init(context, 0, true)`.

`Button` gives you `IsPressed()`, `JustPressed()`, `JustReleased()`, `IsHeld(ms)`, `HeldTime()`, `LastPressTime()`, `Count()` and `Toggled()`. Debouncing is built in.
Call `Update(context)` on every button at the start of `render()`, before you check it.

The ultrasonic sensor never makes `render()` wait. Up to 4 can be used; they take turns, so each updates about every 60 ms × number of sensors.
**Power it from 3.3 V**: the Bela's digital pins can be damaged by 5 V.

### I²C sensors

These connect to the Bela's I²C pins (SDA, SCL). `Init()` returns `false` if the sensor isn't found, so check it in `setup()`.
Call `Update(context)` at the top of `render()`, then use the values. The sensor is read in the background, so it never slows the sound down.

| Sensor | Class | Values |
| --- | --- | --- |
| Accelerometer (Adafruit MMA8451) | `Accelerometer` | `x`, `y`, `z` in m/s² (±2 g) |
| Barometer (Adafruit BMP585) | `Barometer` | `temperature` in °C, `pressure` in Pa |
| Capacitive touch (Adafruit MPR121) | `CapacitiveTouch` | `IsTouched(pad)`, `JustTouched(pad)`, `JustReleased(pad)`, `Raw(pad)` for pads 0–11 |

```cpp
Accelerometer accel;

bool setup(BelaContext *context, void *userData)
{
	if(!accel.Init())
	{
		printf("Accelerometer not found, check the wiring\n");
		return false;
	}
	...
}
```

`printf()` is fine in `setup()`, just never in `render()`. Returning `false` from `setup()` stops the program.

`CapacitiveTouch::Init(touch, release)` takes optional thresholds (default `12`, `6`). Lower is more sensitive.

## Printing values

Add one line per sensor to `PrintValues()`:

```cpp
void PrintValues(BelaContext *context)
{
	PrintValue("button1", button1.IsPressed());                 // prints 1 or 0
	PrintValue("knob1", dsp::FullRange(knob1.Read(context)));   // prints e.g. 0.512
	PrintLineEnd();
}
```

Turn printing on with `bool gPrintValues = true;`. It prints about 10 times a second, which is safe.
Never put `printf()` in `render()` yourself.

## Troubleshooting

| Problem | Try |
| --- | --- |
| The IDE page doesn't open | Wait a minute after plugging in. Try the other address in [step 1](#1-connect-the-bela), or another USB cable (some only charge) |
| `make run`: `Could not resolve hostname` or `Connection refused` | The Bela isn't connected or hasn't finished starting. Check that the IDE page opens |
| `make run`: `Host key verification failed` | The board was reinstalled and `ssh` no longer recognises it. Run `ssh-keygen -R 192.168.7.2`, then `make run` again |
| Build error in an example's copy | Copy the **whole** example file, from the very first line to the last, so it replaces all of `render.cpp` |
| `multiple definition of render` (or `setup`) | A second `.cpp` file has its own `render()`. Examples must stay `.cpp.txt` |
| Runs, but no sound | Did you call `NoteOn()`? Is the volume up? Are headphones in the **output** jack? |
| A knob only reaches about 0.8 | Wrap it in `dsp::FullRange()` |
| `Sampler: can't open ...` in the terminal | The file isn't on the Bela, or its name is spelled differently. The path starts at the project folder: `samples/name.wav` |
| A button doesn't react | Is `Update(context)` called at the top of `render()`? Does it have its 10 kΩ pull-down resistor? |
| An I²C sensor `Init()` returns `false` | Check SDA and SCL aren't swapped, and the sensor has power and GND |
| Clicks or dropouts | Something slow is in `render()`: `printf()`, `new`, a file, or waiting. Run `make cpu` while it plays: the `MSW` (mode switch) number of your program should stay the same. If it keeps going up, something in `render()` isn't real-time safe |

## Links

- [Bela documentation](https://learn.bela.io/) · [Bela C++ API](https://docs.bela.io/)
- [Bela forum](https://forum.bela.io/)
- The Daisy version of this course: [MarLaslo/SMC26---DSP](https://github.com/MarLaslo/SMC26---DSP)

## Credits

The course project, the `dsp::` library and the examples come from [MarLaslo/SMC26---DSP](https://github.com/MarLaslo/SMC26---DSP), ported to Bela here.
Check [MarLaslo/SMC--DSP-Bela](https://github.com/MarLaslo/SMC--DSP-Bela) for the latest version.

Sensor classes from SMCWinterSchool_Bela.
Built on [Bela](https://github.com/BelaPlatform/Bela)'s libraries and on [DaisySP](https://github.com/daisyaudio/DaisySP) by Daisy, formerly Electro-Smith (MIT).
`Reverb` uses `ReverbSc` from DaisySP-LGPL, under the LGPL (see `libs/DaisySP/DaisySP-LGPL/LICENSE`).
