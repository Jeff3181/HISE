/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::Types;

namespace fx
{

template <int V> class sampleandhold_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif

	SET_HISE_POLY_NODE_ID("sampleandhold");
	GET_SELF_AS_OBJECT(sampleandhold_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	sampleandhold_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		Data& v = data.get();

		if (v.counter > d.getNumSamples())
		{
			int i = 0;

			for (auto c : d)
				hmath::vset(d.toChannelData(c), v.currentValues[i++]);

			v.counter -= d.getNumSamples();
		}
		else
			DspHelpers::forwardToFrame16(this, d);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& v = data.get();
		int i = 0;

		if (v.counter == 0)
		{
			for (auto& s : d)
				v.currentValues[i++] = s;
				
			v.counter = v.factor;
		}
		else
		{
			for (auto& s : d)
				s = v.currentValues[i++];

			v.counter--;
		}
	}


	void reset() noexcept;;

	
	bool handleModulation(double&) noexcept { return false; };
	void createParameters(Array<ParameterData>& data) override;

	void setFactor(double value);

private:

	struct Data
	{
		Data()
		{
			clear();
		}

		void clear(int numChannelsToClear = NUM_MAX_CHANNELS)
		{
			currentValues = 0.0f;
			
			for (auto& s : currentValues)
				s = 0.0f;

			counter = 0;
		}

		int factor = 1;
		int counter = 0;
		span<float, NUM_MAX_CHANNELS> currentValues;
	};

	PolyData<Data, NumVoices> data;
	int lastChannelAmount = NUM_MAX_CHANNELS;
};

DEFINE_EXTERN_NODE_TEMPLATE(sampleandhold, sampleandhold_poly, sampleandhold_impl);

template <typename T> static void getBitcrushedValue(T& data, float bitDepth)
{
	const float invStepSize = hmath::pow(2.0f, bitDepth);
	const float stepSize = 1.0f / invStepSize;

	for(auto& s: data)
		s = (stepSize * ceil(s * invStepSize) - 0.5f * stepSize);
}

template <int V> class bitcrush_impl : public HiseDspBase
{
public:

	enum class Parameters
	{
		BitDepth
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(BitDepth, bitcrush_impl);
	}

	static constexpr int NumVoices = V;

	SET_HISE_POLY_NODE_ID("bitcrush");
	GET_SELF_AS_OBJECT(bitcrush_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	bitcrush_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	// ======================================================================================================
	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		for (auto ch : d)
			getBitcrushedValue(d.toChannelData(ch), bitDepth.get());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		getBitcrushedValue(data, bitDepth.get());
	}

	void reset() noexcept;;
	bool handleModulation(double&) noexcept;;
	void createParameters(Array<ParameterData>& data) override;

	void setBitDepth(double newBitDepth);

private:

	PolyData<float, NumVoices> bitDepth;
};

template <int V> class phase_delay_impl : public HiseDspBase
{
public:


	static constexpr int NumVoices = V;
	using Delays = span<PolyData<AllpassDelay, NumVoices>, 2>;

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif
	SET_HISE_POLY_NODE_ID("phase_delay");
	GET_SELF_AS_OBJECT(phase_delay_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	phase_delay_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto i = IndexType::clamped(delays);

		for (auto ch : data)
		{
			auto& dl = delays[i++].get();

			for (auto& s : data.toChannelData(ch))
				s = dl.getNextSample(s);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto i = IndexType::clamped(delays);

		for (auto& s : data)
			s = delays[i++].get().getNextSample(s);
	}

	void reset() noexcept;;
	
	bool handleModulation(double&) noexcept;;
	void createParameters(Array<ParameterData>& data) override;

	void setFrequency(double frequency);

	Delays delays;
	double sr = 44100.0;
};

DEFINE_EXTERN_NODE_TEMPLATE(phase_delay, phase_delay_poly, phase_delay_impl);

DEFINE_EXTERN_NODE_TEMPLATE(bitcrush, bitcrush_poly, bitcrush_impl);

class reverb : public HiseDspBase
{
public:

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif

	SET_HISE_NODE_ID("reverb");
	GET_SELF_AS_OBJECT(reverb);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	reverb();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (d.getNumChannels() == 1)
			r.processMono(d[0].data, d.getNumSamples());
		else
			r.processStereo(d[0].data, d[1].data, d.getNumSamples());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		if (d.size() == 1)
			r.processMono(d.begin(), 1);
		else
			r.processStereo(d.begin(), d.begin() + 1, 1);
	}

	void reset() noexcept;;
	bool handleModulation(double&) noexcept;;
	void createParameters(Array<ParameterData>& data) override;

	void setDamping(double newDamping);
	void setWidth(double width);
	void setSize(double size);

private:

	juce::Reverb r;

};


template <int V> class haas_impl : public HiseDspBase
{
public:

	static const int NumChannels = 2;
	using DelayType = span<DelayLine<2048, DummyCriticalSection>, NumChannels>;
	using FrameType = span<float, NumChannels>;
	using ProcessType = ProcessDataFix<NumChannels>;

	static constexpr int NumVoices = V;

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif

	SET_HISE_POLY_NODE_ID("haas");
	GET_SELF_AS_OBJECT(haas_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data) override;
	void prepare(PrepareSpecs ps);
	void reset();

	void processFrame(FrameType& data);

	void process(ProcessType& d);

	bool handleModulation(double&);
	void setPosition(double newValue);

	double position = 0.0;
	PolyData<DelayType, NumVoices> delay;
};

DEFINE_EXTERN_NODE_TEMPLATE(haas, haas_poly, haas_impl);

}

}
