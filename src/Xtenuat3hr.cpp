#include "plugin.hpp"


struct Xtenuat3hr : Module {
	enum ParamId {
		CROSS_PARAM,
		CROSS_CV_PARAM,
		ATTEN_PARAM,
		ATTEN_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		CROSS_CV_INPUT,
		ATTEN_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Xtenuat3hr() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CROSS_PARAM, 0.f, 1.f, 0.5f, "Crossfade");
		configParam(CROSS_CV_PARAM, 0.f, 1.f, 0.f, "Crossfade CV");
		configParam(ATTEN_PARAM, -1.f, 1.f, 0.f, "Attenuverter");
		configParam(ATTEN_CV_PARAM, 0.f, 1.f, 0.f, "Attenuverter CV");
		configInput(A_INPUT, "Audio Signal A");
		configInput(B_INPUT, "Audio Signal B");
		configInput(CROSS_CV_INPUT, "Crossfade CV");
		configInput(ATTEN_CV_INPUT, "Attenuverter CV");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

	float CrossKnob = 0;
	float CrossCV = 0;
	float AttenKnob = 0;
	float AttenCV = 0;

	float Mix = 0;

	float CrossAmount = 0;
	float AttenAmount = 0;

	void process(const ProcessArgs& args) override {
		Collect();

		Mix = inputs[A_INPUT].getVoltage()*(1-CrossAmount) + inputs[B_INPUT].getVoltage()*(CrossAmount);

		if(AttenAmount < 0){
			outputs[AUDIO_OUTPUT].setVoltage(abs(Mix-10)*abs(AttenAmount));
		}
		else{
			outputs[AUDIO_OUTPUT].setVoltage(Mix*AttenAmount);
		}
	}
	void Collect(){
		CrossKnob = params[CROSS_PARAM].getValue();
		CrossCV = params[CROSS_CV_PARAM].getValue();
		AttenKnob = params[ATTEN_PARAM].getValue();
		AttenCV = params[ATTEN_CV_PARAM].getValue();

		CrossAmount = clamp(CrossKnob + CrossCV * inputs[CROSS_CV_INPUT].getVoltage()*0.1F,0.f,1.f);
		AttenAmount = clamp(AttenKnob + AttenCV * inputs[ATTEN_CV_INPUT].getVoltage()*0.2F,-1.f,1.f);
	}
};



struct Xtenuat3hrWidget : ModuleWidget {
	Xtenuat3hrWidget(Xtenuat3hr* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Xtenuat3hr.svg")));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(5.08, 30.403)), module, Xtenuat3hr::CROSS_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(5.08, 40.403)), module, Xtenuat3hr::CROSS_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(5.08, 82.403)), module, Xtenuat3hr::ATTEN_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(5.08, 92.403)), module, Xtenuat3hr::ATTEN_CV_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 10.403)), module, Xtenuat3hr::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 20.403)), module, Xtenuat3hr::B_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(5.08, 50.403)), module, Xtenuat3hr::CROSS_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(5.08, 102.403)), module, Xtenuat3hr::ATTEN_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 112.403)), module, Xtenuat3hr::AUDIO_OUTPUT));
	}
};


Model* modelXtenuat3hr = createModel<Xtenuat3hr, Xtenuat3hrWidget>("Xtenuat3hr");