#pragma once

/** Set of classes for handling a general GUI - ideally there shouldn't be
*   anything specific to a particular game in these classes.
*/

#include "resources.h"

namespace Gigalomania {
	class Image;
}

using namespace Gigalomania;


/*inline bool mouse_left(Uint32 m_b) {
	return ( m_b & SDL_BUTTON(1) ) != 0;
}

inline bool mouse_right(Uint32 m_b) {
	return ( m_b & SDL_BUTTON(3) ) != 0;
}*/

void registerClick();

using std::vector;
using std::string;

namespace Gigalomania {
	class PanelPage : public TrackedObject {
	private:
		void init_panelpage();
	protected:
		bool enabled;
		vector <PanelPage *> *children;
		int popup_x;
		int popup_y;
		bool popup_item;

		bool helpTextOn;
		string infoLMB;
		string infoRMB;
		//string infoBMB;

		int offset_x, offset_y;
		int w, h;
		int tolerance;

		PanelPage *owner;
		bool survive_owner;

		PanelPage *modal_child;

		virtual void drawPopups();
	public:

		PanelPage(int offset_x,int offset_y);
		PanelPage(int offset_x,int offset_y,const char *infoLMB);
		PanelPage(int offset_x,int offset_y,int w,int h);
		virtual ~PanelPage();

		virtual const char *getClass() const { return "CLASS_PANELPAGE"; }
		virtual void add(PanelPage *panel);
		virtual void remove(PanelPage *panel);
		virtual PanelPage *get(int index) {
			//return (PanelPage *)this->children->elementAt(index);
			return this->children->at(index);
		}
		const virtual PanelPage *get(int index) const {
			//return (PanelPage *)this->children->elementAt(index);
			return this->children->at(index);
		}
		virtual int nChildren() const {
			return children->size();
		}
		/*void setModalChild(PanelPage *panel) {
			// must already be a child!
			this->modal_child = panel;
		}*/
		void setModal() {
			// must already be owned!
			this->owner->modal_child = this;
		}
		virtual bool hasModal() const {
			return this->modal_child != NULL;
		}
		virtual void setEnabled(bool enabled);
		virtual bool isEnabled() const {
			return this->enabled;
		}
		virtual void enableHelpText(bool helpTextOn) {
			this->helpTextOn = helpTextOn;
		}
		virtual bool isHelpTextOn() const {
			return this->helpTextOn;
		}
		virtual void setInfoLMB(const char *text);
		virtual void setInfoRMB(const char *text);
		//virtual void setInfoBMB(const char *text);
		virtual const char *getInfoLMB() const;
		virtual const char *getInfoRMB() const;
		//virtual const char *getInfoBMB() const;
		void setSurviveOwner(bool survive_owner) {
			this->survive_owner = survive_owner;
		}

		// gets position relative to parent
		int getOffsetX() const {
			return this->offset_x;
		}
		int getOffsetY() const {
			return this->offset_y;
		}
		// gets the abolute position (taking into account parents)
		virtual int getLeft() const;
		virtual int getTop() const;
		virtual int getRight() const;
		virtual int getXCentre() const;
		virtual int getBottom() const;
		int getWidth() const {
			return this->w;
		}
		int getHeight() const {
			return this->h;
		}
		void setTolerance(int tolerance) {
			this->tolerance = tolerance;
		}

		virtual void free(bool free_this);
		virtual void draw();
		virtual bool mouseOver(int m_x,int m_y);
		virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
	};

	class Button : public PanelPage {
		Image **font;
		string text;
	public:

		Button(int x,int y,const char *text,Image *font[]);
		Button(int x,int y,int h,const char *text,Image *font[]);
		virtual ~Button();

		virtual const char *getClass() const { return "CLASS_BUTTON"; }
		virtual void draw();
	};

	class ImageButton : public PanelPage {
		const Image *image;
		/*bool has_alpha;
		unsigned char alpha;*/
	public:

		ImageButton(int x,int y,const Image *image);
		ImageButton(int x,int y,const Image *image,const char *infoLMB);
		ImageButton(int x,int y,int w,int h,const Image *image);
		ImageButton(int x,int y,int w,int h,const Image *image,const char *infoLMB);
		virtual ~ImageButton();

		void setImage(const Image *image) {
			this->image = image;
		}
		/*void setAlpha(bool has_alpha, unsigned char alpha) {
			this->has_alpha = has_alpha;
			this->alpha = alpha;
		}*/
		virtual const char *getClass() const { return "CLASS_IMAGEBUTTON"; }
		virtual void draw();
	};

	class CycleButton : public PanelPage {
		int active;
		Image **font;
		char **texts;
		int n_texts;

	public:
		CycleButton(int x,int y,const char *texts[],int n_texts,Image *font[]);
		virtual ~CycleButton();

		virtual const char *getClass() const { return "CLASS_CYCLEBUTTON"; }
		virtual void draw();
		virtual int getActive() const {
			return this->active;
		}
		virtual void setActive(int active);
		virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
	};

	class MultiPanel : public PanelPage {
	protected:
		int c_page;
	public:
		MultiPanel(int n_pages,int x,int y);
		virtual ~MultiPanel();

		//virtual void free();
		virtual void addToPanel(int page,PanelPage *panel);

		//virtual void drawPopups();
		virtual void draw();
		virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
		virtual void setPage(int page) {
			this->c_page = page;
		}
		virtual int getPage() const {
			return this->c_page;
		}
		virtual bool hasModal() const {
			return this->modal_child != NULL || this->get(this->c_page)->hasModal();
		}
	};
}
