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

/** a simple markdown parser that renders markdown formatted code. */
class MarkdownParser
{
public:
	
	struct YamlHeader
	{
		StringArray getKeywords()
		{
			checkValid();
			return items[0].values;
		}

		String getDescription()
		{
			checkValid();
			return items[1].values[0];
		}

		String getKeyValue(const String& key) const
		{
			for (const auto& item : items)
			{
				if (item.key == key)
					return item.values[0];
			}

			return {};
		}

		StringArray getKeyList(const String& key) const
		{
			for (const auto& item : items)
			{
				if (item.key == key)
					return item.values;
			}

			return {};
		}

		struct Item
		{
			String key;
			StringArray values;
		};

		void checkValid()
		{
			if (items[0].key != "keywords")
				throw String(items[0].key + "; expected: keywords");

			if(items[1].key != "summary")
				throw String(items[1].key + "; expected: summary");

			if (items[1].values.size() != 1)
				throw String("summary value not single string");
		}

		Array<Item> items;
	};

	enum ResolveType
	{
		Fallback = 0, ///< creates a empty content
		FileBased, ///< creates the content from a file
		Autogenerated, ///< autogenerates the content
		WebBased, ///< creates the content from a web resource
		Cached, ///< creates the content from a cached database
	};

	struct HyperLink
	{
		bool valid = false;
		Rectangle<float> area = {};
		String url = {};
		String tooltip;
		String displayString;
		Range<int> urlRange = {};
	};

	struct LinkResolver
	{
		struct Sorter
		{
			static int compareElements(LinkResolver* first, LinkResolver* second)
			{
				auto fp = first->getPriority();
				auto sp = second->getPriority();

				if (fp > sp)
					return -1;
				else if (fp < sp)
					return 1;
				else
					return 0;
			}
		};

		virtual ~LinkResolver() {};
		virtual String getContent(const String& url) = 0;
		virtual LinkResolver* clone(MarkdownParser* parent) const = 0;
		virtual Identifier getId() const = 0;
		
		virtual ResolveType getPriority() const = 0;

		/** Overwrite this method and do something if the url matches, then return 
		    true or false whether the resolver consumed this event. 
		*/
		virtual bool linkWasClicked(const String& url) 
		{ 
			return false; 
		}
	};

	class ImageProvider
	{
	public:

		struct Sorter
		{
			static int compareElements(ImageProvider* first, ImageProvider* second)
			{
				auto fp = first->getPriority();
				auto sp = second->getPriority();

				if (fp > sp)
					return -1;
				else if (fp < sp)
					return 1;
				else
					return 0;
			}
		};

		bool operator==(const ImageProvider& other) const
		{
			return getId() == other.getId();
		}

		ImageProvider(MarkdownParser* parent_) :
			parent(parent_)
		{}

		virtual ResolveType getPriority() const { return ResolveType::Fallback; }

		virtual ~ImageProvider() {};

		/** Overwrite this method and return an image for the given URL.
		*
		*	The default function just returns a placeholder.
		*/
		virtual Image getImage(const String& /*imageURL*/, float width);
		virtual ImageProvider* clone(MarkdownParser* newParent) const { return new ImageProvider(newParent); }
		virtual Identifier getId() const { RETURN_STATIC_IDENTIFIER("EmptyImageProvider"); };



		/** Helper function that makes sure that the image doesn't exceed the given width. */
		static Image resizeImageToFit(const Image& otherImage, float width);

	protected:
		MarkdownParser * parent;
	};

	class DefaultLinkResolver;	class FileLinkResolver;			class FolderTocCreator;
	class URLImageProvider;		class FileBasedImageProvider;	class GlobalPathProvider;
	
	MarkdownParser(const String& markdownCode);

	~MarkdownParser();

	void setFonts(Font normalFont, Font codeFont, Font headlineFont, float defaultFontSize);

	void setTextColour(Colour c) { styleData.textColour = c; }

	String resolveLink(const String& url);

	Image resolveImage(const String& imageUrl, float width);

	void setLinkResolver(LinkResolver* ownedResolver)
	{
		ScopedPointer<LinkResolver> owned = ownedResolver;

		for (auto r : linkResolvers)
		{
			if (r->getId() == owned->getId())
				return;
		}

		linkResolvers.addSorted(LinkResolver::Sorter(), owned.release());
	}

	void setImageProvider(ImageProvider* newProvider)
	{
		ScopedPointer<ImageProvider> owned = newProvider;

		for (auto p : imageProviders)
		{
			if (*p == *newProvider)
				return;
		}

		imageProviders.addSorted(ImageProvider::Sorter(), owned.release());
	};

	virtual void parse();

	Result getParseResult() const { return currentParseResult; }

	void setDefaultTextSize(float fontSize);

	String getCurrentText() const;

	String getLastLink(bool includeLink, bool includeAnchor) const 
	{ 
		String l;

		if (includeLink)
			l << lastLink;

		if (includeAnchor)
			l << lastAnchor;

		return l;
	}

	void setNewText(const String& newText);
	bool gotoLink(const MouseEvent& event, Rectangle<float> area);

	bool gotoLink(const String& url);

	StringArray getImageLinks() const;

	HyperLink getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area);

	static void createDatabaseEntriesForFile(File root, MarkdownDataBase::Item& item, File f, Colour c);

	struct SnippetTokeniser : public CodeTokeniser
	{
		SnippetTokeniser() {};

		int readNextToken(CodeDocument::Iterator& source) override;
		CodeEditorComponent::ColourScheme getDefaultColourScheme();
	};

	struct Tokeniser: public CodeTokeniser
	{
		Tokeniser() {};

		int readNextToken(CodeDocument::Iterator& source);
		CodeEditorComponent::ColourScheme getDefaultColourScheme();
	};

	void setStyleData(MarkdownLayout::StyleData newStyleData)
	{
		styleData = newStyleData;

		if (markdownCode.isNotEmpty())
		{
			setNewText(markdownCode);
		}
	}

	MarkdownLayout::StyleData& getStyleData() { return styleData; }
	
protected:

	struct Element
	{
		Element(MarkdownParser* parent_) :
			parent(parent_)
		{
			hyperLinks.insertArray(0, parent->currentLinks.getRawDataPointer(), parent->currentLinks.size());
		};

		virtual ~Element() {};

		virtual void draw(Graphics& g, Rectangle<float> area) = 0;
		virtual float getHeightForWidth(float width) = 0;
		virtual int getTopMargin() const = 0;
		virtual Component* createComponent(int maxWidth) { return nullptr; }
		
		float getZoomRatio() const { return parent->styleData.fontSize / 17.0f; }

		virtual String getTextForRange(Range<int> range) const
		{
			jassertfalse;
			return {};
		}

		void drawHighlight(Graphics& g, Rectangle<float> area);;

		virtual void searchInContent(const String& s) { }
		virtual void addImageLinks(StringArray& sa) {};

		Array<HyperLink> hyperLinks;

		float getLastHeight()
		{
			return cachedHeight;
		}

		float getHeightForWidthCached(float width);

		static void recalculateHyperLinkAreas(MarkdownLayout& l, Array<HyperLink>& links, float topMargin);

		void setSelected(bool isSelected)
		{
			selected = isSelected;
		}

		virtual String getTextToCopy() const = 0;

		bool selected = false;
		RectangleList<float> searchResults;

		virtual void prepareLinksForHtmlExport(const String& baseURL);

		String generateHtmlAndResolveLinks() const
		{
			auto s = generateHtml();
			
			int index = 0;

			for (const auto& link : hyperLinks)
			{
				String linkWildcard = "{LINK" + String(index++) + "}";
				s = s.replace(linkWildcard, link.url);
			}

			return s;
		}

	protected:

		virtual String generateHtml() const { return {}; }

		static Array<Range<int>> getMatchRanges(const String& fullText, const String& searchString, bool countNewLines);

		void searchInStringInternal(const AttributedString& textToSearch, const String& searchString);

		MarkdownParser* parent;

		float lastWidth = -1.0f;
		float cachedHeight = 0.0f;
	};

	struct TextBlock;	struct Headline;		struct BulletPointList;		struct Comment;
	struct CodeBlock;	struct MarkdownTable;	struct ImageElement;		struct EnumerationList;
	struct ActionButton;

	struct CellContent
	{
		bool isEmpty() const
		{
			return imageURL.isEmpty() && s.getText().isEmpty();
		}

		AttributedString s;

		String imageURL;
		Array<HyperLink> cellLinks;
	};

	using RowContent = Array<CellContent>;

	OwnedArray<Element> elements;

	String markdownCode;

private:

	YamlHeader yamlHeader;

	String lastLink;
	String lastAnchor;

    class Iterator
	{
	public:

		Iterator(const String& text_);

		juce_wchar peek();
		bool advanceIfNotEOF(int numCharsToSkip = 1);
		bool advance(int numCharsToSkip = 1);
		bool advance(const String& stringToSkip);
		bool next(juce::juce_wchar& c);
		bool match(juce_wchar expected);
		void skipWhitespace();
		String getRestString() const;
		String advanceLine();

	private:

		String text;
		CharPointer_UTF8 it;
	};

	struct Helpers
	{
		static bool isNewElement(juce_wchar c);
		static bool isEndOfLine(juce_wchar c);
		static bool isNewToken(juce_wchar c, bool isCode);
		static bool belongsToTextBlock(juce_wchar c, bool isCode, bool stopAtLineEnd);
	};

	void resetForNewLine();

	void parseLine();
	void parseHeadline();
	void parseBulletList();
	void parseEnumeration();

	void parseText(bool stopAtEndOfLine=true);
	void parseYamlHeader();
	void parseBlock();
	void parseJavascriptBlock();
	void parseTable();
	void parseButton();
	void parseComment();
	ImageElement* parseImage();
	RowContent parseTableRow();
	bool isJavascriptBlock() const;
	bool isImageLink() const;
	void addCharacterToCurrentBlock(juce_wchar c);
	void resetCurrentBlock();
	void skipTagAndTrailingSpace();

	bool isBold = false;
	bool isItalic = false;
	bool isCode = false;

#if 0
	float defaultFontSize = 17.0f;
	Colour textColour = Colours::black;
	

	Font normalFont;
	Font boldFont;
	Font codeFont;
	Font headlineFont;
	
#endif
	
	Colour currentColour;
	Font currentFont;

	MarkdownLayout::StyleData styleData;
	
	Iterator it;
	Result currentParseResult;
	OwnedArray<ImageProvider> imageProviders;
	AttributedString currentlyParsedBlock;
	Array<HyperLink> currentLinks;
	
	OwnedArray<LinkResolver> linkResolvers;
	
	
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownParser);
};




}