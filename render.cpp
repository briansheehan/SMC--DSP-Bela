/** SMC26 DSP course example: a tour of every synthesis voice.
 *
 *  Every sound source in dsp/Synthesis, one at a time, played with the
 *  same five knobs and one button - so you can hear what each technique
 *  sounds like before you build an instrument with it.
 *
 *  Press nextButton to move to the next voice. Its name, and what the
 *  five knobs do for it, is printed in the terminal. Press playButton
 *  to play: notes sound for as long as you hold it, drums and the
 *  struck/plucked models are hit once per press.
 *
 *  The voices, in order:
 *     0. white noise     - every frequency at once: "shhhh"
 *     1. coloured noise  - noise from dark (rumble) to bright (hiss)
 *     2. stepped noise   - a new random value at a set rate: bleeps
 *     3. subtractive     - a buzzy wave carved by a filter
 *     4. additive        - sine waves stacked into one sound
 *     5. FM              - one wave wobbling another's pitch: bells
 *     6. FM simple       - the same, with one envelope for both
 *     7. granular        - clouds of tiny slices of sound
 *     8. bass drum
 *     9. snare drum
 *    10. hi-hat
 *    11. Karplus-Strong  - a plucked string, from a burst of noise
 *    12. string voice    - a richer plucked/struck string model
 *    13. modal voice     - a struck object: bar, bell, bowl
 *    14. sampler         - a recorded drum loop, at any speed
 *
 *  Each voice uses all five knobs for its own settings - see gNames
 *  below. Mostly:
 *  knob0  the pitch (or the drum's tuning, or the noise's rate)
 *  knob1, three settings that change the sound
 *  knob2,
 *  knob3
 *  knob4  for the voices you hold, the release: how long the note
 *         fades out after you let go. For the drums and the
 *         struck/plucked models, how hard you hit them.
 *  The volume is fixed at -8 dB, a safe level for every voice.
 *
 *  To try it, copy this whole file into render.cpp and type
 *  "make run" in the terminal. Ctrl-C stops it. The sampler needs the
 *  samples/ folder on the Bela too.
 *
 *  Wiring:
 *    nextButton - D0     knob0 - A0     knob3 - A3
 *    playButton - D1     knob1 - A1     knob4 - A4
 *                        knob2 - A2
 *  Each button connects its pin to 3.3V when pressed, with a 10k
 *  resistor from the pin to GND (a pull-down).
 */

// The building blocks: the board (Bela.h), the sensors, and the
// synthesis, effects, filters and utilities. You do not need to
// change these.
#include <Bela.h>
#include <cmath>
#include "sensors/sensors.h"
#include "dsp/dsp.h"

// ---------------------------------------------------------------------
// 1. Sensors
// ---------------------------------------------------------------------

Button nextButton;
Button playButton;

// The five knobs do something different for every voice, so they are
// just numbered
Potentiometer knob0;
Potentiometer knob1;
Potentiometer knob2;
Potentiometer knob3;
Potentiometer knob4;

// Change false to true to print the sensor values in the terminal
bool gPrintValues = false;

// Prints every sensor's value on one line. Add a line here for each
// sensor you add. Only runs when gPrintValues is true.
void PrintValues(BelaContext *context)
{
	PrintValue("next", nextButton.IsPressed());
	PrintValue("play", playButton.IsPressed());
	PrintValue("A0", dsp::FullRange(knob0.Read(context)));
	PrintValue("A1", dsp::FullRange(knob1.Read(context)));
	PrintValue("A2", dsp::FullRange(knob2.Read(context)));
	PrintValue("A3", dsp::FullRange(knob3.Read(context)));
	PrintValue("A4", dsp::FullRange(knob4.Read(context)));
	PrintLineEnd();
}

// ---------------------------------------------------------------------
// 2. Sound
// ---------------------------------------------------------------------

// One of each voice. Only the one you picked makes sound.
dsp::Noise whiteNoise;
dsp::ColoredNoise coloredNoise;
dsp::SteppedNoise steppedNoise;
dsp::Adsr noiseGate; // the noises have no notes, so this fades them in and out
dsp::Subtractive subtractive;
dsp::Additive additive;
dsp::FM fm;
dsp::FMSimple fmSimple;
dsp::Granular granular;
dsp::SynthBassDrum kick;
dsp::SynthSnareDrum snare;
dsp::HiHat hiHat;
dsp::KarplusString karplus;
dsp::StringVoice stringVoice;
dsp::ModalVoice modal;
dsp::Sampler sampler;

// A name for every voice, in order. An enum gives each number a name,
// so the code below can say SNARE instead of 9.
enum Voice
{
	WHITE_NOISE, COLORED_NOISE, STEPPED_NOISE, SUBTRACTIVE, ADDITIVE, FM, FM_SIMPLE,
	GRANULAR, KICK, SNARE, HIHAT, KARPLUS, STRING_VOICE, MODAL, SAMPLER, NUM_VOICES
};

// What is printed when you move to a voice: what each knob does
const char* gNames[NUM_VOICES] = {
	"white noise    (A0: -, A1: attack, A2: decay, A3: sustain, A4: release)",
	"coloured noise (A0: colour, A1: attack, A2: decay, A3: sustain, A4: release)",
	"stepped noise  (A0: step rate, A1: attack, A2: decay, A3: sustain, A4: release)",
	"subtractive    (A0: pitch, A1: cutoff, A2: resonance, A3: filter sweep, A4: release)",
	"additive       (A0: pitch, A1: rolloff, A2: number of harmonics, A3: stretch, A4: release)",
	"FM             (A0: pitch, A1: mod depth, A2: mod ratio, A3: brightness while held, A4: release)",
	"FM simple      (A0: pitch, A1: mod depth, A2: mod ratio, A3: brightness follows volume, A4: release)",
	"granular       (A0: pitch, A1: scan speed, A2: grain size, A3: attack, A4: release)",
	"bass drum      (A0: tuning, A1: decay, A2: tone, A3: dirtiness, A4: pitch sweep)",
	"snare drum     (A0: tuning, A1: snappy, A2: decay, A3: pitch sweep, A4: how hard)",
	"hi-hat         (A0: tuning, A1: decay, A2: tone, A3: noisiness, A4: how hard)",
	"Karplus-Strong (A0: pitch, A1: damping, A2: brightness, A3: bridge/dispersion, A4: how hard)",
	"string voice   (A0: pitch, A1: structure, A2: brightness, A3: damping, A4: how hard)",
	"modal voice    (A0: pitch, A1: structure, A2: brightness, A3: damping, A4: how hard)",
	"sampler        (A0: speed, A1: start, A2: length, A3: reverse, A4: release)",
};

// The voice playing now
int gVoice = 0;

// The volume in dB: -8 dB is a safe level for every voice
float gVolumeDb = -8.f;

// ---------------------------------------------------------------------
// 3. render()
// ---------------------------------------------------------------------

// Called by the board over and over, very fast, each time for the
// next few samples of sound. Keep it quick: no printf() and no waiting
// in here, or the sound will click and drop out.
void render(BelaContext *context, void *userData)
{
	// Read the buttons first, so JustPressed() and JustReleased()
	// below see the latest change
	nextButton.Update(context);
	playButton.Update(context);

	// Next voice, wrapping back to the first after the last one.
	// rt_printf is Bela's printf for render(): use it only now and
	// then, like here once per press.
	if(nextButton.JustPressed())
	{
		gVoice = (gVoice + 1) % NUM_VOICES;
		rt_printf("%d: %s\n", gVoice, gNames[gVoice]);
	}

	// All knobs 0 - 1
	float k0 = dsp::FullRange(knob0.Read(context));
	float k1 = dsp::FullRange(knob1.Read(context));
	float k2 = dsp::FullRange(knob2.Read(context));
	float k3 = dsp::FullRange(knob3.Read(context));
	float k4 = dsp::FullRange(knob4.Read(context));

	// The pitch, for the voices that play notes: 55 Hz (A1) to
	// 880 Hz (A5)
	float freq = dsp::MapExponential(k0, 55.f, 880.f);

	// The release, for the voices you hold: 10 ms (stops at once)
	// to 3 s
	float release = dsp::MapExponential(k4, 0.01f, 3.f);

	// Give the knobs to the voice that is playing
	switch(gVoice)
	{
		case WHITE_NOISE:
		case COLORED_NOISE:
		case STEPPED_NOISE:
			coloredNoise.SetColor(k0);
			steppedNoise.SetFreq(dsp::MapExponential(k0, 50.f, 10000.f));
			// The noises have no notes of their own, so all four
			// stages of the envelope get a knob
			noiseGate.SetAttack(dsp::MapExponential(k1, 0.005f, 2.f));
			noiseGate.SetDecay(dsp::MapExponential(k2, 0.01f, 2.f));
			noiseGate.SetSustain(k3);
			noiseGate.SetRelease(release);
			break;
		case SUBTRACTIVE:
			subtractive.SetFreq(freq);
			subtractive.SetCutoff(dsp::MapExponential(k1, 100.f, 8000.f));
			subtractive.SetResonance(k2 * 0.9f); // 1 would whistle
			// How far the filter opens at the start of each note
			subtractive.SetFilterEnvAmount(k3 * 5000.f);
			subtractive.SetAmpRelease(release);
			break;
		case ADDITIVE:
			additive.SetFreq(freq);
			additive.SetRolloff(k1);
			// 1 (a pure sine) up to 16 harmonics. roundf() makes it a
			// whole number.
			additive.SetNumHarmonics((int)roundf(dsp::MapLinear(k2, 1.f, 16.f)));
			// Stretch: at 0 every harmonic is a whole number times the
			// pitch (in tune). Turned up, the higher ones are pushed
			// further up, out of tune: the sound turns into a bell.
			for(int h = 0; h < 16; h++)
				additive.SetHarmonicRatio(h, powf(h + 1, 1.f + k3 * 0.3f));
			additive.SetAmpRelease(release);
			break;
		case FM:
			fm.SetFreq(freq);
			fm.SetModDepth(k1 * 8.f);
			// Whole-number ratios 1 - 8 stay in tune; in between they
			// would sound clangy
			fm.SetModRatio(roundf(dsp::MapLinear(k2, 1.f, 8.f)));
			// The modulation has its own envelope: bright at the start
			// of the note, then down to this much while you hold it
			fm.SetModSustain(k3);
			fm.SetAmpRelease(release);
			break;
		case FM_SIMPLE:
			fmSimple.SetFreq(freq);
			fmSimple.SetModDepth(k1 * 8.f);
			fmSimple.SetModRatio(roundf(dsp::MapLinear(k2, 1.f, 8.f)));
			// 0: the tone stays the same for the whole note. 1: it gets
			// darker as the note gets quieter, like a real instrument.
			fmSimple.SetBrightness(k3);
			fmSimple.SetRelease(release);
			break;
		case GRANULAR:
			granular.SetFreq(freq);
			granular.SetSpeed(dsp::MapLinear(k1, -1.f, 1.f));
			granular.SetGrainSize(dsp::MapExponential(k2, 5.f, 90.f)); // in ms
			granular.SetAmpAttack(dsp::MapExponential(k3, 0.01f, 3.f));
			granular.SetAmpRelease(release);
			break;
		case KICK:
			kick.SetFreq(dsp::MapExponential(k0, 30.f, 120.f));
			kick.SetDecay(k1);
			kick.SetTone(k2);
			kick.SetDirtiness(k3);
			kick.SetFmEnvelopeAmount(k4);
			break;
		case SNARE:
			snare.SetFreq(dsp::MapExponential(k0, 100.f, 400.f));
			snare.SetSnappy(k1);
			snare.SetDecay(k2);
			snare.SetFmAmount(k3);
			snare.SetAccent(k4);
			break;
		case HIHAT:
			hiHat.SetFreq(dsp::MapExponential(k0, 1000.f, 8000.f));
			hiHat.SetDecay(k1);
			hiHat.SetTone(k2);
			hiHat.SetNoisiness(k3);
			hiHat.SetAccent(k4);
			break;
		case KARPLUS:
			karplus.SetFreq(freq);
			karplus.SetDamping(k1);
			karplus.SetBrightness(k2);
			// Below halfway a curved bridge (the pitch bends), above
			// it dispersion (a stiff, metallic string)
			karplus.SetNonLinearity(dsp::MapLinear(k3, -1.f, 1.f));
			// How hard to pluck is given to Pluck() below
			break;
		case STRING_VOICE:
			stringVoice.SetFreq(freq);
			stringVoice.SetStructure(k1);
			stringVoice.SetBrightness(k2);
			stringVoice.SetDamping(k3);
			stringVoice.SetAccent(k4);
			break;
		case MODAL:
			modal.SetFreq(freq);
			modal.SetStructure(k1);
			modal.SetBrightness(k2);
			modal.SetDamping(k3);
			modal.SetAccent(k4);
			break;
		case SAMPLER:
			// The loop's normal speed is at C4 (about 262 Hz) - see
			// SetRootFreq in dsp/Synthesis/Sampler.h
			sampler.SetFreq(freq);
			// The end is always after the start: A2 picks how much of
			// the rest of the loop plays
			sampler.SetStart(k1);
			sampler.SetEnd(k1 + k2 * (1.f - k1));
			// Above halfway, it plays backwards
			sampler.SetReverse(k3 > 0.5f);
			sampler.SetAmpRelease(release);
			break;
	}

	// playButton: start a note, or hit a drum / pluck a string
	if(playButton.JustPressed())
	{
		switch(gVoice)
		{
			case WHITE_NOISE:
			case COLORED_NOISE:
			case STEPPED_NOISE: noiseGate.NoteOn(); break;
			case SUBTRACTIVE: subtractive.NoteOn(); break;
			case ADDITIVE: additive.NoteOn(); break;
			case FM: fm.NoteOn(); break;
			case FM_SIMPLE: fmSimple.NoteOn(); break;
			case GRANULAR: granular.NoteOn(); break;
			case KICK: kick.Hit(); break;
			case SNARE: snare.Hit(); break;
			case HIHAT: hiHat.Hit(); break;
			// Never fully 0, so the string always makes a sound
			case KARPLUS: karplus.Pluck(dsp::MapLinear(k4, 0.2f, 1.f)); break;
			case STRING_VOICE: stringVoice.Strike(); break;
			case MODAL: modal.Strike(); break;
			case SAMPLER: sampler.NoteOn(); break;
		}
	}

	// Let go of every held note - also when you move to the next
	// voice, so no note is left playing
	if(playButton.JustReleased() || nextButton.JustPressed())
	{
		noiseGate.NoteOff();
		subtractive.NoteOff();
		additive.NoteOff();
		fm.NoteOff();
		fmSimple.NoteOff();
		granular.NoteOff();
		sampler.NoteOff();
	}

	// About 10 times a second, print the sensor values - only if
	// gPrintValues is true
	if(gPrintValues && TimeToPrint(context))
		PrintValues(context);

	// Runs once per sample - context->audioFrames samples each time
	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		// The next sample of the voice that is playing. The noises
		// are multiplied by noiseGate, which fades them in and out.
		float out = 0.f;
		switch(gVoice)
		{
			case WHITE_NOISE: out = whiteNoise.Process() * noiseGate.Process(); break;
			case COLORED_NOISE: out = coloredNoise.Process() * noiseGate.Process(); break;
			case STEPPED_NOISE: out = steppedNoise.Process() * noiseGate.Process(); break;
			case SUBTRACTIVE: out = subtractive.Process(); break;
			case ADDITIVE: out = additive.Process(); break;
			case FM: out = fm.Process(); break;
			case FM_SIMPLE: out = fmSimple.Process(); break;
			case GRANULAR: out = granular.Process(); break;
			case KICK: out = kick.Process(); break;
			case SNARE: out = snare.Process(); break;
			case HIHAT: out = hiHat.Process(); break;
			case KARPLUS: out = karplus.Process(); break;
			case STRING_VOICE: out = stringVoice.Process(); break;
			case MODAL: out = modal.Process(); break;
			case SAMPLER: out = sampler.Process(); break;
		}

		// Turned down to a safe level
		float sample = dsp::ApplyGain(out, gVolumeDb);

		// Send it to the left (0) and right (1) outputs
		audioWrite(context, n, 0, sample);
		audioWrite(context, n, 1, sample);
	}
}

// ---------------------------------------------------------------------
// 4. setup()
// ---------------------------------------------------------------------

// Runs once, before the sound starts
bool setup(BelaContext *context, void *userData)
{
	// Keeps fading sounds from overloading the CPU - leave this here
	dsp::FlushDenormals();

	// Tell each sensor which pin it is plugged into
	nextButton.Init(context, 0); // digital pin D0
	playButton.Init(context, 1); // digital pin D1
	knob0.Init(0);               // analog input A0
	knob1.Init(1);               // analog input A1
	knob2.Init(2);               // analog input A2
	knob3.Init(3);               // analog input A3
	knob4.Init(4);               // analog input A4

	// Every voice needs to know the sample rate (plain Noise doesn't)
	float sampleRate = context->audioSampleRate;
	whiteNoise.Init();
	coloredNoise.Init(sampleRate);
	steppedNoise.Init(sampleRate);
	noiseGate.Init(sampleRate);
	subtractive.Init(sampleRate);
	additive.Init(sampleRate);
	fm.Init(sampleRate);
	// A slower fade from the bright start of the note, so you can
	// hear it change
	fm.SetModDecay(0.5f);
	fmSimple.Init(sampleRate);
	// The note settles to half its volume, so you can hear the
	// brightness (A3) follow it
	fmSimple.SetDecay(0.6f);
	fmSimple.SetSustain(0.5f);
	granular.Init(sampleRate);
	kick.Init(sampleRate);
	snare.Init(sampleRate);
	hiHat.Init(sampleRate);
	karplus.Init(sampleRate);
	stringVoice.Init(sampleRate);
	modal.Init(sampleRate);

	// The sampler reads its sound file now - never in render(). It
	// loops for as long as playButton is held.
	sampler.Init(sampleRate);
	sampler.Load("samples/drum_loop.wav");
	sampler.SetLoop(true);

	// Say which voice we start with
	rt_printf("%d: %s\n", gVoice, gNames[gVoice]);

	// From here on, the board keeps calling render()
	return true;
}

// Runs once when the program stops
void cleanup(BelaContext *context, void *userData)
{
	// Nothing to do here
}
