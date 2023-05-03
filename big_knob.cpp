#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisy::seed;
using namespace daisysp;

DaisySeed hw;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size) {
    for (size_t i = 0; i < size; i++) {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

int main(void) {
    hw.Init();
    hw.SetAudioBlockSize(4);  // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);

    // adc setup
    AdcChannelConfig adc_config;
    adc_config.InitMux(A0, 8, D10, D11, D12);
    hw.adc.Init(&adc_config, 1);
    hw.adc.Start();

    // GPIO setup
    GPIO outWire;
    GPIO inWire;
    inWire.Init(D13, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    outWire.Init(D14, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    outWire.Write(1);

    hw.StartLog();

    int idx = 0;

    while (1) {
        for (; idx < 4; idx++) {
            float value = hw.adc.GetMuxFloat(0, idx);
            hw.PrintLine("%d: %f", idx, value);
        }
        idx = 0;

        if (inWire.Read()) {
            hw.PrintLine("yes");
        } else {
            hw.PrintLine("no");
        }

        System::Delay(3000);
    }
}
