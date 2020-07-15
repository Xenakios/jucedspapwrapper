#include <JuceHeader.h>

using namespace juce;


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
    std::function<void(DSPType&, int,float)> ParamSetterFunc;
    template<typename... Args>
    DSPWrapper(std::vector<DSP_Parameter> params,
               std::function<void(DSPType&, int,float)> psf, Args... args) : m_dsp(args...)
    {
        ParamSetterFunc = psf;
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
        auto& pars = getParameters();
        for (int i=0;i<pars.size();++i)
        {
            auto par = dynamic_cast<AudioParameterFloat*>(pars[i]);
            float parVal = *par;
            if (ParamSetterFunc)
                ParamSetterFunc(m_dsp,i,parVal);
                
        }
        
        dsp::AudioBlock<float> block(buffer);
        m_dsp.process(dsp::ProcessContextReplacing<float>(block));
    }
    
    double getTailLengthSeconds() const override { return 0.0; }
    
    bool acceptsMidi() const override { return false; }
    
    bool producesMidi() const override { return false; }
    
    juce::AudioProcessorEditor *createEditor() override
    {
        return new GenericAudioProcessorEditor(*this);
    }
    
    bool hasEditor() const override { return true; }
    
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
inline std::unique_ptr<AudioProcessor> make_dsp_wrapper(
                                                        std::vector<DSP_Parameter> params,
                                                        std::function<void(DSPType<float>&,int,float)> pf,
                                                        Args... args)
{
    return std::make_unique<DSPWrapper<DSPType<float>>>(params, pf, args...);
}

//==============================================================================
int main (int argc, char* argv[])
{
    auto parsetfunc = [](auto&, int index, float)
    {
        
    };
    auto parsetfuncdelay = [](dsp::DelayLine<float>& dsp, int index, float v)
    {
        std::cout << "setting delay to " << v << "\n";
        dsp.setDelay(v);
    };
    auto parsetcompressor = [](dsp::Compressor<float>& dsp, int index, float val)
    {
        std::cout << index << " set to " << val << "\n";
        if (index == 0)
            dsp.setThreshold(val);
        if (index == 1)
            dsp.setRatio(val);
        if (index == 2)
            dsp.setAttack(val);
        if (index == 3)
            dsp.setRelease(val);
    };
    auto proc1 = make_dsp_wrapper<dsp::DelayLine>({{"DELAYTIME","Delay",10.0f,44100.0f,22050.f}},parsetfuncdelay,44100);
    auto proc2 = make_dsp_wrapper<dsp::Panner>({{"PAN","Pan",-1.0f,1.0f,0.0f}},parsetfunc);
    auto proc3 = make_dsp_wrapper<dsp::Compressor>(
                                                   {{"THRESHOLD","Threshold",-60.0f,0.0f,-12.0f},
                                                    {"RATIO","Ratio",1.0f,16.0f,2.0f},
                                                    {"ATTACK","Attack",0.1f,100.0f,20.0f},
                                                    {"RELEASE","Release",0.1f,100.0f,20.0f}
                                                   },parsetcompressor);
    // DSPWrapper<juce::dsp::Panner<float>> proc;
    AudioBuffer<float> buf(1,512);
    buf.clear();
    MidiBuffer dbuf;
    proc1->prepareToPlay(44100, 512);
    proc2->prepareToPlay(44100, 512);
    proc3->prepareToPlay(44100, 512);
    proc1->getParameters()[0]->setValue(0.1);
    proc3->getParameters()[0]->setValue(0.1);
    proc1->processBlock(buf, dbuf);
    proc2->processBlock(buf, dbuf);
    proc3->processBlock(buf, dbuf);
    return 0;
}
