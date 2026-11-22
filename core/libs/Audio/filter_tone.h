#ifndef LIBS_AUDIO_FILTER_TONE_H_
#define LIBS_AUDIO_FILTER_TONE_H_

#include "AudioStream.h"

// Bipolar, non-resonant, two-pole Butterworth filter. Negative values select low-pass,
// positive values select high-pass, and zero is an exact block pass-through.
// Coefficients are shared; each channel retains independent transposed-DF-II state.
class AudioToneFilter : public AudioStream
{
  public:
    static constexpr float kMinCutoffHz = 20.0f;
    static constexpr float kMaxCutoffHz = 20000.0f;
    static constexpr float kStateSettleThreshold = 0.001f;

    AudioToneFilter()
      : AudioStream(2, inputQueueArray)
    {
    }

    void update() override;
    void setValue(int8_t value);
    void reset();
    static float cutoffHz(int8_t value);

  private:
    bool isSettled(uint8_t channel) const;
    void clearState(uint8_t channel);

    audio_block_t* inputQueueArray[2];
    bool active = false;
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1[2] = { 0.0f, 0.0f };
    float z2[2] = { 0.0f, 0.0f };
};

#endif /* LIBS_AUDIO_FILTER_TONE_H_ */
