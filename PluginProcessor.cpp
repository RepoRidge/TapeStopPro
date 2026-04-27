#include "PluginProcessor.h"
#include "PluginEditor.h"

TapeStopAudioProcessor::TapeStopAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    circularBufferL.resize(BUFFER_SIZE, 0.0f);
    circularBufferR.resize(BUFFER_SIZE, 0.0f);

    apvts.addParameterListener("stopTime",  this);
    apvts.addParameterListener("startTime", this);
}

TapeStopAudioProcessor::~TapeStopAudioProcessor()
{
    apvts.removeParameterListener("stopTime",  this);
    apvts.removeParameterListener("startTime", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout TapeStopAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "stopTime", "Stop Time",
        juce::NormalisableRange<float>(10.0f, 5000.0f, 1.0f, 0.4f),
        500.0f, "ms"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "startTime", "Start Time",
        juce::NormalisableRange<float>(10.0f, 5000.0f, 1.0f, 0.4f),
        500.0f, "ms"));

    return layout;
}

void TapeStopAudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (paramID == "stopTime")
        stopTimeSamples = (newValue / 1000.0) * sampleRate_;
    else if (paramID == "startTime")
        startTimeSamples = (newValue / 1000.0) * sampleRate_;
}

void TapeStopAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;
    stopTimeSamples  = (apvts.getRawParameterValue("stopTime")->load()  / 1000.0) * sampleRate;
    startTimeSamples = (apvts.getRawParameterValue("startTime")->load() / 1000.0) * sampleRate;

    std::fill(circularBufferL.begin(), circularBufferL.end(), 0.0f);
    std::fill(circularBufferR.begin(), circularBufferR.end(), 0.0f);
    writePos = 0.0;
    readPos  = 0.0;
    currentSpeed.store(1.0f);
    tapeState.store(TapeState::Idle);
    rampProgress = 0.0;
}

void TapeStopAudioProcessor::releaseResources() {}

void TapeStopAudioProcessor::triggerStop()
{
    if (tapeState.load() == TapeState::Idle)
    {
        tapeState.store(TapeState::Stopping);
        rampProgress = 0.0;
    }
}

void TapeStopAudioProcessor::triggerStart()
{
    if (tapeState.load() == TapeState::Stopped)
    {
        tapeState.store(TapeState::Starting);
        rampProgress = 0.0;
    }
}

void TapeStopAudioProcessor::triggerStopStart()
{
    auto state = tapeState.load();
    if (state == TapeState::Idle || state == TapeState::Starting)
    {
        tapeState.store(TapeState::Stopping);
        rampProgress = 0.0;
    }
    else if (state == TapeState::Stopped || state == TapeState::Stopping)
    {
        tapeState.store(TapeState::Starting);
        rampProgress = 0.0;
    }
}

float TapeStopAudioProcessor::interpolateSample(const std::vector<float>& buf, double pos)
{
    int   i0 = (int)pos & bufferMask;
    int   i1 = (i0 + 1) & bufferMask;
    float t  = (float)(pos - std::floor(pos));
    return buf[i0] * (1.0f - t) + buf[i1] * t;
}

void TapeStopAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples  = buffer.getNumSamples();
    const bool hasStereo  = buffer.getNumChannels() >= 2;

    const float* inL = buffer.getReadPointer(0);
    const float* inR = hasStereo ? buffer.getReadPointer(1) : inL;
    float* outL = buffer.getWritePointer(0);
    float* outR = hasStereo ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        int wi = (int)writePos & bufferMask;
        circularBufferL[wi] = inL[i];
        circularBufferR[wi] = hasStereo ? inR[i] : inL[i];
        writePos += 1.0;

        auto state = tapeState.load();
        float speed = currentSpeed.load();

        if (state == TapeState::Stopping)
        {
            double totalSamples = stopTimeSamples > 0 ? stopTimeSamples : 1.0;
            rampProgress += 1.0 / totalSamples;
            if (rampProgress >= 1.0) rampProgress = 1.0;

            float t = (float)rampProgress;
            speed = 1.0f - (t * t * (3.0f - 2.0f * t));
            currentSpeed.store(speed);

            if (rampProgress >= 1.0)
            {
                tapeState.store(TapeState::Stopped);
                speed = 0.0f;
            }
        }
        else if (state == TapeState::Starting)
        {
            double totalSamples = startTimeSamples > 0 ? startTimeSamples : 1.0;
            rampProgress += 1.0 / totalSamples;
            if (rampProgress >= 1.0) rampProgress = 1.0;

            float t = (float)rampProgress;
            speed = t * t * (3.0f - 2.0f * t);
            currentSpeed.store(speed);

            if (rampProgress >= 1.0)
            {
                tapeState.store(TapeState::Idle);
                speed = 1.0f;
                currentSpeed.store(1.0f);
                readPos = writePos;
            }
        }

        readPos += (double)speed;

        double maxDelay = (double)(BUFFER_SIZE / 2);
        if (writePos - readPos > maxDelay)
            readPos = writePos - maxDelay;

        float sampleL = interpolateSample(circularBufferL, readPos);
        float sampleR = interpolateSample(circularBufferR, readPos);

        outL[i] = sampleL;
        if (outR) outR[i] = sampleR;
    }
}

void TapeStopAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TapeStopAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* TapeStopAudioProcessor::createEditor()
{
    return new TapeStopAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapeStopAudioProcessor();
}
