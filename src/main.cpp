#include <Arduino.h>

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// VIOLA sample taken from https://ccrma.stanford.edu/~jos/pasp/Sound_Examples.html
// #include "viola.h"
// #include <PDM.h>
// #include <doggone_inferencing.h>
#include <edge-impulse-sdk/classifier/ei_run_classifier.h>

AudioGeneratorWAV *wav;
AudioFileSourcePROGMEM *file;
AudioOutputI2S *out;

boolean isFailed = false;
boolean isPlaying = false;

void IRAM_ATTR ISR()
{
    if (!isPlaying)
    {
        isFailed = true;
    }
}

extern const uint8_t wav_start[] asm("_binary_src_bark1_wav_start");
extern const uint8_t wav_end[] asm("_binary_src_bark1_wav_end");

static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

void startSound()
{
    isPlaying = true;
    digitalWrite(42, HIGH);
    out = new AudioOutputI2S();
    out->SetPinout(40, 39, 1);
    out->SetOutputModeMono(true);
    out->SetGain(1.0);
    wav = new AudioGeneratorWAV();
    file = new AudioFileSourcePROGMEM(wav_start, wav_end - wav_start);
    wav->begin(file, out);

    USBSerial.printf("WAV start\n");
}

void setup()
{
    USBSerial.begin(115200);
    pinMode(42, OUTPUT);
    digitalWrite(42, HIGH);
    pinMode(46, INPUT_PULLDOWN);
    delay(5000);

    audioLogger = &Serial;

    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_46,1);
    attachInterrupt(46, ISR, RISING);
}

void loop()
{
    if (isFailed && !isPlaying)
    {
        startSound();

        isFailed = false;
    }
    if (isPlaying && wav && wav->isRunning())
    {
        if (!wav->loop())
        {
            USBSerial.println("WAV Done");
            wav->stop();
            delete (wav);
            wav = NULL;
            delete (out);
            out = NULL;
            delete (file);
            file = NULL;
            digitalWrite(42, LOW);
            isPlaying = false;
        }
    }
}