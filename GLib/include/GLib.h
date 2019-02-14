#pragma once

#include <vector>
#include <map>
#include <string>
#include <functional>

#include <Windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace GLib
{
	typedef ID2D1HwndRenderTarget RT;
	typedef D2D1::ColorF C;

	class View;
	class Frame;
	class Color;
	class Writer;
	class MovingBar;
	class MainBar;
	class OutputView;
	struct OutputForward;

	class View
	{
	public:
		View(View* parent, int x, int y, int width, int height);
		virtual void init() {};
		virtual ~View();

		template <typename R, typename... Ts> R* addView(Ts&&... args)
		{
			R* e = new R(this, 0, 0, -1, -1);
			e->parentView = this;
			e->init(std::forward<Ts>(args)...);
			subViews.push_back(e);
			return e;
		}

		template <typename R, typename... Ts> R* addView(int x, int y, Ts&&... args)
		{
			R* e = new R(this, x, y, -1, -1);
			e->parentView = this;
			e->init(std::forward<Ts>(args)...);
			subViews.push_back(e);
			return e;
		}

		template <typename R, typename... Ts> R* addView(int x, int y, int width, int height, Ts&&... args)
		{
			R* e = new R(this, x, y, width, height);
			e->parentView = this;
			e->init(std::forward<Ts>(args)...);
			subViews.push_back(e);
			return e;
		}

		void addMouseListener(int type, std::function<void(int, int)> f);

		std::pair<int, int> getMousePosition();

		virtual void parentResized(D2D1_RECT_F p) {};
		View* getParentView();

	public:

		D2D1_RECT_F place;

		bool activateFlag = true;
		bool renderFlag = true;
		bool updateFlag = true;
		bool winEventFlag = true;
		bool mouseEventFlag = true;

		std::vector<View*> subViews;

	protected:
		void renderControl(RT* rt, Writer* w, Color* c, int x, int y, D2D1_RECT_F& visibleRect);
		void updateControl();
		void winEventControl(Frame* frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		void mouseEventControl(Frame* frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		virtual void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) {};
		virtual void update() {};
		virtual void winEvent(Frame* frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {};
		virtual void worker() {};

		void forwardMouseEvent(int type, int x, int y);

	private:
		View* parentView = nullptr;

		std::map<int, std::function<void(int, int)>> mouseFunctions;

		int mouseX = 0;
		int mouseY = 0;
	};

	class Writer
	{
	public:
		void init(RT* _rt);

		IDWriteTextFormat* get(int size, int weight = DWRITE_FONT_WEIGHT_LIGHT, std::string font = "Verdana");

		void print(std::string text, ID2D1SolidColorBrush*color, IDWriteTextFormat*font, D2D1_RECT_F place);

	private:
		RT* rt;
		IDWriteFactory* writeFactory;

		std::map<std::tuple<int, int, std::string>, IDWriteTextFormat*> data;
	};

	class Color
	{
	public:
		void init(ID2D1HwndRenderTarget* rt);

		ID2D1SolidColorBrush* get(int i);
		ID2D1SolidColorBrush* get(int r, int g, int b);

	private:
		ID2D1HwndRenderTarget* renderTarget;
		std::map<int, ID2D1SolidColorBrush*> data;
	};

	class Frame : public View
	{
	public:
		using View::View;
		//Frame();
		void init(std::string name, int width, int height);
		void runMessageLoop();

		static void askRepaint();
		static void showWindow(int mode);
		static void closeWindow();

		static HWND getHWND();

	private:
		static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		HRESULT createDeviceIndependentResources();
		HRESULT createDeviceResources();

		static bool repaint;
		static bool close;

		bool continues = true;
		bool mouseTracked = false;
		TRACKMOUSEEVENT tme;

		static HWND hwnd;
		RT* rt;
		ID2D1Factory* Direct2dFactory;
		Writer writer;
		Color color;
	};

	class Button : public View
	{
	public:
		using View::View;
		void init(std::function<void()> _onClick, std::string _title = "");

		void render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect) override;

		void setHorizontalDragable(int _maxLeft, int _maxRight, std::function<void(float)> _onVerticalDrag);
		void moveHorizontalPlace(int i);
		float getHorizontalRatio();

		void setVerticalDragable(int _maxTop, int _maxBotton, std::function<void(float)> _onVerticalDrag);
		void moveVerticalPlace(int i);
		float getVerticalRatio();

		bool activated = true;

		int maxLeft;
		int maxRight;
		int maxTop;
		int maxBottom;

		D2D1_RECT_F box;

	private:
		std::string title;
		D2D1_RECT_F titleBox;
		std::function<void()> onClick;
		int state = 0;

		bool isDragging = false;

		bool horizontalDrag = false;
		std::function<void(float)> onHorizontalDrag;
		int horizontalStart;

		bool verticalDrag = false;
		std::function<void(float)> onVerticalDrag;
		int verticalStart;
	};

	class MovingView : public View
	{
	public:
		using View::View;

		void init(bool _horizontal, bool _vertical);

		void update() override;
		void winEvent(Frame * frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;

		View* getMovingView();

		void setScrollZoom(bool _horizontal, bool _vertical);

	private:
		void setup(int xSize, int ySize);
		void resize(int horizontalSize, int verticalSize);

		View* staticView;
		View* movingView;

		bool vertical;
		bool horizontal;

		bool verticalScrollZoom;
		bool horizontalScrollZoom;

		Button* left;
		Button* right;
		Button* horizontalBar;

		Button* up;
		Button* down;
		Button* verticalBar;

		int buttonSize = 20;
		int spaceSize = 5;
	};

	class TabView : public View
	{
	public:
		using View::View;
		View* getNewTab(std::string name);

		void render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect) override;

	private:
		std::vector<View*> tabs;
		int currentTab = -1;
	};

	class MainBar : public View
	{
	public:
		using View::View;
		void init(std::string _title = "");

		void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override;

	private:
		D2D1_RECT_F background;
		std::string title;
		D2D1_RECT_F titleBox;
		Button* closeButton;
		bool move = false;
		POINT movePoint;
	};

	class OutputView : public View
	{
	public:
		using View::View;
		void init();

		void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override;
		void update() override;
		void winEvent(Frame* frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;

		void setDefault();
		void write(std::string s);

	private:
		std::vector<std::string> text;

		int xOffset = 10;
		int yOffset = 10;
		int fontSize = 14;
	};

	struct OutputForward
	{
	public:
		OutputView* outputView = nullptr;
	};

	extern OutputForward Out;

	template<class T> OutputForward& operator<<(OutputForward& out, T t)
	{
		if (out.outputView != nullptr)
		{
			out.outputView->write(std::to_string(t));
		}
		return out;
	}
	template<> OutputForward& operator<< <const char*>(OutputForward& out, const char* s);
	template<> OutputForward& operator<< <std::string>(OutputForward& out, std::string s);

}