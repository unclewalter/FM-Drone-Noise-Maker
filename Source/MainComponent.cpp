#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent : public AudioAppComponent,
        public Slider::Listener
{
public:
//==============================================================================
float dBtoGain (float decibels) {
        return std::pow (10.0, decibels * 0.05);
}

MainContentComponent()
{
        // Setting ranges
        levelSlider.setRange(-100.0, -15.0);
        freqSlider.setRange(1.0, 2000.0);
        freqSlider2.setRange(1.0, 2000.0);
        noiseLevelSlider.setRange(-100.0, 0.0);
        noiseResolutionSlider.setRange(1, 10000);
        noiseSmoothingSlider.setRange(1, 20000);

        // Enabling slider value text boxes
        levelSlider.setTextBoxStyle(Slider::TextBoxRight, false, 100, 20);
        freqSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        freqSlider2.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        noiseLevelSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        noiseResolutionSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);
        noiseSmoothingSlider.setTextBoxStyle (Slider::TextBoxRight, false, 100, 20);

        // Labels
        levelLabel.setText("Level", dontSendNotification);
        freqLabel.setText("Freq 1", dontSendNotification);
        freqLabel2.setText("Freq 2", dontSendNotification);
        noiseLevelLabel.setText("Noise Level", dontSendNotification);
        noiseResolutionLabel.setText("Noise Resolution", dontSendNotification);
        noiseSmoothingLabel.setText("Noise Smoothing", dontSendNotification);

        // Initializing slider values
        levelSlider.setValue(-100.0);
        freqSlider.setValue(168.0);
        freqSlider2.setValue(81.0);
        noiseLevelSlider.setValue(-8.0);
        noiseResolutionSlider.setValue(11);
        noiseSmoothingSlider.setValue(4157);

        levelSlider.setTextValueSuffix("dB");
        noiseLevelSlider.setTextValueSuffix("dB");

        // Making the value scaling a bit more practical
        freqSlider.setSkewFactorFromMidPoint (500.0); // [4]
        freqSlider2.setSkewFactorFromMidPoint(500.0); // [4]
        noiseLevelSlider.setSkewFactorFromMidPoint(-15.0);
        noiseResolutionSlider.setSkewFactorFromMidPoint(100.0);
        noiseSmoothingSlider.setSkewFactorFromMidPoint(1500.0);

        // Listeners to ensure they they update in real-time
        levelSlider.addListener(this);
        noiseLevelSlider.addListener(this);
        noiseResolutionSlider.addListener(this);
        noiseSmoothingSlider.addListener(this);
        freqSlider.addListener(this);
        freqSlider2.addListener(this);

        // Display sliders
        addAndMakeVisible(levelSlider);
        addAndMakeVisible(freqSlider);
        addAndMakeVisible(freqSlider2);
        addAndMakeVisible(noiseLevelSlider);
        addAndMakeVisible(noiseResolutionSlider);
        addAndMakeVisible(noiseSmoothingSlider);

        // Display labels
        addAndMakeVisible(levelLabel);
        addAndMakeVisible(freqLabel);
        addAndMakeVisible(freqLabel2);
        addAndMakeVisible(noiseLevelLabel);
        addAndMakeVisible(noiseResolutionLabel);
        addAndMakeVisible(noiseSmoothingLabel);

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

        // Initializing atomic parameters
        noiseSmoothing = 1.0f;
        targetLevel = 0.0f;
        targetNoiseLevel = 0.0f;
        downsampleFactor = 1.0f;

        // Initializing non-atomic parameters
        level = 0.0f;
        noiseLevel = 0.0f;

        updateAngleDelta();
}

void sliderValueChanged (Slider* slider) override {
        if (slider == &freqSlider || slider == &freqSlider2) {
                if (currentSampleRate > 0.0) {
                        updateAngleDelta();
                }
        }

        targetLevel.store(dBtoGain(levelSlider.getValue()));
        targetNoiseLevel.store(dBtoGain(noiseLevelSlider.getValue()));
        downsampleFactor.store(noiseResolutionSlider.getValue());
        noiseSmoothing.store(noiseSmoothingSlider.getValue());
}

// The audio callback: where all your wildest dreams come true.
void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
{
        for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
                // Adding ballistic response to parameter change to prevent zipper noise
                level = (targetLevel.load() - level)/60 + level;
                noiseLevel = (targetNoiseLevel.load() - noiseLevel)/60 + noiseLevel;

                // Applying DC offset for easier math.
                levelScale = level * 2.0f;
                noiseLevelScale = noiseLevel * 2.0f;

                updateNoise = downsampleCounter >= downsampleFactor;
                downsampleCounter++;
                // Lower temperal resolution of the noise by changing value every n samples
                if (updateNoise) {
                        noise = random.nextFloat() * noiseLevelScale - noiseLevel;
                        noise2 = random.nextFloat() * noiseLevelScale - noiseLevel;
                        downsampleCounter = 0;
                }

                smoothedNoise += (noise - smoothedNoise)/noiseSmoothing;
                smoothedNoise2 += (noise2 - smoothedNoise2)/noiseSmoothing;
                for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel) {

                        buffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);


                        // A basic FM synthesis operator
                        currentSample = (float) std::sin (currentAngle+std::sin (currentAngle2));

                        currentAngle += angleDelta*(smoothedNoise+1.0f);;
                        currentAngle2 += angleDelta2*(smoothedNoise2+1.0f);

                        // Hot diggity damn! We have samples to put in the buffer!
                        buffer[sample] = (currentSample * levelScale - level);
                }
        }
}

void updateAngleDelta() {
        cyclesPerSample = freqSlider.getValue() / currentSampleRate;
        cyclesPerSample2 = freqSlider2.getValue() / currentSampleRate;
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

        noiseSmoothingLabel.setBounds (10, 160, 90, 20);
        noiseSmoothingSlider.setBounds (100, 160, getWidth() - 110, 20);
}


private:
//==============================================================================

Random random;

Slider levelSlider, freqSlider, freqSlider2, noiseLevelSlider, noiseResolutionSlider, noiseSmoothingSlider;

Label levelLabel, freqLabel, freqLabel2, noiseLevelLabel, noiseResolutionLabel, noiseSmoothingLabel;

double currentSampleRate, currentAngle, currentAngle2, angleDelta, angleDelta2, cyclesPerSample, cyclesPerSample2;

// These parameters stay in the audio thread.
float smoothedNoise, smoothedNoise2, noise, noise2, currentSample, levelScale, noiseLevel, noiseLevelScale, level;

// Using atomic to prevent data-races, locks and priority inversion between the audio and GUI thread.
std::atomic<float> targetLevel, targetNoiseLevel, noiseSmoothing, downsampleFactor;

int downsampleCounter = 0;
bool updateNoise = false;


// Our dear audio buffer! Oooooooh yeah!
float* buffer;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


Component* createMainContentComponent()  {
        return new MainContentComponent();
}

#endif  // MAINCOMPONENT_H_INCLUDED
