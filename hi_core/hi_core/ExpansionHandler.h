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

#ifndef EXPANSION_HANDLER_H_INCLUDED
#define EXPANSION_HANDLER_H_INCLUDED

namespace hise {
using namespace juce;

class FloatingTile;

#define DECLARE_ID(x) static const Identifier x(#x);

namespace ExpansionIds
{
DECLARE_ID(ExpansionInfo);
DECLARE_ID(PrivateInfo);
DECLARE_ID(Name);
DECLARE_ID(ProjectName);
DECLARE_ID(Version);
DECLARE_ID(Key);
DECLARE_ID(PoolData);
DECLARE_ID(Data);
}

#undef DECLARE_ID

class Expansion: public FileHandlerBase
{
public:

	enum ExpansionType
	{
		FileBased,
		Intermediate,
		Encrypted,
		numExpansionType
	};

	Expansion(MainController* mc, const File& expansionFolder) :
		FileHandlerBase(mc),
		root(expansionFolder)
	{
		afm.registerBasicFormats();
		afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
	}

	~Expansion()
	{
		saveExpansionInfoFile();
	}

	

	/** Override this method and initialise the expansion. You need to do at least those things:
	
		- create a Data object
	    - setup the pools
	*/
	virtual void initialise() 
	{
		addMissingFolders();
		pool->getSampleMapPool().loadAllFilesFromProjectFolder();

		data = new Data(root, Helpers::loadValueTreeForFileBasedExpansion(root));

		saveExpansionInfoFile();
	};

	virtual void encodeExpansion()
	{
		jassertfalse;
	}

	virtual ExpansionType getExpansionType() const { return ExpansionType::FileBased; }

	File getRootFolder() const override { return root; }

	virtual PoolReference createReferenceForFile(const String& relativePath, FileHandlerBase::SubDirectories fileType);

	PooledAudioFile loadAudioFile(const PoolReference& audioFileId);

	PooledImage loadImageFile(const PoolReference& imageId);

	PooledAdditionalData loadAdditionalData(const String& relativePath);

#if 0
	ValueTree getSampleMap(const String& sampleMapId)
	{
		if (isEncrypted)
		{
			jassertfalse;
			return ValueTree();
		}
		else
		{
			Array<File> sampleMapFiles;
			getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

			String expStart = "{EXP::" + name.get() + "}";

			for (auto f : sampleMapFiles)
			{
				String thisId = expStart + f.getFileNameWithoutExtension();

				if (thisId == sampleMapId)
				{
					ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

					if (xml != nullptr)
					{
						return ValueTree::fromXml(*xml);
					}
				}
			}

			throw String("!Sample Map with id " + sampleMapId.fromFirstOccurrenceOf("}", false, false) + " not found");
		}
	}
#endif

	bool isActive() const noexcept { return numActiveReferences != 0; }

	void incActiveRefCount() { numActiveReferences++; }
	void decActiveRefCount() 
	{
		jassert(numActiveReferences > 0);

		numActiveReferences = jmax(0, numActiveReferences - 1);
	}

	String getProperty(const Identifier& id) const
	{
		if(data != nullptr)
			return data->v.getProperty(id).toString();

		jassertfalse;
		return {};

	}

protected:

	File root;

	struct Data
	{
		

		Data(const File& root, ValueTree expansionInfo) :
			v(expansionInfo),
			name(v, "Name", nullptr, root.getFileNameWithoutExtension()),
#if USE_BACKEND
			projectName(v, "ProjectName", nullptr, "unused"),
			projectVersion(v, "ProjectName", nullptr, "1.0.0"),
#else
			projectName(v, "ProjectName", nullptr, FrontendHandler::getProjectName()),
			projectVersion(v, "ProjectName", nullptr, FrontendHandler::getVersionString()),
#endif
			version(v, "Version", nullptr, "1.0.0")
		{
			Helpers::initCachedValue(v, name);
			Helpers::initCachedValue(v, version);
			Helpers::initCachedValue(v, projectName);
			Helpers::initCachedValue(v, projectVersion);
		}

		virtual ~Data() {};

		ValueTree v;

		CachedValue<String> name;
		CachedValue<String> projectName;
		CachedValue<String> version;
		CachedValue<String> projectVersion;
	};

	ScopedPointer<Data> data;

	int numActiveReferences = 0;

	AudioFormatManager afm;

	Array<SubDirectories> getListOfPooledSubDirectories()
	{
		Array<SubDirectories> sub;
		sub.add(AdditionalSourceCode);
		sub.add(AudioFiles);
		sub.add(Images);
		sub.add(MidiFiles);
		sub.add(SampleMaps);
		
		return sub;
	}

	void saveExpansionInfoFile()
	{
		if (Helpers::getExpansionInfoFile(root, Intermediate).existsAsFile() ||
			Helpers::getExpansionInfoFile(root, Encrypted).existsAsFile())
			return;

		auto file = Helpers::getExpansionInfoFile(root, FileBased);

		file.replaceWithText(data->v.toXmlString());
	}

	void addMissingFolders()
	{
		addFolder(ProjectHandler::SubDirectories::Samples);

		if (getExpansionType() != FileBased)
			return;

		addFolder(ProjectHandler::SubDirectories::AdditionalSourceCode);
		addFolder(ProjectHandler::SubDirectories::AudioFiles);
		addFolder(ProjectHandler::SubDirectories::Images);
		addFolder(ProjectHandler::SubDirectories::SampleMaps);
		addFolder(ProjectHandler::SubDirectories::MidiFiles);
		addFolder(ProjectHandler::SubDirectories::UserPresets);
	}

	void addFolder(ProjectHandler::SubDirectories directory)
	{
		jassert(getExpansionType() == FileBased || directory == Samples);

		auto d = root.getChildFile(ProjectHandler::getIdentifier(directory));

		subDirectories.add({ directory, false, d });

		if (!d.isDirectory())
			d.createDirectory();
	}

	String getWildcard() const
	{
		String s;
		s << "{EXP::" << getProperty(ExpansionIds::Name) << "}";
		return s;
	}

	struct Helpers
	{


		static ValueTree loadValueTreeForFileBasedExpansion(const File& root)
		{
			auto infoFile = Helpers::getExpansionInfoFile(root, FileBased);

			if (infoFile.existsAsFile())
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(infoFile);

				if (xml != nullptr)
				{
					auto tree = ValueTree::fromXml(*xml);
					return tree;
				}
			}

			return ValueTree("ExpansionInfo");
		};

		template <class T> static void initCachedValue(ValueTree v, const T& cachedValue)
		{
			if (!v.hasProperty(cachedValue.getPropertyID()))
			{
				v.setProperty(cachedValue.getPropertyID(), cachedValue.getDefault(), nullptr);
			}
		}

		static File getExpansionInfoFile(const File& expansionRoot, ExpansionType type)
		{
			if (type == Encrypted)		   return expansionRoot.getChildFile("info.hxp");
			else if (type == Intermediate) return expansionRoot.getChildFile("info.hxi");
			else						   return expansionRoot.getChildFile("expansion_info.xml");
		}

		static String getExpansionIdFromReference(const String& referenceId);

	};

	friend class ExpansionHandler;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(Expansion)

};

class ExpansionHandler
{
public:

	struct Reference
	{
		String referenceString;
	};

	class Listener
	{
	public:

		virtual ~Listener()
		{}

		virtual void expansionPackCreated(Expansion* newExpansion) { expansionPackLoaded(newExpansion); };
		virtual void expansionPackLoaded(Expansion* currentExpansion) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	ExpansionHandler(MainController* mc);

	struct Helpers
	{
		static void createFrontendLayoutWithExpansionEditing(FloatingTile* root, bool putInTabWithMainInterface=true);
		static bool isValidExpansion(const File& directory);
		static Identifier getSanitizedIdentifier(const String& idToSanitize);
	};

	void createNewExpansion(const File& expansionFolder);
	File getExpansionFolder() const;
	void createAvailableExpansions();

	Expansion* createExpansionForFile(const File& f);

	var getListOfAvailableExpansions() const;

	bool setCurrentExpansion(const String& expansionName);

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	PooledAudioFile loadAudioFileReference(const PoolReference& sampleId);

	double getSampleRateForFileReference(const PoolReference& sampleId);

	const var getMetadata(const PoolReference& sampleId);



	PooledImage loadImageReference(const PoolReference& imageId, PoolHelpers::LoadingType loadingType = PoolHelpers::LoadAndCacheWeak);

	PooledSampleMap loadSampleMap(const PoolReference& sampleMapId);

	bool isActive() const { return currentExpansion != nullptr; }


	Expansion* getExpansionForWildcardReference(const String& stringToTest) const;

	int getNumExpansions() const { return expansionList.size(); }

	Expansion* getExpansion(int index) const
	{
		return expansionList[index];
	}

	Expansion* getCurrentExpansion() const { return currentExpansion.get(); }

	template <class DataType> SharedPoolBase<DataType>* getCurrentPool()
	{
		return getCurrentPoolCollection()->getPool<DataType>();
	}

    PoolCollection* getCurrentPoolCollection();
    
	void clearPools();

    
    
private:

    FileHandlerBase* getFileHandler(MainController* mc);
    
	template <class DataType> void getPoolForReferenceString(const PoolReference& p, SharedPoolBase<DataType>** pool)
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		ignoreUnused(p, type);
		jassert(p.getFileType() == type);
		
		if (auto e = getExpansionForWildcardReference(p.getReferenceString()))
		{
			*pool = e->pool->getPool<DataType>();
		}
		else
			*pool = getFileHandler(mc)->pool->getPool<DataType>();
	}

	struct Notifier : private AsyncUpdater
	{
	public:
		enum class EventType
		{
			ExpansionLoaded,
			ExpansionCreated,
			numModes
		};

		Notifier(ExpansionHandler& parent_) :
			parent(parent_)
		{};

		

		void sendNotification(EventType eventType, NotificationType notificationType = sendNotificationAsync)
		{
			m = eventType;

			if (notificationType == sendNotificationAsync)
			{
				IF_NOT_HEADLESS(triggerAsyncUpdate());
			}
			else if (notificationType == sendNotificationSync)
			{
				handleAsyncUpdate();
			}
		}

	private:

		void handleAsyncUpdate()
		{
			ScopedLock sl(parent.listeners.getLock());

			for (auto l : parent.listeners)
			{
				if (l.get() != nullptr)
				{
					if (m == EventType::ExpansionLoaded)
						l->expansionPackLoaded(parent.currentExpansion);
					else
						l->expansionPackCreated(parent.currentExpansion);
				}

			}

			return;
		}

		ExpansionHandler& parent;

		EventType m;
		WeakReference<Expansion> currentExpansion;
	};

	Notifier notifier;

	Array<WeakReference<Listener>, CriticalSection> listeners;

	OwnedArray<Expansion> expansionList;

	WeakReference<Expansion> currentExpansion;

	MainController * mc;
};


}

#endif

