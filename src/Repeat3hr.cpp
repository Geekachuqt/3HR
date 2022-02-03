#include "plugin.hpp"
/*

Creative, volume-interpolating digital delay with a decimator in the feedback path designed for modulating the time parameter.
It also pushes a trig every time the playhead resets, which can be used for interesting effects when modulating the time parameter.

The volume-interpolation algorithm fades the feedback down to 0 over the amount of samples remaining until a reset, 
and then fading in the feedback over a set amount of samples after a reset. This effectively removes all clicks from the feedback path upon 
changing the time parameter, as the feedback will always be 0 at the jump point.

Robin Hammar 2022-01-17

*/

struct Repeat3hr : Module {
	enum ParamId {
		TIME_CV_PARAM,
		FINE_PARAM,
		FINE_CV_PARAM,
		TIME_PARAM,
		CRUSH_PARAM,
		CRUSH_CV_PARAM,
		FEEDBACK_CV_PARAM,
		FEEDBACK_PARAM,
		INPUT_PARAM,
		INPUT_CV_PARAM,
		DRY_CV_PARAM,
		DRY_PARAM,
		WET_CV_PARAM,
		WET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TIME_CV_INPUT,
		FINE_CV_INPUT,
		CRUSH_CV_INPUT,
		FEEDBACK_CV_INPUT,
		INPUT_CV_INPUT,
		DRY_CV_INPUT,
		WET_CV_INPUT,
		AUDIO_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		TEMPO_TRIG_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		TEMPO_LIGHT,
		LIGHTS_LEN
	};

	Repeat3hr() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(WET_PARAM, 0.f, 1.f, 0.2f, "Wet");
		configParam(DRY_PARAM, 0.f, 1.f, 1.f, "Dry");
		configParam(FINE_PARAM, -0.099f, 0.099f, 0.f, "Fine");
		configParam(TIME_PARAM, 0.1f, 4.f, 1.f, "Time");
		configParam(CRUSH_PARAM, 0.1f, 8.f, 8.f, "Bitcrush");
		configParam(FEEDBACK_PARAM, 0.f, 1.2f, 0.2f, "Feedback");
		configParam(INPUT_PARAM, 0.f, 1.f, 1.f, "Input");
		configParam(CRUSH_CV_PARAM, 0.f, 1.f, 0.f, "Bitcrush CV Control");
		configParam(FEEDBACK_CV_PARAM, 0.f, 1.f, 0.f, "Feedback CV Control");
		configParam(FINE_CV_PARAM, 0.f, 1.f, 0.f, "Fine CV Control");
		configParam(INPUT_CV_PARAM, 0.f, 1.f, 0.f, "Input CV Control");
		configParam(DRY_CV_PARAM, 0.f, 1.f, 0.f, "Dry CV Control");
		configParam(TIME_CV_PARAM, 0.f, 1.f, 0.f, "Time CV Control");
		configParam(WET_CV_PARAM, 0.f, 1.f, 0.f, "Wet CV Control");
		configInput(TIME_CV_INPUT, "Time Control Signal");
		configInput(FINE_CV_INPUT, "Fine Control Signal");
		configInput(CRUSH_CV_INPUT, "Bitcrush Control Signal");
		configInput(FEEDBACK_CV_INPUT, "Feedback Control Signal");
		configInput(INPUT_CV_INPUT, "Input Control Signal");
		configInput(DRY_CV_INPUT, "Dry Control Signal");
		configInput(WET_CV_INPUT, "Wet Control Signal");
		configInput(AUDIO_INPUT, "Input Audio Signal");
		configOutput(AUDIO_OUTPUT, "Output Audio Signal");
		configOutput(TEMPO_TRIG_OUTPUT, "Output Time Signal");

	}

//GLOBALS

	float BitcrushCV = 0;
	float FeedbackCV = 0;
	float FineCV = 0;
	float InputCV = 0;
	float DryCV = 0;
	float TimeCV = 0;
	float WetCV = 0;

	float WetKnob = 0;
	float BitcrushKnob = 0;
	float FeedbackKnob = 0;
	float FineKnob = 0;
	float InputKnob = 0;
	float DryKnob = 0;
	float TimeKnob = 0;

	float WetInput = 0;
	float BitcrushInput = 0;
	float FeedbackInput = 0;
	float FineInput = 0;
	float InputInput = 0;
	float DryInput = 0;
	float TimeInput = 0;
	float AudioInput = 0;

	float Time = 0.f;
	float feedbackAmount = 0.f;
	float inputAmount = 0.f;
	float dryAmount = 0.f;
	float wetAmount = 0.f;
	float bitcrushAmount = 8.f;
	
	bool startInterpolating = false;
	bool startupComplete = false;
	bool fadeIn = false;
	bool fadeOut = false;
	bool setupCompleted = false;

	std::vector<float> PlayBuffer = {};

	float fraction = 1;
	float Crossfader = 0;
	float DelayLineLength = 0;
	float deltaTime = 0;
	float remainingSamples = 0;
	int resetTicker = 0;
	float TickCounter = 0;
	float Downsampler = 0;
	float sample = 0;
	float SampleReductionFraction = 0;
	float CrossfadeFraction = 1;

	float CrossfadeSamples = 250; //experimentally determined value

	dsp::PulseGenerator pulseGen;


//END GLOBALS

	void process(const ProcessArgs& args) override {
		if(!setupCompleted)SetupInits(args); //setup vector
		CollectSignals(args); //collect all inputs and assemble amounts

		lights[TEMPO_LIGHT].setBrightnessSmooth(0.f,args.sampleTime*clamp(Time,0.f,1.f)); //fade lights if they are on

		if(outputs[TEMPO_TRIG_OUTPUT].isConnected())outputs[TEMPO_TRIG_OUTPUT].setVoltage(0.f); //stop trig pulse if being sent

		if(deltaTime != Time && resetTicker > 0 && !startInterpolating){ //checks if the time parameter has been modified since last sample. If it has, starts volume interpolation
			startInterpolating = true;
			fadeOut = true;
			fadeIn = false;
		}

		deltaTime = Time;	
		outputs[TEMPO_TRIG_OUTPUT].setVoltage((pulseGen.process(args.sampleTime)) ? 10.f : 0.f);
		outputs[AUDIO_OUTPUT].setVoltage(dryAmount * AudioInput + wetAmount * PlayBuffer[TickCounter]); //sends signal out

		if(startInterpolating){
			if(fadeOut){
				fraction -= fraction*(1 / remainingSamples); //set the volume reduction-
				if(1 / remainingSamples >= 1){
					fadeOut = false;
					fadeIn = true;
					fraction = 0; //ensure that final returnvalue is 0
					Crossfader = 0;
					CrossfadeFraction = 0;
				}
			}
			if(fadeIn){
				fraction = 1;
				CrossfadeFraction = Crossfader / CrossfadeSamples;
				if(Crossfader++ == CrossfadeSamples){
					fraction = 1;
					fadeIn = false;
					fadeOut = false;
					startInterpolating = false;
				}
			}
		}

		Downsampler++; //increase the downsampling ticker. 
		PlayBuffer[TickCounter] = CrossfadeFraction*fraction*Bitcrush(Downsample(inputAmount * AudioInput + (feedbackAmount * PlayBuffer[TickCounter]),args),args);

		TickCounter++; //increase the ticker

		if(TickCounter >= DelayLineLength)Reset();	//resets tickers
		if(!startupComplete)startupComplete = true;
	}

	void SetupInits(const ProcessArgs& args){
		PlayBuffer.resize(std::ceil(10 / args.sampleTime),0.f);
		setupCompleted = true;
	}
	void Reset(){
		Blink();
		TickCounter = 0;
		resetTicker++;
	}
	void Blink(){
		lights[TEMPO_LIGHT].setBrightness(1.f);
		if(outputs[TEMPO_TRIG_OUTPUT].isConnected()) pulseGen.trigger(0.001f);
	}
	float Bitcrush(float input, const ProcessArgs& args){
		//standard bitcrush algorithm
		float Quantization = simd::pow(2,BitcrushKnob);
		return (round((input)*Quantization) / Quantization);
	}
	float Downsample(float input, const ProcessArgs& args){
		SampleReductionFraction = 1+500*(1-bitcrushAmount/8.f); //sample the input stream onces every this many samples
		if(Downsampler >= SampleReductionFraction){
			sample = input;
			Downsampler = 0;
		}
		return sample;
	}
	void CollectSignals(const ProcessArgs& args){
		//collect and set "amount" parameters
		BitcrushKnob = params[CRUSH_PARAM].getValue();
		FeedbackKnob = params[FEEDBACK_PARAM].getValue();
		FineKnob = params[FINE_PARAM].getValue();
		TimeKnob = params[TIME_PARAM].getValue();
		InputKnob = params[INPUT_PARAM].getValue();
		DryKnob = params[DRY_PARAM].getValue();
		WetKnob = params[WET_PARAM].getValue();

		if(inputs[CRUSH_CV_INPUT].isConnected())BitcrushCV = params[CRUSH_CV_PARAM].getValue();
		if(inputs[FEEDBACK_CV_INPUT].isConnected())FeedbackCV = params[FEEDBACK_CV_PARAM].getValue();
		if(inputs[FINE_CV_INPUT].isConnected())FineCV = params[FINE_CV_PARAM].getValue();
		if(inputs[TIME_CV_INPUT].isConnected())TimeCV = params[TIME_CV_PARAM].getValue();
		if(inputs[INPUT_CV_INPUT].isConnected())InputCV = params[INPUT_CV_PARAM].getValue();
		if(inputs[DRY_CV_INPUT].isConnected())DryCV = params[DRY_CV_PARAM].getValue();
		if(inputs[WET_CV_INPUT].isConnected())WetCV = params[WET_CV_PARAM].getValue();

		BitcrushInput = (inputs[CRUSH_CV_INPUT].isConnected()) ? inputs[CRUSH_CV_INPUT].getVoltage() * 0.8f : 0;
		FeedbackInput = (inputs[FEEDBACK_CV_INPUT].isConnected()) ? inputs[FEEDBACK_CV_INPUT].getVoltage()*0.12f: 0;
		FineInput = (inputs[FINE_CV_INPUT].isConnected()) ? inputs[FINE_CV_INPUT].getVoltage()*0.0198f : 0;
		TimeInput = (inputs[TIME_CV_INPUT].isConnected()) ? inputs[TIME_CV_INPUT].getVoltage()*0.98f : 0;
		InputInput = (inputs[INPUT_CV_INPUT].isConnected()) ? inputs[INPUT_CV_INPUT].getVoltage()*0.1f : 0;
		DryInput = (inputs[DRY_CV_INPUT].isConnected()) ? inputs[DRY_CV_INPUT].getVoltage()*0.1f : 0;
		WetInput = (inputs[WET_CV_INPUT].isConnected()) ? inputs[WET_CV_INPUT].getVoltage()*0.1f : 0;

		AudioInput = (inputs[AUDIO_INPUT].isConnected()) ? inputs[AUDIO_INPUT].getVoltageSum() : 0;

		Time = TimeKnob + (TimeCV*TimeInput) + FineKnob + (FineCV*FineInput);
		feedbackAmount = clamp(simd::pow(FeedbackKnob + (FeedbackCV*FeedbackInput),2),0.f,1.2f); //uses squared value to improve "feel" of turning knob
		inputAmount = InputKnob + (InputCV*InputInput);
		dryAmount = clamp((DryKnob + (DryCV * DryInput)),0.f,1.f);
		wetAmount = clamp((WetKnob + (WetCV * WetCV)),0.f,1.f);
		bitcrushAmount = clamp(BitcrushKnob-(BitcrushCV*BitcrushInput),0.f,8.f);
		DelayLineLength = simd::round(Time / args.sampleTime);
		remainingSamples = DelayLineLength - TickCounter;
	}
};

struct Repeat3hrWidget : ModuleWidget {
	Repeat3hrWidget(Repeat3hr* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Repeat3hr.svg")));

		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(10.16, 18.803)), module, Repeat3hr::TIME_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.422, 18.803)), module, Repeat3hr::FINE_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(60.72, 17.603)), module, Repeat3hr::FINE_CV_PARAM));
		addParam(createParamCentered<Rogan1PSWhite>(mm2px(Vec(35.422, 33.603)), module, Repeat3hr::TIME_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.16, 51.403)), module, Repeat3hr::CRUSH_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(20.32, 51.403)), module, Repeat3hr::CRUSH_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(50.64, 51.403)), module, Repeat3hr::FEEDBACK_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.72, 51.403)), module, Repeat3hr::FEEDBACK_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.16, 86.403)), module, Repeat3hr::INPUT_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(20.32, 86.403)), module, Repeat3hr::INPUT_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(30.48, 116.403)), module, Repeat3hr::DRY_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.32, 116.557)), module, Repeat3hr::DRY_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(40.56, 116.403)), module, Repeat3hr::WET_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(50.64, 116.403)), module, Repeat3hr::WET_PARAM));

		addInput(createInputCentered<TinyJack>(mm2px(Vec(10.16, 26.403)), module, Repeat3hr::TIME_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(60.72, 26.403)), module, Repeat3hr::FINE_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(20.32, 60.403)), module, Repeat3hr::CRUSH_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(50.64, 60.403)), module, Repeat3hr::FEEDBACK_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(20.32, 95.403)), module, Repeat3hr::INPUT_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(30.496, 107.403)), module, Repeat3hr::DRY_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(40.56, 107.403)), module, Repeat3hr::WET_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 116.403)), module, Repeat3hr::AUDIO_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60.72, 116.403)), module, Repeat3hr::AUDIO_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.422, 60.403)), module, Repeat3hr::TEMPO_TRIG_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.422, 50.403)), module, Repeat3hr::TEMPO_LIGHT));
	}
};


Model* modelRepeat3hr = createModel<Repeat3hr, Repeat3hrWidget>("Repeat3hr");