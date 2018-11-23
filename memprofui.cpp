#include <cheerp/client.h>
#include <deque>

namespace [[cheerp::genericjs]] client
{
	class CheerpMemProf
	{
	private:
	public:
		CheerpMemProf();
		client::TArray<client::Object*>* liveAllocations();
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
	bool hasBoxWidthChanged(int width)
	{
		bool flag = (width != widthBox);
		widthBox = width;
		if (flag)
		{
			setBarWidth();
			howOftenDoLine = 300 / barTotal();
		}
		return flag;
	}
	bool areWeGoingToOverflow(int samples) const
	{
		return (samples * barTotal() - barSpacing + marginWidth * 2 > widthBox);
	}
private:
	void setBarWidth()
	{
		int widthGraph = widthBox - 2*marginWidth;
		int pixelPerSample = widthGraph / (minSamplesToShow * 1.2);
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
	GraphData(int windowLenghtInSamples) : firstIndex(0), erasedSamples(0), minimumSamplesToRemember(windowLenghtInSamples)
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
		int limit = shownSamples() / 2;
		if (ammount > limit) ammount = limit;
		while (ammount-- && size() > firstIndex)
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
	}
	void backInTime()
	{
		firstIndex = erasedSamples;
	}
private:
	std::deque<int> samples;
	int firstIndex;
	int erasedSamples;
	const int minimumSamplesToRemember;
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
	bool checkWidthBox()
	{
		return appearence.hasBoxWidthChanged(box->get_offsetWidth());
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
	static client::String *representMemory(double X)
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
		return (new client::String("current: "))->concat(cutRepresentation(new client::String(X),-1))->concat(suffix);
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
	client::HTMLElement* buildCurrentMemoryText(double currLiveMemory)
	{
		return text(appearence.textBaseLine, 5, appearence.widthBox - 12, Alignment::Right, representMemory(currLiveMemory));
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
public:
	void redrawSamples(const GraphData& data)
	{
		for (int i = data.skipped(); i < data.size(); i++)
		{
			rectangle(appearence.marginWidth, height(data.sample(i)), xLocation(i - data.skipped()), appearence.barWidth, "lightblue", box);
		}
	}
	void drawTimelineLocations(int deltaOldSamples, double timeFromStart, double frequencyMilliseconds)
	{
		appearence.hasBoxWidthChanged(box->get_offsetWidth());
		int X = 0;
		int Y = appearence.barTotal();
		int index = 0;
		while (X == 0 || X + appearence.howOftenDoLine*Y < appearence.widthBox)
		{
			rectangle(appearence.marginWidth, appearence.heightGraph + appearence.marginWidth, X + appearence.marginWidth, 1, "grey", box);
			text(appearence.textBaseLine, 5, 12 + X, Alignment::Left, representTime(timeFromStart + (index * appearence.howOftenDoLine + deltaOldSamples) * frequencyMilliseconds));
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
	void printCurrentMemoryText(int currentLiveMemory)
	{
		if (currMemoryText != nullptr)
			client::document.get_body()->removeChild(currMemoryText);
		currMemoryText = buildCurrentMemoryText(currentLiveMemory);
	}
	int height(int sample_memory)
	{
		return sample_memory/appearence.maxMemory;
	}
	int xLocation(int number)
	{
		return appearence.marginWidth + number * appearence.barTotal();
	}
	void createBar(double memory, int position)
	{
		rectangle(appearence.marginWidth, height(memory), xLocation(position), appearence.barWidth, "lightblue", box);
	}
	GraphAppearence appearence;
private:
	int samplesPerPixel;
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
		graphDrawer.appearence.minSamplesToShow = intervalLengthMilliseconds / samplingPeriodMilliseconds;
		if (graphDrawer.appearence.minSamplesToShow < 0) graphDrawer.appearence.minSamplesToShow = 1000;
		client::console.log("MemProfGraph invoked.\nInterval length: ", movingWindowSeconds, " seconds\nSampling period: ", frequencyMilliseconds, " milliseconds");
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
		graphDrawer.drawTimelineLocations(graphData.skipped(), 0.0, frequencyMilliseconds);
		graphDrawer.redrawSamples(graphData);
	}
	void addSample()
	{
		double currLiveMemory = JsMemProf->totalLiveMemory();
		graphData.push_back(currLiveMemory);
		bool hasChanged = graphDrawer.appearence.checkAgainstMaxMemory(currLiveMemory);
		if (graphDrawer.checkWidthBox())
		{
			hasChanged = true;
			graphData.backInTime();
		}
		while (graphDrawer.appearence.areWeGoingToOverflow(graphData.shownSamples()))
		{
			hasChanged = true;
			graphData.walkBack(graphDrawer.appearence.howOftenDoLine);
		}
		if (!hasChanged)
			graphDrawer.createBar(currLiveMemory, graphData.shownSamples() -1);
		else
			redraw();
		graphDrawer.printCurrentMemoryText(currLiveMemory);
	}
	static void mainLoop()
	{
		instance()->addSample();
	}
	static CheerpMemProfGraphInternals* memProfGraph;
	const int frequencyMilliseconds;
	client::CheerpMemProf* JsMemProf;
	int samplesPerPixel;
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
        size_t totalLiveMemory()
        {
                return cheerpMemProf->totalLiveMemory();
        }
private:
	client::CheerpMemProf* cheerpMemProf;
	CheerpMemProfGraphInternals* memProfGraph;
};

void webMain()
{
}
