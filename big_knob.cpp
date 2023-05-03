#include "daisy_seed.h"
#include "daisysp.h"

#define NUM_OUT_PINS 13
#define NUM_IN_PINS  9
#define NUM_KNOBS    18
#define NUM_MUXES    4

using namespace daisy;
using namespace daisy::seed;
using namespace daisysp;

enum INS {
    I_FLT1 = 0,
    I_FLT2 = 1,
    I_FLT3 = 2,
    I_ENV1 = 3,
    I_ENV2 = 4,
    I_ENV3 = 5,
    I_REVERB = 4,
    I_DELAY = 5,
    I_SPEAKERS = 6,
};

enum OUTS {
    O_OSC1 = 0,
    O_OSC2 = 1,
    O_OSC3 = 2,
    O_NOISE1 = 3,
    O_NOISE2 = 4,
    O_FLT1 = 5,
    O_FLT2 = 6,
    O_FLT3 = 7,
    O_ENV1 = 8,
    O_ENV2 = 9,
    O_ENV3 = 10,
    O_REVERB = 11,
    O_DELAY = 12,
};

DaisySeed hw;

Oscillator osc1;
Oscillator osc2;
Oscillator osc3;
Biquad     flt1;
Biquad     flt2;
Biquad     flt3;
ReverbSc   reverb;

GPIO power;
GPIO outPins[NUM_OUT_PINS];
GPIO inPins[NUM_IN_PINS];
/* LEVELS
0: OSC1 S
1: OSC1 F
2: OSC1 L
3: OSC2 S
4: OSC2 F
5: OSC2 L
6: OSC3 S
7: OSC3 F
8: OSC3 L
9: FLT1 C
10: FLT1 R
11: FLT2 C
12: FLT2 R
13: FLT3 C
14: FLT3 R
15: REV F
16: REV C
17: REV M
*/
float levels[8 * NUM_MUXES];
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

    // read inputs
    for (int i = 0; i < NUM_MUXES; i++) {
        for (int j = 0; j < 8; j++) {
            levels[(i * 8) + j] = hw.adc.GetMuxFloat(i, j);
        }
    }

    // osc1
    int wf = pot_switch(levels[0]);
    if (wf == 1) {
        osc1.SetWaveform(Oscillator::WAVE_SIN);
    } else if (wf == 2) {
        osc1.SetWaveform(Oscillator::WAVE_TRI);
    } else if (wf == 3) {
        osc1.SetWaveform(Oscillator::WAVE_SAW);
    }
    osc1.SetFreq(levels[1] * 1000);
    osc1.SetAmp(levels[2] * 0.95);

    // osc2
    wf = pot_switch(levels[3]);
    if (wf == 1) {
        osc2.SetWaveform(Oscillator::WAVE_SIN);
    } else if (wf == 2) {
        osc2.SetWaveform(Oscillator::WAVE_TRI);
    } else if (wf == 3) {
        osc2.SetWaveform(Oscillator::WAVE_SAW);
    }
    osc2.SetFreq(levels[4] * 1000);
    osc2.SetAmp(levels[5] * 0.95);

    // osc3
    wf = pot_switch(levels[6]);
    if (wf == 1) {
        osc3.SetWaveform(Oscillator::WAVE_SIN);
    } else if (wf == 2) {
        osc3.SetWaveform(Oscillator::WAVE_TRI);
    } else if (wf == 3) {
        osc3.SetWaveform(Oscillator::WAVE_SAW);
    }
    osc3.SetFreq(levels[7] * 1000);
    osc3.SetAmp(levels[8] * 0.95);

    // flt1
    flt1.SetCutoff(levels[9] * hw.AudioSampleRate() / 2.0f);
    flt1.SetRes(levels[10] * 0.5);

    // flt2
    flt2.SetCutoff(levels[11] * hw.AudioSampleRate() / 2.0f);
    flt2.SetRes(levels[12] * 0.5);

    // flt3
    flt3.SetCutoff(levels[13] * hw.AudioSampleRate() / 2.0f);
    flt3.SetRes(levels[14] * 0.5);

    // reverb
    reverb.SetFeedback(levels[15] * 0.9);
    reverb.SetLpFreq(levels[16] * (hw.AudioSampleRate() / 2.0f));

    // generate samples
    for (size_t i = 0; i < size; i++) {
        float sig1, sig2, sig3, verb_sig;
        verb_sig = 0.0f;
        sig1 = osc1.Process();
        sig2 = osc2.Process();
        sig3 = osc3.Process();

        // osc1 out
        if (patchbay[O_OSC1][I_FLT1]) {
            sig1 = flt1.Process(sig1);
        } else if (patchbay[O_OSC1][I_FLT2]) {
            sig2 = flt2.Process(sig1);
        } else if (patchbay[O_OSC1][I_FLT3]) {
            sig3 = flt3.Process(sig1);
        } else if (patchbay[O_OSC1][I_REVERB]) {
            reverb.Process(sig1, sig1, &verb_sig, &verb_sig);
        } else if (!patchbay[O_OSC1][I_SPEAKERS]) {
            sig1 = 0.0f;
        }

        // osc2 out
        if (patchbay[O_OSC2][I_FLT1]) {
            sig2 = flt1.Process(sig2);
        } else if (patchbay[O_OSC2][I_FLT2]) {
            sig2 = flt2.Process(sig2);
        } else if (patchbay[O_OSC2][I_FLT3]) {
            sig2 = flt3.Process(sig2);
        } else if (patchbay[O_OSC2][I_REVERB]) {
            float sig2_copy = sig2;
            reverb.Process(sig2_copy, sig2_copy, &sig2, &sig2);
        } else if (!patchbay[O_OSC2][I_SPEAKERS]) {
            sig2 = 0.0f;
        }

        // osc3 out
        if (patchbay[O_OSC3][I_FLT1]) {
            sig3 = flt1.Process(sig3);
        } else if (patchbay[O_OSC3][I_FLT2]) {
            sig3 = flt2.Process(sig3);
        } else if (patchbay[O_OSC3][I_FLT3]) {
            sig3 = flt3.Process(sig3);
        } else if (patchbay[O_OSC3][I_REVERB]) {
            float sig3_copy = sig3;
            reverb.Process(sig3_copy, sig3_copy, &sig3, &sig3);
        } else if (!patchbay[O_OSC3][I_SPEAKERS]) {
            sig3 = 0.0f;
        }

        // normalize
        float sig_out;
        if (sig1 && sig2 && sig3) {
            sig_out = (sig1 * 0.33) + (sig2 * 0.33) + (sig3 * 0.33);
        } else if ((sig1 && sig2) || (sig1 && sig3) || (sig2 && sig3)) {
            sig_out = (sig1 * 0.5) + (sig2 * 0.5) + (sig3 * 0.5);
        } else {
            sig_out = sig1 + sig2 + sig3;
        }

        out[0][i] = sig_out;
        out[1][i] = sig_out;
    }
}

int main(void) {
    hw.Init();

    // Oscillators setup
    osc1.Init(hw.AudioSampleRate());
    osc2.Init(hw.AudioSampleRate());
    osc3.Init(hw.AudioSampleRate());

    // Filters setup
    flt1.Init(hw.AudioSampleRate());
    flt2.Init(hw.AudioSampleRate());
    flt3.Init(hw.AudioSampleRate());

    // FXs setup
    reverb.Init(hw.AudioSampleRate());

    // adc setup
    AdcChannelConfig adc_config[NUM_MUXES];
    adc_config[0].InitMux(A0, 8, D1, D2, D3);
    adc_config[1].InitMux(A1, 8, D4, D5, D6);
    adc_config[2].InitMux(A2, 8, D7, D8, D9);
    adc_config[3].InitMux(A3, 8, D10, D11, D12);
    hw.adc.Init(adc_config, NUM_MUXES);
    hw.adc.Start();

    // GPIO setup
    power.Init(D0, GPIO::Mode::INPUT, GPIO::Pull::NOPULL);

    // init patchbay outs and ins
    outPins[O_OSC1].Init(D13, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outPins[O_OSC2].Init(D14, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outPins[O_OSC2].Init(D19, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outPins[O_FLT1].Init(D20, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outPins[O_FLT2].Init(D21, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outPins[O_FLT3].Init(D22, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outPins[O_REVERB].Init(D23, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);

    inPins[I_FLT1].Init(D24, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    inPins[I_FLT2].Init(D25, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    inPins[I_FLT3].Init(D26, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    inPins[I_REVERB].Init(D27, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    inPins[I_SPEAKERS].Init(D28, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);

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
