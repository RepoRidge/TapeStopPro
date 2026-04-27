#include "PluginEditor.h"
#include "PluginProcessor.h"

// ═══════════════════════════════════════════════════════════════════
//  Colour palette
// ═══════════════════════════════════════════════════════════════════
namespace Pal
{
    const juce::Colour bg        { 0xFF111318 };
    const juce::Colour panel     { 0xFF1A1E27 };
    const juce::Colour border    { 0xFF2C3040 };
    const juce::Colour knobBg    { 0xFF22273A };
    const juce::Colour knobRim   { 0xFF3A4060 };
    const juce::Colour accent    { 0xFFE8A835 };   // warm amber
    const juce::Colour accentDim { 0xFF9A6E1A };
    const juce::Colour red       { 0xFFE84040 };
    const juce::Colour green     { 0xFF40D080 };
    const juce::Colour textBright{ 0xFFF0EEE8 };
    const juce::Colour textDim   { 0xFF6A7090 };
    const juce::Colour reel1     { 0xFF2A2E3E };
    const juce::Colour reel2     { 0xFF1A1D28 };
    const juce::Colour reelSpoke { 0xFF3A4060 };
    const juce::Colour tape      { 0xFF8B6A2A };
}

// ═══════════════════════════════════════════════════════════════════
//  Custom LookAndFeel for knobs
// ═══════════════════════════════════════════════════════════════════
class TapeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TapeLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId,          Pal::accent);
        setColour(juce::Slider::rotarySliderFillColourId, Pal::accent);
        setColour(juce::Slider::rotarySliderOutlineColourId, Pal::knobRim);
        setColour(juce::Label::textColourId,            Pal::textDim);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override
    {
        const float radius   = juce::jmin(width, height) * 0.5f - 4.0f;
        const float centreX  = x + width  * 0.5f;
        const float centreY  = y + height * 0.5f;
        const float angle    = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Outer glow ring
        {
            juce::Path ring;
            ring.addEllipse(centreX - radius - 4, centreY - radius - 4,
                            (radius + 4) * 2, (radius + 4) * 2);
            g.setColour(Pal::accentDim.withAlpha(0.15f));
            g.fillPath(ring);
        }

        // Body gradient
        {
            juce::ColourGradient grad(Pal::knobBg.brighter(0.08f), centreX - radius * 0.3f,
                                     centreY - radius * 0.3f,
                                     Pal::reel2, centreX + radius * 0.4f,
                                     centreY + radius * 0.4f, true);
            g.setGradientFill(grad);
            g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);
        }

        // Outer rim
        g.setColour(Pal::knobRim);
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, 1.5f);

        // Arc track
        {
            juce::Path arcTrack;
            arcTrack.addArc(centreX - radius + 5, centreY - radius + 5,
                            (radius - 5) * 2, (radius - 5) * 2,
                            rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(Pal::border);
            g.strokePath(arcTrack, juce::PathStrokeType(2.5f));
        }

        // Filled arc
        {
            juce::Path filledArc;
            filledArc.addArc(centreX - radius + 5, centreY - radius + 5,
                             (radius - 5) * 2, (radius - 5) * 2,
                             rotaryStartAngle, angle, true);
            g.setColour(Pal::accent);
            g.strokePath(filledArc, juce::PathStrokeType(2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Pointer line
        {
            const float px = centreX + (radius - 9) * std::sin(angle);
            const float py = centreY - (radius - 9) * std::cos(angle);
            g.setColour(Pal::accent);
            g.drawLine(centreX, centreY, px, py, 2.5f);

            // Dot at tip
            g.fillEllipse(px - 3, py - 3, 6, 6);
        }

        // Centre dot
        g.setColour(Pal::border);
        g.fillEllipse(centreX - 4, centreY - 4, 8, 8);
    }
};

static TapeLookAndFeel& getLAF()
{
    static TapeLookAndFeel laf;
    return laf;
}

// ═══════════════════════════════════════════════════════════════════
//  ReelComponent
// ═══════════════════════════════════════════════════════════════════
ReelComponent::ReelComponent()
{
    startTimerHz(60);
}

void ReelComponent::setSpeed(float spd)
{
    speed = spd;
}

void ReelComponent::timerCallback()
{
    angle    += speed * 0.06f;
    wobble   += wobbleDelta;
    wobbleDelta = wobbleDelta * 0.95f + (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.002f * (1.0f - speed);
    repaint();
}

void ReelComponent::paint(juce::Graphics& g)
{
    const float W = (float)getWidth();
    const float H = (float)getHeight();
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;
    const float R  = juce::jmin(W, H) * 0.5f - 2.0f;

    // ── Outer housing ──
    {
        juce::ColourGradient grad(Pal::panel, cx - R * 0.5f, cy - R * 0.5f,
                                   Pal::reel2, cx + R, cy + R, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - R, cy - R, R * 2, R * 2);
    }
    g.setColour(Pal::border);
    g.drawEllipse(cx - R, cy - R, R * 2, R * 2, 1.5f);

    // ── Hub ──
    const float hubR = R * 0.22f;
    g.setColour(Pal::bg);
    g.fillEllipse(cx - hubR, cy - hubR, hubR * 2, hubR * 2);
    g.setColour(Pal::knobRim);
    g.drawEllipse(cx - hubR, cy - hubR, hubR * 2, hubR * 2, 1.5f);

    // ── Spokes ──
    const int numSpokes = 6;
    for (int s = 0; s < numSpokes; ++s)
    {
        const float a = angle + wobble + s * juce::MathConstants<float>::twoPi / numSpokes;
        const float x1 = cx + hubR * std::sin(a);
        const float y1 = cy - hubR * std::cos(a);
        const float spokeR = R * 0.72f;
        const float x2 = cx + spokeR * std::sin(a);
        const float y2 = cy - spokeR * std::cos(a);
        g.setColour(Pal::reelSpoke.withAlpha(0.7f));
        g.drawLine(x1, y1, x2, y2, 1.8f);

        // Spoke end dot
        g.setColour(Pal::knobRim);
        g.fillEllipse(x2 - 2.5f, y2 - 2.5f, 5.0f, 5.0f);
    }

    // ── Tape roll ring ──
    const float tapeOuter = R * 0.92f;
    const float tapeInner = R * 0.75f;
    juce::ColourGradient tapeGrad(Pal::tape.withAlpha(0.4f), cx - tapeOuter, cy,
                                   Pal::tape.withAlpha(0.15f), cx + tapeOuter, cy, true);
    g.setGradientFill(tapeGrad);
    g.fillEllipse(cx - tapeOuter, cy - tapeOuter, tapeOuter * 2, tapeOuter * 2);
    g.setColour(Pal::reel2);
    g.fillEllipse(cx - tapeInner, cy - tapeInner, tapeInner * 2, tapeInner * 2);

    // ── Centre pin ──
    g.setColour(Pal::accent.withAlpha(0.8f));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);
}

// ═══════════════════════════════════════════════════════════════════
//  SpeedMeter
// ═══════════════════════════════════════════════════════════════════
SpeedMeter::SpeedMeter()
{
    startTimerHz(60);
}

void SpeedMeter::setTargetSpeed(float s) { targetSpeed = s; }

void SpeedMeter::timerCallback()
{
    displaySpeed += (targetSpeed - displaySpeed) * 0.08f;
    repaint();
}

void SpeedMeter::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    const float fill = displaySpeed;

    // Background
    g.setColour(Pal::knobBg);
    g.fillRoundedRectangle(0, 0, W, H, 4.0f);
    g.setColour(Pal::border);
    g.drawRoundedRectangle(0.5f, 0.5f, W - 1, H - 1, 4.0f, 1.0f);

    // Bar
    const int barW = (int)((W - 4) * fill);
    if (barW > 0)
    {
        juce::ColourGradient grad(Pal::accent, 2, H / 2,
                                   (fill < 0.5f ? Pal::red : Pal::green), W - 2, H / 2, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(2, 2, (float)barW, (float)(H - 4), 3.0f);
    }

    // Speed label
    g.setFont(juce::Font("Courier New", 10.5f, juce::Font::bold));
    g.setColour(Pal::textBright);
    g.drawText(juce::String((int)(fill * 100)) + "%", 0, 0, W, H,
               juce::Justification::centred);
}

// ═══════════════════════════════════════════════════════════════════
//  LabelledKnob
// ═══════════════════════════════════════════════════════════════════
LabelledKnob::LabelledKnob(const juce::String& labelText, const juce::String& suffix)
    : unitSuffix(suffix)
{
    slider.setLookAndFeel(&getLAF());
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setColour(juce::Slider::textBoxTextColourId,       Pal::textBright);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, Pal::knobBg);
    slider.setColour(juce::Slider::textBoxOutlineColourId,    Pal::border);
    slider.setTextValueSuffix(" " + suffix);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font("Courier New", 10.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, Pal::textDim);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void LabelledKnob::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromTop(18));
    slider.setBounds(b);
}

// ═══════════════════════════════════════════════════════════════════
//  TapeStopAudioProcessorEditor
// ═══════════════════════════════════════════════════════════════════
TapeStopAudioProcessorEditor::TapeStopAudioProcessorEditor(TapeStopAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(420, 300);
    setResizable(false, false);
    lastState = TapeStopAudioProcessor::TapeState::Idle;

    // Knob attachments
    stopAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                      p.apvts, "stopTime",  stopKnob.slider);
    startAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                      p.apvts, "startTime", startKnob.slider);

    addAndMakeVisible(reel);
    addAndMakeVisible(speedMeter);
    addAndMakeVisible(stopKnob);
    addAndMakeVisible(startKnob);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(startButton);
    addAndMakeVisible(tapButton);

    styleButton(stopButton,  Pal::red);
    styleButton(startButton, Pal::green);
    styleButton(tapButton,   Pal::accent);

    stopButton.onClick  = [this] { processor.triggerStop();  };
    startButton.onClick = [this] { processor.triggerStart(); };
    tapButton.onClick   = [this] { processor.triggerStopStart(); };

    startTimerHz(30);
}

TapeStopAudioProcessorEditor::~TapeStopAudioProcessorEditor()
{
    stopTimer();
}

void TapeStopAudioProcessorEditor::styleButton(juce::TextButton& btn, juce::Colour accent)
{
    btn.setLookAndFeel(nullptr);
    btn.setColour(juce::TextButton::buttonColourId,   Pal::panel);
    btn.setColour(juce::TextButton::buttonOnColourId,  accent.withAlpha(0.3f));
    btn.setColour(juce::TextButton::textColourOffId,   accent);
    btn.setColour(juce::TextButton::textColourOnId,    accent.brighter(0.3f));
    btn.setColour(juce::ComboBox::outlineColourId,     accent.withAlpha(0.5f));
}

void TapeStopAudioProcessorEditor::timerCallback()
{
    const float spd = processor.getCurrentSpeed();
    reel.setSpeed(spd);
    speedMeter.setTargetSpeed(spd);

    auto state = processor.getTapeState();
    const bool isStop = (state == TapeStopAudioProcessor::TapeState::Stopped ||
                         state == TapeStopAudioProcessor::TapeState::Stopping);
    stopButton.setToggleState(isStop, juce::dontSendNotification);
    startButton.setToggleState(state == TapeStopAudioProcessor::TapeState::Starting,
                               juce::dontSendNotification);
}

void TapeStopAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    // Main background
    g.fillAll(Pal::bg);

    // Subtle vignette
    {
        juce::ColourGradient vignette(juce::Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                      juce::Colour(0x44000000), 0, 0, true);
        g.setGradientFill(vignette);
        g.fillRect(0, 0, W, H);
    }

    // Top header strip
    {
        juce::ColourGradient header(Pal::panel.brighter(0.05f), 0, 0,
                                     Pal::panel, 0, 44, false);
        g.setGradientFill(header);
        g.fillRect(0, 0, W, 44);
    }
    g.setColour(Pal::border);
    g.drawLine(0, 44, W, 44, 1.0f);

    // Accent top edge
    g.setColour(Pal::accent);
    g.fillRect(0, 0, W, 2);

    // Title
    g.setFont(juce::Font("Courier New", 15.0f, juce::Font::bold));
    g.setColour(Pal::textBright);
    g.drawText("TAPE STOP PRO", 16, 0, 240, 44, juce::Justification::centredLeft);

    // Subtitle
    g.setFont(juce::Font("Courier New", 9.0f, juce::Font::plain));
    g.setColour(Pal::textDim);
    g.drawText("PITCH RAMP  //  5000ms", 16, 24, 240, 18, juce::Justification::centredLeft);

    // Right tag
    g.setFont(juce::Font("Courier New", 9.0f, juce::Font::bold));
    g.setColour(Pal::accentDim);
    g.drawText("v1.0", W - 40, 0, 36, 44, juce::Justification::centredRight);

    // Panel for reel
    g.setColour(Pal::panel);
    g.fillRoundedRectangle(12, 52, 130, 130, 8.0f);
    g.setColour(Pal::border);
    g.drawRoundedRectangle(12, 52, 130, 130, 8.0f, 1.0f);

    // Speed label
    g.setFont(juce::Font("Courier New", 9.0f, juce::Font::bold));
    g.setColour(Pal::textDim);
    g.drawText("SPEED", 12, 186, 130, 16, juce::Justification::centred);

    // Divider
    g.setColour(Pal::border);
    g.drawLine(0, 250, W, 250, 1.0f);

    // Bottom strip label
    g.setFont(juce::Font("Courier New", 9.0f, juce::Font::plain));
    g.setColour(Pal::textDim);
    g.drawText("STOP", 110, 254, 70, 18, juce::Justification::centred);
    g.drawText("START", 200, 254, 70, 18, juce::Justification::centred);
    g.drawText("TRIGGER", 288, 254, 110, 18, juce::Justification::centred);
}

void TapeStopAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);
}

void TapeStopAudioProcessorEditor::resized()
{
    // Reel
    reel.setBounds(18, 58, 118, 118);

    // Speed meter
    speedMeter.setBounds(16, 200, 126, 18);

    // Stop knob
    stopKnob.setBounds(150, 48, 100, 118);

    // Start knob
    startKnob.setBounds(268, 48, 100, 118);

    // Buttons row
    stopButton .setBounds(108, 272, 64, 22);
    startButton.setBounds(198, 272, 64, 22);
    tapButton  .setBounds(286, 272, 118, 22);
}
