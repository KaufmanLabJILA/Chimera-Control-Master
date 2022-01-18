#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include <map>
#include <functional>
#include <boost/variant.hpp>

enum class MessageDestination {
	KA007
};

enum class MessageDAC {
	DAC0,
	DAC1,
	DAC2,
	DAC3
};

enum class MessageSetting {
	LOADFREQUENCY,
	MOVEFREQUENCY,
	TERMINATE_SEQ
};

typedef std::map<std::string, boost::variant<MessageDestination, MessageDAC, MessageSetting, int, double>> tKA007MessageParameters;

class KA007ParameterContainer
{
public:
	KA007ParameterContainer() {};
	~KA007ParameterContainer() {};
	
	template<typename T> void set(const std::string& key, const T& value)
	{
		if (exists(key))
			thrower("Duplicate key");

		mParameters[key] = value;
	}
	
	//TODO: template previously not working when compiling in release mode. Solved with overloads below, but then miraculously compiles. 

	//void set(const std::string& key, const int& value)
	//{
	//	if (exists(key))
	//		thrower("Duplicate key");

	//	mParameters[key] = value;
	//}
	//void set(const std::string& key, const MessageDestination& value)
	//{
	//	if (exists(key))
	//		thrower("Duplicate key");

	//	mParameters[key] = value;
	//}
	//void set(const std::string& key, const MessageDAC& value)
	//{
	//	if (exists(key))
	//		thrower("Duplicate key");

	//	mParameters[key] = value;
	//}
	//void set(const std::string& key, const MessageSetting& value)
	//{
	//	if (exists(key))
	//		thrower("Duplicate key");

	//	mParameters[key] = value;
	//}
	//void set(const std::string& key, const double& value)
	//{
	//	if (exists(key))
	//		thrower("Duplicate key");

	//	mParameters[key] = value;
	//}


	template<typename T> const T& get(const std::string& key)const
	{
		if (!exists(key))
			thrower("Can't find key");

		return boost::get<T>(mParameters.find(key)->second);
	}

	bool exists(const std::string& key)const
	{
		if (mParameters.find(key) == mParameters.end())
			return false;

		return true;
	}

private:
	tKA007MessageParameters mParameters;
};

class Message
{
public:
	friend class MessageBuilder;  // our builder populates private fields

	static MessageBuilder make();

	KA007ParameterContainer getParameters() const {
		return parameters_;
	}

private:
	Message() = default; // no public constructor

	KA007ParameterContainer parameters_;
};

class KA007_Message_Base
{
	virtual std::vector<int> getBytes(KA007ParameterContainer) const = 0;

public:
	static unsigned long long int getFTW(double frequency) {
		//36-bit DDS
		//1 MHz = 223696213.33333333333333333333333 FTW
		double MHz = 223696213.33333333333333333333333;
		return (unsigned long long int)(frequency * MHz);
	}
};

struct SetLoadFrequency : KA007_Message_Base
{
	std::vector<int> getBytes(KA007ParameterContainer p) const override
	{
		std::vector<int> bytes;
		//message type 1 (upper nibble) to DDS (lower nibble = 2)
		bytes.push_back(0x12);
		//bytes.push_back(0x10); //loopback for testing
		//length is 2
		bytes.push_back(2);
		//channel is selected in header data
		bytes.push_back(0);
		int mult;
		switch (p.get<MessageDAC>("DAC")) {
			case MessageDAC::DAC0: mult = 0;
								break;
			case MessageDAC::DAC1: mult = 1;
								break;
			case MessageDAC::DAC2: mult = 2;
								break;
			case MessageDAC::DAC3: mult = 3;
								break;
		}
		bytes.push_back(p.get<int>("Channel") + 64*mult);
		//payload
		//36-bit FTW
		//16-bit ATW
		//12-bit PTW
		auto bits = KA007_Message_Base::getFTW(p.get<double>("Frequency"));
		
		//process amplitude
		//Percent to 16-bit number
		auto ATW = p.get<double>("Amplitude");
		if(ATW < 0.0 || ATW > 100.0) thrower("invalid amplitude");
		ATW *= 655.35;
		int ATW_bits = (int)ATW;
		
		bits = bits << 16;
		bits = bits | ATW_bits;

		//process phase
		//Degrees to 12-bit
		auto PTW = p.get<double>("Phase");
		if (PTW < 0.0 || PTW > 360.0) thrower("invalid phase");
		PTW /= 360.0;
		PTW *= 4096;
		int PTW_bits = (int)PTW;

		bits = bits << 12;
		bits = bits | PTW_bits;
		
		for (int i = 7; i >=0 ; i--) {
			bytes.push_back((bits >> (8 * i)) & 0xFF);
		}

		std::cout << "Sending bytes: ";
		for(auto& byte : bytes)
			std::cout << byte << " ";
		std::cout << "\n";

		return bytes;
	}
};

struct SetMoveFrequency : KA007_Message_Base
{
	std::vector<int> getBytes(KA007ParameterContainer p) const override
	{
		std::vector<int> bytes;
		//message type 2 (upper nibble) to DDS (lower nibble = 2)
		bytes.push_back(0x22);
		//bytes.push_back(0x10); //loopback for testing
		//length is 4
		bytes.push_back(4);
		//channel is selected in header data
		bytes.push_back(0);
		int mult;
		switch (p.get<MessageDAC>("DAC")) {
		case MessageDAC::DAC0: mult = 0;
			break;
		case MessageDAC::DAC1: mult = 1;
			break;
		case MessageDAC::DAC2: mult = 2;
			break;
		case MessageDAC::DAC3: mult = 3;
			break;
		}
		bytes.push_back(p.get<int>("Channel") + 64 * mult);
		//payload
		//upper 64 bit
		  //36-bit FTW
		  //16-bit ATW
		  //12-bit PTW
		//lower 64 bit
		  // 4-bit instant move
		  //28-bit ATW increment
		  // 8-bit step sequence
		  // 8-bit empty
		  //12-bit FTW increment
		  // 4-bit Phase jump enable
		auto bits = KA007_Message_Base::getFTW(p.get<double>("Frequency"));

		//process amplitude
		//Percent to 16-bit number
		auto ATW = p.get<double>("Amplitude");
		if (ATW < 0.0 || ATW > 100.0) thrower("invalid amplitude");
		ATW *= 655.35;
		int ATW_bits = (int)ATW;

		bits = bits << 16;
		bits = bits | ATW_bits;

		//process phase
		//Degrees to 12-bit
		auto PTW = p.get<double>("Phase");
		if (PTW < 0.0 || PTW > 360.0) thrower("invalid phase");
		PTW /= 360.0;
		PTW *= 4096;
		int PTW_bits = (int)PTW;

		bits = bits << 12;
		bits = bits | PTW_bits;

		//enqueue upper 64 bits (8 bytes)
		for (int i = 7; i >= 0; i--) {
			bytes.push_back((bits >> (8 * i)) & 0xFF);
		}

		bits = p.get<int>("InstantFTW");

		bits = bits << 28;
		//conversion to 26-bit signed number. Highest 2 bits in 28 bit message get ignored.
		int temp = p.get<int>("ATWIncr");
		if (temp < -33554432)temp = -33554432;
		if (temp > 33554431)temp = 33554431;
		if (temp < 0)temp = temp + 67108864;
		bits = bits | temp;

		//bits = bits << 28;
		////conversion to 28-bit signed number
		//int temp = p.get<int>("ATWIncr");
		//if (temp < -134217728)temp = -134217728;
		//if (temp > 134217727)temp = 134217727;
		//if (temp < 0)temp = temp + 268435456;
		//bits = bits | temp;

		//bits = bits << 26; // This does not seem to be correct
		////conversion to 26-bit signed number
		//int temp = p.get<int>("ATWIncr");
		//if (temp < -33554432)temp = -33554432;
		//if (temp > 33554431)temp = 33554431;
		//if (temp < 0)temp = temp + 67108864;
		//bits = bits | temp;

		bits = bits << 8;
		bits = bits | p.get<int>("StepSequenceID");

		//empty byte
		bits = bits << 8;

		bits = bits << 12;
		//conversion to 10-bit signed number
		temp = p.get<int>("FTWIncr");
		if (temp < -512)temp = -512;
		if (temp > 511)temp = 511;
		if (temp < 0)temp = temp + 1024;
		bits = bits | temp;

		bits = bits << 4;
		bits = bits | p.get<int>("PhaseJump");

		//enqueue lower 64 bits (8 bytes)
		for (int i = 7; i >= 0; i--) {
			bytes.push_back((bits >> (8 * i)) & 0xFF);
		}

		std::cout << "Sending bytes: ";
		for (auto& byte : bytes)
			std::cout << byte << " ";
		std::cout << "\n";

		return bytes;
	}
};

struct TerminateSequence : KA007_Message_Base
{
	std::vector<int> getBytes(KA007ParameterContainer) const override
	{
		std::vector<int> bytes;
		//message type 3 to DDS
		bytes.push_back(0x32);
		//length is 2
		bytes.push_back(0);
		//no header data
		bytes.push_back(0);
		bytes.push_back(0);

		std::cout << "Sending bytes: ";
		for (auto& byte : bytes)
			std::cout << byte << " ";
		std::cout << "\n";

		return bytes;
	}
};

class KA007_MessageFactory
{
	std::map<MessageSetting, std::function<std::vector<int>(KA007ParameterContainer)>> factories;

public:
	KA007_MessageFactory()
	{
		if (sizeof(unsigned long long int) != 8) thrower("unsigned long long int needs to be 64-bit wide");
		
		factories[MessageSetting::LOADFREQUENCY] = [] (KA007ParameterContainer params) {
			auto bytes = SetLoadFrequency();
			return bytes.getBytes(params);
		};

		factories[MessageSetting::MOVEFREQUENCY] = [](KA007ParameterContainer params) {
			auto bytes = SetMoveFrequency();
			return bytes.getBytes(params);
		};

		factories[MessageSetting::TERMINATE_SEQ] = [](KA007ParameterContainer params) {
			auto bytes = TerminateSequence();
			return bytes.getBytes(params);
		};
	}
	
	std::vector<int> getBytes(KA007ParameterContainer params) {
		return factories[params.get<MessageSetting>("Setting")](params);
	}
};

class MessageBuilder
{
public:
	MessageBuilder& destination(const MessageDestination &dest) {
		message_.parameters_.set("Destination", dest);
		return *this;
	}

	MessageBuilder& DAC(const MessageDAC &DAC) {
		message_.parameters_.set("DAC", DAC);
		return *this;
	}

	MessageBuilder& setting(const MessageSetting &setting) {
		message_.parameters_.set("Setting", setting);
		return *this;
	}

	MessageBuilder& channel(const int &channel) {
		message_.parameters_.set("Channel", channel);
		return *this;
	}

	MessageBuilder& frequencyMHz(const double &frequency) {
		message_.parameters_.set("Frequency", frequency);
		return *this;
	}

	MessageBuilder& amplitudePercent(const double &amplitude) {
		message_.parameters_.set("Amplitude", amplitude);
		return *this;
	}

	MessageBuilder& phaseDegrees(const double &phase) {
		message_.parameters_.set("Phase", phase);
		return *this;
	}

	MessageBuilder& instantFTW(const bool &freqjump) {
		message_.parameters_.set("InstantFTW", freqjump);
		return *this;
	}

	MessageBuilder& ATWIncr(const int &ampstep) {
		message_.parameters_.set("ATWIncr", ampstep);
		return *this;
	}

	MessageBuilder& stepSequenceID(const int &stepnum) {
		message_.parameters_.set("StepSequenceID", stepnum);
		return *this;
	}

	MessageBuilder& FTWIncr(const int &freqstep) {
		message_.parameters_.set("FTWIncr", freqstep);
		return *this;
	}

	MessageBuilder& phaseJump(const bool &phasejump) {
		message_.parameters_.set("PhaseJump", phasejump);
		return *this;
	}

	operator Message && () {
		return std::move(message_);
	}

private:
	Message message_;
};