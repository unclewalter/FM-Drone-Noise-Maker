#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent   : public AudioAppComponent,
                               public Slider::Listener
{
public:
    //==============================================================================
    MainContentComponent()
    {
        levelSlider.setRange (0.0, 0.75);
        levelSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        levelLabel.setText ("Level", dontSendNotification);

        noiseLevelSlider.setRange (0.0, 10.0);
        noiseLevelSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        noiseLevelLabel.setText ("Noise Level", dontSendNotification);

        noiseResolutionSlider.setRange (0, 512);
        noiseResolutionSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        noiseResolutionLabel.setText ("Noise Resolution", dontSendNotification);

        addAndMakeVisible(freqSlider);
        freqSlider.setRange (1.0, 2000.0);
        freqSlider.setSkewFactorFromMidPoint (500.0); // [4]
        freqSlider.addListener(this);
        freqSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        freqLabel.setText ("Freq 1", dontSendNotification);

        addAndMakeVisible(freqSlider2);
        freqSlider2.setRange (1.0, 2000.0);
        freqSlider2.setSkewFactorFromMidPoint (500.0); // [4]
        freqSlider2.addListener(this);
        freqSlider2.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        freqLabel2.setText ("Freq 2", dontSendNotification);

        addAndMakeVisible (levelSlider);
        addAndMakeVisible (levelLabel);

        addAndMakeVisible (noiseLevelSlider);
        addAndMakeVisible (noiseLevelLabel);

        addAndMakeVisible (noiseResolutionSlider);
        addAndMakeVisible (noiseResolutionLabel);

        addAndMakeVisible(freqLabel);
        addAndMakeVisible(freqLabel2);

        setSize (600, 200);
        setAudioChannels (0, 2);
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        currentSampleRate = sampleRate;
        updateAngleDelta();
    }

    void sliderValueChanged (Slider* slider) override {
        if (slider == &freqSlider || slider == &freqSlider2) {
            if (currentSampleRate > 0.0) {
                updateAngleDelta();
            }
        }
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        const float level = levelSlider.getValue();
        const float levelScale = level * 2.0f;

        const float noiseLevel = noiseLevelSlider.getValue();
        const float noiseLevelScale = noiseLevel * 2.0f;

        int downsample = 512;

        for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel) {

                float* const buffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);

                if ((sample % downsample) == 0) {
                  noise = random.nextFloat() * noiseLevelScale - noiseLevel;
                  noise2 = random.nextFloat() * noiseLevelScale - noiseLevel;
                }

                const float currentSample = (float) std::sin (currentAngle+std::sin (currentAngle2));
                currentAngle += angleDelta*(noise+1.0f);;
                currentAngle2 += angleDelta2*(noise2+1.0f);

                buffer[sample] = (currentSample * levelScale - level);
            }
        }
    }

    void updateAngleDelta() {
        const double cyclesPerSample = freqSlider.getValue() / currentSampleRate;
        const double cyclesPerSample2 = freqSlider2.getValue() / currentSampleRate;
        angleDelta = cyclesPerSample * 2.0 * double_Pi;
        angleDelta2 = cyclesPerSample2 * 2.0 * double_Pi;
    }

    void releaseResources() override
    {
        Logger::getCurrentLogger()->writeToLog ("Releasing audio resources");
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::antiquewhite);
    }

    void resized() override
    {
        levelLabel.setBounds (10, 10, 90, 20);
        levelSlider.setBounds (100, 10, getWidth() - 110, 20);
        freqLabel.setBounds (10, 40, 90, 20);
        freqLabel2.setBounds (10, 70, 90, 20);
        freqSlider.setBounds(100, 40, getWidth() - 110, 20);
        freqSlider2.setBounds(100, 70, getWidth() - 110, 20);

        noiseLevelLabel.setBounds (10, 100, 90, 20);
        noiseLevelSlider.setBounds (100, 100, getWidth() - 110, 20);

        noiseResolutionLabel.setBounds (10, 130, 90, 20);
        noiseResolutionSlider.setBounds (100, 130, getWidth() - 110, 20);
    }


private:
    //==============================================================================
    Random random;

    Slider levelSlider;
    Label levelLabel;
    Slider freqSlider;
    Slider freqSlider2;
    Label freqLabel;
    Label freqLabel2;

    Slider noiseLevelSlider;
    Label noiseLevelLabel;

    Slider noiseResolutionSlider;
    Label noiseResolutionLabel;

    double currentSampleRate, currentAngle, currentAngle2, angleDelta, angleDelta2;

    float noise, noise2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED