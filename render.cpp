/** SMC26 DSP course example: granular drone.
 *
 *  Granular synthesis chops a sound into tiny pieces called grains -
 *  a few thousandths of a second each - and plays lots of them, one
 *  after another and overlapping. How long the grains are, and how
 *  they move through the sound, changes its texture from smooth to
 *  buzzy to fluttering - without changing its pitch.
 *
 *  Here three dsp::Granular voices - a root note, a fifth above and an
 *  octave above - make a slow, evolving drone, with a long echo
 *  behind it for space. The drone starts by itself when the board
 *  turns on.
 *
 *  Press droneButton to fade the drone out, and again to fade it back
 *  in. Press freezeButton to freeze the grains in place (they stop
 *  moving), and again to let them move.
 *
 *  pitchKnob  picks the root note (A minor scale, one octave up/down)
 *  grainKnob  grain length: short = buzzy and grainy, long = smooth
 *  motionKnob how fast the grains move: still, up to a shimmering flutter
 *  detuneKnob pulls the three voices slightly out of tune with each
 *             other, for a wider, wavier sound
 *  spaceKnob  how much echo. All the way down = dry
 *
 *  To try it, copy this whole file into render.cpp and type
 *  "make run" in the terminal. Ctrl-C stops it.
 *
 *  Wiring:
 *    droneButton  - D0     pitchKnob  - A0
 *    freezeButton - D1     grainKnob  - A1
 *                          motionKnob - A2
 *                          detuneKnob - A3
 *                          spaceKnob  - A4
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
Ultrasonic ultra;

//Potentiometer pitchKnob;
//Potentiometer grainKnob;
//Potentiometer motionKnob;
Potentiometer detuneWobbler;
//Potentiometer spaceKnob;


// Change false to true to print the sensor values in the terminal
bool gPrintValues = false;

// Prints every sensor's value on one line. Add a line here for each
// sensor you add. Only runs when gPrintValues is true.
void PrintValues(BelaContext *context)
{
	//PrintValue("droneOff", droneButton.Toggled());
	//PrintValue("frozen", freezeButton.Toggled());
	PrintValue("Ultra", ultra.Read(context));
	//PrintValue("pitch", dsp::FullRange(pitchKnob.Read(context)));
	//PrintValue("grain", dsp::FullRange(grainKnob.Read(context)));
	//PrintValue("motion", dsp::FullRange(motionKnob.Read(context)));
	//PrintValue("piezo", dsp::FullRange(piezo.Read(context)));	
	PrintValue("detune", dsp::FullRange(detuneWobbler.Read(context)));
	//PrintValue("space", dsp::FullRange(spaceKnob.Read(context)));
	PrintLineEnd();
}

// ---------------------------------------------------------------------
// 2. Sound
// ---------------------------------------------------------------------

// Three granular voices. voices[0] plays the root note, voices[1] and
// voices[2] the notes above it.
const int kNumVoices = 3;
dsp::Granular voices[kNumVoices];

// How many semitones above the root each voice plays: the root
// itself, a fifth (7) and an octave (12) - a calm, open chord
float gVoiceSemitones[kNumVoices] = {0.f, 7.f, 12.f};

// Each voice's grains move at a different speed - one even goes
// backwards (negative) - so the three never move in step, and the
// drone keeps slowly changing
float gVoiceMotion[kNumVoices] = {1.f, -0.7f, 0.45f};

// Which way each voice is pulled out of tune by detuneWobbler: the root
// stays put, one goes sharp (+1), one goes flat (-1)
float gVoiceDetune[kNumVoices] = {1.f, 1.f, 1.f};

// A long echo behind the drone, for a sense of space
//dsp::Delay echo;

// The note pitchKnob is centered on
float gRootNote = dsp::NoteToFreq("A2");

float gUltraMax = 22.f; // Max reading (in cm) I expect: it is diameter of tambourine
float gUltraThreshCeiling = 16.f; // Readings above this count as off
float gUltraThreshFloor = 5.f; //Readings below this get squirrelly.

float gWobblerMaxRaw = 0.5;
float gWobblerZeroRaw = 0.376;
float gWobblerMax = gWobblerMaxRaw - gWobblerZeroRaw;
float gWobblerScaling = 1/gWobblerMax;

// ---------------------------------------------------------------------
// 3. render()
// ---------------------------------------------------------------------

// Called by the board over and over, very fast, each time for the
// next few samples of sound. Keep it quick: no printf() and no waiting
// in here, or the sound will click and drop out.
void render(BelaContext *context, void *userData)
{
	// Read the buttons first, so JustPressed() below sees the latest
	// press
	//droneButton.Update(context);
	//freezeButton.Update(context);

	// Toggled() flips on every press. It starts false while the drone
	// is already playing - so true means "fade it out".
	// if(droneButton.JustPressed())
	// {
	// 	for(int v = 0; v < kNumVoices; v++)
	// 	{
	// 		if(droneButton.Toggled())
	// 			voices[v].NoteOff();
	// 		else
	// 			voices[v].NoteOn();
	// 	}
	// }
	float rawDistance = ultra.Read(context);
	float distanceNorm = rawDistance/gUltraMax;

	if (distanceNorm > 0.9)
	{
		for(int v = 0; v < kNumVoices; v++)
		{
			voices[v].NoteOff();
		}	
	} else
	{
		for(int v = 0; v < kNumVoices; v++)
		{
			voices[v].NoteOn();
		}	
	}
	// Grain length from 5 ms (buzzy) to 80 ms (smooth). MapExponential
	// so the short end gets as much of the knob as the long end (see
	// dsp/Utility.h).false
	//float grainMs = dsp::MapExponential(dsp::FullRange(grainKnob.Read(context)), 5.f, 80.f);
	float grainMs = dsp::MapExponential(distanceNorm, 5.f, 250.f);
	// Root note: snaps to the notes of A minor, an octave either way
	//float root = dsp::MapSemitones(dsp::FullRange(pitchKnob.Read(context)), gRootNote, 12.f, dsp::SCALE_MINOR);

	float root = dsp::MapSemitones(dsp::FullRange(distanceNorm), gRootNote, 18.f, dsp::SCALE_MINOR);
//	float root = dsp::MapPitchBend(dsp::FullRange(distanceNorm), gRootNote, 12.f);

	// Detune: up to 25 cents (a quarter of a semitone) - enough to
	// make the voices "beat" against each other slowly
	float wobbler = (dsp::FullRange(detuneWobbler.Read(context)) - gWobblerZeroRaw) * gWobblerScaling; //gives approx -1 to +1
	//wobbler = dsp::Clamp(wobbler, -1, 1);
	float detuneCents = dsp::MapLinear(wobbler * -1, 0.f, 500.f);



	// Motion: how fast the grains move through the sound. 0 when
	// frozen, otherwise from 0 (still) up to 0.3.
	//float motion = dsp::MapLinear(dsp::FullRange(motionKnob.Read(context)), 0.f, 0.3f);
	float motion = dsp::MapLinear((wobbler + 1)/2, 0.f, 0.3f);

	// if(freezeButton.Toggled())
	// 	motion = 0.f;



	for(int v = 0; v < kNumVoices; v++)
	{
		// Each voice: its interval above the root, plus its share of
		// the detune. 100 cents = 1 semitone, and every semitone up
		// multiplies the pitch by 2^(1/12).
		float semitones = gVoiceSemitones[v] + gVoiceDetune[v] * detuneCents / 100.f;
		voices[v].SetFreq(root * powf(2.f, semitones / 12.f));

		voices[v].SetGrainSize(grainMs);
		voices[v].SetSpeed(motion * gVoiceMotion[v]);
	}

	// Echo: from dry up to half echo, half drone
	//echo.SetMix(dsp::MapLinear(dsp::FullRange(spaceKnob.Read(context)), 0.f, 0.5f));
	//echo.SetMix(dsp::MapLinear(dsp::FullRange(1), 0.f, 0.5f));

	// About 10 times a second, print the sensor values - only if
	// gPrintValues is true
	if(gPrintValues && TimeToPrint(context))
		PrintValues(context);

	// Runs once per sample - context->audioFrames samples each time
	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		// Add the three voices together
		float sample = 0.f;
		for(int v = 0; v < kNumVoices; v++)
			sample += voices[v].Process();

		// Three voices added up get loud, so turn the mix down by
		// 12 dB (see dsp::ApplyGain in dsp/Utility.h), then add the echo
		//sample = echo.Process(dsp::ApplyGain(sample, -12.f));
		sample = dsp::ApplyGain(sample, -12.f);

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
	//droneButton.Init(context, 0);  // digital pin D0
	//freezeButton.Init(context, 1); // digital pin D1
	ultra.Init(context, 0, 1); // trig pin on 2, echo on 3
	//pitchKnob.Init(0);             // analog input A0
	//grainKnob.Init(1);             // analog input A1
	//motionKnob.Init(2);            // analog input A2
	//piezo.Init(1);
	detuneWobbler.Init(0);            // analog input A0
	//spaceKnob.Init(4);             // analog input A4

	float sampleRate = context->audioSampleRate;

	// A drone should fade in and out slowly: 3 seconds in, 4 seconds
	// out, and stay at full volume in between (sustain 1)
	for(int v = 0; v < kNumVoices; v++)
	{
		voices[v].Init(sampleRate);
		voices[v].SetAmpAttack(3.f);
		voices[v].SetAmpSustain(1.f);
		voices[v].SetAmpRelease(4.f);
	}

	// A slow echo that repeats for a long time
	// echo.Init(sampleRate);
	// echo.SetTimeMs(700.f);
	// echo.SetFeedback(0.0f);

	// Start the drone straight away
	// for(int v = 0; v < kNumVoices; v++)
	// 	voices[v].NoteOn();

	// From here on, the board keeps calling render()
	return true;
}

// Runs once when the program stops
void cleanup(BelaContext *context, void *userData)
{
	// Nothing to do here
}
