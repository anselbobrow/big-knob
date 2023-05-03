#include "daisy_seed.h"
#include "daisysp.h"

#define NUM_OUT_PINS 13
#define NUM_IN_PINS  9
#define NUM_KNOBS    18
#define NUM_MUXES    3

using namespace daisy;
using namespace daisy::seed;
using namespace daisysp;

enum struct INS {
    FLT1 = 0,
    FLT2 = 1,
    FLT3 = 2,
    // ENV1 = 3,
    // ENV2 = 4,
    // ENV3 = 5,
    REVERB = 3,
    // DELAY = 4,
    SPEAKERS = 4,
};

enum struct OUTS {
    OSC1 = 0,
    OSC2 = 1,
    OSC3 = 2,
    // NOISE1 = 3,
    // NOISE2 = 4,
    FLT1 = 3,
    FLT2 = 4,
    FLT3 = 5,
    // ENV1 = 8,
    // ENV2 = 9,
    // ENV3 = 10,
    REVERB = 6,
    // DELAY = 12,
};

DaisySeed hw;

Oscillator osc1;
Biquad     flt1;
ReverbSc   reverb;

float levels[17];
GPIO  power;
GPIO  outPins[NUM_OUT_PINS];
GPIO  inPins[NUM_IN_PINS];
int   patchbay[NUM_OUT_PINS][NUM_IN_PINS];

// for 3-way switch inputs
int pot_switch(float n) {
    float val = n * 3;
    if (val > 2) {
        return 3;
    }
    if (val > 1) {
        return 2;
    }
    return 1;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size) {
    // if power switch is off, no sound
    if (!power.Read()) {
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;
            out[1][i] = 0.0f;
        }
        return;
    }

    for (int i = 0; i < 4; i++) {
        levels[i] = hw.adc.GetMuxFloat(0, i);
    }

    // set levels
    osc1.SetFreq(levels[0] * 1000);

    flt1.SetRes(levels[2] * 0.5);
    flt1.SetCutoff(100 + (levels[3] * 1000));

    int wf = pot_switch(levels[1]);
    if (wf == 1) {
        osc1.SetWaveform(Oscillator::WAVE_SIN);
    } else if (wf == 2) {
        osc1.SetWaveform(Oscillator::WAVE_TRI);
    } else if (wf == 3) {
        osc1.SetWaveform(Oscillator::WAVE_SAW);
    }

    // generate samples
    for (size_t i = 0; i < size; i++) {
        float sig1 = osc1.Process();

        if (patchbay[OUTS::OSC1][INS::FLT1]) {
            sig1 = flt1.Process(sig1);
        } else if (patchbay[OUTS::OSC1][INS::FLT2]) {
            sig1 = flt2.Process(sig1);
        }

        float sig_out = (sig1 * 0.33) + (sig2 * 0.33) + (sig3 * 0.33);
        out[0][i] = sig_out;
        out[1][i] = sig_out;
    }
}

int main(void) {
    hw.Init();

    // Oscillators setup
    osc1.Init(hw.AudioSampleRate());

    // Filters setup
    flt1.Init(hw.AudioSampleRate());

    // adc setup
    AdcChannelConfig adc_config;
    adc_config.InitMux(A0, 8, D10, D11, D12);
    hw.adc.Init(&adc_config, 1);
    hw.adc.Start();

    // GPIO setup
    power.Init(D0, GPIO::Mode::INPUT, GPIO::Pull::NOPULL);
    // osc1 out
    outPins[0].Init(D14, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    // flt1 in
    inPins[0].Init(D13, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);

    // Audio start
    hw.StartAudio(AudioCallback);

    while (1) {
        // read patchbay connections
        for (int i = 0; i < NUM_OUT_PINS; i++) {
            outPins[i].Write(1);
            for (int j = 0; j < NUM_IN_PINS; j++) {
                if (inPins[j].Read()) {
                    patchbay[i][j] = 1;
                } else {
                    patchbay[i][j] = 0;
                }
            }
            outPins[i].Write(0);
        }

        System::Delay(100);
    }
}
