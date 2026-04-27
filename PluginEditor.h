#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ReelComponent : public juce::Component, public juce::Timer
{
public:
    ReelComponent();
    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    void setSpeed(float spd);

private:
    float speed       = 1.0f;
    float angle       = 0.0f;
    float wobble      = 0.0f;
    float wobbleDelta = 0.0f;
};

class LabelledKnob : public juce::Component
{
public:
    LabelledKnob(const juce::String& labelText, const juce::String& suffix);
    void resized() override;
    juce::Slider slider;

private:
    juce::Label  label;
    juce::String unitSuffix;
};

class SpeedMeter : public juce::Component, public juce::Timer
{
public:
    SpeedMeter();
    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    void setTargetSpeed(float s);

private:
    float displaySpeed = 1.0f;
    float targetSpeed  = 1.0f;
};

class TapeStopAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::Timer
{
public:
    TapeStopAudioProcessorEditor(TapeStopAudioProcessor&);
    ~TapeStopAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    TapeStopAudioProcessor& processor;

    ReelComponent  reel;
    SpeedMeter     speedMeter;
    LabelledKnob   stopKnob  { "STOP TIME",  "ms" };
    LabelledKnob   startKnob { "START TIME", "ms" };

    juce::TextButton stopButton  { "STOP" };
    juce::TextButton startButton { "START" };
    juce::TextButton tapButton   { "TAP" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stopAttach, startAttach;

    // Attachment per il TAP automatizzabile
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tapAttach;

    TapeStopAudioProcessor::TapeState lastState;

    void styleButton(juce::TextButton&, juce::Colour accent);
    void drawBackground(juce::Graphics&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeStopAudioProcessorEditor)
};
