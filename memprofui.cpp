// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

#include <cheerp/client.h>
#include <deque>

namespace [[cheerp::genericjs]] client
{
	class CheerpMemProf: public Object
	{
	private:
	public:
		CheerpMemProf();
		client::TArray<client::Object*>* liveAllocations();
		client::Object* liveAllocationsTree(bool isTopDown);
		size_t totalLiveMemory();
	};
};

class GraphAppearence
{
public:
	GraphAppearence()
		: maxMemory(1.0), widthBox (-1), howOftenDoLine(1000)
	{
	}
	int barTotal() const
	{
		return barSpacing + barWidth;
	}
	bool checkAgainstMaxMemory(double sample)
	{
		bool modified = false;
		while (sample > heightGraph * maxMemory)
		{
			modified = true;
			maxMemory *= 1.1;
		}
		return modified;
	}
	int heightBox() const
	{
		return marginWidth*3 + heightGraph;
	}
	bool hasBoxWidthChanged(int width, int samples = 0)
	{
		bool flag = (width != widthBox);
		widthBox = width;
		return flag;
	}
	bool areWeGoingToOverflow(int samples) const
	{
		return (samples * barTotal() - barSpacing  > widthGraph());
	}
	int widthGraph() const
	{
		return widthBox - 2*marginWidth;
	}
	void setBarWidth(int samples)
	{
		int pixelPerSample = widthGraph() / (minSamplesToShow);
		if (samples > minSamplesToShow)
		{
			pixelPerSample = widthGraph() / samples;
		}
		if (pixelPerSample < 1) pixelPerSample = 1;
		barWidth = pixelPerSample * barProportions;
		barSpacing = pixelPerSample * (1.0 - barProportions);
		if (barTotal() < pixelPerSample)
		{
			//this means that there is one extra pixel to be assigned
			if (barWidth == 0)
				barWidth ++;
			else if (barSpacing == 0 || barProportions < 0.5)
				barSpacing ++;
			else
				barWidth ++;
		}
		howOftenDoLine = 300 / barTotal();
	}
public:
	static constexpr int heightLine = 2;
	static constexpr int heightGraph = 130;
	static constexpr int marginWidth = 10;
	static constexpr int textBaseLine = heightGraph + marginWidth * 1.6;
	static constexpr double barProportions = 0.65;
	double maxMemory;
	int widthBox;
	int barWidth;
	int barSpacing;
	int minSamplesToShow;
	int howOftenDoLine;
};

class GraphData
{
public:
	GraphData(int windowLenghtInSamples) : firstIndex(0), erasedSamples(0),
		infiniteLength(windowLenghtInSamples <= 0 ? true : false),
		minimumSamplesToRemember(infiniteLength ? -1 : windowLenghtInSamples), maxMemory(0)
	{
	}
	client::TArray<int>* lastSamples()
	{
		client::TArray<int>* res = new client::TArray<int>();
		for (int i = 0; i<samples.size(); i++)
		{
			res->push(samples[i]);
		}
		return res;
	}
	void walkBack(int ammount)
	{
		if (minimumSamplesToRemember < 0)
			return;
		int limit = shownSamples() / 2;
		if (ammount > limit) ammount = limit;
		while (ammount-- && size() > firstIndex && shownSamples() > minimumSamplesToRemember)
		{
			firstIndex++;
		}
		if (shownSamples() > minimumSamplesToRemember)
		{
			while (erasedSamples < firstIndex)
			{
				erasedSamples++;
				samples.pop_front();
			}
		}
	}
	int getMaxMemory() const
	{
		return maxMemory;
	}
	int shownSamples() const
	{
		return size() - firstIndex;
	}
	int sample(int index) const
	{
		return samples[index - erasedSamples];
	}
	int skipped() const
	{
		return firstIndex;
	}
	int size() const
	{
		return samples.size() + erasedSamples;
	}
	void push_back(int x)
	{
		samples.push_back(x);
		maxMemory = std::max(maxMemory, x);
	}
	void backInTime()
	{
		firstIndex = erasedSamples;
	}
private:
	std::deque<int> samples;
	int firstIndex;
	int erasedSamples;
public:
	const bool infiniteLength;
private:
	const int minimumSamplesToRemember;
	int maxMemory;
};

class GraphDrawer
{
public:
	GraphDrawer() :
		appearence(),
		samplesPerPixel(1),
		box(nullptr),
		line(nullptr),
		currMemoryText(nullptr)
	{
	}
	bool checkWidthBox(int collectedSamples)
	{
		return appearence.hasBoxWidthChanged(box->get_offsetWidth(), collectedSamples);
	}
private:
	static client::String *cutRepresentation(client::String* nbr, int digits)
	{
		int index = nbr->indexOf(".");
		if (index > 0)
			return nbr->substr(0, nbr->indexOf(".") + digits + 1);
		else
			return nbr;
	}
	static client::String *representTime(double X)
	{
		X /= 1000.0;
		double M = int(X/60);
		double S = X - M * 60.0 + 1e-6;

		S += 100.0;
		client::String *seconds = cutRepresentation(new client::String(S), 1)->substr(1)->concat("s");

		if (M == 0) return seconds->substr(S>=110?0:1);
		return (cutRepresentation(new client::String(M), -1))->concat(":")->concat(seconds);
	}
	static client::String *representMemory(const char * name, double X)
	{
		int steps = 0;
		while (X >= 16768.0 && steps < 3)
		{
			X/=1024.0;
			steps++;
		}
		client::String *suffix;
		if (steps == 0) suffix = new client::String(" B");
		if (steps == 1) suffix = new client::String(" KB");
		if (steps == 2) suffix = new client::String(" MB");
		if (steps == 3) suffix= new client::String(" GB");
		return (new client::String(name))->concat(cutRepresentation(new client::String(X),-1))->concat(suffix);
	}
	enum class Alignment{
		Left, Right, Center
	};
	client::HTMLElement* text(int bottom, int height, int middle, Alignment alignment, client::String *str, client::HTMLElement* parent = client::document.get_body())
	{
		client::HTMLElement* curr = client::document.createElement("span");
		auto style = curr->get_style();
		style->set_height(getNumberPX(height));
		style->set_position("fixed");
		style->set_bottom(getNumberPX(bottom));
		switch (alignment)
		{
		case Alignment::Left:
			{
				style->set_textAlign("left");
				style->set_right(getNumberPX(middle+300));
				style->set_left(getNumberPX(middle));
				break;
			}
		case Alignment::Right:
			{
				style->set_textAlign("right");
				style->set_right(getNumberPX(middle));
				style->set_left(getNumberPX(middle-300));
				break;
			}
		case Alignment::Center:
			{
				style->set_textAlign("center");
				style->set_right(getNumberPX(middle+150));
				style->set_left(getNumberPX(middle-150));
				break;
			}
		}
		style->set_width("300px");
		parent->appendChild(curr);
		curr->set_innerHTML(str);
		return curr;
	}
	client::HTMLElement* buildCurrentMemoryText(double maxMemory, double currLiveMemory)
	{
		return text(appearence.textBaseLine, 5, appearence.widthBox - 12, Alignment::Right,
				representMemory("Max:", maxMemory)->concat(representMemory(" |  Current:", currLiveMemory)));
	}
	static client::HTMLElement* rectangle(int bottom, int height, int left, int width, const char *color, client::HTMLElement* parent)
	{
		client::HTMLElement* curr = rectangleBase(bottom, height, color, parent);
		auto style = curr->get_style();
		style->set_left(getNumberPX(left));
		style->set_width(getNumberPX(width));
		return curr;
	}
	static client::String* getNumberPX(int a)
	{
		return (new client::String (a))->concat("px");
	}
	static client::HTMLElement* rectangleWide(int bottom, int height, const char *color, client::HTMLElement* parent)
	{
		client::HTMLElement* curr = rectangleBase(bottom, height, color, parent);
		auto style = curr->get_style();
		style->set_left(0);
		style->set_right(0);
		return curr;
	}
	static client::HTMLElement* rectangleBase(int bottom, int height, const char *color, client::HTMLElement* parent)
	{
		client::HTMLElement* curr = client::document.createElement("div");
		auto style = curr->get_style();
		style->set_position("fixed");
		style->set_bottom(getNumberPX(bottom));
		style->set_height(getNumberPX(height));
		style->set_background(client::String(color));
		parent->appendChild(curr);
		return curr;
	}
	void drawSingleBar(int value, int index, const char*color)
	{
		rectangle(appearence.marginWidth, height(value), xLocation((index) / samplesPerPixel), appearence.barWidth, color, box);
	}
public:
	void drawBar(const GraphData& data, int firstSample, int endSample)
	{
		int low = data.sample(firstSample);
		int high = low;
		for (int i=firstSample + 1; i < endSample; i++)
		{
			int curr = data.sample(i);
			low = std::min(low, curr);
			high = std::max(high, curr);
		}
		if (endSample - firstSample > 1)
		{
			drawSingleBar(high, endSample - 1 - data.skipped(), "cornflowerblue");
		}
		drawSingleBar(low, endSample - 1 - data.skipped(), "lightblue");
	}
	void redrawSamples(const GraphData& data)
	{
		samplesPerPixel = 1;
		while (data.shownSamples() / samplesPerPixel * appearence.barTotal() > appearence.widthGraph()) {
			samplesPerPixel += std::max(1, (int)(samplesPerPixel * 0.3));
		}
		for (int i = data.skipped(); i < data.size(); i+= samplesPerPixel)
		{
			drawBar(data, i, std::min(i+samplesPerPixel, data.size()));
		}
	}
	void drawTimelineLocations(int deltaOldSamples, double timeFromStart, double frequencyMilliseconds)
	{
		appearence.hasBoxWidthChanged(box->get_offsetWidth());
		int X = 0;
		int Y = appearence.barTotal();
		int index = 0;
		while (X + appearence.howOftenDoLine*Y < appearence.widthBox)
		{
			rectangle(appearence.marginWidth, appearence.heightGraph + appearence.marginWidth, X + appearence.marginWidth, 1, "grey", box);
			text(appearence.textBaseLine, 5, 12 + X, Alignment::Left, representTime(timeFromStart + (index * appearence.howOftenDoLine + deltaOldSamples) * frequencyMilliseconds * samplesPerPixel));
			X += appearence.howOftenDoLine * Y;
			index++;
		}
	}
	void redrawBox()
	{
		if (box)
		{
			for (int i = 0; i < box->get_children()->get_length(); i++)
			{
				box->removeChild((*box->get_children())[i]);
			}
			client::document.get_body()->removeChild(box);
		}
		box = rectangleWide(0, appearence.heightBox(), "white", client::document.get_body());
		line = rectangleWide(appearence.heightBox(), appearence.heightLine, "black", box);
	}
	void printCurrentMemoryText(int maxMemory, int currentLiveMemory)
	{
		if (currMemoryText != nullptr)
			client::document.get_body()->removeChild(currMemoryText);
		currMemoryText = buildCurrentMemoryText(maxMemory, currentLiveMemory);
	}
	int height(int sample_memory)
	{
		return sample_memory/appearence.maxMemory;
	}
	int xLocation(int number)
	{
		return appearence.marginWidth + number * appearence.barTotal();
	}
	GraphAppearence appearence;
	int samplesPerPixel;
public:
	client::HTMLElement* box;
	client::HTMLElement* line;
	client::HTMLElement* currMemoryText;
};


class CheerpMemProfGraphInternals {
public:
	CheerpMemProfGraphInternals(int intervalLengthMilliseconds, int samplingPeriodMilliseconds) :
		frequencyMilliseconds(samplingPeriodToMilliseconds(samplingPeriodMilliseconds)),
		graphDrawer(), graphData(intervalLengthMilliseconds / samplingPeriodMilliseconds)
	{
		int movingWindowSeconds = intervalLenghtToSeconds(intervalLengthMilliseconds);
		graphDrawer.appearence.minSamplesToShow = intervalLengthMilliseconds / samplingPeriodMilliseconds * 1.2;
		if (graphDrawer.appearence.minSamplesToShow < 0) graphDrawer.appearence.minSamplesToShow = 120;
	if (graphData.infiniteLength)
		client::console.log("MemProfGraph invoked.\nInterval length: unbounded\nSampling period: ", frequencyMilliseconds, " milliseconds");
	else
		client::console.log("MemProfGraph invoked.\nInterval length: ", movingWindowSeconds, " seconds\nSampling period: ", frequencyMilliseconds, " milliseconds");
	client::console.log("For additional data, call from the Console cheerpMemUI.liveAllocations() for the details with stack strace of the current allocations or cheerpMemUI.lastSamples() for the array with the memory consumption data");
		JsMemProf = new client::CheerpMemProf;
		memProfGraph = this;
		redraw();
		auto interval = client::setInterval(cheerp::Callback(mainLoop), frequencyMilliseconds);
	}
	client::TArray<int>* lastSamples()
	{
		return graphData.lastSamples();
	}
private:
	static int intervalLenghtToSeconds(int intervalLengthMilliseconds)
	{
		if (intervalLengthMilliseconds == -1)
			return -1;
		else
			return (intervalLengthMilliseconds + 999) / 1000;
	}
	static int samplingPeriodToMilliseconds(int samplingPeriod)
	{
		if (samplingPeriod == -1)
			return 1000;
		else
			return samplingPeriod;
	}
	static CheerpMemProfGraphInternals* instance()
	{
		return memProfGraph;
	}
	void redraw()
	{
		graphDrawer.redrawBox();
		graphDrawer.appearence.setBarWidth(graphData.shownSamples());
		graphDrawer.drawTimelineLocations(graphData.skipped(), 0.0, frequencyMilliseconds);
		graphDrawer.redrawSamples(graphData);
	}
	void addSample()
	{
		double currLiveMemory = JsMemProf->totalLiveMemory();
		graphData.push_back(currLiveMemory);
		bool hasChanged = graphDrawer.appearence.checkAgainstMaxMemory(currLiveMemory);
		if (graphDrawer.checkWidthBox(graphData.shownSamples()))
		{
			hasChanged = true;
			graphData.backInTime();
		}
		int tries = 10;
		while (tries-- && graphDrawer.appearence.areWeGoingToOverflow(graphData.shownSamples() / graphDrawer.samplesPerPixel))
		{
			hasChanged = true;
			graphData.walkBack(graphDrawer.appearence.howOftenDoLine);
		}
		if (!hasChanged && (graphData.shownSamples() % graphDrawer.samplesPerPixel == graphDrawer.samplesPerPixel -1))
			graphDrawer.drawBar(graphData, graphData.size() - graphDrawer.samplesPerPixel, graphData.size());
		else
			redraw();
		graphDrawer.printCurrentMemoryText(graphData.getMaxMemory(), currLiveMemory);
	}
	static void mainLoop()
	{
		instance()->addSample();
	}
	static CheerpMemProfGraphInternals* memProfGraph;
	const int frequencyMilliseconds;
	client::CheerpMemProf* JsMemProf;
	GraphData graphData;
	GraphDrawer graphDrawer;
};

class [[cheerp::jsexport]] [[cheerp::genericjs]] CheerpMemUI
{
public:
	CheerpMemUI(int windowLenghtSeconds, int samplingPeriodMilliseconds)
        {
		cheerpMemProf = new client::CheerpMemProf();
		memProfGraph = new CheerpMemProfGraphInternals(windowLenghtSeconds * 1000, samplingPeriodMilliseconds);
	}
	client::TArray<int>* lastSamples()
	{
		return memProfGraph->lastSamples();
	}
	client::TArray<client::Object*>* liveAllocations()
        {
                return cheerpMemProf->liveAllocations();
        }
	client::Object* liveAllocationsBottomUp()
        {
                return cheerpMemProf->liveAllocationsTree(false);
        }
	client::Object* liveAllocationsTopDown()
        {
                return cheerpMemProf->liveAllocationsTree(true);
        }
        size_t totalLiveMemory()
        {
                return cheerpMemProf->totalLiveMemory();
        }
private:
	client::CheerpMemProf* cheerpMemProf;
	CheerpMemProfGraphInternals* memProfGraph;
};
