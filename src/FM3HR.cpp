#define _USE_MATH_DEFINES
#include "plugin.hpp"


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

	float phase = 0;
	float Frequency = 0;

	float CoarseAmount = 0;
	float WaveAmount = 0;
	float FineAmount = 0;
	float FM1Amount = 0;
	float FM2Amount = 0;
	float FM3Amount = 0;
	float SmoothAmount = 0;

	dsp::ExponentialSlewLimiter slew{};

	void process(const ProcessArgs& args) override {

		CollectSignals(args);

		phase += args.sampleTime * Frequency;

//Ensures that reset only happens once per pulse

		if(ReadyToReset && ResetPhase){
			phase = 0;
			ResetPhase = false;
		}
		if(phase>=1){
			phase -= 1;
		}

//Oscillator Stage

		float OscOutput = 0;

		if(WaveAmount <= 1){
			OscOutput = (1-WaveAmount)*SineWave(phase) + WaveAmount * Triangle(phase);
		}
		else if(WaveAmount <= 2){
			OscOutput = (2-WaveAmount)*Triangle(phase) + (WaveAmount-1)*SineSquare(phase);
		}
		else if(WaveAmount <= 3){
			OscOutput = (3-WaveAmount)*SineSquare(phase) + (WaveAmount-2)*SawWave(phase);
		}
		else if(WaveAmount <= 4){
			OscOutput = (4-WaveAmount)*SawWave(phase) + (WaveAmount-3)*Square(phase,0.5f);
		}
		else if(WaveAmount <= 5){
			OscOutput = Square(phase,(0.5-((WaveAmount-4)*0.45))); //0.45 chosen so as to not make pulse too small
		}
		
		outputs[OSC_OUTPUT].setVoltage(OscOutput*5);

//Additive FM Stage

		float TypeOneCarrier = OscOutput;
		float TypeOneModulator = (inputs[FM1_MOD_INPUT].isConnected()) ? inputs[FM1_MOD_INPUT].getVoltage()*0.2f : OscOutput;

		float TypeOneOutput = ((TypeOneCarrier + FM1Amount*(sin(2*M_PI*TypeOneModulator)))/(1+FM1Amount)*5.f);
		outputs[FM1_OUTPUT].setVoltage(TypeOneOutput);

//Linear FM Stage

		float TypeTwoCarrier = TypeOneOutput*0.05f;
		float TypeTwoModulator = (inputs[FM2_MOD_INPUT].isConnected()) ? inputs[FM2_MOD_INPUT].getVoltage()*0.2f : TypeOneOutput;

		float TypeTwoOutput = sin(2*M_PI*TypeTwoCarrier*std::pow(2,FM2Amount*5*sin(TypeTwoModulator)))*5.f;
		outputs[FM2_OUTPUT].setVoltage(TypeTwoOutput);

//Exponential FM Stage

		float TypeThreeCarrier = TypeTwoOutput*0.05f;
		float TypeThreeModulator = (inputs[FM3_MOD_INPUT].isConnected()) ? inputs[FM3_MOD_INPUT].getVoltage()*0.2f : TypeTwoOutput;

		float TypeThreeOutput = sin(2*M_PI*TypeThreeCarrier + FM3Amount*sin(2*M_PI*TypeThreeModulator))*5;
		outputs[FM3_OUTPUT].setVoltage(TypeThreeOutput);

//Slew Stage

		float SlewOutput = slew.process(args.sampleTime,TypeThreeOutput);
		outputs[MAIN_OUTPUT].setVoltage(SlewOutput*(1+std::pow(SmoothAmount,100)));
	}


	
	void CollectSignals(const ProcessArgs& args){
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

		CoarseAmount = clamp(CoarseKnob + CoarseCV * inputs[COARSE_INPUT].getVoltage(),-5.f,5.f);
		FineAmount = clamp(FineKnob + FineCV * inputs[FINE_INPUT].getVoltage()*0.02F,-0.1f,0.1f);
		WaveAmount = clamp(WaveKnob + WaveCV * inputs[WAVE_INPUT].getVoltage()*0.5F,0.f,5.f);
		FM1Amount = clamp(FM1Knob + FM1CV * inputs[FM1_INPUT].getVoltage()*0.1F,0.f,1.f);
		FM2Amount = clamp(FM2Knob + FM2CV * inputs[FM2_INPUT].getVoltage()*0.1F,0.f,1.f);
		FM3Amount = clamp(FM3Knob + FM3CV * inputs[FM3_INPUT].getVoltage()*0.1F,0.f,1.f);
		SmoothAmount = clamp(SmoothKnob + SmoothCV * inputs[SMOOTH_INPUT].getVoltage()*0.1F,0.f,1.f);

		if(inputs[RESET_INPUT].getVoltage() == 10.f){
			ResetPhase = true;
			ReadyToReset = false;
		}

		if(inputs[RESET_INPUT].getVoltage() == 0.f && !ReadyToReset){
			ReadyToReset = true;
		}

		float FrequencySource = (inputs[NOTE_INPUT].isConnected()) ? inputs[NOTE_INPUT].getVoltage()+FineAmount : CoarseAmount+FineAmount;
		Frequency = dsp::FREQ_C4*std::pow(2,FrequencySource);

		slew.setRiseFall(2*Frequency+(1-SmoothAmount)*(args.sampleRate-2*Frequency),2*Frequency+(1-SmoothAmount)*(args.sampleRate-2*Frequency));

	}
	
//Wave generators. Constants chosen so as to normalize the output to -1,1.

	float SawWave(float Phase){
		return (0.5-Phase)*2;
	}
	float SineSquare(float Phase){
		float rval = (Phase < 0.5) ? Phase : -Phase;
		return (rval+0.25)*1.25;
	}
	float Triangle(float Phase){
		float tri = (Phase < 0.5) ? Phase : 1-Phase;
		return (tri-0.25)*4;
	}
	float Square(float Phase, float PWM){
		return (Phase < PWM) ? 1 : -1;
	}
	float SineWave(float Phase){
		return sin(2*M_PI*Phase);
	}
};


struct FM3HRWidget : ModuleWidget {
	FM3HRWidget(FM3HR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/FM3HR.svg")));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.92, 17.951)), module, FM3HR::COARSE_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(26.084, 17.951)), module, FM3HR::FINE_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(19.18, 28.125)), module, FM3HR::COARSE_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(32.591, 28.125)), module, FM3HR::FINE_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 51.433)), module, FM3HR::WAVE_CV_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.152, 58.714)), module, FM3HR::WAVE_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 63.433)), module, FM3HR::FM1_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.152, 70.541)), module, FM3HR::FM1_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 75.433)), module, FM3HR::FM2_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.152, 82.541)), module, FM3HR::FM2_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 87.433)), module, FM3HR::FM3_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.152, 94.577)), module, FM3HR::FM3_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(14.625, 99.433)), module, FM3HR::SMOOTH_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.152, 106.54)), module, FM3HR::SMOOTH_PARAM));

		addInput(createInputCentered<TinyJack>(mm2px(Vec(11.92, 36.049)), module, FM3HR::COARSE_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(26.084, 36.049)), module, FM3HR::FINE_INPUT));
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