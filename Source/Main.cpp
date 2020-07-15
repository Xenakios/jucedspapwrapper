/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>

using namespace juce;

void wrapper_setParameter(dsp::Panner<float>& dsp, int index, float val)
{
    std::cout << "setting pan to " << val << "\n";
    dsp.setPan(val);
}

void wrapper_setParameter(dsp::DelayLine<float>& dsp, int index, float val)
{
    std::cout << "setting delay to " << val << "\n";
    dsp.setDelay(val);
}

struct DSP_Parameter
{
    String id;
    String name;
    float minvalue = 0.0f;
    float maxvalue = 0.0f;
    float defaultValue = 0.0f;
};

template<typename DSPType>
class DSPWrapper : public juce::AudioProcessor
{
public:
    DSPType m_dsp;
    template<typename... Args>
    DSPWrapper(std::vector<DSP_Parameter> params, Args... args) : m_dsp(args...)
    {
        for (int i=0;i<params.size();++i)
        {
            DSP_Parameter& par = params[i];
            addParameter(new AudioParameterFloat(par.id,par.name,par.minvalue,par.maxvalue,par.defaultValue));
                                            
        }
    }
    
    const juce::String getName() const override { return String(); }
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        dsp::ProcessSpec spec;
        spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
        spec.numChannels = 1;
        spec.sampleRate = sampleRate;
        m_dsp.prepare(spec);
    }
    float getFloatParameter(int index)
    {
        auto par = dynamic_cast<AudioParameterFloat*>(getParameters()[index]);
        return *par;
    }
    void setFloatParameter(int index, float val)
    {
        auto par = dynamic_cast<AudioParameterFloat*>(getParameters()[index]);
        *par = val;
    }
    void releaseResources() override
    {
    }
    
    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override
    {
        auto par = dynamic_cast<AudioParameterFloat*>(getParameters()[0]);
        float parVal = *par;
        wrapper_setParameter(m_dsp, 0, parVal);
        dsp::AudioBlock<float> block(buffer);
        m_dsp.process(dsp::ProcessContextReplacing<float>(block));
    }
    
    double getTailLengthSeconds() const override { return 0.0; }
    
    bool acceptsMidi() const override { return false; }
    
    bool producesMidi() const override { return false; }
    
    juce::AudioProcessorEditor *createEditor() override { return nullptr; }
    
    bool hasEditor() const override { return false; }
    
    int getNumPrograms() override { return 0; }
    
    int getCurrentProgram() override { return 0; }
    
    void setCurrentProgram(int index) override {}
    
    const juce::String getProgramName(int index) override { return String(); }
    
    void changeProgramName(int index, const juce::String &newName) override {}
    
    void getStateInformation(juce::MemoryBlock &destData) override {}
    
    void setStateInformation(const void *data, int sizeInBytes) override {}
    
};

template<typename DSPType, typename... Args>
inline std::unique_ptr<AudioProcessor> make_dsp_wrapper2(std::vector<DSP_Parameter> params, Args... args)
{
    return std::make_unique<DSPWrapper<DSPType>>(params, args...);
}

template<template<class...> typename DSPType, typename... Args>
inline std::unique_ptr<AudioProcessor> make_dsp_wrapper(std::vector<DSP_Parameter> params, Args... args)
{
    return std::make_unique<DSPWrapper<DSPType<float>>>(params, args...);
}

//==============================================================================
int main (int argc, char* argv[])
{
    auto proc1 = make_dsp_wrapper<dsp::DelayLine>({{"DELAYTIME","Delay",10.0f,44100.0f,22050.f}},44100);
    auto proc2 = make_dsp_wrapper<dsp::Panner>({{"PAN","Pan",-1.0f,1.0f,0.0f}});
    // DSPWrapper<juce::dsp::Panner<float>> proc;
    AudioBuffer<float> buf(1,512);
    MidiBuffer dbuf;
    proc1->prepareToPlay(44100, 512);
    proc2->prepareToPlay(44100, 512);
    proc1->getParameters()[0]->setValue(0.5);
    //std::cout << proc1->getParameters().size() << " parameters\n";
    proc1->processBlock(buf, dbuf);
    proc2->processBlock(buf, dbuf);

    return 0;
}
