#pragma once
#include <JuceHeader.h>

class TapeStopAudioProcessor : public juce::AudioProcessor,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    TapeStopAudioProcessor();
    ~TapeStopAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TapeStop Pro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& paramID, float newValue) override;

    // Called from editor to trigger stop/start
    void triggerStop();
    void triggerStart();
    void triggerStopStart(); // combined: stop then auto-start

    // State for UI
    enum class TapeState { Idle, Stopping, Stopped, Starting };
    TapeState getTapeState() const { return tapeState.load(); }
    float getCurrentSpeed() const { return currentSpeed.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Circular buffer for pitch/speed manipulation
    static constexpr int BUFFER_SIZE = 1 << 20; // ~1M samples
    std::vector<float> circularBufferL, circularBufferR;
    double writePos = 0.0;
    double readPos  = 0.0;
    int    bufferMask = BUFFER_SIZE - 1;

    // Speed ramp state
    std::atomic<TapeState> tapeState { TapeState::Idle };
    std::atomic<float> currentSpeed { 1.0f };

    double sampleRate_ = 44100.0;
    double stopTimeSamples  = 44100.0;
    double startTimeSamples = 44100.0;

    double rampProgress = 0.0; // 0..1 during ramp

    // Interpolation helpers
    float interpolateSample(const std::vector<float>& buf, double pos);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeStopAudioProcessor)
};
