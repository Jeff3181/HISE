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

namespace hise {
using namespace juce;

class UIComponentDatabase
{
public:

	struct CommonData
	{
		struct Data
		{
			Data();

			~Data()
			{
				MessageManagerLock mmLock;
				jmp = nullptr;
				root = nullptr;
			}

			void createRootWindow()
			{
				MessageManagerLock mmlock;

				jassert(bp != nullptr);
				root = bp->getDocWindow();
			}

			void init(BackendProcessor* bp);

			ScopedPointer<JavascriptMidiProcessor> jmp;
			ReferenceCountedArray<ScriptingApi::Content::ScriptComponent> list;

			BackendProcessor* bp = nullptr;
			Component::SafePointer<BackendRootWindow> root;

			Array<Identifier> commonIds;

		};

		ScriptComponent* getComponentForURL(const MarkdownLink& url);

		String getComponentIDFromURL(const MarkdownLink& url);

		SharedResourcePointer<Data> d;
	};

	struct ItemGenerator : public CommonData,
		public MarkdownDataBase::ItemGeneratorBase
	{
		ItemGenerator(File root, MarkdownDatabaseHolder& holder_) :
			ItemGeneratorBase(root),
			holder(holder_)
		{
			d->init(dynamic_cast<BackendProcessor*>(&holder));
			colour = Colour(0xFF136faa);
		}

		MarkdownDataBase::Item createRootItem(MarkdownDataBase& parent) override;

		void createFloatingTileApi(MarkdownDataBase::Item& item);

		MarkdownDataBase::Item createItemForFloatingTile(MarkdownDataBase::Item& item, FloatingTileContent::Factory& f, Identifier& id, FloatingTile* root);

		MarkdownDatabaseHolder& holder;
	};

	struct Resolver : public CommonData,
		public MarkdownParser::LinkResolver
	{
		Resolver(File root_, BackendProcessor* bp);


		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("UIComponentResolver"); }
		LinkResolver* clone(MarkdownParser* newParent) const override { return new Resolver(root, d->bp); }

		String getContent(const MarkdownLink& url) override;

		String getInitialisationForComponent(const String& name);

		File root;
	};

	struct FloatingTileResolver : public CommonData,
		public MarkdownParser::LinkResolver
	{
		FloatingTileResolver(MarkdownDatabaseHolder& holder);

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FloatingTileResolver"); }
		LinkResolver* clone(MarkdownParser* newParent) const override { return new FloatingTileResolver(holder); }

		String getContent(const MarkdownLink& url) override;

		String getCategoryContent(const String& categoryName);
		String getFloatingTileContent(const MarkdownLink& url, FloatingTileContent::Factory& f, const Identifier& id);

		MarkdownDatabaseHolder& holder;
		MarkdownLink root;
	};

	struct ScreenshotProvider : public CommonData,
		public MarkdownParser::ImageProvider
	{
		ScreenshotProvider(MarkdownParser* parent);;

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("UIScreenshotProvider"); }
		ImageProvider* clone(MarkdownParser* newParent) const override { return new ScreenshotProvider(newParent); }

		Image getImage(const MarkdownLink& url, float width) override;

		LookAndFeel_V3 laf;
	};

	static constexpr char uiComponentWildcard[] = "/ui-components";

	static constexpr char floatingTileWildcard[] = "/ui-components/floating-tiles";
};

struct AutogeneratedDocHelpers
{
	static File getCachedDocFolder();
	static void addItemGenerators(MarkdownDatabaseHolder& holder);
	static void registerContentProcessor(MarkdownContentProcessor* p);
	static void registerGlobalPathFactory(MarkdownContentProcessor* p, const File& markdownRoot);
	static void createDocFloatingTile(FloatingTile* ft);
};

}
