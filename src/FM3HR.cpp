#include "plugin.hpp"
using simd::float_4;

template <typename T> struct OscillatorCore{
	float_4 Frequency = 1;
	float_4 OscOutput = 0;
	float_4 TypeOneOutput = 0;
	float_4 TypeTwoOutput = 0;
	float_4 TypeThreeOutput = 0;
	float_4 SmoothOutput = 0;
	float_4 Phase = 0;

	dsp::ExponentialSlewLimiter slew1{};
	dsp::ExponentialSlewLimiter slew2{};	
	dsp::ExponentialSlewLimiter slew3{};	
	dsp::ExponentialSlewLimiter slew4{};	

	void Advance(float_4 sampleTime, float_4 CoarseAmount, float_4 FineAmount, float_4 WaveAmount, float_4 FM1Amount, float_4 FM2Amount, float_4 FM3Amount, float_4 SmoothAmount, float_4 sampleRate, Input FM1, Input FM2, Input FM3, Output OscOut, Output FM1Out, Output FM2Out, Output FM3Out, Output MainOut, int firstChannel){
		Phase += sampleTime * Frequency;
		for (int p = 0; p < Phase.size; p++){
			if(Phase[p]>=1){
				Phase[p] -= 1;
			}
		}
		for(int c = 0; c <= 3; c++){
			if(WaveAmount[c] <= 1){
				OscOutput[c] = (1-WaveAmount[c])*SineWave(Phase[c]) + WaveAmount[c] * Triangle(Phase[c]);
			}
			else if(WaveAmount[c] <= 2){
				OscOutput[c] = (2-WaveAmount[c])*Triangle(Phase[c]) + (WaveAmount[c]-1)*SineSquare(Phase[c]);
			}
			else if(WaveAmount[c] <= 3){
				OscOutput[c] = (3-WaveAmount[c])*SineSquare(Phase[c]) + (WaveAmount[c]-2)*SawWave(Phase[c]);
			}
			else if(WaveAmount[c] <= 4){
				OscOutput[c] = (4-WaveAmount[c])*SawWave(Phase[c]) + (WaveAmount[c]-3)*Square(Phase[c],0.5f);
			}
			else if(WaveAmount[c] <= 5){
				OscOutput[c] = Square(Phase[c],(0.5-((WaveAmount[c]-4)*0.45))); //0.45 chosen so as to not make pulse too small
			}
		}


		float_4 TypeOneCarrier = OscOutput;
		float_4 TypeOneModulator = (FM1.isConnected()) ?  FM1.getPolyVoltageSimd<float_4>(firstChannel)*0.2f : OscOutput;
		TypeOneOutput = ((TypeOneCarrier + FM1Amount*(sin(2*M_PI*TypeOneModulator)))/(1+FM1Amount)*5);

		float_4 TypeTwoCarrier = TypeOneOutput*0.05f;
		float_4 TypeTwoModulator = (FM2.isConnected()) ? FM2.getPolyVoltageSimd<float_4>(firstChannel)*0.2f : TypeOneOutput;
		TypeTwoOutput = sin(2*M_PI*TypeTwoCarrier*simd::pow(2,FM2Amount*5*sin(TypeTwoModulator)))*5;

		float_4 TypeThreeCarrier = TypeTwoOutput*0.05f;
		float_4 TypeThreeModulator = (FM3.isConnected()) ? FM3.getPolyVoltageSimd<float_4>(firstChannel)*0.2f : TypeTwoOutput;
		TypeThreeOutput = sin(2*M_PI*TypeThreeCarrier + FM3Amount*sin(2*M_PI*TypeThreeModulator))*5;

		slew1.setRiseFall(2*Frequency[0]+(1-SmoothAmount[0])*(sampleRate[0]-2*Frequency[0]),2*Frequency[0]+(1-SmoothAmount[0])*(sampleRate[0]-2*Frequency[0]));
		SmoothOutput[0] = slew1.process(sampleTime[0],TypeThreeOutput[0])*(1+simd::pow(SmoothAmount[0],100));

		slew2.setRiseFall(2*Frequency[1]+(1-SmoothAmount[1])*(sampleRate[1]-2*Frequency[1]),2*Frequency[1]+(1-SmoothAmount[1])*(sampleRate[1]-2*Frequency[1]));
		SmoothOutput[1] = slew2.process(sampleTime[1],TypeThreeOutput[1])*(1+simd::pow(SmoothAmount[1],100));

		slew3.setRiseFall(2*Frequency[2]+(1-SmoothAmount[2])*(sampleRate[2]-2*Frequency[2]),2*Frequency[2]+(1-SmoothAmount[2])*(sampleRate[2]-2*Frequency[2]));
		SmoothOutput[2] = slew3.process(sampleTime[2],TypeThreeOutput[2])*(1+simd::pow(SmoothAmount[2],100));

		slew4.setRiseFall(2*Frequency[3]+(1-SmoothAmount[3])*(sampleRate[3]-2*Frequency[3]),2*Frequency[3]+(1-SmoothAmount[3])*(sampleRate[3]-2*Frequency[3]));
		SmoothOutput[3] = slew4.process(sampleTime[3],TypeThreeOutput[3])*(1+simd::pow(SmoothAmount[3],100));

	}

	void SetFrequency(float_4 F){
		Frequency = dsp::FREQ_C4*simd::pow(2,F);
	}
	float_4 GetOscOutput(){
		return OscOutput*5;
	}
	float_4 GetFM1Output(){
		return TypeOneOutput;
	}
	float_4 GetFM2Output(){
		return TypeTwoOutput;
	}
	float_4 GetFM3Output(){
		return TypeThreeOutput;
	}
	float_4 GetSmoothOutput(){
		return SmoothOutput;
	}

//SIMD waveform generators

	float SawWave(float Phase){
		return (0.5-Phase)*2;
	}
	float SineSquare(float Phase){
		float rval = (Phase < 0.5f) ? Phase : -Phase;
		return (rval+0.25)*1.25;
	}
	float Triangle(float Phase){
		float rval = (Phase < 0.5f) ? Phase : 1-Phase;
		return (rval-0.25)*4;
	}
	float Square(float Phase, float PWM){
		float rval = (Phase < PWM) ? 1 : -1;

		return rval;
	}
	float SineWave(float Phase){
		return sin(2*M_PI*Phase);
	}
};

struct FM3HR : Module {
	enum ParamId {
		COARSE_PARAM,
		FINE_PARAM,
		COARSE_CV_PARAM,
		FINE_CV_PARAM,
		WAVE_CV_PARAM,
		WAVE_PARAM,
		FM1_CV_PARAM,
		FM1_PARAM,
		FM2_CV_PARAM,
		FM2_PARAM,
		FM3_CV_PARAM,
		FM3_PARAM,
		SMOOTH_CV_PARAM,
		SMOOTH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		COARSE_INPUT,
		FINE_INPUT,
		WAVE_INPUT,
		FM1_INPUT,
		FM1_MOD_INPUT,
		FM2_INPUT,
		FM2_MOD_INPUT,
		FM3_INPUT,
		FM3_MOD_INPUT,
		SMOOTH_INPUT,
		RESET_INPUT,
		NOTE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OSC_OUTPUT,
		FM1_OUTPUT,
		FM2_OUTPUT,
		FM3_OUTPUT,
		MAIN_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	FM3HR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(COARSE_PARAM, -5.f, 5.f, 0.f, "Coarse");
		configParam(FINE_PARAM, -0.1f, 0.1f, 0.f, "Fine");
		configParam(COARSE_CV_PARAM, 0.f, 1.f, 0.f, "Coarse CV Amount");
		configParam(FINE_CV_PARAM, 0.f, 1.f, 0.f, "Coarse CV Amount");
		configParam(WAVE_CV_PARAM, 0.f, 1.f, 0.f, "Waveform CV Amount");
		configParam(WAVE_PARAM, 0.f, 5.f, 0.f, "Waveform");
		configParam(FM1_CV_PARAM, 0.f, 1.f, 0.f, "FM1 CV Amount");
		configParam(FM1_PARAM, 0.f, 1.f, 0.f, "FM1 Amount");
		configParam(FM2_CV_PARAM, 0.f, 1.f, 0.f, "FM2 CV Amount");
		configParam(FM2_PARAM, 0.f, 1.f, 0.f, "FM2 Amount");
		configParam(FM3_CV_PARAM, 0.f, 1.f, 0.f, "FM3 CV Amount");
		configParam(FM3_PARAM, 0.f, 1.f, 0.f, "FM3 Amount");
		configParam(SMOOTH_CV_PARAM, 0.f, 1.f, 0.f, "Smooth CV Amount");
		configParam(SMOOTH_PARAM, 0.f, 1.f, 0.f, "Smooth Amount");
		configInput(COARSE_INPUT, "Coarse CV");
		configInput(FINE_INPUT, "Fine CV");
		configInput(WAVE_INPUT, "Waveform CV");
		configInput(FM1_INPUT, "FM1 CV");
		configInput(FM1_MOD_INPUT, "FM1 Modulator");
		configInput(FM2_INPUT, "FM2 CV");
		configInput(FM2_MOD_INPUT, "FM2 Modulator");
		configInput(FM3_INPUT, "FM3 CV");
		configInput(FM3_MOD_INPUT, "FM3 Modulator");
		configInput(SMOOTH_INPUT, "Smooth CV");
		configInput(RESET_INPUT, "Reset");
		configInput(NOTE_INPUT, "1V/Oct");		
		configOutput(OSC_OUTPUT, "Oscillator");
		configOutput(FM1_OUTPUT, "FM1");
		configOutput(FM2_OUTPUT, "FM2");
		configOutput(FM3_OUTPUT, "FM3");
		configOutput(MAIN_OUTPUT, "Main");
	}

	bool ResetPhase = false;
	bool ReadyToReset = true;

	float_4 Frequency = 0;

	float_4 CoarseAmount = 0;
	float_4 WaveAmount = 0;
	float_4 FineAmount = 0;
	float_4 FM1Amount = 0;
	float_4 FM2Amount = 0;
	float_4 FM3Amount = 0;
	float_4 SmoothAmount = 0;

	int Channels = 0;

	OscillatorCore<float_4> oscillatorCore[4];

	void process(const ProcessArgs& args) override {

		Channels = std::max(inputs[NOTE_INPUT].getChannels(),1);
		outputs[OSC_OUTPUT].setChannels(Channels);
		outputs[FM1_OUTPUT].setChannels(Channels);
		outputs[FM2_OUTPUT].setChannels(Channels);
		outputs[FM3_OUTPUT].setChannels(Channels);
		outputs[MAIN_OUTPUT].setChannels(Channels);

		for(int c = 0; c < Channels; c+=4){
			CollectSignals(args,c);
			float_4 Inputs = (inputs[NOTE_INPUT].isConnected()) ? inputs[NOTE_INPUT].getPolyVoltageSimd<float_4>(c)+FineAmount+round(CoarseAmount) : CoarseAmount+FineAmount;
			oscillatorCore[c/4].SetFrequency(Inputs);
			oscillatorCore[c/4].Advance(args.sampleTime, CoarseAmount, FineAmount, WaveAmount, FM1Amount, FM2Amount, FM3Amount, SmoothAmount, args.sampleRate, inputs[FM1_MOD_INPUT], inputs[FM2_MOD_INPUT], inputs[FM3_MOD_INPUT], outputs[OSC_OUTPUT], outputs[FM1_OUTPUT], outputs[FM2_OUTPUT], outputs[FM3_OUTPUT], outputs[MAIN_OUTPUT], c);			
			outputs[OSC_OUTPUT].setVoltageSimd(oscillatorCore[c/4].GetOscOutput(),c);
			outputs[FM1_OUTPUT].setVoltageSimd(oscillatorCore[c/4].GetFM1Output(),c);
			outputs[FM2_OUTPUT].setVoltageSimd(oscillatorCore[c/4].GetFM2Output(),c);
			outputs[FM3_OUTPUT].setVoltageSimd(oscillatorCore[c/4].GetFM3Output(),c);
			outputs[MAIN_OUTPUT].setVoltageSimd(oscillatorCore[c/4].GetSmoothOutput(),c);
		}	
	}
	
	void CollectSignals(const ProcessArgs& args,int c){
		float CoarseKnob = params[COARSE_PARAM].getValue();
		float CoarseCV = inputs[COARSE_INPUT].isConnected() ? params[COARSE_CV_PARAM].getValue() : 0;
		float FineKnob = params[FINE_PARAM].getValue();
		float FineCV = inputs[FINE_INPUT].isConnected() ?  params[FINE_CV_PARAM].getValue() : 0;
		float WaveKnob = params[WAVE_PARAM].getValue();
		float WaveCV = inputs[WAVE_INPUT].isConnected() ?  params[WAVE_CV_PARAM].getValue() : 0;
		float FM1Knob = params[FM1_PARAM].getValue();
		float FM1CV = inputs[FM1_INPUT].isConnected() ?  params[FM1_CV_PARAM].getValue() : 0;
		float FM2Knob = params[FM2_PARAM].getValue();
		float FM2CV = inputs[FM2_INPUT].isConnected() ?  params[FM2_CV_PARAM].getValue() : 0;
		float FM3Knob = params[FM3_PARAM].getValue();
		float FM3CV = inputs[FM3_INPUT].isConnected() ?  params[FM3_CV_PARAM].getValue() : 0;
		float SmoothKnob = params[SMOOTH_PARAM].getValue();
		float SmoothCV = inputs[SMOOTH_INPUT].isConnected() ?  params[SMOOTH_CV_PARAM].getValue() : 0;

		CoarseAmount = clamp(CoarseKnob + CoarseCV * inputs[COARSE_INPUT].getPolyVoltageSimd<float_4>(c),-5.f,5.f);
		FineAmount = clamp(FineKnob + FineCV * inputs[FINE_INPUT].getPolyVoltageSimd<float_4>(c)*0.02F,-0.1f,0.1f);
		WaveAmount = clamp(WaveKnob + WaveCV * inputs[WAVE_INPUT].getPolyVoltageSimd<float_4>(c)*0.5F,0.f,5.f);
		FM1Amount = clamp(FM1Knob + FM1CV * inputs[FM1_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F,0.f,1.f);
		FM2Amount = clamp(FM2Knob + FM2CV * inputs[FM2_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F,0.f,1.f);
		FM3Amount = clamp(FM3Knob + FM3CV * inputs[FM3_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F,0.f,1.f);
		SmoothAmount = clamp(SmoothKnob + SmoothCV * inputs[SMOOTH_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F,0.f,1.f);

		if(inputs[RESET_INPUT].getVoltage() == 10.f){
			ResetPhase = true;
			ReadyToReset = false;
		}

		if(inputs[RESET_INPUT].getVoltage() == 0.f && !ReadyToReset){
			ReadyToReset = true;
		}
		

	}
};


struct FM3HRWidget : ModuleWidget {
	FM3HRWidget(FM3HR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/FM3HR.svg")));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.92, 19.151)), module, FM3HR::COARSE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(26.084, 19.151)), module, FM3HR::FINE_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(19.18, 29.325)), module, FM3HR::COARSE_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(32.591, 29.325)), module, FM3HR::FINE_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 51.433)), module, FM3HR::WAVE_CV_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.152, 58.714)), module, FM3HR::WAVE_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 63.433)), module, FM3HR::FM1_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.152, 70.541)), module, FM3HR::FM1_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 75.433)), module, FM3HR::FM2_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.152, 82.541)), module, FM3HR::FM2_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 87.433)), module, FM3HR::FM3_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.152, 94.577)), module, FM3HR::FM3_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 99.433)), module, FM3HR::SMOOTH_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.152, 106.54)), module, FM3HR::SMOOTH_PARAM));

		addInput(createInputCentered<TinyJack>(mm2px(Vec(11.92, 37.249)), module, FM3HR::COARSE_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(26.084, 37.249)), module, FM3HR::FINE_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(7.193, 58.714)), module, FM3HR::WAVE_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(7.193, 70.541)), module, FM3HR::FM1_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(14.625, 70.541)), module, FM3HR::FM1_MOD_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(7.193, 82.541)), module, FM3HR::FM2_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(14.625, 82.541)), module, FM3HR::FM2_MOD_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(7.193, 94.577)), module, FM3HR::FM3_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(14.625, 94.577)), module, FM3HR::FM3_MOD_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(7.193, 106.54)), module, FM3HR::SMOOTH_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(29.731, 116.577)), module, FM3HR::RESET_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(10.909, 116.577)), module, FM3HR::NOTE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.321, 58.714)), module, FM3HR::OSC_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.321, 70.541)), module, FM3HR::FM1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.321, 82.541)), module, FM3HR::FM2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.321, 94.577)), module, FM3HR::FM3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.321, 106.54)), module, FM3HR::MAIN_OUTPUT));
	}
};


Model* modelFM3HR = createModel<FM3HR, FM3HRWidget>("FM3HR");