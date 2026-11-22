#ifndef LIBS_AUDIO_EFFECT_SAMPLERATEREDUCTION_H_
#define LIBS_AUDIO_EFFECT_SAMPLERATEREDUCTION_H_

#include "AudioStream.h"

// Sample-and-hold decimator. The output clock remains AUDIO_SAMPLE_RATE_EXACT; only
// the instants at which input values are captured are reduced. Both channels share
// one phase accumulator, while their held sample values remain independent.
class AudioSampleRateReduction : public AudioStream
{
  public:
    static constexpr float kMaxRateHz = AUDIO_SAMPLE_RATE_EXACT;
    static constexpr float kMinRateHz = 100.0f;

    AudioSampleRateReduction()
      : AudioStream(2, inputQueueArray)
    {
    }

    void update() override;
    void setReduction(uint8_t value);
    void reset();
    static float effectiveRateHz(uint8_t value);

  private:
    audio_block_t* inputQueueArray[2];
    uint32_t phaseIncrement = 0;
    uint32_t phase = 0;
    bool captureNext = true;
    int16_t held[2] = { 0, 0 };
};

#endif /* LIBS_AUDIO_EFFECT_SAMPLERATEREDUCTION_H_ */
