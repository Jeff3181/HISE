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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

#define SEND_MESSAGE(broadcaster) {	if (MessageManager::getInstance()->isThisTheMessageThread()) broadcaster->sendSynchronousChangeMessage(); else broadcaster->sendChangeMessage();}
#define ADD_TO_TYPE_SELECTOR(x) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))

#if USE_BACKEND

ApiHelpers::Api::Api()
{
	apiTree = ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize));
}

void ApiHelpers::getColourAndCharForType(int type, char &c, Colour &colour)
{

	const float alpha = 0.6f;
	const float brightness = 0.8f;


	switch (type)
	{
	case (int)DebugInformation::Type::InlineFunction:	c = 'I'; break;
	case (int)DebugInformation::Type::Callback:			c = 'F'; break;
	case (int)DebugInformation::Type::Variables:		c = 'V'; break;
	case (int)DebugInformation::Type::Globals:			c = 'G'; break;
	case (int)DebugInformation::Type::Constant:			c = 'C'; break;
	case (int)DebugInformation::Type::RegisterVariable:	c = 'R'; break;
	case (int)DebugInformation::Type::ExternalFunction: c = 'E'; break;
	case (int)DebugInformation::Type::Namespace:		c = 'N'; break;
	case 8:												c = 'A'; break;
	default:											c = 'V'; break;
	}

	switch (c)
	{
	case 'I': colour = Colours::blue.withAlpha(alpha).withBrightness(brightness); break;
	case 'V': colour = Colours::cyan.withAlpha(alpha).withBrightness(brightness); break;
	case 'G': colour = Colours::green.withAlpha(alpha).withBrightness(brightness); break;
	case 'C': colour = Colours::yellow.withAlpha(alpha).withBrightness(brightness); break;
	case 'R': colour = Colours::red.withAlpha(alpha).withBrightness(brightness); break;
	case 'A': colour = Colours::orange.withAlpha(alpha).withBrightness(brightness); break;
	case 'F': colour = Colours::purple.withAlpha(alpha).withBrightness(brightness); break;
	case 'E': colour = Colours::chocolate.withAlpha(alpha).withBrightness(brightness); break;
	case 'N': colour = Colours::pink.withAlpha(alpha).withBrightness(brightness); break;
	}
}



String ApiHelpers::getValueType(const var &v)
{
	const bool isObject = v.isObject();
	const bool isCreatableScriptObject = dynamic_cast<DynamicScriptingObject*>(v.getDynamicObject()) != nullptr;

	if (v.isBool()) return "bool";
	else if (v.isInt() || v.isInt64()) return "int";
	else if (v.isDouble()) return "double";
	else if (v.isString()) return "String";
	else if (v.isArray()) return "Array";
	else if (v.isMethod()) return "Function";
	else if (isObject && isCreatableScriptObject)
	{
		DynamicScriptingObject * obj = dynamic_cast<DynamicScriptingObject*>(v.getDynamicObject());

		if (obj != nullptr) return obj->getObjectName().toString();
		else return String();
	}
	else return String();
}


AttributedString ApiHelpers::createAttributedStringFromApi(const ValueTree &method, const String &/*className*/, bool multiLine, Colour textColour)
{
	AttributedString help;

	const String name = method.getProperty(Identifier("name")).toString();
	const String arguments = method.getProperty(Identifier("arguments")).toString();
	const String description = method.getProperty(Identifier("description")).toString();
	const String returnType = method.getProperty("returnType", "void");


	help.setWordWrap(AttributedString::byWord);


	if (multiLine)
	{
		help.setJustification(Justification::topLeft);
		help.setLineSpacing(1.5f);
		help.append("Name:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(name, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		help.append(arguments + "\n\n", GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.6f));
		help.append("Description:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(description + "\n\n", GLOBAL_FONT(), textColour.withAlpha(0.8f));

		help.append("Return Type:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(method.getProperty("returnType", "void"), GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
	}

	else
	{
		help.setJustification(Justification::centredLeft);
		help.append(description, GLOBAL_BOLD_FONT(), textColour.withAlpha(0.8f));

		const String oneLineReturnType = method.getProperty("returnType", "");

		if (oneLineReturnType.isNotEmpty())
		{
			help.append("\nReturn Type: ", GLOBAL_BOLD_FONT(), textColour);
			help.append(oneLineReturnType, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		}
	}

	return help;
}



String ApiHelpers::createCodeToInsert(const ValueTree &method, const String &className)
{
	const String name = method.getProperty(Identifier("name")).toString();

	if (name == "setMouseCallback")
	{
		const String argumentName = "event";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setLoadingCallback")
	{
		const String argumentName = "isPreloading";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setTimerCallback")
	{
		const String argumentName = "";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setPaintRoutine")
	{
		const String argumentName = "g";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else
	{
		const String arguments = method.getProperty(Identifier("arguments")).toString();

		return String(className + "." + name + arguments);
	}
}

#endif


String ApiHelpers::getFileNameFromErrorMessage(const String &message)
{
	if (message.startsWith("Line")) return String();

	String fileName = message.upToFirstOccurrenceOf("-", false, true);

	fileName = fileName.trimEnd();

	return fileName;
}



StringArray ApiHelpers::getJustificationNames()
{
	static StringArray sa;

	if (sa.isEmpty())
	{
		sa.add("left");
		sa.add("right");
		sa.add("top");
		sa.add("bottom");
		sa.add("centred");
		sa.add("centredTop");
		sa.add("centredBottom");
		sa.add("topLeft");
		sa.add("topRight");
		sa.add("bottomLeft");
		sa.add("bottomRight");
	}

	return sa;
}

Justification ApiHelpers::getJustification(const String& justificationName, Result* r/*=nullptr*/)
{
	static Array<Justification::Flags> justifications;

	if (justifications.isEmpty())
	{
		justifications.add(Justification::left);
		justifications.add(Justification::right);
		justifications.add(Justification::top);
		justifications.add(Justification::bottom);
		justifications.add(Justification::centred);
		justifications.add(Justification::centredTop);
		justifications.add(Justification::centredBottom);
		justifications.add(Justification::topLeft);
		justifications.add(Justification::topRight);
		justifications.add(Justification::bottomLeft);
		justifications.add(Justification::bottomRight);
	}

	auto names = getJustificationNames();

	int index = names.indexOf(justificationName);

	if (index != -1)
	{
		return justifications[index];
	}
	
	if (r != nullptr)
		*r = Result::fail("Justification not found: " + justificationName);

	return Justification::centred;
}

Point<float> ApiHelpers::getPointFromVar(const var& data, Result* r /*= nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 2)
		{
            auto d0 = (float)d->getUnchecked(0);
            auto d1 = (float)d->getUnchecked(1);
            
            Point<float> p(SANITIZED(d0), SANITIZED(d1));

			return p;
		}
		else
		{
			if (r != nullptr) *r = Result::fail("Point array needs 2 elements");

			return Point<float>();
		}
	}
	else
	{
		if (r != nullptr) *r = Result::fail("Point is not an array");

		return Point<float>();
	}
}

Rectangle<float> ApiHelpers::getRectangleFromVar(const var &data, Result *r/*=nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 4)
		{
            auto d0 = (float)d->getUnchecked(0);
            auto d1 = (float)d->getUnchecked(1);
            auto d2 = (float)d->getUnchecked(2);
            auto d3 = (float)d->getUnchecked(3);
            
			Rectangle<float> rectangle(SANITIZED(d0), SANITIZED(d1), SANITIZED(d2), SANITIZED(d3));

            if(r != nullptr) *r = Result::ok();
            
			return rectangle;
		}
		else
		{
			if (r != nullptr) *r = Result::fail("Rectangle array needs 4 elements");
			return Rectangle<float>();
		}
	}
	else
	{
		if (r != nullptr) *r = Result::fail("Rectangle data is not an array");
		return Rectangle<float>();
	}
}

Rectangle<int> ApiHelpers::getIntRectangleFromVar(const var &data, Result* r/*=nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 4)
		{
			Rectangle<int> rectangle((int)d->getUnchecked(0), (int)d->getUnchecked(1), (int)d->getUnchecked(2), (int)d->getUnchecked(3));

			return rectangle;
		}
		else
		{
			if (r != nullptr) *r = Result::fail("Rectangle array needs 4 elements");
			return Rectangle<int>();
		}
	}
	else
	{
		if (r != nullptr) *r = Result::fail("Rectangle data is not an array");
		return Rectangle<int>();
	}
}

// ====================================================================================================== Message functions


struct ScriptingApi::Message::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Message, setNoteNumber);
	API_VOID_METHOD_WRAPPER_1(Message, setVelocity);
	API_VOID_METHOD_WRAPPER_1(Message, setControllerNumber);
	API_VOID_METHOD_WRAPPER_1(Message, setControllerValue);
	API_METHOD_WRAPPER_0(Message, getNoteNumber);
	API_METHOD_WRAPPER_0(Message, getVelocity);
	API_METHOD_WRAPPER_0(Message, getControllerNumber);
	API_METHOD_WRAPPER_0(Message, getControllerValue);
	API_METHOD_WRAPPER_0(Message, isProgramChange);
	API_METHOD_WRAPPER_0(Message, getProgramChangeNumber);
	API_VOID_METHOD_WRAPPER_1(Message, ignoreEvent);
	API_VOID_METHOD_WRAPPER_1(Message, delayEvent);
	API_METHOD_WRAPPER_0(Message, getEventId);
	API_METHOD_WRAPPER_0(Message, getChannel);
	API_VOID_METHOD_WRAPPER_1(Message, setChannel);
	API_VOID_METHOD_WRAPPER_1(Message, setTransposeAmount);
	API_METHOD_WRAPPER_0(Message, getTransposeAmount);
	API_VOID_METHOD_WRAPPER_1(Message, setCoarseDetune);
	API_METHOD_WRAPPER_0(Message, getCoarseDetune);
	API_VOID_METHOD_WRAPPER_1(Message, setFineDetune);
	API_METHOD_WRAPPER_0(Message, getFineDetune);
	API_VOID_METHOD_WRAPPER_1(Message, setGain);
	API_METHOD_WRAPPER_0(Message, getGain);
	API_VOID_METHOD_WRAPPER_1(Message, setStartOffset);
	API_METHOD_WRAPPER_0(Message, getStartOffset)
	API_METHOD_WRAPPER_0(Message, getTimestamp);
	API_VOID_METHOD_WRAPPER_1(Message, store);
	API_METHOD_WRAPPER_0(Message, makeArtificial);
	API_METHOD_WRAPPER_0(Message, isArtificial);
	
};


ScriptingApi::Message::Message(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
messageHolder(nullptr),
constMessageHolder(nullptr)
{
	memset(artificialNoteOnIds, 0, sizeof(uint16) * 128);

	ADD_API_METHOD_1(setNoteNumber);
	ADD_API_METHOD_1(setVelocity);
	ADD_API_METHOD_1(setControllerNumber);
	ADD_API_METHOD_1(setControllerValue);
	ADD_API_METHOD_0(getControllerNumber);
	ADD_API_METHOD_0(getControllerValue);
	ADD_API_METHOD_0(isProgramChange);
	ADD_API_METHOD_0(getProgramChangeNumber);
	ADD_API_METHOD_0(getNoteNumber);
	ADD_API_METHOD_0(getVelocity);
	ADD_API_METHOD_1(ignoreEvent);
	ADD_API_METHOD_1(delayEvent);
	ADD_API_METHOD_0(getEventId);
	ADD_API_METHOD_0(getChannel);
	ADD_API_METHOD_1(setChannel);
	ADD_API_METHOD_0(getGain);
	ADD_API_METHOD_1(setGain);
	ADD_API_METHOD_1(setTransposeAmount);
	ADD_API_METHOD_0(getTransposeAmount);
	ADD_API_METHOD_1(setCoarseDetune);
	ADD_API_METHOD_0(getCoarseDetune);
	ADD_API_METHOD_1(setFineDetune);
	ADD_API_METHOD_0(getFineDetune);
	ADD_API_METHOD_0(getTimestamp);
	ADD_API_METHOD_0(getStartOffset);
	ADD_API_METHOD_1(setStartOffset);
	ADD_API_METHOD_1(store);
	ADD_API_METHOD_0(makeArtificial);
	ADD_API_METHOD_0(isArtificial);
}


ScriptingApi::Message::~Message()
{
	messageHolder = nullptr;
	constMessageHolder = nullptr;
}

int ScriptingApi::Message::getNoteNumber() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || !constMessageHolder->isNoteOnOrOff())
	{
		reportIllegalCall("getNoteNumber()", "onNoteOn / onNoteOff");
		RETURN_IF_NO_THROW(-1)
	}
#endif

	return constMessageHolder->getNoteNumber();
};


void ScriptingApi::Message::delayEvent(int samplesToDelay)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("delayEvent()", "midi event");
		return;
	}	
#endif

	messageHolder->addToTimeStamp((int16)samplesToDelay);
};

void ScriptingApi::Message::setNoteNumber(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setNoteNumber()", "midi event");
		return;
	}
#endif

	if(!messageHolder->isNoteOnOrOff())
	{
		reportIllegalCall("setNoteNumber()", "noteOn / noteOff");
	}

	messageHolder->setNoteNumber(newValue);
};

void ScriptingApi::Message::setVelocity(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return;
	}

	if(!messageHolder->isNoteOn())
	{
		reportIllegalCall("setVelocity()", "onNoteOn");
	}
#endif

	messageHolder->setVelocity((uint8)newValue);
};




void ScriptingApi::Message::setControllerNumber(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setControllerNumber()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerNumber()", "onController");
	}
#endif
	
	messageHolder->setControllerNumber(newValue);
};

void ScriptingApi::Message::setControllerValue(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setControllerValue()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerValue()", "onController");
	}
#endif

	

	
	messageHolder->setControllerValue(newValue);
};

bool ScriptingApi::Message::isProgramChange()
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("isProgramChange()", "midi event");
		return false;
	}
#endif

	return constMessageHolder->isProgramChange();
}

int ScriptingApi::Message::getProgramChangeNumber()
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return -1;
	}
#endif

	if (constMessageHolder->isProgramChange())
		return constMessageHolder->getProgramChangeNumber();
	else
		return -1;
}

var ScriptingApi::Message::getControllerNumber() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || ( !constMessageHolder->isController() && !constMessageHolder->isPitchWheel() && !constMessageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerNumber()", "onController");
		RETURN_IF_NO_THROW(var())
	}
#endif

	if(constMessageHolder->isController())		  return constMessageHolder->getControllerNumber();
	else if (constMessageHolder->isPitchWheel())	  return 128;
	else if (constMessageHolder->isAftertouch())   return 129;
	else									  return var::undefined();
};


var ScriptingApi::Message::getControllerValue() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || ( !constMessageHolder->isController() && !constMessageHolder->isPitchWheel() && !constMessageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerValue()", "onController");
		RETURN_IF_NO_THROW(var())
	}
#endif

	if      (constMessageHolder->isController())	  return constMessageHolder->getControllerValue();
	else if (constMessageHolder->isAftertouch())	  return constMessageHolder->getAfterTouchValue();
	else if (constMessageHolder->isPitchWheel())	  return constMessageHolder->getPitchWheelValue();
	else									  return var::undefined();
};

int ScriptingApi::Message::getVelocity() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || (!constMessageHolder->isNoteOnOrOff()))
	{
		reportIllegalCall("getVelocity()", "onNoteOn");
		RETURN_IF_NO_THROW(-1)
	}
#endif

	return constMessageHolder->getVelocity();
};

void ScriptingApi::Message::ignoreEvent(bool shouldBeIgnored/*=true*/)
{
	if (messageHolder == nullptr)
	{
		reportIllegalCall("ignoreEvent()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	messageHolder->ignoreEvent(shouldBeIgnored);
}

int ScriptingApi::Message::getChannel() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr)
	{
		reportScriptError("Can only be called in MIDI callbacks");
		RETURN_IF_NO_THROW(-1)
	}
#endif

	return constMessageHolder->getChannel();
};

void ScriptingApi::Message::setChannel(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setChannel()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	if(newValue < 1 || newValue > 16)
	{
		reportScriptError("Channel must be between 1 and 16.");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	messageHolder->setChannel(newValue);
};

int ScriptingApi::Message::getEventId() const
{ 
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getEventId()", "midi event");
		FRONTEND_ONLY(return 0;)
	}
#endif

	return constMessageHolder->getEventId();
}

void ScriptingApi::Message::setTransposeAmount(int tranposeValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setTransposeAmount()", "midi event");
		FRONTEND_ONLY(return;)
	}
#endif

	messageHolder->setTransposeAmount(tranposeValue);

}

int ScriptingApi::Message::getTransposeAmount() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getTransposeAmount()", "midi event");
		FRONTEND_ONLY(return 0;)
	}
#endif

	return constMessageHolder->getTransposeAmount();
}

void ScriptingApi::Message::setCoarseDetune(int semiToneDetune)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setCoarseDetune()", "midi event");
		return;
	}
#endif

	messageHolder->setCoarseDetune(semiToneDetune);

}

int ScriptingApi::Message::getCoarseDetune() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getCoarseDetune()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getCoarseDetune();

}

void ScriptingApi::Message::setFineDetune(int cents)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setFineDetune()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	messageHolder->setFineDetune(cents);
}

int ScriptingApi::Message::getFineDetune() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getFineDetune()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getFineDetune();
}


void ScriptingApi::Message::setGain(int gainInDecibels)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setGain()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	messageHolder->setGain(gainInDecibels);
}

int ScriptingApi::Message::getGain() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getGain()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getGain();
}

int ScriptingApi::Message::getTimestamp() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getTimestamp()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getTimeStamp();
}

void ScriptingApi::Message::setStartOffset(int newStartOffset)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("setStartOffset()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	if (newStartOffset > UINT16_MAX)
		reportScriptError("Max start offset is 65536 (2^16)");

#endif

	messageHolder->setStartOffset((uint16)newStartOffset);
}

int ScriptingApi::Message::getStartOffset() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("setStartOffset()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getStartOffset();
}

void ScriptingApi::Message::store(var messageEventHolder) const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("store()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	ScriptingObjects::ScriptingMessageHolder* holder = dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageEventHolder.getObject());

	if (holder != nullptr && constMessageHolder != nullptr)
	{
		holder->setMessage(*constMessageHolder);
	}
}

int ScriptingApi::Message::makeArtificial()
{
	if (messageHolder != nullptr)
	{
		if (messageHolder->isArtificial()) return messageHolder->getEventId();

		HiseEvent copy(*messageHolder);

		copy.setArtificial();

		if (copy.isNoteOn())
		{
			getScriptProcessor()->getMainController_()->getEventHandler().pushArtificialNoteOn(copy);
			artificialNoteOnIds[copy.getNoteNumber()] = copy.getEventId();

		}
		else if (copy.isNoteOff())
		{
			HiseEvent e = getScriptProcessor()->getMainController_()->getEventHandler().popNoteOnFromEventId(artificialNoteOnIds[copy.getNoteNumber()]);
			
			if (e.isEmpty())
			{
				artificialNoteOnIds[copy.getNoteNumber()] = 0;
				copy.ignoreEvent(true);
			}
			
			copy.setEventId(artificialNoteOnIds[copy.getNoteNumber()]);
		}

		copy.swapWith(*messageHolder);

		return messageHolder->getEventId();
	}

	return 0;
}

bool ScriptingApi::Message::isArtificial() const
{
	if (constMessageHolder != nullptr)
	{
		return constMessageHolder->isArtificial();
	}

	return false;
}

void ScriptingApi::Message::setHiseEvent(HiseEvent &m)
{
	messageHolder = &m;
	constMessageHolder = messageHolder;
}

void ScriptingApi::Message::setHiseEvent(const HiseEvent& m)
{
	messageHolder = nullptr;
	constMessageHolder = &m;
}

// ====================================================================================================== Engine functions

struct ScriptingApi::Engine::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(Engine, allNotesOff);
	API_METHOD_WRAPPER_0(Engine, getUptime);
	API_METHOD_WRAPPER_0(Engine, getHostBpm);
	API_VOID_METHOD_WRAPPER_1(Engine, setHostBpm);
	API_METHOD_WRAPPER_0(Engine, getCpuUsage);
	API_METHOD_WRAPPER_0(Engine, getNumVoices);
	API_METHOD_WRAPPER_0(Engine, getMemoryUsage);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForTempo);
	API_METHOD_WRAPPER_1(Engine, getSamplesForMilliSeconds);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForSamples);
	API_METHOD_WRAPPER_1(Engine, getGainFactorForDecibels);
	API_METHOD_WRAPPER_1(Engine, getDecibelsForGainFactor);
	API_METHOD_WRAPPER_1(Engine, getFrequencyForMidiNoteNumber);
	API_METHOD_WRAPPER_1(Engine, getPitchRatioFromSemitones);
	API_METHOD_WRAPPER_1(Engine, getSemitonesFromPitchRatio);
	API_METHOD_WRAPPER_0(Engine, getSampleRate);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteName);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteFromName);
	API_METHOD_WRAPPER_1(Engine, getMacroName);
	API_VOID_METHOD_WRAPPER_2(Engine, setKeyColour);
	API_VOID_METHOD_WRAPPER_2(Engine, showErrorMessage);
	API_VOID_METHOD_WRAPPER_1(Engine, showMessage);
	API_VOID_METHOD_WRAPPER_1(Engine, setLowestKeyToDisplay);
    API_VOID_METHOD_WRAPPER_1(Engine, openWebsite);
	API_VOID_METHOD_WRAPPER_1(Engine, loadNextUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, loadPreviousUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, loadUserPreset);
	API_METHOD_WRAPPER_0(Engine, getUserPresetList);
	API_METHOD_WRAPPER_0(Engine, getCurrentUserPresetName);
	API_VOID_METHOD_WRAPPER_1(Engine, saveUserPreset);
	API_METHOD_WRAPPER_0(Engine, isMpeEnabled);
	API_METHOD_WRAPPER_0(Engine, createSliderPackData);
	API_METHOD_WRAPPER_0(Engine, createMidiList);
	API_METHOD_WRAPPER_0(Engine, createTimerObject);
	API_METHOD_WRAPPER_0(Engine, createMessageHolder);
	API_METHOD_WRAPPER_0(Engine, getPlayHead);
	API_METHOD_WRAPPER_0(Engine, getExpansionHandler);
	API_VOID_METHOD_WRAPPER_2(Engine, dumpAsJSON);
	API_METHOD_WRAPPER_1(Engine, loadFromJSON);
	API_VOID_METHOD_WRAPPER_1(Engine, setCompileProgress);
	API_METHOD_WRAPPER_2(Engine, matchesRegex);
	API_METHOD_WRAPPER_2(Engine, getRegexMatches);
	API_METHOD_WRAPPER_2(Engine, doubleToString);
	API_METHOD_WRAPPER_0(Engine, getOS);
	API_METHOD_WRAPPER_0(Engine, isPlugin);
	API_METHOD_WRAPPER_0(Engine, getPreloadProgress);
	API_METHOD_WRAPPER_0(Engine, getDeviceType);
	API_METHOD_WRAPPER_0(Engine, getDeviceResolution);
	API_METHOD_WRAPPER_0(Engine, getZoomLevel);
	API_METHOD_WRAPPER_0(Engine, getVersion);
	API_METHOD_WRAPPER_0(Engine, getFilterModeList);
	API_METHOD_WRAPPER_1(Engine, isControllerUsedByAutomation);
	API_METHOD_WRAPPER_0(Engine, getSettingsWindowObject);
	API_METHOD_WRAPPER_1(Engine, getMasterPeakLevel);
	API_VOID_METHOD_WRAPPER_1(Engine, loadFont);
	API_VOID_METHOD_WRAPPER_2(Engine, loadFontAs);
	API_VOID_METHOD_WRAPPER_0(Engine, undo);
	API_VOID_METHOD_WRAPPER_0(Engine, redo);
};

ScriptingApi::Engine::Engine(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0)
{
	ADD_API_METHOD_0(allNotesOff);
	ADD_API_METHOD_0(getUptime);
	ADD_API_METHOD_0(getHostBpm);
	ADD_API_METHOD_1(setHostBpm);
	ADD_API_METHOD_0(getCpuUsage);
	ADD_API_METHOD_0(getNumVoices);
	ADD_API_METHOD_0(getMemoryUsage);
	ADD_API_METHOD_1(getMilliSecondsForTempo);
	ADD_API_METHOD_1(getSamplesForMilliSeconds);
	ADD_API_METHOD_1(getMilliSecondsForSamples);
	ADD_API_METHOD_1(getGainFactorForDecibels);
	ADD_API_METHOD_1(getDecibelsForGainFactor);
	ADD_API_METHOD_1(getFrequencyForMidiNoteNumber);
	ADD_API_METHOD_1(getPitchRatioFromSemitones);
	ADD_API_METHOD_1(getSemitonesFromPitchRatio);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_1(getMidiNoteName);
	ADD_API_METHOD_1(getMidiNoteFromName);
	ADD_API_METHOD_1(getMacroName);
	ADD_API_METHOD_2(setKeyColour);
	ADD_API_METHOD_2(showErrorMessage);
	ADD_API_METHOD_1(showMessage);
	ADD_API_METHOD_1(setLowestKeyToDisplay);
    ADD_API_METHOD_1(openWebsite);
	ADD_API_METHOD_0(getExpansionHandler);
	ADD_API_METHOD_1(loadNextUserPreset);
	ADD_API_METHOD_1(loadPreviousUserPreset);
	ADD_API_METHOD_0(getCurrentUserPresetName);
	ADD_API_METHOD_1(saveUserPreset);
	ADD_API_METHOD_1(loadUserPreset);
	ADD_API_METHOD_0(getUserPresetList);
	ADD_API_METHOD_0(isMpeEnabled);
	ADD_API_METHOD_0(createMidiList);
	ADD_API_METHOD_0(getPlayHead);
	ADD_API_METHOD_2(dumpAsJSON);
	ADD_API_METHOD_1(loadFromJSON);
	ADD_API_METHOD_1(setCompileProgress);
	ADD_API_METHOD_2(matchesRegex);
	ADD_API_METHOD_2(getRegexMatches);
	ADD_API_METHOD_2(doubleToString);
	ADD_API_METHOD_1(getMasterPeakLevel);
	ADD_API_METHOD_0(getOS);
	ADD_API_METHOD_0(getDeviceType);
	ADD_API_METHOD_0(getDeviceResolution);
	ADD_API_METHOD_0(isPlugin);
	ADD_API_METHOD_0(getPreloadProgress);
	ADD_API_METHOD_0(getZoomLevel);
	ADD_API_METHOD_0(getVersion);
	ADD_API_METHOD_0(getFilterModeList);
	ADD_API_METHOD_1(isControllerUsedByAutomation);
	ADD_API_METHOD_0(getSettingsWindowObject);
	ADD_API_METHOD_0(createTimerObject);
	ADD_API_METHOD_0(createMessageHolder);
	ADD_API_METHOD_0(createSliderPackData);
	ADD_API_METHOD_1(loadFont);
	ADD_API_METHOD_2(loadFontAs);
	ADD_API_METHOD_0(undo);
	ADD_API_METHOD_0(redo);
}


void ScriptingApi::Engine::allNotesOff()
{
	getProcessor()->getMainController()->allNotesOff();
};



void ScriptingApi::Engine::loadFont(const String &fileName)
{
	debugError(getProcessor(), "loadFont is deprecated. Use loadFontAs() instead to prevent cross platform issues");
	loadFontAs(fileName, String());
}

void ScriptingApi::Engine::loadFontAs(String fileName, String fontId)
{

#if USE_BACKEND

	const String absolutePath = GET_PROJECT_HANDLER(getProcessor()).getFilePath(fileName, ProjectHandler::SubDirectories::Images);
	File f(absolutePath);
	ScopedPointer<FileInputStream> fis = f.createInputStream();

	if (fis == nullptr)
	{
		reportScriptError("File not found");
		return;
	}
	else
	{
		MemoryBlock mb;

		fis->readIntoMemoryBlock(mb);
		getProcessor()->getMainController()->loadTypeFace(fileName, mb.getData(), mb.getSize(), fontId);
	}

#else

	// Nothing to do here, it will be loaded on startup...

	ignoreUnused(fileName);

#endif
}

double ScriptingApi::Engine::getSampleRate() const { return const_cast<MainController*>(getProcessor()->getMainController())->getMainSynthChain()->getSampleRate(); }
double ScriptingApi::Engine::getSamplesForMilliSeconds(double milliSeconds) const { return (milliSeconds / 1000.0) * getSampleRate(); }


double ScriptingApi::Engine::getUptime() const		 
{
	const ScriptBaseMidiProcessor* jmp = dynamic_cast<const ScriptBaseMidiProcessor*>(getProcessor());

	if (jmp != nullptr && jmp->getCurrentHiseEvent() != nullptr)
	{
		return jmp->getMainController()->getUptime() + jmp->getCurrentHiseEvent()->getTimeStamp() / getSampleRate();
	}

	return getProcessor()->getMainController()->getUptime(); 
}
double ScriptingApi::Engine::getHostBpm() const		 { return getProcessor()->getMainController()->getBpm(); }

void ScriptingApi::Engine::setHostBpm(double newTempo)
{
	getProcessor()->getMainController()->setHostBpm(newTempo);
}

double ScriptingApi::Engine::getMemoryUsage() const
{
	auto bytes = getProcessor()->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->getMemoryUsageForAllSamples();

	return (double)bytes / 1024.0 / 1024.0;
}

double ScriptingApi::Engine::getCpuUsage() const { return (double)getProcessor()->getMainController()->getCpuUsage(); }
int ScriptingApi::Engine::getNumVoices() const { return getProcessor()->getMainController()->getNumActiveVoices(); }

String ScriptingApi::Engine::getMacroName(int index)
{
	if (index >= 1 && index <= 8)
		return getProcessor()->getMainController()->getMainSynthChain()->getMacroControlData(index-1)->getMacroName();
	else
	{
		reportScriptError("Illegal Macro Index");
		return "Undefined";
	}
}

String ScriptingApi::Engine::getOS()
{
#if JUCE_WINDOWS
	return "WIN";
#else
	return "OSX";
#endif
}

String ScriptingApi::Engine::getDeviceType()
{
	return HiseDeviceSimulator::getDeviceName();
}

var ScriptingApi::Engine::getDeviceResolution()
{
	auto r = HiseDeviceSimulator::getDisplayResolution();

	Array<var> a = { r.getX(), r.getY(), r.getWidth(), r.getHeight() };

	return a;
	
}

bool ScriptingApi::Engine::isPlugin() const
{
#if HISE_IOS
    
    return HiseDeviceSimulator::isAUv3();
    
#else
    
#if IS_STANDALONE_APP
	return false;
#else
	return true;
#endif
#endif
}

double ScriptingApi::Engine::getPreloadProgress()
{
	return getScriptProcessor()->getMainController_()->getSampleManager().getPreloadProgress();
}

var ScriptingApi::Engine::getZoomLevel() const
{
	return dynamic_cast<const GlobalSettingManager*>(getScriptProcessor()->getMainController_())->getGlobalScaleFactor();
}

var ScriptingApi::Engine::getFilterModeList() const
{
	return var(new ScriptingObjects::ScriptingEffect::FilterModeObject(getScriptProcessor()));
}

String ScriptingApi::Engine::getVersion()
{
#if USE_BACKEND
	return dynamic_cast<GlobalSettingManager*>(getProcessor()->getMainController())->getSettingsObject().getSetting(HiseSettings::Project::Name);
#else
	return FrontendHandler::getVersionString();
#endif


}

double ScriptingApi::Engine::getMasterPeakLevel(int channel)
{
	// currently only stereo supported for this method...

	if (channel == 0)
		return getScriptProcessor()->getMainController_()->getMainSynthChain()->getDisplayValues().outL;
	else
		return getScriptProcessor()->getMainController_()->getMainSynthChain()->getDisplayValues().outR;
}

var ScriptingApi::Engine::getSettingsWindowObject()
{
	reportScriptError("Deprecated");
	return var();
}

int ScriptingApi::Engine::getMidiNoteFromName(String midiNoteName) const
{
	for (int i = 0; i < 127; i++)
	{
		if (getMidiNoteName(i) == midiNoteName)
			return i;
	}
	return -1;
}



void ScriptingApi::Engine::setKeyColour(int keyNumber, int colourAsHex) { getProcessor()->getMainController()->setKeyboardCoulour(keyNumber, Colour(colourAsHex));}

var ScriptingApi::Engine::getExpansionHandler()
{
	return new ScriptingObjects::ExpansionHandlerObject(getScriptProcessor());
}

void ScriptingApi::Engine::setLowestKeyToDisplay(int keyNumber) { getProcessor()->getMainController()->setLowestKeyToDisplay(keyNumber); }

void ScriptingApi::Engine::showErrorMessage(String message, bool isCritical)
{
#if USE_FRONTEND
	getProcessor()->getMainController()->sendOverlayMessage(isCritical ? DeactiveOverlay::State::CriticalCustomErrorMessage :
																		 DeactiveOverlay::State::CustomErrorMessage,
																		 message);

	if (isCritical)
		throw message;

#else

	ignoreUnused(isCritical);

	reportScriptError(message);

#endif
}

void ScriptingApi::Engine::showMessage(String message)
{
#if USE_FRONTEND

	getProcessor()->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomInformation, message);

#else
	debugToConsole(getProcessor(), message);
#endif
}

double ScriptingApi::Engine::getMilliSecondsForTempo(int tempoIndex) const { return (double)TempoSyncer::getTempoInMilliSeconds(getHostBpm(), (TempoSyncer::Tempo)tempoIndex); }


void ScriptingApi::Engine::openWebsite(String url)
{
    URL u(url);
    
    if (u.isWellFormed())
    {
        auto& tmp = u;
        
        auto f = [tmp]()
        {
            tmp.launchInDefaultBrowser();
        };
        
        new DelayedFunctionCaller(f, 300);
        
        
    }
    else
    {
        reportScriptError("not a valid URL");
    }
    
    
}
    
void ScriptingApi::Engine::loadNextUserPreset(bool stayInDirectory)
{
	getProcessor()->getMainController()->getUserPresetHandler().incPreset(true, stayInDirectory);
}

void ScriptingApi::Engine::loadPreviousUserPreset(bool stayInDirectory)
{
	getProcessor()->getMainController()->getUserPresetHandler().incPreset(false, stayInDirectory);
}

bool ScriptingApi::Engine::isMpeEnabled() const
{
	return getScriptProcessor()->getMainController_()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().isMpeEnabled();
}

String ScriptingApi::Engine::getCurrentUserPresetName()
{
	return getProcessor()->getMainController()->getUserPresetHandler().getCurrentlyLoadedFile().getFileNameWithoutExtension();
}

void ScriptingApi::Engine::saveUserPreset(String presetName)
{
	getProcessor()->getMainController()->getUserPresetHandler().savePreset(presetName);
}

void ScriptingApi::Engine::loadUserPreset(const String& relativePath)
{
#if USE_BACKEND
	File userPresetRoot = GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
	File userPresetRoot = FrontendHandler::getUserPresetDirectory();
#endif

	auto userPreset = userPresetRoot.getChildFile(relativePath + ".preset");

	if (userPreset.existsAsFile())
	{
		getProcessor()->getMainController()->getUserPresetHandler().loadUserPreset(userPreset);
	}
	else
	{
		reportScriptError("User preset " + userPreset.getFullPathName() + " doesn't exist");
	}
}

var ScriptingApi::Engine::getUserPresetList() const
{
#if USE_BACKEND
	File userPresetRoot = GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
	File userPresetRoot = FrontendHandler::getUserPresetDirectory();
#endif

	Array<File> presets;

	userPresetRoot.findChildFiles(presets, File::findFiles, true, "*.preset");

	Array<var> list;

	for (auto& pr : presets)
	{
		auto name = pr.getRelativePathFrom(userPresetRoot).upToFirstOccurrenceOf(".preset", false, true);
		name = name.replaceCharacter('\\', '/');

		list.add(var(name));
	}

	return var(list);
}

DynamicObject * ScriptingApi::Engine::getPlayHead() { return getProcessor()->getMainController()->getHostInfoObject(); }

int ScriptingApi::Engine::isControllerUsedByAutomation(int controllerNumber)
{
	auto handler = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler();

	for (int i = 0; i < handler->getNumActiveConnections(); i++)
	{
		if (handler->getDataFromIndex(i).ccNumber == controllerNumber)
			return i;
	}

	return -1;
}

ScriptingObjects::MidiList *ScriptingApi::Engine::createMidiList() { return new ScriptingObjects::MidiList(getScriptProcessor()); };

ScriptingObjects::ScriptSliderPackData* ScriptingApi::Engine::createSliderPackData() { return new ScriptingObjects::ScriptSliderPackData(getScriptProcessor()); }

ScriptingObjects::TimerObject* ScriptingApi::Engine::createTimerObject() { return new ScriptingObjects::TimerObject(getScriptProcessor()); }

ScriptingObjects::ScriptingMessageHolder* ScriptingApi::Engine::createMessageHolder()
{
	return new ScriptingObjects::ScriptingMessageHolder(getScriptProcessor());
}

void ScriptingApi::Engine::dumpAsJSON(var object, String fileName)
{
	if (!object.isObject())
	{
		reportScriptError("Only objects can be exported as JSON");
		return;
	}

	File f;

	if (File::isAbsolutePath(fileName))
		f = File(fileName);
	else
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));

	f.replaceWithText(JSON::toString(object, false));
	
}

var ScriptingApi::Engine::loadFromJSON(String fileName)
{
	File f;

	if (File::isAbsolutePath(fileName))
		f = File(fileName);
	else
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));

	if (f.existsAsFile())
		return JSON::parse(f);
	else
	{
		reportScriptError("File not found");
		RETURN_IF_NO_THROW(var())
	}
}


void ScriptingApi::Engine::setCompileProgress(var progress)
{
	JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

	if (sp != nullptr)
		sp->setCompileProgress((double)progress);
}



bool ScriptingApi::Engine::matchesRegex(String stringToMatch, String wildcard)
{
	try
	{
		std::regex reg(wildcard.toStdString());

		return std::regex_search(stringToMatch.toStdString(), reg);
	}
	catch (std::regex_error e)
	{
		debugError(getProcessor(), e.what());
		return false;
	}
}

var ScriptingApi::Engine::getRegexMatches(String stringToMatch, String wildcard)
{
    try
    {
        std::string s = stringToMatch.toStdString();
        std::regex reg(wildcard.toStdString());
        std::smatch match;
        
        if (std::regex_search(s, match, reg))
        {
            var returnArray = var();
            
            for (auto x:match)
            {
                returnArray.insert(-1, String(x));
            }
            
            return returnArray;
        }
    }
    catch (std::regex_error e)
    {
        debugError(getProcessor(), e.what());
        return var::undefined();
    }
    
    return var::undefined();
}

String ScriptingApi::Engine::doubleToString(double value, int digits)
{
    return String(value, digits);
}

void ScriptingApi::Engine::undo()
{
	getProcessor()->getMainController()->getControlUndoManager()->undo();
}

void ScriptingApi::Engine::redo()
{
	getProcessor()->getMainController()->getControlUndoManager()->redo();
}

// ====================================================================================================== Sampler functions

struct ScriptingApi::Sampler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Sampler, enableRoundRobin);
	API_VOID_METHOD_WRAPPER_1(Sampler, setActiveGroup);
	API_METHOD_WRAPPER_2(Sampler, getRRGroupsForMessage);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshRRMap);
	API_VOID_METHOD_WRAPPER_1(Sampler, selectSounds);
	API_METHOD_WRAPPER_0(Sampler, getNumSelectedSounds);
	API_VOID_METHOD_WRAPPER_2(Sampler, setSoundPropertyForSelection);
	API_VOID_METHOD_WRAPPER_2(Sampler, setSoundPropertyForAllSamples);
	API_METHOD_WRAPPER_2(Sampler, getSoundProperty);
	API_VOID_METHOD_WRAPPER_3(Sampler, setSoundProperty);
	API_VOID_METHOD_WRAPPER_2(Sampler, purgeMicPosition);
	API_METHOD_WRAPPER_0(Sampler, getNumMicPositions);
	API_METHOD_WRAPPER_1(Sampler, isMicPositionPurged);
	API_METHOD_WRAPPER_1(Sampler, getMicPositionName);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshInterface);
	API_VOID_METHOD_WRAPPER_1(Sampler, loadSampleMap);
	API_METHOD_WRAPPER_0(Sampler, getSampleMapList);
    API_METHOD_WRAPPER_0(Sampler, getCurrentSampleMapId);
    API_VOID_METHOD_WRAPPER_2(Sampler, setAttribute);
    API_METHOD_WRAPPER_1(Sampler, getAttribute);
	API_VOID_METHOD_WRAPPER_1(Sampler, setUseStaticMatrix);
};


ScriptingApi::Sampler::Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler_) :
ConstScriptingObject(p, SampleIds::numProperties),
sampler(sampler_)
{
	ADD_API_METHOD_1(enableRoundRobin);
	ADD_API_METHOD_1(setActiveGroup);
	ADD_API_METHOD_2(getRRGroupsForMessage);
	ADD_API_METHOD_0(refreshRRMap);
	ADD_API_METHOD_1(selectSounds);
	ADD_API_METHOD_0(getNumSelectedSounds);
	ADD_API_METHOD_2(setSoundPropertyForSelection);
	ADD_API_METHOD_2(setSoundPropertyForAllSamples);
	ADD_API_METHOD_2(getSoundProperty);
	ADD_API_METHOD_3(setSoundProperty);
	ADD_API_METHOD_2(purgeMicPosition);
	ADD_API_METHOD_1(getMicPositionName);
	ADD_API_METHOD_0(getNumMicPositions);
	ADD_API_METHOD_1(isMicPositionPurged);
	ADD_API_METHOD_0(refreshInterface);
	ADD_API_METHOD_1(loadSampleMap);
    ADD_API_METHOD_0(getCurrentSampleMapId);
	ADD_API_METHOD_0(getSampleMapList);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setUseStaticMatrix);

	sampleIds.add(SampleIds::ID);
	sampleIds.add(SampleIds::FileName);
	sampleIds.add(SampleIds::Root);
	sampleIds.add(SampleIds::HiKey);
	sampleIds.add(SampleIds::LoKey);
	sampleIds.add(SampleIds::LoVel);
	sampleIds.add(SampleIds::HiVel);
	sampleIds.add(SampleIds::RRGroup);
	sampleIds.add(SampleIds::Volume);
	sampleIds.add(SampleIds::Pan);
	sampleIds.add(SampleIds::Normalized);
	sampleIds.add(SampleIds::Pitch);
	sampleIds.add(SampleIds::SampleStart);
	sampleIds.add(SampleIds::SampleEnd);
	sampleIds.add(SampleIds::SampleStartMod);
	sampleIds.add(SampleIds::LoopStart);
	sampleIds.add(SampleIds::LoopEnd);
	sampleIds.add(SampleIds::LoopXFade);
	sampleIds.add(SampleIds::LoopEnabled);
	sampleIds.add(SampleIds::LowerVelocityXFade);
	sampleIds.add(SampleIds::UpperVelocityXFade);
	sampleIds.add(SampleIds::SampleState);
	sampleIds.add(SampleIds::Reversed);

	for (int i = 1; i < sampleIds.size(); i++)
	{
		addConstant(sampleIds[i].toString(), (int)i);
	}
}

void ScriptingApi::Sampler::enableRoundRobin(bool shouldUseRoundRobin)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		s->setUseRoundRobinLogic(shouldUseRoundRobin);
	}
	else
	{
		reportScriptError("enableRoundRobin() only works with Samplers.");
	}
}

void ScriptingApi::Sampler::setActiveGroup(int activeGroupIndex)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setActiveGroup() only works with Samplers.");
		return;
	}

	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		return;
	}

	bool ok = s->setCurrentGroupIndex(activeGroupIndex);

	if (!ok)
	{
		reportScriptError(String(activeGroupIndex) + " is not a valid group index.");
	}
}

int ScriptingApi::Sampler::getRRGroupsForMessage(int noteNumber, int velocity)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getRRGroupsForMessage() only works with Samplers.");
		RETURN_IF_NO_THROW(0)
	}

	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		RETURN_IF_NO_THROW(0)
	}

	return s->getRRGroupsForMessage(noteNumber, velocity);
}

void ScriptingApi::Sampler::refreshRRMap()
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("refreshRRMap() only works with Samplers.");
		return;
	}

	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		return;
	}

	s->refreshRRMap();
}

void ScriptingApi::Sampler::selectSounds(String regexWildcard)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("selectSounds() only works with Samplers.");
		return;
	}

	ModulatorSamplerSound::selectSoundsBasedOnRegex(regexWildcard, s, soundSelection);
}

int ScriptingApi::Sampler::getNumSelectedSounds()
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getNumSelectedSounds() only works with Samplers.");
		RETURN_IF_NO_THROW(-1)
	}

	return soundSelection.getNumSelected();
}

void ScriptingApi::Sampler::setSoundPropertyForSelection(int propertyId, var newValue)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundsProperty() only works with Samplers.");
		return;
	}

	auto sounds = soundSelection.getItemArray();

	auto id = sampleIds[propertyId];

	const int numSelected = sounds.size();

	for (int i = 0; i < numSelected; i++)
	{
		if (sounds[i].get() != nullptr)
		{
			sounds[i]->setSampleProperty(id, newValue, false);
		}
	}
}

void ScriptingApi::Sampler::setSoundPropertyForAllSamples(int propertyIndex, var newValue)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundsProperty() only works with Samplers.");
		return;
	}

	auto id = sampleIds[propertyIndex];

	for (int i = 0; i < s->getNumSounds(); i++)
	{
		if (auto sound = s->getSampleMap()->getSound(i))
			sound->setSampleProperty(id, newValue, false);
	}
}

var ScriptingApi::Sampler::getSoundProperty(int propertyIndex, int soundIndex)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getSoundProperty() only works with Samplers.");
		RETURN_IF_NO_THROW(var())
	}

	if (auto sound = s->getSampleMap()->getSound(soundIndex))
	{
		auto id = sampleIds[propertyIndex];
		return sound->getSampleProperty(id);
	}
	else
	{
		reportScriptError("no sound with index " + String(soundIndex));
		RETURN_IF_NO_THROW(var())
	}
}

void ScriptingApi::Sampler::setSoundProperty(int soundIndex, int propertyIndex, var newValue)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundProperty() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	if (auto sound = soundSelection.getSelectedItem(soundIndex).get())
	{
		auto id = sampleIds[propertyIndex];
		sound->setSampleProperty(id, newValue, false);
	}
	else
	{
		reportScriptError("no sound with index " + String(soundIndex));
        RETURN_VOID_IF_NO_THROW()
	}
}

void ScriptingApi::Sampler::purgeMicPosition(String micName, bool shouldBePurged)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if(micName.isEmpty())
    {
        reportScriptError("Mic position name must not be empty.");
        RETURN_VOID_IF_NO_THROW()
    }
    
	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	if (!s->isUsingStaticMatrix() && s->getNumMicPositions() == 1)
	{
		reportScriptError("purgeMicPosition() only works with multi mic Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

    
	for (int i = 0; i < s->getNumMicPositions(); i++)
	{
		if (micName == s->getChannelData(i).suffix)
		{
			s->setMicEnabled(i, !shouldBePurged);

			return;
		}
	}

	reportScriptError("Channel not found. Use getMicPositionName()");
    RETURN_VOID_IF_NO_THROW()
}

String ScriptingApi::Sampler::getMicPositionName(int channelIndex)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getMicPositionName() only works with Samplers.");
		RETURN_IF_NO_THROW("")
	}

	if (!s->isUsingStaticMatrix() && s->getNumMicPositions() == 1)
	{
		reportScriptError("getMicPositionName() only works with multi mic Samplers.");
		RETURN_IF_NO_THROW("")
	}

	return s->getChannelData(channelIndex).suffix;
}

int ScriptingApi::Sampler::getNumMicPositions() const
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getNumMicPositions() only works with Samplers.");
		RETURN_IF_NO_THROW(0)
	}

	return s->getNumMicPositions();
}

bool ScriptingApi::Sampler::isMicPositionPurged(int micIndex)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("isMicPositionPurged() only works with Samplers.");
		RETURN_IF_NO_THROW(false)
	}

	if (micIndex >= 0 && micIndex < s->getNumMicPositions())
	{
		return !s->getChannelData(micIndex).enabled;
	}
	else return false;

	
}

void ScriptingApi::Sampler::refreshInterface()
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("refreshInterface() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	s->sendChangeMessage();
	s->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
}

void ScriptingApi::Sampler::loadSampleMap(const String &fileName)
{
    if(fileName.isEmpty()) return;
    
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		PoolReference ref(s->getMainController(), fileName, FileHandlerBase::SampleMaps);

		s->loadSampleMap(ref, true);
	}
}

String ScriptingApi::Sampler::getCurrentSampleMapId() const
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());
    
    if (s != nullptr)
    {
        auto map = s->getSampleMap();
        
        if(map != nullptr)
            return map->getId().toString();
    }
    
    return String();
}


var ScriptingApi::Sampler::getSampleMapList() const
{
	Array<var> sampleMapNames;

	auto pool = getProcessor()->getMainController()->getCurrentSampleMapPool();
	auto references = pool->getListOfAllReferences(true);

	sampleMapNames.ensureStorageAllocated(references.size());

	for (auto r : references)
		sampleMapNames.add(r.getReferenceString());

	return sampleMapNames;
}

var ScriptingApi::Sampler::getAttribute(int index) const
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());
    
    if (s == nullptr)
    {
        reportScriptError("getAttribute() only works with Samplers.");
		RETURN_IF_NO_THROW(var());
    }
    
    return s->getAttribute(index);
}

void ScriptingApi::Sampler::setAttribute(int index, var newValue)
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());
    
    if (s == nullptr)
    {
        reportScriptError("setAttribute() only works with Samplers.");
        RETURN_VOID_IF_NO_THROW()
    }
    
    s->setAttribute(index, newValue, sendNotification);
}

void ScriptingApi::Sampler::setUseStaticMatrix(bool shouldUseStaticMatrix)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setAttribute() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	s->setUseStaticMatrix(shouldUseStaticMatrix);
}

// ====================================================================================================== Synth functions



struct ScriptingApi::Synth::Wrapper
{
	API_METHOD_WRAPPER_0(Synth, getNumChildSynths);
	API_VOID_METHOD_WRAPPER_1(Synth, addToFront);
	API_VOID_METHOD_WRAPPER_1(Synth, deferCallbacks);
	API_VOID_METHOD_WRAPPER_1(Synth, noteOff);
	API_VOID_METHOD_WRAPPER_1(Synth, noteOffByEventId);
	API_VOID_METHOD_WRAPPER_2(Synth, noteOffDelayedByEventId);
	API_METHOD_WRAPPER_2(Synth, playNote);
	API_METHOD_WRAPPER_4(Synth, playNoteWithStartOffset);
	API_VOID_METHOD_WRAPPER_2(Synth, setAttribute);
	API_METHOD_WRAPPER_1(Synth, getAttribute);
	API_METHOD_WRAPPER_4(Synth, addNoteOn);
	API_VOID_METHOD_WRAPPER_3(Synth, addNoteOff);
	API_VOID_METHOD_WRAPPER_3(Synth, addVolumeFade);
	API_VOID_METHOD_WRAPPER_4(Synth, addPitchFade);
	API_VOID_METHOD_WRAPPER_4(Synth, addController);
	API_METHOD_WRAPPER_1(Synth, addMessageFromHolder);
	API_VOID_METHOD_WRAPPER_2(Synth, setVoiceGainValue);
	API_VOID_METHOD_WRAPPER_2(Synth, setVoicePitchValue);
	API_VOID_METHOD_WRAPPER_1(Synth, startTimer);
	API_VOID_METHOD_WRAPPER_0(Synth, stopTimer);
	API_METHOD_WRAPPER_0(Synth, isTimerRunning);
	API_METHOD_WRAPPER_0(Synth, getTimerInterval);
	API_VOID_METHOD_WRAPPER_2(Synth, setMacroControl);
	API_VOID_METHOD_WRAPPER_2(Synth, sendController);
	API_VOID_METHOD_WRAPPER_2(Synth, sendControllerToChildSynths);
	API_VOID_METHOD_WRAPPER_4(Synth, setModulatorAttribute);
	API_METHOD_WRAPPER_3(Synth, addModulator);
	API_METHOD_WRAPPER_1(Synth, removeModulator);
	API_METHOD_WRAPPER_3(Synth, addEffect);
	API_METHOD_WRAPPER_1(Synth, removeEffect);
	API_METHOD_WRAPPER_1(Synth, getModulator);
	API_METHOD_WRAPPER_1(Synth, getAudioSampleProcessor);
	API_METHOD_WRAPPER_1(Synth, getTableProcessor);
	API_METHOD_WRAPPER_1(Synth, getSampler);
	API_METHOD_WRAPPER_1(Synth, getSlotFX);
	API_METHOD_WRAPPER_1(Synth, getEffect);
	API_METHOD_WRAPPER_1(Synth, getMidiProcessor);
	API_METHOD_WRAPPER_1(Synth, getChildSynth);
	API_METHOD_WRAPPER_1(Synth, getChildSynthByIndex);
	API_METHOD_WRAPPER_1(Synth, getIdList);
	API_METHOD_WRAPPER_2(Synth, getModulatorIndex);
	API_METHOD_WRAPPER_1(Synth, getAllModulators);
	API_METHOD_WRAPPER_0(Synth, getNumPressedKeys);
	API_METHOD_WRAPPER_0(Synth, isLegatoInterval);
	API_METHOD_WRAPPER_0(Synth, isSustainPedalDown);
	API_METHOD_WRAPPER_1(Synth, isKeyDown);
	API_VOID_METHOD_WRAPPER_1(Synth, setClockSpeed);
	API_VOID_METHOD_WRAPPER_1(Synth, setShouldKillRetriggeredNote);
};


ScriptingApi::Synth::Synth(ProcessorWithScriptingContent *p, ModulatorSynth *ownerSynth) :
	ScriptingObject(p),
	ApiClass(0),
	moduleHandler(dynamic_cast<Processor*>(p)),
	owner(ownerSynth),
	numPressedKeys(0),
	keyDown(0),
	sustainState(false)
{
	jassert(owner != nullptr);

	keyDown.setRange(0, 128, false);

	ADD_API_METHOD_0(getNumChildSynths);
	ADD_API_METHOD_1(addToFront);
	ADD_API_METHOD_1(deferCallbacks);
	ADD_API_METHOD_1(noteOff);
	ADD_API_METHOD_1(noteOffByEventId);
	ADD_API_METHOD_2(noteOffDelayedByEventId);
	ADD_API_METHOD_2(playNote);
	ADD_API_METHOD_4(playNoteWithStartOffset);
	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_4(addNoteOn);
	ADD_API_METHOD_3(addNoteOff);
	ADD_API_METHOD_3(addVolumeFade);
	ADD_API_METHOD_4(addPitchFade);
	ADD_API_METHOD_4(addController);
	ADD_API_METHOD_1(addMessageFromHolder);
	ADD_API_METHOD_2(setVoiceGainValue);
	ADD_API_METHOD_2(setVoicePitchValue);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_0(isTimerRunning);
	ADD_API_METHOD_0(getTimerInterval);
	ADD_API_METHOD_2(setMacroControl);
	ADD_API_METHOD_2(sendController);
	ADD_API_METHOD_2(sendControllerToChildSynths);
	ADD_API_METHOD_4(setModulatorAttribute);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_3(addEffect);
	ADD_API_METHOD_1(removeEffect);
	ADD_API_METHOD_1(removeModulator);
	ADD_API_METHOD_1(getModulator);
	ADD_API_METHOD_1(getAudioSampleProcessor);
	ADD_API_METHOD_1(getTableProcessor);
	ADD_API_METHOD_1(getSampler);
	ADD_API_METHOD_1(getSlotFX);
	ADD_API_METHOD_1(getEffect);
	ADD_API_METHOD_1(getMidiProcessor);
	ADD_API_METHOD_1(getChildSynth);
	ADD_API_METHOD_1(getChildSynthByIndex);
	ADD_API_METHOD_1(getIdList);
	ADD_API_METHOD_2(getModulatorIndex);
	ADD_API_METHOD_1(getAllModulators);
	ADD_API_METHOD_0(getNumPressedKeys);
	ADD_API_METHOD_0(isLegatoInterval);
	ADD_API_METHOD_0(isSustainPedalDown);
	ADD_API_METHOD_1(isKeyDown);
	ADD_API_METHOD_1(setClockSpeed);
	ADD_API_METHOD_1(setShouldKillRetriggeredNote);
	
};


int ScriptingApi::Synth::getNumChildSynths() const
{
	if(dynamic_cast<Chain*>(owner) == nullptr) 
	{
		reportScriptError("getNumChildSynths() can only be called on Chains!");
		FRONTEND_ONLY(return -1;)
	}

	return dynamic_cast<Chain*>(owner)->getHandler()->getNumProcessors();
};

void ScriptingApi::Synth::noteOff(int noteNumber)
{
	jassert(owner != nullptr);

	addNoteOff(1, noteNumber, 0);

#if ENABLE_SCRIPTING_SAFE_CHECKS
	reportScriptError("noteOff is deprecated. Use noteOfByEventId instead");
#endif
}

void ScriptingApi::Synth::noteOffByEventId(int eventId)
{
	noteOffDelayedByEventId(eventId, 0);
}

void ScriptingApi::Synth::noteOffDelayedByEventId(int eventId, int timestamp)
{
	const HiseEvent e = getProcessor()->getMainController()->getEventHandler().popNoteOnFromEventId((uint16)eventId);

	if (!e.isEmpty())
	{
#if ENABLE_SCRIPTING_SAFE_CHECKS
		if (!e.isArtificial())
		{
			reportScriptError("Hell breaks loose if you kill real events artificially!");
		}
#endif
		const HiseEvent* current = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor())->getCurrentHiseEvent();

		if (current != nullptr)
		{
			timestamp += current->getTimeStamp();
		}

		HiseEvent noteOff(HiseEvent::Type::NoteOff, (uint8)e.getNoteNumber(), 1, (uint8)e.getChannel());
		noteOff.setEventId((uint16)eventId);
		noteOff.setTimeStamp((uint16)timestamp);

		if (e.isArtificial()) noteOff.setArtificial();

		ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor());

		if (sp != nullptr)
		{
			sp->addHiseEventToBuffer(noteOff);
		}
	}
	else
	{
		reportScriptError("NoteOn with ID" + String(eventId) + " wasn't found");
	}
}

void ScriptingApi::Synth::addToFront(bool addToFront)
{	
	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->addToFront(addToFront);
}

void ScriptingApi::Synth::deferCallbacks(bool deferCallbacks)
{
	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->deferCallbacks(deferCallbacks);
}

int ScriptingApi::Synth::playNote(int noteNumber, int velocity)
{
	if(velocity == 0)
	{
		reportScriptError("A velocity of 0 is not valid!");
		RETURN_IF_NO_THROW(-1)
	}

	return internalAddNoteOn(1, noteNumber, velocity, 0, 0); // the timestamp will be added from the current event
}


int ScriptingApi::Synth::playNoteWithStartOffset(int channel, int number, int velocity, int offset)
{
	if (velocity == 0)
	{
		reportScriptError("A velocity of 0 is not valid!");
		RETURN_IF_NO_THROW(-1)
	}

	return internalAddNoteOn(channel, number, velocity, 0, offset); // the timestamp will be added from the current event
}

void ScriptingApi::Synth::addVolumeFade(int eventId, int fadeTimeMilliseconds, int targetVolume)
{
	if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor()))
	{
		if (eventId > 0)
		{
			if (fadeTimeMilliseconds >= 0)
			{
				HiseEvent e = HiseEvent::createVolumeFade((uint16)eventId, fadeTimeMilliseconds, (uint8)targetVolume);

				if (const HiseEvent* current = sp->getCurrentHiseEvent())
				{
					e.setTimeStamp(current->getTimeStamp());
				}

				sp->addHiseEventToBuffer(e);
                
                if(targetVolume == -100)
                {
                    HiseEvent no = getProcessor()->getMainController()->getEventHandler().popNoteOnFromEventId((uint16)eventId);
                    
                    if (!no.isEmpty())
                    {
#if ENABLE_SCRIPTING_SAFE_CHECKS
                        if (!no.isArtificial())
                        {
                            reportScriptError("Hell breaks loose if you kill real events artificially!");
                        }
#endif
                        const uint16 timeStampOffset = (uint16)(1.0 + (double)fadeTimeMilliseconds / 1000.0 * getProcessor()->getSampleRate());
                        
                        uint16 timestamp = timeStampOffset;
                        
                        const HiseEvent* current = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor())->getCurrentHiseEvent();
                        
                        if (current != nullptr)
                        {
                            timestamp += current->getTimeStamp();
                        }
                        
                        HiseEvent noteOff(HiseEvent::Type::NoteOff, (uint8)no.getNoteNumber(), 1, (uint8)no.getChannel());
                        noteOff.setEventId((uint16)eventId);
                        noteOff.setTimeStamp(timestamp);
                        noteOff.setArtificial();
                        
                        if (sp != nullptr)
                            sp->addHiseEventToBuffer(noteOff);
                        
                    }
                    else
                    {
                        reportScriptError("NoteOn with ID" + String(eventId) + " wasn't found");
                    }
                }
                
			}
			else reportScriptError("Fade time must be positive");
		}
		else reportScriptError("Event ID must be positive");
	}
	else reportScriptError("Only valid in MidiProcessors");
}

void ScriptingApi::Synth::addPitchFade(int eventId, int fadeTimeMilliseconds, int targetCoarsePitch, int targetFinePitch)
{
	if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor()))
	{
		if (eventId > 0)
		{
			if (fadeTimeMilliseconds >= 0)
			{
				HiseEvent e = HiseEvent::createPitchFade((uint16)eventId, fadeTimeMilliseconds, (uint8)targetCoarsePitch, (uint8)targetFinePitch);
				
				if(sp->getCurrentHiseEvent())
					e.setTimeStamp(sp->getCurrentHiseEvent()->getTimeStamp());

				sp->addHiseEventToBuffer(e);
			}
			else reportScriptError("Fade time must be positive");
		}
		else reportScriptError("Event ID must be positive");
	}
	else reportScriptError("Only valid in MidiProcessors");
}

int ScriptingApi::Synth::addMessageFromHolder(var messageHolder)
{
	if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor()))
	{
		ScriptingObjects::ScriptingMessageHolder* m = dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageHolder.getObject());

		if (m != nullptr)
		{
			HiseEvent e = m->getMessageCopy();

			if (e.getType() != HiseEvent::Type::Empty)
			{
				e.setArtificial();

				if (e.isNoteOn())
				{
					sp->getMainController()->getEventHandler().pushArtificialNoteOn(e);
					sp->addHiseEventToBuffer(e);
					return e.getEventId();
				}
				else if (e.isNoteOff())
				{
					e.setEventId(sp->getMainController()->getEventHandler().getEventIdForNoteOff(e));

					sp->addHiseEventToBuffer(e);
					return e.getTimeStamp();
				}
				else
				{
					sp->addHiseEventToBuffer(e);
					return 0;
				}
			}
			else reportScriptError("Event is empty");
		}
		else reportScriptError("Not a message holder");
	}
	else reportScriptError("Only valid in MidiProcessors");

	return 0;
}

void ScriptingApi::Synth::startTimer(double intervalInSeconds)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(intervalInSeconds < 0.004)
	{
		reportScriptError("Go easy on the timer!");
		return;
	}
#endif

	
	auto p = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor());

	auto jmp = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor());

	if (p == nullptr) return;

	if(jmp != nullptr && jmp->isDeferred())
	{
		owner->stopSynthTimer(p->getIndexInChain());
		jmp->startTimer((int)(intervalInSeconds * 1000));
		p->setIndexInChain(-1);
	}
	else
	{
		int freeTimerSlot = p->getIndexInChain() != -1 ? p->getIndexInChain() : owner->getFreeTimerSlot();

		if (freeTimerSlot == -1)
		{
			reportScriptError("All 4 timers are used");
			return;
		}

		p->setIndexInChain(freeTimerSlot);

		auto* e = p->getCurrentHiseEvent();

		int timestamp = 0;

		if (e != nullptr)
		{
			timestamp = e->getTimeStamp();
		}

		owner->startSynthTimer(p->getIndexInChain(), intervalInSeconds, timestamp);
	}
}

void ScriptingApi::Synth::stopTimer()
{
	auto p = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor());
	auto sbmp = static_cast<ScriptBaseMidiProcessor*>(getScriptProcessor());


	if(p != nullptr && p->isDeferred())
	{
		owner->stopSynthTimer(p->getIndexInChain());
		p->stopTimer();
	}
	else
	{
		if(sbmp != nullptr) owner->stopSynthTimer(sbmp->getIndexInChain());

		sbmp->setIndexInChain(-1);
	}
}

bool ScriptingApi::Synth::isTimerRunning() const
{
	const JavascriptMidiProcessor *p = dynamic_cast<const JavascriptMidiProcessor*>(getScriptProcessor());

	if (p != nullptr && p->isDeferred())
	{
		return p->isTimerRunning();

	}
	else
	{
		if (p != nullptr) return owner->getTimerInterval(p->getIndexInChain()) != 0.0;
		else return false;
	}
}

double ScriptingApi::Synth::getTimerInterval() const
{
	const JavascriptMidiProcessor *p = dynamic_cast<const JavascriptMidiProcessor*>(getScriptProcessor());

	if (p != nullptr && p->isDeferred())
	{
		return (double)p->getTimerInterval() / 1000.0;
	}
	else
	{
		if (p != nullptr) return owner->getTimerInterval(p->getIndexInChain());
		else return 0.0;
	}
}

void ScriptingApi::Synth::sendController(int controllerNumber, int controllerValue)
{
	if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor()))
	{
		if (controllerNumber > 0)
		{
			if (controllerValue >= 0)
			{
                HiseEvent e;
                
                if(controllerNumber == 129)
                {
                    e = HiseEvent(HiseEvent::Type::PitchBend, 0, 0);
                    e.setPitchWheelValue(controllerValue);
                }
                else
                {
                    e = HiseEvent(HiseEvent::Type::Controller, (uint8)controllerNumber, (uint8)controllerValue);
                }
                
				
				if (const HiseEvent* current = sp->getCurrentHiseEvent())
				{
					e.setTimeStamp(current->getTimeStamp());
				}

				sp->addHiseEventToBuffer(e);
			}
			else reportScriptError("CC value must be positive");
		}
		else reportScriptError("CC number must be positive");
	}
	else reportScriptError("Only valid in MidiProcessors");
};

void ScriptingApi::Synth::sendControllerToChildSynths(int controllerNumber, int controllerValue)
{
	sendController(controllerNumber, controllerValue);
};



void ScriptingApi::Synth::setMacroControl(int macroIndex, float newValue)
{
	if(ModulatorSynthChain *chain = dynamic_cast<ModulatorSynthChain*>(owner))
	{
		if(macroIndex > 0 && macroIndex < 8)
		{
			chain->setMacroControl(macroIndex - 1, newValue, sendNotification);
		}
		else reportScriptError("macroIndex must be between 1 and 8!");
	}
	else
	{
		reportScriptError("setMacroControl() can only be called on ModulatorSynthChains");
	}
}

ScriptingObjects::ScriptingModulator *ScriptingApi::Synth::getModulator(const String &name)
{
	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<Modulator> it(owner);

		Modulator *m;
		
		while((m = it.getNextProcessor()) != 0)
		{
			if(m->getId() == name)
			{
				return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), m);	
			}
		}

		reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingModulator(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getModulator()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingModulator(getScriptProcessor(), nullptr))
	}	
}


ScriptingObjects::ScriptingMidiProcessor *ScriptingApi::Synth::getMidiProcessor(const String &name)
{
	if(name == getProcessor()->getId())
	{
		reportScriptError("You can't get a reference to yourself!");
	}

	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<MidiProcessor> it(owner);

		MidiProcessor *mp;
		
		while((mp = it.getNextProcessor()) != nullptr)
		{
			if(mp->getId() == name)
			{
				return new ScriptingObjects::ScriptingMidiProcessor(getScriptProcessor(), mp);	
			}
		}

        reportScriptError(name + " was not found. ");
        
		RETURN_IF_NO_THROW(new ScriptMidiProcessor(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getMidiProcessor()", "onInit");

		RETURN_IF_NO_THROW(new ScriptMidiProcessor(getScriptProcessor(), nullptr))

	}	
}


ScriptingObjects::ScriptingSynth *ScriptingApi::Synth::getChildSynth(const String &name)
{
	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ModulatorSynth> it(owner);

		ModulatorSynth *m;
		

		while((m = it.getNextProcessor()) != 0)
		{
			if(m->getId() == name)
			{
				return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), m);	
			}
		}
        
        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getChildSynth()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))

	}	
}

ScriptingApi::Synth::ScriptSynth* ScriptingApi::Synth::getChildSynthByIndex(int index)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		if (Chain* c = dynamic_cast<Chain*>(owner))
		{
			if (index >= 0 && index < c->getHandler()->getNumProcessors())
			{
				return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), dynamic_cast<ModulatorSynth*>(c->getHandler()->getProcessor(index)));
			}
		}

		return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getChildSynth()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))
	}
}

var ScriptingApi::Synth::getIdList(const String &type)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<Processor> it(owner);

		Array<var> idList;

		while (Processor* p = it.getNextProcessor())
		{
			// don't include selfie-boy
			if (dynamic_cast<Processor*>(getScriptProcessor()) == p)
				continue;

			if (p->getName() == type)
				idList.add(p->getId());
		}

		return var(idList);
	}
	else
	{
		return var();
	}
}

ScriptingObjects::ScriptingEffect *ScriptingApi::Synth::getEffect(const String &name)
{
	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<EffectProcessor> it(owner);

		EffectProcessor *fx;
		

		while((fx = it.getNextProcessor()) != nullptr)
		{
			if(fx->getId() == name)
			{
				return new ScriptEffect(getScriptProcessor(), fx);	
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptEffect(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getEffect()", "onInit");
		RETURN_IF_NO_THROW(new ScriptEffect(getScriptProcessor(), nullptr))
	}	
}

ScriptingObjects::ScriptingAudioSampleProcessor * ScriptingApi::Synth::getAudioSampleProcessor(const String &name)
{
	Processor::Iterator<AudioSampleProcessor> it(owner);

		AudioSampleProcessor *asp;


		while ((asp = it.getNextProcessor()) != nullptr)
		{
			if (dynamic_cast<Processor*>(asp)->getId() == name)
			{
				return new ScriptAudioSampleProcessor(getScriptProcessor(), asp);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptAudioSampleProcessor(getScriptProcessor(), nullptr))
}



ScriptingObjects::ScriptingTableProcessor *ScriptingApi::Synth::getTableProcessor(const String &name)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<LookupTableProcessor> it(owner);

		while (LookupTableProcessor *lut = it.getNextProcessor())
		{
			if (dynamic_cast<Processor*>(lut)->getId() == name)
			{
				
				return new ScriptTableProcessor(getScriptProcessor(), lut);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptTableProcessor(getScriptProcessor(), nullptr));
	}
	else
	{
		reportIllegalCall("getScriptingTableProcessor()", "onInit");
		RETURN_IF_NO_THROW(new ScriptTableProcessor(getScriptProcessor(), nullptr));
	}
}

ScriptingApi::Sampler * ScriptingApi::Synth::getSampler(const String &name)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ModulatorSampler> it(owner);

		while (ModulatorSampler *s = it.getNextProcessor())
		{
			if (s->getId() == name)
			{
				
				return new Sampler(getScriptProcessor(), s);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new Sampler(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getScriptingAudioSampleProcessor()", "onInit");
		RETURN_IF_NO_THROW(new Sampler(getScriptProcessor(), nullptr))
	}
}

ScriptingApi::Synth::ScriptSlotFX* ScriptingApi::Synth::getSlotFX(const String& name)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<SlotFX> it(owner);

		while (SlotFX *s = it.getNextProcessor())
		{
			if (s->getId() == name)
			{

				return new ScriptSlotFX(getScriptProcessor(), s);
			}
		}

		reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptSlotFX(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getScriptingAudioSampleProcessor()", "onInit");
		RETURN_IF_NO_THROW(new ScriptSlotFX(getScriptProcessor(), nullptr))
	}
}

void ScriptingApi::Synth::setAttribute(int attributeIndex, float newAttribute)
{
	if(owner == nullptr)
	{
		jassertfalse;
		return;
	}

	owner->setAttribute(attributeIndex, newAttribute, sendNotification);
}

void ScriptingApi::Synth::setVoiceGainValue(int voiceIndex, float gainValue)
{
	if (owner == nullptr)
	{
		jassertfalse;
		return;
	}

	owner->setScriptGainValue(voiceIndex, gainValue);
}

void ScriptingApi::Synth::setVoicePitchValue(int voiceIndex, double pitchValue)
{
	if (owner == nullptr)
	{
		jassertfalse;
		return;
	}

	owner->setScriptPitchValue(voiceIndex, pitchValue);
}

float ScriptingApi::Synth::getAttribute(int attributeIndex) const
{
	if (owner == nullptr)
	{
		jassertfalse;
		return -1.0f;
	}

	return owner->getAttribute(attributeIndex);
}

int ScriptingApi::Synth::internalAddNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples, int startOffset)
{
	if (channel > 0 && channel <= 16)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			if (velocity >= 0 && velocity <= 127)
			{
				if (timeStampSamples >= 0)
				{
					if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor()))
					{
						HiseEvent m = HiseEvent(HiseEvent::Type::NoteOn, (uint8)noteNumber, (uint8)velocity, (uint8)channel);

						if (sp->getCurrentHiseEvent() != nullptr)
						{
							m.setTimeStamp((uint16)sp->getCurrentHiseEvent()->getTimeStamp() + (uint16)timeStampSamples);
						}
						else
						{
							m.setTimeStamp((uint16)timeStampSamples);
						}

						if (startOffset > UINT16_MAX)
							reportScriptError("Max start offset is 65536 (2^16)");

						m.setStartOffset((uint16)startOffset);

						m.setArtificial();

						sp->getMainController()->getEventHandler().pushArtificialNoteOn(m);
						sp->addHiseEventToBuffer(m);

						return m.getEventId();
					}
					else reportScriptError("Only valid in MidiProcessors");
				}
				else reportScriptError("Timestamp must be >= 0");
			}
			else reportScriptError("Velocity must be between 0 and 127");
		}
		else reportScriptError("Note number must be between 0 and 127");
	}
	else reportScriptError("Channel must be between 1 and 16.");

	RETURN_IF_NO_THROW(-1)
}

int ScriptingApi::Synth::addNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples)
{
	return internalAddNoteOn(channel, noteNumber, velocity, timeStampSamples, 0);
}

void ScriptingApi::Synth::addNoteOff(int channel, int noteNumber, int timeStampSamples)
{
	if (channel > 0 && channel <= 16)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			if (timeStampSamples >= 0)
			{
				if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor()))
				{
					timeStampSamples = jmax<int>(1, timeStampSamples);

					HiseEvent m = HiseEvent(HiseEvent::Type::NoteOff, (uint8)noteNumber, 127, (uint8)channel);

					if (sp->getCurrentHiseEvent() != nullptr)
					{
						m.setTimeStamp((uint16)sp->getCurrentHiseEvent()->getTimeStamp() + (uint16)timeStampSamples);
					}
					else
					{
						m.setTimeStamp((uint16)timeStampSamples);
					}

					m.setArtificial();

					const uint16 eventId = sp->getMainController()->getEventHandler().getEventIdForNoteOff(m);

					m.setEventId(eventId);

					sp->addHiseEventToBuffer(m);

				}
			}
			else reportScriptError("Timestamp must be > 0");
		}
		else reportScriptError("Note number must be between 0 and 127");
	}
	else reportScriptError("Channel must be between 1 and 16.");
}

void ScriptingApi::Synth::addController(int channel, int number, int value, int timeStampSamples)
{
	if (channel > 0 && channel <= 16)
	{
		if (number >= 0 && number <= 127)
		{
			if (value >= 0 && value <= 127)
			{
				if (timeStampSamples >= 0)
				{
					if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor()))
					{
						HiseEvent m = HiseEvent(HiseEvent::Type::Controller, (uint8)number, (uint8)value, (uint8)channel);
						
						if (sp->getCurrentHiseEvent() != nullptr)
						{
							m.setTimeStamp((uint16)sp->getCurrentHiseEvent()->getTimeStamp() + (uint16)timeStampSamples);
						}
						else
						{
							m.setTimeStamp((uint16)timeStampSamples);
						}

						m.setArtificial();

						sp->addHiseEventToBuffer(m);
					}
					
				}
				else reportScriptError("Timestamp must be > 0");
			}
			else reportScriptError("CC Value must be between 0 and 127");
		}
		else reportScriptError("CC number must be between 0 and 127");
	}
	else reportScriptError("Channel must be between 1 and 16.");
}

void ScriptingApi::Synth::setClockSpeed(int clockSpeed)
{
	switch (clockSpeed)
	{
	case 0:	 owner->setClockSpeed(ModulatorSynth::Inactive); break;
	case 1:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Bar); break;
	case 2:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Half); break;
	case 4:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Quarters); break;
	case 8:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Eighths); break;
	case 16: owner->setClockSpeed(ModulatorSynth::ClockSpeed::Sixteens); break;
	case 32: owner->setClockSpeed(ModulatorSynth::ThirtyTwos); break;
	default: reportScriptError("Unknown clockspeed. Use 1,2,4,8,16 or 32");
	}
}

void ScriptingApi::Synth::setShouldKillRetriggeredNote(bool killNote)
{
	if (owner != nullptr)
	{
		owner->setKillRetriggeredNote(killNote);
	}
}

var ScriptingApi::Synth::getAllModulators(String regex)
{
	Processor::Iterator<Modulator> iter(owner->getMainController()->getMainSynthChain());

	Array<var> list;

	while (auto m = iter.getNextProcessor())
	{
		if (RegexFunctions::matchesWildcard(regex, m->getId(), owner->getMainController()->getMainSynthChain()))
		{
			auto sm = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), m);
			var smv(sm);

			list.add(smv);
		}
	}

	return var(list);
}

void ScriptingApi::Synth::setModulatorAttribute(int chain, int modulatorIndex, int attributeIndex, float newValue)
{
	if(owner == nullptr)
	{
		jassertfalse;
		return;
	}

	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return;
	}

	Processor *modulator = c->getHandler()->getProcessor(modulatorIndex);

	if(modulator == nullptr)
	{
		String errorMessage;
		errorMessage << "No Modulator found in " << (chain == 1 ? "GainModulation" : "PitchModulation") << " at index " << String(modulatorIndex);
		reportScriptError(errorMessage);
		return;
	}

	if(attributeIndex == -12)
	{
		if (chain == ModulatorSynth::PitchModulation) 
		{
			const float pitchValue = jlimit(0.5f, 2.0f, powf(2, newValue/12.0f));

			dynamic_cast<Modulation*>(modulator)->setIntensity(pitchValue);
		}
		else
		{
			dynamic_cast<Modulation*>(modulator)->setIntensity(newValue);
			
		}
	}
	else if(attributeIndex == -13) modulator->setBypassed(newValue == 1.0f);
	
	else modulator->setAttribute(attributeIndex, newValue, dontSendNotification);

	modulator->sendChangeMessage();

	
}


ScriptingObjects::ScriptingModulator* ScriptingApi::Synth::addModulator(int chain, const String &type, const String &id)
{
	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return nullptr;
	}

	Processor* p = moduleHandler.addModule(c, type, id, -1);

	return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
}

bool ScriptingApi::Synth::removeModulator(var mod)
{
	if (auto m = dynamic_cast<ScriptingObjects::ScriptingModulator*>(mod.getObject()))
	{
		Modulator* modToRemove = m->getModulator();

		return moduleHandler.removeModule(modToRemove);
	}

	return false;
}

ScriptingApi::Synth::ScriptEffect* ScriptingApi::Synth::addEffect(const String &type, const String &id, int index)
{
	EffectProcessorChain* c = owner->effectChain;
	Processor* p = moduleHandler.addModule(c, type, id, index);

	return new ScriptingObjects::ScriptingEffect(getScriptProcessor(), dynamic_cast<EffectProcessor*>(p));
}

bool ScriptingApi::Synth::removeEffect(var effect)
{
	if (auto fx = dynamic_cast<ScriptingObjects::ScriptingEffect*>(effect.getObject()))
	{
		EffectProcessor* effectToRemove = fx->getEffect();
		return moduleHandler.removeModule(effectToRemove);
	}

	return false;
}

int ScriptingApi::Synth::getModulatorIndex(int chain, const String &id) const
{
	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return -1;
	}

	for(int i = 0; i < c->getHandler()->getNumProcessors(); i++)
	{
		if(c->getHandler()->getProcessor(i)->getId() == id) return i;
	}

	reportScriptError("Modulator " + id + " was not found in " + c->getId());

	RETURN_IF_NO_THROW(-1)
}

// ====================================================================================================== Console functions

struct ScriptingApi::Console::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Console, print);
	API_VOID_METHOD_WRAPPER_0(Console, start);
	API_VOID_METHOD_WRAPPER_0(Console, stop);
	API_VOID_METHOD_WRAPPER_0(Console, clear);
	API_VOID_METHOD_WRAPPER_1(Console, assertTrue);
	API_VOID_METHOD_WRAPPER_2(Console, assertEqual);
	API_VOID_METHOD_WRAPPER_1(Console, assertIsDefined);
	API_VOID_METHOD_WRAPPER_1(Console, assertIsObjectOrArray);
	API_VOID_METHOD_WRAPPER_1(Console, assertLegalNumber);
};

ScriptingApi::Console::Console(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
startTime(0.0)
{
	ADD_API_METHOD_1(print);
	ADD_API_METHOD_0(start);
	ADD_API_METHOD_0(stop);
	ADD_API_METHOD_0(clear);

	ADD_API_METHOD_1(assertTrue);
	ADD_API_METHOD_2(assertEqual);
	ADD_API_METHOD_1(assertIsDefined);
	ADD_API_METHOD_1(assertIsObjectOrArray);
	ADD_API_METHOD_1(assertLegalNumber);
}



void ScriptingApi::Console::print(var x)
{
#if USE_BACKEND
	debugToConsole(getProcessor(), x);
#endif
}

void ScriptingApi::Console::stop()
{
#if USE_BACKEND
	if(startTime == 0.0)
	{
		reportScriptError("The Benchmark was not started!");
		return;
	}

	const double now = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
	const double ms = (now - startTime) * 1000.0;
	startTime = 0.0;

	debugToConsole(getProcessor(), "Benchmark Result: " + String(ms, 3) + " ms");
#endif
}





void ScriptingApi::Console::clear()
{
	getProcessor()->getMainController()->getConsoleHandler().clearConsole();
}



void ScriptingApi::Console::assertTrue(var condition)
{
	if (!(bool)condition)
		reportScriptError("Assertion failure: condition is false");
}

void ScriptingApi::Console::assertEqual(var v1, var v2)
{
	if (v1 != v2)
		reportScriptError("Assertion failure: values are unequal");
}

void ScriptingApi::Console::assertIsDefined(var v1)
{
	if (v1.isUndefined() || v1.isVoid())
		reportScriptError("Assertion failure: value is undefined");
}

struct VarTypeHelpers
{
	static bool isFunction(const var& v) noexcept
	{
		return dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*> (v.getObject()) != nullptr;
	}

	static bool isNumeric(const var& v) noexcept
	{
		return v.isInt() || v.isDouble() || v.isInt64() || v.isBool();
	}

	static String getVarType(var v)
	{
		if (v.isVoid())                      return "void";
		if (v.isString())                    return "string";
		if (isNumeric(v))                   return "number";
		if (isFunction(v) || v.isMethod())  return "function";
		if (v.isObject())                    return "object";

		return "undefined";
	}
};



void ScriptingApi::Console::assertIsObjectOrArray(var value)
{
	if (!(value.isObject() || value.isArray()))
	{
		reportScriptError("Assertion failure: value is not object or array. Type: " + VarTypeHelpers::getVarType(value));
	}
}

void ScriptingApi::Console::assertLegalNumber(var value)
{
	if (!VarTypeHelpers::isNumeric(value))
	{
		reportScriptError("Assertion failure: value is not a number. Type: " + VarTypeHelpers::getVarType(value) + " Value: " + value.toString());
	}

	const float v1 = (float)value;
	float v2 = v1;


	if (v1 != FloatSanitizers::sanitizeFloatNumber(v2))
	{
		reportScriptError("Assertion failure: value is not a legal number. Value: " + value.toString());
	}
}

#undef SEND_MESSAGE
#undef ADD_TO_TYPE_SELECTOR
#undef ADD_AS_SLIDER_TYPE

ScriptingApi::ModulatorApi::ModulatorApi(Modulator* mod_) :
ApiClass(0),
mod(mod_),
m(dynamic_cast<Modulation*>(mod_))
{
	ADD_API_METHOD_1(setIntensity);
	ADD_API_METHOD_1(setBypassed);
}

struct ScriptingApi::Colours::Wrapper
{
	API_METHOD_WRAPPER_2(Colours, withAlpha);
};

ScriptingApi::Colours::Colours() :
ApiClass(139)
{
	addConstant("transparentBlack", (int64)0);
	addConstant("transparentWhite", (int64)0x00ffffff);
	addConstant("aliceblue", (int64)0xfff0f8ff);
	addConstant("antiquewhite", (int64)0xfffaebd7);
	addConstant("aqua", (int64)0xff00ffff);
	addConstant("aquamarine", (int64)0xff7fffd4);
	addConstant("azure", (int64)0xfff0ffff);
	addConstant("beige", (int64)0xfff5f5dc);
	addConstant("bisque", (int64)0xffffe4c4);
	addConstant("black", (int64)0xff000000);
	addConstant("blanchedalmond", (int64)0xffffebcd);
	addConstant("blue", (int64)0xff0000ff);
	addConstant("blueviolet", (int64)0xff8a2be2);
	addConstant("brown", (int64)0xffa52a2a);
	addConstant("burlywood", (int64)0xffdeb887);
	addConstant("cadetblue", (int64)0xff5f9ea0);
	addConstant("chartreuse", (int64)0xff7fff00);
	addConstant("chocolate", (int64)0xffd2691e);
	addConstant("coral", (int64)0xffff7f50);
	addConstant("cornflowerblue", (int64)0xff6495ed);
	addConstant("cornsilk", (int64)0xfffff8dc);
	addConstant("crimson", (int64)0xffdc143c);
	addConstant("cyan", (int64)0xff00ffff);
	addConstant("darkblue", (int64)0xff00008b);
	addConstant("darkcyan", (int64)0xff008b8b);
	addConstant("darkgoldenrod", (int64)0xffb8860b);
	addConstant("darkgrey", (int64)0xff555555);
	addConstant("darkgreen", (int64)0xff006400);
	addConstant("darkkhaki", (int64)0xffbdb76b);
	addConstant("darkmagenta", (int64)0xff8b008b);
	addConstant("darkolivegreen", (int64)0xff556b2f);
	addConstant("darkorange", (int64)0xffff8c00);
	addConstant("darkorchid", (int64)0xff9932cc);
	addConstant("darkred", (int64)0xff8b0000);
	addConstant("darksalmon", (int64)0xffe9967a);
	addConstant("darkseagreen", (int64)0xff8fbc8f);
	addConstant("darkslateblue", (int64)0xff483d8b);
	addConstant("darkslategrey", (int64)0xff2f4f4f);
	addConstant("darkturquoise", (int64)0xff00ced1);
	addConstant("darkviolet", (int64)0xff9400d3);
	addConstant("deeppink", (int64)0xffff1493);
	addConstant("deepskyblue", (int64)0xff00bfff);
	addConstant("dimgrey", (int64)0xff696969);
	addConstant("dodgerblue", (int64)0xff1e90ff);
	addConstant("firebrick", (int64)0xffb22222);
	addConstant("floralwhite", (int64)0xfffffaf0);
	addConstant("forestgreen", (int64)0xff228b22);
	addConstant("fuchsia", (int64)0xffff00ff);
	addConstant("gainsboro", (int64)0xffdcdcdc);
	addConstant("gold", (int64)0xffffd700);
	addConstant("goldenrod", (int64)0xffdaa520);
	addConstant("grey", (int64)0xff808080);
	addConstant("green", (int64)0xff008000);
	addConstant("greenyellow", (int64)0xffadff2f);
	addConstant("honeydew", (int64)0xfff0fff0);
	addConstant("hotpink", (int64)0xffff69b4);
	addConstant("indianred", (int64)0xffcd5c5c);
	addConstant("indigo", (int64)0xff4b0082);
	addConstant("ivory", (int64)0xfffffff0);
	addConstant("khaki", (int64)0xfff0e68c);
	addConstant("lavender", (int64)0xffe6e6fa);
	addConstant("lavenderblush", (int64)0xfffff0f5);
	addConstant("lemonchiffon", (int64)0xfffffacd);
	addConstant("lightblue", (int64)0xffadd8e6);
	addConstant("lightcoral", (int64)0xfff08080);
	addConstant("lightcyan", (int64)0xffe0ffff);
	addConstant("lightgoldenrodyellow", (int64)0xfffafad2);
	addConstant("lightgreen", (int64)0xff90ee90);
	addConstant("lightgrey", (int64)0xffd3d3d3);
	addConstant("lightpink", (int64)0xffffb6c1);
	addConstant("lightsalmon", (int64)0xffffa07a);
	addConstant("lightseagreen", (int64)0xff20b2aa);
	addConstant("lightskyblue", (int64)0xff87cefa);
	addConstant("lightslategrey", (int64)0xff778899);
	addConstant("lightsteelblue", (int64)0xffb0c4de);
	addConstant("lightyellow", (int64)0xffffffe0);
	addConstant("lime", (int64)0xff00ff00);
	addConstant("limegreen", (int64)0xff32cd32);
	addConstant("linen", (int64)0xfffaf0e6);
	addConstant("magenta", (int64)0xffff00ff);
	addConstant("maroon", (int64)0xff800000);
	addConstant("mediumaquamarine", (int64)0xff66cdaa);
	addConstant("mediumblue", (int64)0xff0000cd);
	addConstant("mediumorchid", (int64)0xffba55d3);
	addConstant("mediumpurple", (int64)0xff9370db);
	addConstant("mediumseagreen", (int64)0xff3cb371);
	addConstant("mediumslateblue", (int64)0xff7b68ee);
	addConstant("mediumspringgreen", (int64)0xff00fa9a);
	addConstant("mediumturquoise", (int64)0xff48d1cc);
	addConstant("mediumvioletred", (int64)0xffc71585);
	addConstant("midnightblue", (int64)0xff191970);
	addConstant("mintcream", (int64)0xfff5fffa);
	addConstant("mistyrose", (int64)0xffffe4e1);
	addConstant("navajowhite", (int64)0xffffdead);
	addConstant("navy", (int64)0xff000080);
	addConstant("oldlace", (int64)0xfffdf5e6);
	addConstant("olive", (int64)0xff808000);
	addConstant("olivedrab", (int64)0xff6b8e23);
	addConstant("orange", (int64)0xffffa500);
	addConstant("orangered", (int64)0xffff4500);
	addConstant("orchid", (int64)0xffda70d6);
	addConstant("palegoldenrod", (int64)0xffeee8aa);
	addConstant("palegreen", (int64)0xff98fb98);
	addConstant("paleturquoise", (int64)0xffafeeee);
	addConstant("palevioletred", (int64)0xffdb7093);
	addConstant("papayawhip", (int64)0xffffefd5);
	addConstant("peachpuff", (int64)0xffffdab9);
	addConstant("peru", (int64)0xffcd853f);
	addConstant("pink", (int64)0xffffc0cb);
	addConstant("plum", (int64)0xffdda0dd);
	addConstant("powderblue", (int64)0xffb0e0e6);
	addConstant("purple", (int64)0xff800080);
	addConstant("red", (int64)0xffff0000);
	addConstant("rosybrown", (int64)0xffbc8f8f);
	addConstant("royalblue", (int64)0xff4169e1);
	addConstant("saddlebrown", (int64)0xff8b4513);
	addConstant("salmon", (int64)0xfffa8072);
	addConstant("sandybrown", (int64)0xfff4a460);
	addConstant("seagreen", (int64)0xff2e8b57);
	addConstant("seashell", (int64)0xfffff5ee);
	addConstant("sienna", (int64)0xffa0522d);
	addConstant("silver", (int64)0xffc0c0c0);
	addConstant("skyblue", (int64)0xff87ceeb);
	addConstant("slateblue", (int64)0xff6a5acd);
	addConstant("slategrey", (int64)0xff708090);
	addConstant("snow", (int64)0xfffffafa);
	addConstant("springgreen", (int64)0xff00ff7f);
	addConstant("steelblue", (int64)0xff4682b4);
	addConstant("tan", (int64)0xffd2b48c);
	addConstant("teal", (int64)0xff008080);
	addConstant("thistle", (int64)0xffd8bfd8);
	addConstant("tomato", (int64)0xffff6347);
	addConstant("turquoise", (int64)0xff40e0d0);
	addConstant("violet", (int64)0xffee82ee);
	addConstant("wheat", (int64)0xfff5deb3);
	addConstant("white", (int64)0xffffffff);
	addConstant("whitesmoke", (int64)0xfff5f5f5);
	addConstant("yellow", (int64)0xffffff00);
	addConstant("yellowgreen", (int64)0xff9acd32);

	ADD_API_METHOD_2(withAlpha);
}

int ScriptingApi::Colours::withAlpha(int colour, float alpha)
{
	Colour c((uint32)colour);

	return (int)c.withAlpha(alpha).getARGB();
}

ScriptingApi::ModuleIds::ModuleIds(ModulatorSynth* s):
	ApiClass(getTypeList(s).size()),
	ownerSynth(s)
{
	auto list = getTypeList(ownerSynth);

	list.sort();

	for (int i = 0; i < list.size(); i++)
	{
		addConstant(list[i].toString(), list[i].toString());
	}
}

Array<Identifier> ScriptingApi::ModuleIds::getTypeList(ModulatorSynth* s)
{
	Array<Identifier> ids;
	
	for (int i = 0; i < s->getNumInternalChains(); i++)
	{
		Chain* c = dynamic_cast<Chain*>(s->getChildProcessor(i));

		

		jassert(c != nullptr);

		if (c != nullptr)
		{
			FactoryType* t = c->getFactoryType();
			auto list = t->getAllowedTypes();
			
			for (int j = 0; j < list.size(); j++)
			{
				ids.addIfNotAlreadyThere(list[j].type);
			}
		}
	}

	return ids;
}

} // namespace hise
