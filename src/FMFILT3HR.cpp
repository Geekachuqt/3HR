#include "plugin.hpp"
using simd::float_4;

template <typename T> struct FilterCorePrototype{

	float_4 Buffer_Factor_1_A = 0;
	float_4 Buffer_Factor_2_A = 0;

	float_4 Input_Factor_1_A = 0;
	float_4 Input_Factor_2_A = 0;
	float_4 Input_Factor_3_A = 0;

	float_4 Input_Buffer_1_A = 0;
	float_4 Input_Buffer_2_A = 0;

	float_4 Output_Buffer_1_A = 0;
	float_4 Output_Buffer_2_A = 0;

	float_4 InFraction1_A = 0;
	float_4 InFraction2_A = 0;
	float_4 InFraction3_A = 0;

	float_4 OutFraction1_A = 0;
	float_4 OutFraction2_A = 0;

	float_4 Buffer_Factor_1_B = 0;
	float_4 Buffer_Factor_2_B = 0;

	float_4 Input_Factor_1_B = 0;
	float_4 Input_Factor_2_B = 0;
	float_4 Input_Factor_3_B = 0;

	float_4 Input_Buffer_1_B = 0;
	float_4 Input_Buffer_2_B = 0;

	float_4 Output_Buffer_1_B = 0;
	float_4 Output_Buffer_2_B = 0;

	float_4 InFraction1_B = 0;
	float_4 InFraction2_B = 0;
	float_4 InFraction3_B = 0;

	float_4 OutFraction1_B = 0;
	float_4 OutFraction2_B = 0;

	float_4 Output = 0;
	float_4 Output_A = 0;

	bool debug = false;

	void Advance(float_4 InputPre, float_4 CutoffAmount, float_4 Reso, float_4 sampleRate, float_4 DriveAmount){
		float_4 M_PI_ = M_PI;
		float_4 Cutoff = 1.0f / simd::tan(M_PI_ * ((0.5 * (CutoffAmount*CutoffAmount) * sampleRate) / sampleRate));
		float_4 Resonance = (1-Reso);
		float_4 Resonance2 = simd::pow(Resonance,0.75);

		float_4 Input = InputPre * simd::pow(1.f+DriveAmount, 2);

		Input_Factor_1_A = 1.0f / ( 1.0f + (Resonance * Cutoff + Cutoff * Cutoff));
		Input_Factor_2_A = (2.f * Input_Factor_1_A);
		Input_Factor_3_A = Input_Factor_1_A;

		Buffer_Factor_1_A = 2.0 * ( 1.0 - Cutoff*Cutoff) * Input_Factor_1_A;
		Buffer_Factor_2_A = ( 1.0 - Resonance * Cutoff + Cutoff * Cutoff) * Input_Factor_1_A;

		InFraction1_A = Input_Factor_1_A * Input;
		InFraction2_A = Input_Factor_2_A * Input_Buffer_1_A;
		InFraction3_A = Input_Factor_3_A * Input_Buffer_2_A;

		OutFraction1_A = Buffer_Factor_1_A * Output_Buffer_1_A;
		OutFraction2_A = Buffer_Factor_2_A * Output_Buffer_2_A;

		Output_A = (InFraction1_A + InFraction2_A + InFraction3_A) - (OutFraction1_A + OutFraction2_A);
		
		Input_Buffer_2_A = Input_Buffer_1_A;
    	Input_Buffer_1_A = Input;

        Output_Buffer_2_A = Output_Buffer_1_A;
        Output_Buffer_1_A = clamp(Output_A,-5.f,5.f);

///////////////////////////////////////////////////////////////////////////

		Input_Factor_1_B = 1.0f / ( 1.0f + (Resonance2 * Cutoff + Cutoff * Cutoff));
		Input_Factor_2_B = (2.f * Input_Factor_1_B);
		Input_Factor_3_B = Input_Factor_1_B;

		Buffer_Factor_1_B = 2.0f * ( 1.0f - Cutoff*Cutoff) * Input_Factor_1_B;
		Buffer_Factor_2_B = ( 1.0f - Resonance2 * Cutoff + Cutoff * Cutoff) * Input_Factor_1_B;

		InFraction1_B = Input_Factor_1_B * Output_A;
		InFraction2_B = Input_Factor_2_B * Input_Buffer_1_B;
		InFraction3_B = Input_Factor_3_B * Input_Buffer_2_B;

		OutFraction1_B = Buffer_Factor_1_B * Output_Buffer_1_B;
		OutFraction2_B = Buffer_Factor_2_B * Output_Buffer_2_B;

		Output = (InFraction1_B + InFraction2_B + InFraction3_B) - (OutFraction1_B + OutFraction2_B);
	
		Input_Buffer_2_B = Input_Buffer_1_B;
    	Input_Buffer_1_B = Output_A;

        Output_Buffer_2_B = Output_Buffer_1_B;
        Output_Buffer_1_B = clamp(Output,-5.f,5.f);
	}
	float_4 GetOutput(float_4 BassAmount){
		return clamp((Output+BassAmount*OutFraction2_B)*0.5f,-10.f, 10.f);
	}	
};


struct FMFILT3HR : Module {
	enum ParamId {
		CUTOFF_CV_PARAM,
		FM_ADD_PARAM,
		CUTOFF_PARAM,
		FM_MULT_PARAM,
		FM_ADD_CV_PARAM,
		FM_MULT_CV_PARAM,
		RES_PARAM,
		RES_CV_PARAM,
		DRIVE_PARAM,
		BASS_PARAM,
		DRIVE_CV_PARAM,
		BASS_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CUTOFF_CV_INPUT,
		FM_ADD_AUDIO_INPUT,
		FM_MULT_AUDIO_INPUT,
		FM_ADD_CV_INPUT,
		FM_MULT_CV_INPUT,
		RES_CV_INPUT,
		DRIVE_CV_INPUT,
		BASS_CV_INPUT,
		AUDIO_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	FMFILT3HR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CUTOFF_CV_PARAM, 0.014032f, 0.95f, 0.f, "Cutoff CV");
		configParam(FM_ADD_PARAM, 0.f, 1.f, 0.f, "Additive FM Amount");
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.f, "Cutoff");
		configParam(FM_MULT_PARAM, 0.f, 1.f, 0.f, "Multiplicate FM Amount");
		configParam(FM_ADD_CV_PARAM, 0.f, 1.f, 0.f, "Additive FM Amount CV");
		configParam(FM_MULT_CV_PARAM, 0.f, 1.f, 0.f, "Multiplicative FM Amount CV");
		configParam(RES_PARAM, 0.01f, 0.98f, 0.f, "Resonance");
		configParam(RES_CV_PARAM, 0.f, 1.f, 0.f, "Resonance CV");
		configParam(DRIVE_PARAM, -1.f, 1.f, 0.f, "Drive");
		configParam(BASS_PARAM, 0.f, 1.f, 0.f, "Bass");
		configParam(DRIVE_CV_PARAM, 0.f, 1.f, 0.f, "Drive CV");
		configParam(BASS_CV_PARAM, 0.f, 1.f, 0.f, "Bass CV");
		configInput(CUTOFF_CV_INPUT, "Cutoff CV");
		configInput(FM_ADD_AUDIO_INPUT, "Additive FM Audio");
		configInput(FM_MULT_AUDIO_INPUT, "Multiplicative FM Audio");
		configInput(FM_ADD_CV_INPUT, "Additive FM CV");
		configInput(FM_MULT_CV_INPUT, "Multiplicative FM CV");
		configInput(RES_CV_INPUT, "Resonance CV");
		configInput(DRIVE_CV_INPUT, "Drive CV");
		configInput(BASS_CV_INPUT, "Bass CV");
		configInput(AUDIO_INPUT, "Audio");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

	FilterCorePrototype<float_4> filterCore[4];

	float_4 CutoffAmount = 0;
	float_4 ResoAmount = 0;
	float_4 DriveAmount = 0;
	float_4 FMAmount = 0;
	float_4 FMAmountAdd = 0;
	float_4 BassAmount = 0;

	float_4 FMInput = 0;
	float_4 FMInputAdd = 0;
	float_4 Input = 0;

	float_4 Ones = 1;
	float_4 Cutoff = 0;

	float_4 sampleRate = 0;
	float Channels = 0;

	void process(const ProcessArgs& args) override {
		Channels = std::max(inputs[AUDIO_INPUT].getChannels(),1);
		outputs[AUDIO_OUTPUT].setChannels(Channels);
		sampleRate = args.sampleRate;
		for(int c = 0; c < Channels; c+=4){
			ConfigCollect(c);
			filterCore[c/4].Advance(Input,Cutoff,ResoAmount,sampleRate, DriveAmount);
			outputs[AUDIO_OUTPUT].setVoltageSimd(filterCore[c/4].GetOutput(BassAmount),c);
		}
	}

	void ConfigCollect(int c){
	float_4 CutoffKnob = params[CUTOFF_PARAM].getValue();
	float_4 CutoffCV = params[CUTOFF_CV_PARAM].getValue();
	float_4 ResoKnob = params[RES_PARAM].getValue();
	float_4 ResoCV = params[RES_CV_PARAM].getValue();
	float_4 DriveKnob = params[DRIVE_PARAM].getValue();
	float_4 DriveCV = params[DRIVE_CV_PARAM].getValue();
	float_4 FMKnob = params[FM_MULT_PARAM].getValue();
	float_4 FMCV = params[FM_MULT_CV_PARAM].getValue();
	float_4 FMKnobAdd = params[FM_ADD_PARAM].getValue();
	float_4 FMCVAdd = params[FM_ADD_CV_PARAM].getValue();
	float_4 BassKnob = params[BASS_PARAM].getValue();
	float_4 BassCV = params[BASS_CV_PARAM].getValue();

	CutoffAmount = CutoffKnob + CutoffCV * inputs[CUTOFF_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F;
	ResoAmount = clamp(ResoKnob + ResoCV * inputs[RES_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F,0.f,1.f);
	DriveAmount = clamp(DriveKnob + DriveCV * inputs[DRIVE_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1f,-1.f,1.f);
	BassAmount = clamp(BassKnob + BassCV * inputs[BASS_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1f, 0.f, 1.f);
	FMAmount = clamp(FMKnob + FMCV * inputs[FM_MULT_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1f,0.f,1.f);
	FMAmountAdd = clamp(FMKnobAdd + FMCVAdd * inputs[FM_ADD_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1f,0.f,1.f);

	FMInput = clamp(inputs[FM_MULT_AUDIO_INPUT].getNormalPolyVoltageSimd<float_4>(Ones,c)*0.3f,-1.f,1.f);
	FMInputAdd = clamp(inputs[FM_ADD_AUDIO_INPUT].getNormalPolyVoltageSimd<float_4>(Ones,c)*0.2f,-1.f,1.f);
	Input = inputs[AUDIO_INPUT].getPolyVoltageSimd<float_4>(c);

	Cutoff = (inputs[FM_MULT_AUDIO_INPUT].isConnected()) ? clamp((FMAmount*FMInput+1-FMAmount) * CutoffAmount,0.014032f,0.95f) : clamp(CutoffAmount, 0.014032f,0.95f);
	Cutoff = (inputs[FM_ADD_AUDIO_INPUT].isConnected()) ? clamp(FMInputAdd*FMAmountAdd*CutoffAmount + Cutoff,0.014032f,0.95f) : Cutoff;
	}
};


struct FMFILT3HRWidget : ModuleWidget {
	FMFILT3HRWidget(FMFILT3HR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/FMFILT3HR.svg")));

		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(20.63, 34.694)), module, FMFILT3HR::CUTOFF_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(8.19, 48.04)), module, FMFILT3HR::FM_ADD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.63, 48.14)), module, FMFILT3HR::CUTOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(32.45, 48.04)), module, FMFILT3HR::FM_MULT_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(8.19, 57.87)), module, FMFILT3HR::FM_ADD_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(32.451, 57.87)), module, FMFILT3HR::FM_MULT_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.63, 59.54)), module, FMFILT3HR::RES_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(20.63, 69.37)), module, FMFILT3HR::RES_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.736, 79.507)), module, FMFILT3HR::DRIVE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(28.524, 79.637)), module, FMFILT3HR::BASS_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(12.736, 89.337)), module, FMFILT3HR::DRIVE_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(28.524, 89.467)), module, FMFILT3HR::BASS_CV_PARAM));

		addInput(createInputCentered<TinyJack>(mm2px(Vec(20.63, 23.433)), module, FMFILT3HR::CUTOFF_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.19, 37.34)), module, FMFILT3HR::FM_ADD_AUDIO_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.45, 37.34)), module, FMFILT3HR::FM_MULT_AUDIO_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(8.19, 67.699)), module, FMFILT3HR::FM_ADD_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(32.45, 67.699)), module, FMFILT3HR::FM_MULT_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(20.63, 79.199)), module, FMFILT3HR::RES_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(12.736, 99.166)), module, FMFILT3HR::DRIVE_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(28.524, 99.296)), module, FMFILT3HR::BASS_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(6.94, 117.0)), module, FMFILT3HR::AUDIO_INPUT));

		addOutput(createOutputCentered<TinyJack>(mm2px(Vec(33.699, 117.0)), module, FMFILT3HR::AUDIO_OUTPUT));
	}
};


Model* modelFMFILT3HR = createModel<FMFILT3HR, FMFILT3HRWidget>("FMFILT3HR");