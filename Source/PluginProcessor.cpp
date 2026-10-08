#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
PERCULATORAudioProcessor::PERCULATORAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"STATE",createLayout()){}
juce::AudioProcessorValueTreeState::ParameterLayout
PERCULATORAudioProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<F> (
        juce::ParameterID { "harmonics", 1 },
        "Harmonics",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.5f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "balance", 1 },
        "Balance",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "input", 1 },
        "Input",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "bias", 1 },
        "Bias",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "output", 1 },
        "Output",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<C> (
        juce::ParameterID { "circuit", 1 },
        "Circuit",
        juce::StringArray {
            "NPN OD",
            "D310 Diodes",
            "Albino"
        },
        1));

    layout.add (std::make_unique<B> (
        juce::ParameterID { "iron", 1 },
        "IR On",
        false));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "irmix", 1 },
        "IR Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "irlevel", 1 },
        "IR Level",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<B> (
        juce::ParameterID { "phase", 1 },
        "Phase 180",
        false));

    layout.add (std::make_unique<C> (
        juce::ParameterID { "oversampling", 1 },
        "Oversampling",
        juce::StringArray {
            "2x",
            "4x",
            "8x"
        },
        1));

    layout.add (std::make_unique<B> (
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false));

    return layout;
}
void PERCULATORAudioProcessor::prepareToPlay(double sr,int block){sampleRateHz=sr;juce::dsp::ProcessSpec sp{sr,(juce::uint32)block,(juce::uint32)juce::jmax(1,getTotalNumOutputChannels())};convolution.prepare(sp);irMixer.prepare(sp);memory.fill(0);albinoLowState.fill(0);albinoHighState.fill(0);dcX1.fill(0);dcY1.fill(0);inputMeter=outputMeter=0;}
bool PERCULATORAudioProcessor::isBusesLayoutSupported(const BusesLayout& l)const{auto o=l.getMainOutputChannelSet();return(o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo())&&o==l.getMainInputChannelSet();}
float PERCULATORAudioProcessor::processCircuitSample(float x,int ch,int c,float h,float b)noexcept{auto i=(size_t)juce::jlimit(0,1,ch);float d=1+2.7f*h;float asym=1+.35f*b; if(c==0){float z=d*x;return .86f*(z>=0?std::tanh(z*asym):std::tanh(z/asym))+.08f*std::tanh(4*x);}if(c==1){float z=d*x;return z>=0?.88f*std::tanh(1.3f*z*asym):.69f*std::tanh(1.85f*z/asym);}float la=std::exp(-juce::MathConstants<float>::twoPi*18/(float)sampleRateHz);albinoLowState[i]=la*albinoLowState[i]+(1-la)*x;float z=(1.3f+3*h)*(x-.34f*albinoLowState[i]);float ge=z>=0?.74f*std::tanh(1.1f*z*asym):.96f*std::tanh(1.62f*z/asym);float si=.82f*std::tanh(1.52f*ge+.2f*z);float di=si>=0?.86f*std::tanh(1.38f*si*asym):.70f*std::tanh(1.88f*si/asym);float ha=std::exp(-juce::MathConstants<float>::twoPi*6800/(float)sampleRateHz);albinoHighState[i]=ha*albinoHighState[i]+(1-ha)*di;return .84f*albinoHighState[i]+.16f*di;}
float PERCULATORAudioProcessor::peakToMeter(float p)noexcept{if(p<0.00002f)return 0;float db=juce::Decibels::gainToDecibels(p,-60.f);return juce::jlimit(0.f,1.f,(db+48)/48);}
void PERCULATORAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&){juce::ScopedNoDenormals nd;float inDb=apvts.getRawParameterValue("input")->load(),outDb=apvts.getRawParameterValue("output")->load(),h=apvts.getRawParameterValue("harmonics")->load()/10,bal=apvts.getRawParameterValue("balance")->load()/10,bias=apvts.getRawParameterValue("bias")->load()/10,wet=apvts.getRawParameterValue("mix")->load()/100;int cir=juce::roundToInt(apvts.getRawParameterValue("circuit")->load());bool by=apvts.getRawParameterValue("bypass")->load()>.5f;float nominal[]={juce::Decibels::decibelsToGain(15.f),juce::Decibels::decibelsToGain(17.f),juce::Decibels::decibelsToGain(16.f)};float ig=juce::Decibels::decibelsToGain(inDb)*nominal[juce::jlimit(0,2,cir)],og=juce::Decibels::decibelsToGain(outDb);float ip=0,op=0,dcR=std::exp(-juce::MathConstants<float>::twoPi*10/(float)sampleRateHz);for(int c=0;c<b.getNumChannels();++c){auto*d=b.getWritePointer(c);int k=juce::jmin(c,1);for(int n=0;n<b.getNumSamples();++n){float dry=d[n],x=dry*ig;ip=juce::jmax(ip,std::abs(x));if(std::abs(x)<1e-7f){memory[k]*=.995f;albinoLowState[k]*=.995f;albinoHighState[k]*=.995f;}memory[k]=.9975f*memory[k]+.0025f*x;float y=processCircuitSample(x-.12f*memory[k],k,cir,h,bias)*juce::jmap(bal,.15f,1.25f);float hp=y-dcX1[k]+dcR*dcY1[k];dcX1[k]=y;dcY1[k]=hp;if(std::abs(x)<1e-7f&&std::abs(hp)<1e-6f)hp=0;d[n]=by?dry:juce::jmap(wet,dry,hp)*og;}}if(!by&&apvts.getRawParameterValue("iron")->load()>.5f&&irFile.existsAsFile()){irMixer.setWetMixProportion(apvts.getRawParameterValue("irmix")->load()/100);juce::dsp::AudioBlock<float> bl(b);irMixer.pushDrySamples(bl);juce::dsp::ProcessContextReplacing<float>cx(bl);convolution.process(cx);irMixer.mixWetSamples(bl);b.applyGain(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("irlevel")->load()));if(apvts.getRawParameterValue("phase")->load()>.5f)b.applyGain(-1);}for(int c=0;c<b.getNumChannels();++c)op=juce::jmax(op,b.getMagnitude(c,0,b.getNumSamples()));inputMeter.store(juce::jmax(peakToMeter(ip),inputMeter.load()*.82f));outputMeter.store(juce::jmax(peakToMeter(op),outputMeter.load()*.82f));}
void PERCULATORAudioProcessor::loadImpulseResponse(const juce::File&f){if(!f.existsAsFile())return;irFile=f;irName=f.getFileName();convolution.loadImpulseResponse(f,juce::dsp::Convolution::Stereo::yes,juce::dsp::Convolution::Trim::yes,0,juce::dsp::Convolution::Normalise::yes);}
void PERCULATORAudioProcessor::getStateInformation(juce::MemoryBlock&d){auto s=apvts.copyState();s.setProperty("irPath",irFile.getFullPathName(),nullptr);s.setProperty("panelColour",panelColour.load(),nullptr);if(auto x=s.createXml())copyXmlToBinary(*x,d);}void PERCULATORAudioProcessor::setStateInformation(const void*d,int n){if(auto x=getXmlFromBinary(d,n)){auto s=juce::ValueTree::fromXml(*x);if(s.isValid()){apvts.replaceState(s);panelColour=(int)s.getProperty("panelColour",1);auto f=juce::File(s.getProperty("irPath").toString());if(f.existsAsFile())loadImpulseResponse(f);}}}juce::AudioProcessorEditor*PERCULATORAudioProcessor::createEditor(){return new PERCULATORAudioProcessorEditor(*this);}juce::AudioProcessor*JUCE_CALLTYPE createPluginFilter(){return new PERCULATORAudioProcessor();}
