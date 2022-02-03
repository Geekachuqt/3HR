#include "plugin.hpp"
using simd::float_4;

template <typename T> struct EnvelopeGenerator{
	float_4 OutputA = 0;
	float_4 OutputB = 0;
	float_4 TimeA = 0;
	float_4 TimeB = 0;
	float_4 Triggered;

	float_4 AttacksA = 0;
	float_4 DecaysA = 0;
	float_4 SustainsA = 0;
	float_4 ReleasingA = 0;

	float_4 AttacksB = 0;
	float_4 DecaysB = 0;
	float_4 SustainsB = 0;
	float_4 ReleasingB = 0;

	float_4 ResetValuesA = 0;
	float_4 ResetValuesB = 0;

	float_4 ReadyToResetA = 0;
	float_4 ResetA = 0;
	float_4 ReadyToResetB = 0;
	float_4 ResetB = 0;

	float_4 AttackALambdas = 0;
	float_4 DecayALambdas = 0;
	float_4 SustainALambdas = 0;
	float_4 ReleaseALambdas = 0;

	float_4 AttackBLambdas = 0;
	float_4 DecayBLambdas = 0;
	float_4 SustainBLambdas = 0;
	float_4 ReleaseBLambdas = 0;

	float_4 Curvatures = 0;

	float_4 StatesA[4];
	float_4 StatesB[4];

	float_4 LambdasA[4];
	float_4 LambdasB[4];

	float_4 Aoutput;

	void Advance(float_4 Triggered, float sampleTime, float_4 AttackAAmount, float_4 AttackBAmount, float_4 AttackCurvature, float_4 DecayAAmount, float_4 DecayBAmount, float DecayCurvature, float_4 SustainAAmount, float_4 SustainBAmount, float_4 ReleaseAAmount, float_4 ReleaseBAmount, float ReleaseCurvature, float LoopSwitchA, float LoopSwitchB){

		if(LoopSwitchA == 0){
			CheckResetA(Triggered); //Are we retriggering?

			//Determine state of envelope

			for(int c = 0; c <= 3; c++){
				AttacksA[c] = ((TimeA[c] <= AttackAAmount[c]) && (Triggered[c] == 10)) ? 1 : 0;
				DecaysA[c] = ((TimeA[c] > AttackAAmount[c]) && (TimeA[c] <= DecayAAmount[c]+AttackAAmount[c]) && Triggered[c] == 10) ? 1 : 0;
				SustainsA[c] = ((TimeA[c] > DecayAAmount[c]+AttackAAmount[c]) && (Triggered[c] == 10)) ? 1 : 0;
				ReleasingA[c] = (Triggered[c] != 10) ? 1 : 0;
			}
		}
		else{
			CheckResetA(Triggered);
			for(int c = 0; c <= 3; c++){
				AttacksA[c] = ((TimeA[c] <= AttackAAmount[c]) && (Triggered[c] == 10)) ? 1 : 0;
				DecaysA[c] = ((TimeA[c] > AttackAAmount[c]) && (TimeA[c] <= DecayAAmount[c]+AttackAAmount[c]) && Triggered[c] == 10) ? 1 : 0;
				SustainsA[c] = 0;
				ReleasingA[c] = (TimeA[c] > AttackAAmount[c]+DecayAAmount[c] || Triggered[c] != 10) ? 1 : 0;
			}
		}

		if(LoopSwitchB == 0){
			CheckResetB(Triggered); //Are we retriggering?

			//Determine state of envelope

			for(int c = 0; c <= 3; c++){
				AttacksB[c] = ((TimeB[c] <= AttackBAmount[c]) && (Triggered[c] == 10)) ? 1 : 0;
				DecaysB[c] = ((TimeB[c] > AttackBAmount[c]) && (TimeB[c] <= DecayBAmount[c]+AttackBAmount[c]) && Triggered[c] == 10) ? 1 : 0;
				SustainsB[c] = ((TimeB[c] > DecayBAmount[c]+AttackBAmount[c]) && (Triggered[c] == 10)) ? 1 : 0;
				ReleasingB[c] = (Triggered[c] != 10) ? 1 : 0;
			}
		}
		else{
			CheckResetB(Triggered);
			for(int c = 0; c <= 3; c++){
				AttacksB[c] = ((TimeB[c] <= AttackBAmount[c]) && (Triggered[c] == 10)) ? 1 : 0;
				DecaysB[c] = ((TimeB[c] > AttackBAmount[c]) && (TimeB[c] <= DecayBAmount[c]+AttackBAmount[c]) && Triggered[c] == 10) ? 1 : 0;
				SustainsB[c] = 0;
				ReleasingB[c] = (TimeB[c] > AttackBAmount[c]+DecayBAmount[c] || Triggered[c] != 10) ? 1 : 0;
			}
		}


		//Encode current state of each voice

		float_4 State1A = {AttacksA[0],DecaysA[0],SustainsA[0],ReleasingA[0]};
		float_4 State2A = {AttacksA[1],DecaysA[1],SustainsA[1],ReleasingA[1]};
		float_4 State3A = {AttacksA[2],DecaysA[2],SustainsA[2],ReleasingA[2]};
		float_4 State4A = {AttacksA[3],DecaysA[3],SustainsA[3],ReleasingA[3]};

		float_4 State1B = {AttacksB[0],DecaysB[0],SustainsB[0],ReleasingB[0]};
		float_4 State2B = {AttacksB[1],DecaysB[1],SustainsB[1],ReleasingB[1]};
		float_4 State3B = {AttacksB[2],DecaysB[2],SustainsB[2],ReleasingB[2]};
		float_4 State4B = {AttacksB[3],DecaysB[3],SustainsB[3],ReleasingB[3]};

		StatesA[0] = State1A;
		StatesA[1] = State2A;
		StatesA[2] = State3A;
		StatesA[3] = State4A;

		StatesB[0] = State1B;
		StatesB[1] = State2B;
		StatesB[2] = State3B;
		StatesB[3] = State4B;

		//Create a matrix describing the state of all four voices

		//Determine how much a voice should change per sample
		
		AttackALambdas = CalculateAttackLambda(sampleTime, AttackAAmount, ResetValuesA);
		DecayALambdas = CalculateDecayLambda(sampleTime, DecayAAmount, SustainAAmount);
		SustainALambdas = 0;
		ReleaseALambdas = CalculateReleaseLambda(sampleTime, ReleaseAAmount, OutputA);

		AttackBLambdas = CalculateAttackLambda(sampleTime, AttackBAmount, ResetValuesB);
		DecayBLambdas = CalculateDecayLambda(sampleTime, DecayBAmount, SustainBAmount);
		SustainBLambdas = 0;
		ReleaseBLambdas = CalculateReleaseLambda(sampleTime, ReleaseBAmount, OutputB);

		//Encode lambdas for each state

		float_4 Lambdas1A = {AttackALambdas[0],DecayALambdas[0],SustainALambdas[0],ReleaseALambdas[0]};
		float_4 Lambdas2A = {AttackALambdas[1],DecayALambdas[1],SustainALambdas[1],ReleaseALambdas[1]};
		float_4 Lambdas3A = {AttackALambdas[2],DecayALambdas[2],SustainALambdas[2],ReleaseALambdas[2]};
		float_4 Lambdas4A = {AttackALambdas[3],DecayALambdas[3],SustainALambdas[3],ReleaseALambdas[3]};

		float_4 Lambdas1B = {AttackBLambdas[0],DecayBLambdas[0],SustainBLambdas[0],ReleaseBLambdas[0]};
		float_4 Lambdas2B = {AttackBLambdas[1],DecayBLambdas[1],SustainBLambdas[1],ReleaseBLambdas[1]};
		float_4 Lambdas3B = {AttackBLambdas[2],DecayBLambdas[2],SustainBLambdas[2],ReleaseBLambdas[2]};
		float_4 Lambdas4B = {AttackBLambdas[3],DecayBLambdas[3],SustainBLambdas[3],ReleaseBLambdas[3]};

		//Create matrix describing lambdas for each stage

		LambdasA[0] = Lambdas1A;
		LambdasA[1] = Lambdas2A;
		LambdasA[2] = Lambdas3A;
		LambdasA[3] = Lambdas4A;

		LambdasB[0] = Lambdas1B;
		LambdasB[1] = Lambdas2B;
		LambdasB[2] = Lambdas3B;
		LambdasB[3] = Lambdas4B;

		//Calculate the inner product of the states and lambdas and add it to the output.
		//For each state vector, only one element will be 1. Thus, only the lambda of the active state will be added.

		OutputA += GetInnerProductA();
		OutputA = simd::clamp(OutputA,0.f,1.f);

		OutputB += GetInnerProductB();
		OutputB = simd::clamp(OutputB,0.f,1.f);

		//Ensure that sustain value follows changes to sustain while envelope is active

		for(int c = 0;c <= 3; c++ ){
			OutputA[c] = (StatesA[c][2] == 1) ? SustainAAmount[c] : OutputA[c];
			OutputB[c] = (StatesB[c][2] == 1) ? SustainBAmount[c] : OutputB[c];
		}
		
		//As I am using the total attack+decay+release time to determine states, stop accumulation of time for sustained envelopes.
		//Also prevents inactive voices from accumulating time and sets them to 0.
		//Ignore this behavior if looping is activated

		if(LoopSwitchA == 0){
			for(int c = 0; c <= 3; c++){
				TimeA[c] += (TimeA[c] >= DecayAAmount[c]+AttackAAmount[c]) ? (1-SustainsA[c])*sampleTime : sampleTime;
				TimeA[c] = ((StatesA[c][0]+StatesA[c][1]+StatesA[c][2]+StatesA[c][3]) == 0) ? 0 : TimeA[c];
			}
		}
		else{
			for(int c = 0;c <= 3; c++){
				TimeA[c] += (TimeA[c] >= AttackAAmount[c]+DecayAAmount[c]+ReleaseAAmount[c] && Triggered[c] != 10) ? 0 : sampleTime;
				TimeA[c] = (TimeA[c] >= AttackAAmount[c]+DecayAAmount[c]+ReleaseAAmount[c]) ? 0 : TimeA[c];
			}
		}
		if(LoopSwitchB == 0){
			for(int c = 0; c <= 3; c++){
				TimeB[c] += (TimeB[c] >= DecayBAmount[c]+AttackBAmount[c]) ? (1-SustainsB[c])*sampleTime : sampleTime;
				TimeB[c] = ((StatesB[c][0]+StatesB[c][1]+StatesB[c][2]+StatesB[c][3]) == 0) ? 0 : TimeB[c];
			}
		}
		else{
			for(int c = 0; c <= 3; c++){
				TimeB[c] += (TimeB[c] >= AttackBAmount[c]+DecayBAmount[c]+ReleaseBAmount[c] && Triggered[c] != 10) ? 0 : sampleTime;
				TimeB[c] = (TimeB[c] >= AttackBAmount[c]+DecayBAmount[c]+ReleaseBAmount[c]) ? 0 : TimeB[c];
			}
		}		
	}
	float_4 CalculateAttackLambda(float sampleTime, float_4 AttackAmount,float_4 StartValue){
		return (sampleTime/AttackAmount)*(1-StartValue);
	}
	float_4 CalculateDecayLambda(float sampleTime, float_4 DecayAmount, float_4 SustainAmount){
		return -(sampleTime/DecayAmount)*(1-SustainAmount);
	}
	float_4 CalculateReleaseLambda(float sampleTime, float_4 ReleaseAmount, float_4 CurrentValue){
		return -(sampleTime/ReleaseAmount)*(CurrentValue);
	}
	float_4 GetAOutput(float_4 Curvature){
		return simd::pow(OutputA,Curvature)*10.f;
	}
	float_4 GetInvAOutput(float_4 Curvature){
		return simd::abs(simd::pow(OutputA,Curvature)*10.f-10.f);
	}
	float_4 GetABMultOutput(float_4 Curvature){
		return simd::pow(OutputB*OutputA,Curvature)*10.f;
	}
	float_4 GetNormABAddOutput(float_4 Curvature){
		return simd::pow((OutputB+OutputA)/2,Curvature)*10.f;
	}
	float_4 GetBOutput(float_4 Curvature){
		return simd::pow(OutputB,Curvature)*10.f;
	}
	float_4 GetInvBOutput(float_4 Curvature){
		return simd::abs(simd::pow(OutputB,Curvature)*10.f-10.f);
	}
	float_4 GetInvABMultOutput(float_4 Curvature){
		return simd::abs(simd::pow(OutputB*OutputA,Curvature)*10.f-10.f);
	}
	float_4 GetAPowBOutput(float_4 Curvature){
		return simd::abs(clamp(simd::pow(simd::pow(OutputA,OutputB),Curvature)*10.f,0.f,10.f)-10.f);
	}
	float_4 GetInnerProductA(){
		float_4 returnval = 0;
		float_4 temp[4];
		for(int c = 0; c <= 3; c++){
			temp[c] = StatesA[c] * LambdasA[c];
		}
		for(int c = 0; c <= 3; c++){
			returnval[c] = temp[c][0]+temp[c][1]+temp[c][2]+temp[c][3];
		}
		return returnval;
	}
	float_4 GetInnerProductB(){
		float_4 returnval = 0;
		float_4 temp[4];
		for(int c = 0; c <= 3; c++){
			temp[c] = StatesB[c] * LambdasB[c];
		}
		for(int c = 0; c <= 3; c++){
			returnval[c] = temp[c][0]+temp[c][1]+temp[c][2]+temp[c][3];
		}
		return returnval;
	}
	void CheckResetA(float_4 Triggers){
		for(int c = 0; c <= 3; c++){
			TimeA[c] = (Triggers[c] == 10.f && ReadyToResetA[c] == 1) ? 0 : TimeA[c];
			ResetValuesA[c] = (Triggers[c] == 10.f && ReadyToResetA[c] == 1) ? OutputA[c] : ResetValuesA[c];
			ReadyToResetA[c] = (Triggers[c] == 10.f) ? 0 : ReadyToResetA[c];
		}
		for(int c = 0; c <= 3; c++){
			ReadyToResetA[c] = (Triggers[c] == 0 && ReadyToResetA[c] == 0) ? 1 : ReadyToResetA[c];
		}
	}
	void CheckResetB(float_4 Triggers){
		for(int c = 0; c <= 3; c++){
			TimeB[c] = (Triggers[c] == 10.f && ReadyToResetB[c] == 1) ? 0 : TimeB[c];
			ResetValuesB[c] = (Triggers[c] == 10.f && ReadyToResetB[c] == 1) ? OutputB[c] : ResetValuesB[c];
			ReadyToResetB[c] = (Triggers[c] == 10.f) ? 0 : ReadyToResetB[c];
		}
		for(int c = 0; c <= 3; c++){
			ReadyToResetB[c] = (Triggers[c] == 0 && ReadyToResetB[c] == 0) ? 1 : ReadyToResetB[c];
		}
	}
	float_4 GetState(){
		return StatesA[0];
	}
	float_4 GetTime(){
		return TimeA[0];
	}
};

struct ENVELOOP3HR : Module {
	enum ParamId {
		ATTACK_A_CV_PARAM,
		ATTACK_B_CV_PARAM,
		ATTACK_A_PARAM,
		ATTACK_B_PARAM,
		EXPO_PARAM,
		DECAY_A_CV_PARAM,
		DECAY_B_CV_PARAM,
		DECAY_A_PARAM,
		DECAY_B_PARAM,
		SUSTAIN_A_CV_PARAM,
		SUSTAIN_B_CV_PARAM,
		SUSTAIN_A_PARAM,
		SUSTAIN_B_PARAM,
		RELEASE_A_CV_PARAM,
		RELEASE_B_CV_PARAM,
		RELEASE_A_PARAM,
		RELEASE_B_PARAM,
		LOOP_A_SWITCH_PARAM,
		LOOP_B_SWITCH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ATTACK_A_CV_INPUT,
		ATTACK_B_CV_INPUT,
		DECAY_A_CV_INPUT,
		DECAY_B_CV_INPUT,
		SUSTAIN_A_CV_INPUT,
		SUSTAIN_B_CV_INPUT,
		RELEASE_A_CV_INPUT,
		RELEASE_B_CV_INPUT,
		GATE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENV_A_OUTPUT,
		INV_ENV_A_OUTPUT,
		A_B_MULT_OUTPUT,
		NORM_A_B_ADD_OUTPUT,
		ENV_B_OUTPUT,
		INV_ENV_B_OUTPUT,
		INV_A_B_MULT_OUTPUT,
		A_POW_B_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GATE_LIGHT,
		LOOP_A_LIGHT,
		LOOP_B_LIGHT,
		LIGHTS_LEN
	};

	ENVELOOP3HR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ATTACK_A_CV_PARAM, 0.f, 1.f, 0.f, "Attack A CV Amount");
		configParam(ATTACK_B_CV_PARAM, 0.f, 1.f, 0.f, "Attack B CV Amount");
		configParam(ATTACK_A_PARAM, 0.00001f, 3.f, 0.5f, "Attack A Amount");
		configParam(ATTACK_B_PARAM, 0.00001f, 3.f, 0.5f, "Attack B Amount");
		configParam(EXPO_PARAM, 0.5f, 5.f, 1.f, "Curvature");
		configParam(DECAY_A_CV_PARAM, 0.f, 1.f, 0.f, "Decay A CV Amount");
		configParam(DECAY_B_CV_PARAM, 0.f, 1.f, 0.f, "Decay B CV Amount");
		configParam(DECAY_A_PARAM, 0.01f, 3.f, 0.5f, "Decay A Amount");
		configParam(DECAY_B_PARAM, 0.01f, 3.f, 0.5f, "Decay B Amount");
		configParam(SUSTAIN_A_CV_PARAM, 0.f, 1.f, 0.f, "Sustain A CV Amount");
		configParam(SUSTAIN_B_CV_PARAM, 0.f, 1.f, 0.f, "Sustain B CV Amount");
		configParam(SUSTAIN_A_PARAM, 0.01f, 1.f, 0.5f, "Sustain A Amount");
		configParam(SUSTAIN_B_PARAM, 0.01f, 1.f, 0.5f, "Sustain B Amount");
		configParam(RELEASE_A_CV_PARAM, 0.f, 1.f, 0.f, "Release A CV Amount");
		configParam(RELEASE_B_CV_PARAM, 0.f, 1.f, 0.f, "Release B CV Amount");
		configParam(RELEASE_A_PARAM, 0.01f, 5.f, 0.5f, "Release A Amount");
		configParam(RELEASE_B_PARAM, 0.01f, 5.f, 0.5f, "Release B Amount");
		configSwitch(LOOP_A_SWITCH_PARAM, 0.f, 1.f, 0.f, "Loop Switch A", {"Hold", "Loop"});
		configSwitch(LOOP_B_SWITCH_PARAM, 0.f, 1.f, 0.f, "Loop Switch B", {"Hold", "Loop"});
		configInput(ATTACK_A_CV_INPUT, "Attack A CV");
		configInput(ATTACK_B_CV_INPUT, "Attack B CV");
		configInput(DECAY_A_CV_INPUT, "Decay A CV");
		configInput(DECAY_B_CV_INPUT, "Decay B CV");
		configInput(SUSTAIN_A_CV_INPUT, "Sustain A CV");
		configInput(SUSTAIN_B_CV_INPUT, "Sustain B CV");
		configInput(RELEASE_A_CV_INPUT, "Release A CV");
		configInput(RELEASE_B_CV_INPUT, "Release B CV");
		configInput(GATE_INPUT, "Gate");
		configOutput(ENV_A_OUTPUT, "Envelope A");
		configOutput(INV_ENV_A_OUTPUT, "Inverted Envelope A");
		configOutput(A_B_MULT_OUTPUT, "A * B");
		configOutput(NORM_A_B_ADD_OUTPUT, "Normalised A + B");
		configOutput(ENV_B_OUTPUT, "Envelope B");
		configOutput(INV_ENV_B_OUTPUT, "Inverted Envelope B");
		configOutput(INV_A_B_MULT_OUTPUT, "Inverted A * B");
		configOutput(A_POW_B_OUTPUT, "A ^ B");
	}

	float_4 AttackAAmount = 0;
	float_4 AttackBAmount = 0;
	float_4 AttackCurvature = 0;
	float_4 DecayAAmount = 0;
	float_4 DecayBAmount = 0;
	float DecayCurvature = 0;
	float_4 SustainAAmount = 0;
	float_4 SustainBAmount = 0;
	float_4 ReleaseAAmount = 0;
	float_4 ReleaseBAmount = 0;
	float ReleaseCurvature = 0;

	float LoopSwitchA = 0;
	float LoopSwitchB = 0;

	int Channels = 0;

	dsp::PulseGenerator pGenerator;
	dsp::TSchmittTrigger<float_4> sTrigger[4];
	float_4 Triggered;

	EnvelopeGenerator<float_4> EnvelopeGeneratorCore[4];

	void process(const ProcessArgs& args) override {
		SetChannels();
		for(int c = 0; c < Channels; c+=4){
			CollectSignals(args, c);
			
			Triggered = inputs[GATE_INPUT].getPolyVoltageSimd<float_4>(c);

			EnvelopeGeneratorCore[c/4].Advance(Triggered, args.sampleTime, AttackAAmount, AttackBAmount, AttackCurvature, DecayAAmount, DecayBAmount, DecayCurvature, SustainAAmount, SustainBAmount, ReleaseAAmount, ReleaseBAmount, ReleaseCurvature, LoopSwitchA, LoopSwitchB);

			if(outputs[ENV_A_OUTPUT].isConnected())outputs[ENV_A_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetAOutput(AttackCurvature),c);
			if(outputs[INV_ENV_A_OUTPUT].isConnected())outputs[INV_ENV_A_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetInvAOutput(AttackCurvature),c);
			if(outputs[A_B_MULT_OUTPUT].isConnected())outputs[A_B_MULT_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetABMultOutput(AttackCurvature),c);
			if(outputs[NORM_A_B_ADD_OUTPUT].isConnected())outputs[NORM_A_B_ADD_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetNormABAddOutput(AttackCurvature),c);
			if(outputs[ENV_B_OUTPUT].isConnected())outputs[ENV_B_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetBOutput(AttackCurvature),c);
			if(outputs[INV_ENV_B_OUTPUT].isConnected())outputs[INV_ENV_B_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetInvBOutput(AttackCurvature),c);	
			if(outputs[INV_A_B_MULT_OUTPUT].isConnected())outputs[INV_A_B_MULT_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetInvABMultOutput(AttackCurvature),c);
			if(outputs[A_POW_B_OUTPUT].isConnected())outputs[A_POW_B_OUTPUT].setVoltageSimd(EnvelopeGeneratorCore[c/4].GetAPowBOutput(AttackCurvature),c);
	
		}
	}
	void SetChannels(){
		Channels = std::max(1, inputs[GATE_INPUT].getChannels());

		outputs[ENV_A_OUTPUT].setChannels(Channels);
		outputs[INV_ENV_A_OUTPUT].setChannels(Channels);
		outputs[A_B_MULT_OUTPUT].setChannels(Channels);
		outputs[NORM_A_B_ADD_OUTPUT].setChannels(Channels);
		outputs[ENV_B_OUTPUT].setChannels(Channels);
		outputs[INV_ENV_B_OUTPUT].setChannels(Channels);
		outputs[INV_A_B_MULT_OUTPUT].setChannels(Channels);
		outputs[A_POW_B_OUTPUT].setChannels(Channels);
	}
	void CollectSignals(const ProcessArgs& args, int firstChannel){
		float AttackAKnob = params[ATTACK_A_PARAM].getValue();
		float AttackACV = inputs[ATTACK_A_CV_INPUT].isConnected() ? params[ATTACK_A_CV_PARAM].getValue() : 0;
		float AttackBKnob = params[ATTACK_B_PARAM].getValue();
		float AttackBCV = inputs[ATTACK_B_CV_INPUT].isConnected() ? params[ATTACK_B_CV_PARAM].getValue() : 0;
		float DecayAKnob = params[DECAY_A_PARAM].getValue();
		float DecayACV = inputs[DECAY_A_CV_INPUT].isConnected() ? params[DECAY_A_CV_PARAM].getValue() : 0;
		float DecayBKnob = params[DECAY_B_PARAM].getValue();
		float DecayBCV = inputs[DECAY_B_CV_INPUT].isConnected() ? params[DECAY_B_CV_PARAM].getValue() : 0;
		float SustainAKnob = params[SUSTAIN_A_PARAM].getValue();
		float SustainACV = inputs[SUSTAIN_A_CV_INPUT].isConnected() ? params[SUSTAIN_A_CV_PARAM].getValue() : 0;
		float SustainBKnob = params[SUSTAIN_B_PARAM].getValue();
		float SustainBCV = inputs[SUSTAIN_B_CV_INPUT].isConnected() ? params[SUSTAIN_B_CV_PARAM].getValue() : 0;
		float ReleaseAKnob = params[RELEASE_A_PARAM].getValue();
		float ReleaseACV = inputs[RELEASE_A_CV_INPUT].isConnected() ? params[RELEASE_A_CV_PARAM].getValue() : 0;
		float ReleaseBKnob = params[RELEASE_B_PARAM].getValue();
		float ReleaseBCV = inputs[RELEASE_B_CV_INPUT].isConnected() ? params[RELEASE_B_CV_PARAM].getValue() : 0;

		AttackCurvature = params[EXPO_PARAM].getValue();
		LoopSwitchA = params[LOOP_A_SWITCH_PARAM].getValue();
		LoopSwitchB = params[LOOP_B_SWITCH_PARAM].getValue();

		AttackAAmount = clamp(AttackAKnob + AttackACV * inputs[ATTACK_A_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);
		AttackBAmount = clamp(AttackBKnob + AttackBCV * inputs[ATTACK_B_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);	
		DecayAAmount = clamp(DecayAKnob + DecayACV * inputs[DECAY_A_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);
		DecayBAmount = clamp(DecayBKnob + DecayBCV * inputs[DECAY_B_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);
		SustainAAmount = clamp(SustainAKnob + SustainACV * inputs[SUSTAIN_A_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);	
		SustainBAmount = clamp(SustainBKnob + SustainBCV * inputs[SUSTAIN_B_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);
		ReleaseAAmount = clamp(ReleaseAKnob + ReleaseACV * inputs[RELEASE_A_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);
		ReleaseBAmount = clamp(ReleaseBKnob + ReleaseBCV * inputs[RELEASE_B_CV_INPUT].getPolyVoltageSimd<float_4>(firstChannel)*0.1F,0.f,1.f);

		lights[LOOP_A_LIGHT].setBrightness(params[LOOP_A_SWITCH_PARAM].getValue());
		lights[LOOP_B_LIGHT].setBrightness(params[LOOP_B_SWITCH_PARAM].getValue());


	}

};


struct ENVELOOP3HRWidget : ModuleWidget {
	ENVELOOP3HRWidget(ENVELOOP3HR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ENVELOOP3HR.svg")));

		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(3.464, 19.576)), module, ENVELOOP3HR::ATTACK_A_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(37.154, 19.576)), module, ENVELOOP3HR::ATTACK_B_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.067, 22.324)), module, ENVELOOP3HR::ATTACK_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.519, 22.324)), module, ENVELOOP3HR::ATTACK_B_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(20.419, 95.525)), module, ENVELOOP3HR::EXPO_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(3.465, 37.465)), module, ENVELOOP3HR::DECAY_A_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(37.154, 37.465)), module, ENVELOOP3HR::DECAY_B_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.067, 40.214)), module, ENVELOOP3HR::DECAY_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.52, 40.214)), module, ENVELOOP3HR::DECAY_B_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(3.465, 55.349)), module, ENVELOOP3HR::SUSTAIN_A_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(37.154, 55.349)), module, ENVELOOP3HR::SUSTAIN_B_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.067, 58.097)), module, ENVELOOP3HR::SUSTAIN_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.52, 58.097)), module, ENVELOOP3HR::SUSTAIN_B_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(3.465, 73.221)), module, ENVELOOP3HR::RELEASE_A_CV_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(37.154, 73.221)), module, ENVELOOP3HR::RELEASE_B_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.067, 75.969)), module, ENVELOOP3HR::RELEASE_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.52, 75.969)), module, ENVELOOP3HR::RELEASE_B_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(mm2px(Vec(7.256, 93.539)), module, ENVELOOP3HR::LOOP_A_SWITCH_PARAM, ENVELOOP3HR::LOOP_A_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(mm2px(Vec(33.592, 93.539)), module, ENVELOOP3HR::LOOP_B_SWITCH_PARAM, ENVELOOP3HR::LOOP_B_LIGHT));

		addInput(createInputCentered<TinyJack>(mm2px(Vec(3.437, 25.137)), module, ENVELOOP3HR::ATTACK_A_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(37.137, 25.137)), module, ENVELOOP3HR::ATTACK_B_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(3.437, 42.978)), module, ENVELOOP3HR::DECAY_A_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(37.137, 42.978)), module, ENVELOOP3HR::DECAY_B_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(3.437, 60.874)), module, ENVELOOP3HR::SUSTAIN_A_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(37.137, 60.874)), module, ENVELOOP3HR::SUSTAIN_B_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(3.537, 78.743)), module, ENVELOOP3HR::RELEASE_A_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(37.237, 78.743)), module, ENVELOOP3HR::RELEASE_B_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(20.424, 87.949)), module, ENVELOOP3HR::GATE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.256, 105.334)), module, ENVELOOP3HR::ENV_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(16.035, 105.334)), module, ENVELOOP3HR::INV_ENV_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.814, 105.334)), module, ENVELOOP3HR::A_B_MULT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.592, 105.334)), module, ENVELOOP3HR::NORM_A_B_ADD_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.256, 117.13)), module, ENVELOOP3HR::ENV_B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(16.035, 117.13)), module, ENVELOOP3HR::INV_ENV_B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.814, 117.13)), module, ENVELOOP3HR::INV_A_B_MULT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.592, 117.13)), module, ENVELOOP3HR::A_POW_B_OUTPUT));

	}
};


Model* modelENVELOOP3HR = createModel<ENVELOOP3HR, ENVELOOP3HRWidget>("ENVELOOP3HR");