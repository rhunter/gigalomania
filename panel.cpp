//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>

#include "panel.h"
#include "game.h"
#include "utils.h"
#include "sector.h"

#ifdef USING_QT
#include "qt_screen.h"
#include "qt_image.h"
#include "qt_sound.h"
#else
#include "screen.h"
#include "image.h"
#include "sound.h"
#endif

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
#include <QFeedbackEffect>
#endif

//---------------------------------------------------------------------------

void registerClick() {
	//LOG("registerClick()\n");
	// call for gui items to be registered as a mouse click, rather than continuous press
	playSample(s_guiclick, SOUND_CHANNEL_FX);
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
    QtMobility::QFeedbackEffect::playThemeEffect(QtMobility::QFeedbackEffect::ThemeBasicButton);
#endif
    s_guiclick->setVolume(0.125f);
}

PanelPage::PanelPage(int offset_x,int offset_y) {
	init_panelpage();
	this->offset_x = offset_x;
	this->offset_y = offset_y;
}

PanelPage::PanelPage(int offset_x,int offset_y,const char *infoLMB) {
	init_panelpage();
	this->offset_x = offset_x;
	this->offset_y = offset_y;
	this->setInfoLMB(infoLMB);
}

PanelPage::PanelPage(int offset_x,int offset_y,int w,int h) {
	init_panelpage();
	this->offset_x = offset_x;
	this->offset_y = offset_y;
	this->w = w;
	this->h = h;
}

PanelPage::~PanelPage() {
	if( owner != NULL )
		owner->remove(this);
	free(true);
	delete children;
}

void PanelPage::init_panelpage() {
	this->owner = NULL;
	this->modal_child = NULL;
	this->offset_x = 0;
	this->offset_y = 0;
	this->w = 0;
	this->h = 0;
	this->tolerance = mobile_ui ? 2 : 0;
	this->enabled = true;
	this->children = new vector<PanelPage *>();
	this->popup_item = false;
	this->popup_x = 0;
	this->popup_y = 0;
	this->survive_owner = false;
	this->helpTextOn = true;
}

void PanelPage::add(PanelPage *panel) {
	this->children->push_back(panel);
	panel->owner = this;
}

void PanelPage::remove(PanelPage *panel) {
	if( panel == this->modal_child )
		this->modal_child = NULL;
	panel->owner = NULL;
	remove_vec(this->children, panel);
}

int PanelPage::getLeft() const {
	int parent_left = owner != NULL ? owner->getLeft() : 0;
	return parent_left + offset_x;
}

int PanelPage::getTop() const {
	int parent_top = owner != NULL ? owner->getTop() : 0;
	return parent_top + offset_y;
}

int PanelPage::getRight() const {
	return owner->getLeft() + offset_x + w;
}

int PanelPage::getXCentre() const {
	return owner->getLeft() + offset_x + w/2;
}

int PanelPage::getBottom() const {
	return owner->getTop() + offset_y + h;
}

void PanelPage::setEnabled(bool enabled) {
	this->enabled = enabled;
	for(int i=0;i<nChildren();i++) {
		PanelPage *panel = get(i);
		panel->setEnabled(enabled);
	}
}

void PanelPage::setInfoLMB(const char *text) {
	/*strncpy(infoLMB, text, GUI_MAX_STRING);
	infoLMB[GUI_MAX_STRING] = '\0';*/
	infoLMB = text;
}

void PanelPage::setInfoRMB(const char *text) {
	/*strncpy(infoRMB, text, GUI_MAX_STRING);
	infoRMB[GUI_MAX_STRING] = '\0';*/
	infoRMB = text;
}

/*void PanelPage::setInfoBMB(const char *text) {
	infoBMB = text;
}*/

const char *PanelPage::getInfoLMB() const {
	//return ( *infoLMB == '\0' ) ? NULL : infoLMB;
	return infoLMB.length() == 0 ? NULL : infoLMB.c_str();
}

const char *PanelPage::getInfoRMB() const {
	//return ( *infoRMB == '\0' ) ? NULL : infoRMB;
	return infoRMB.length() == 0 ? NULL : infoRMB.c_str();
}

/*const char *PanelPage::getInfoBMB() const {
	return infoBMB.length() == 0 ? NULL : infoBMB.c_str();
}*/

void PanelPage::free(bool free_this) {
	for(unsigned int i=0;i<children->size();i++) {
		PanelPage *panel = children->at(i);
		// panel should be non-NULL, but to satisfy VS Code Analysis...
		if( panel != NULL && !panel->survive_owner ) {
			if( free_this ) {
				panel->owner = NULL; // this must be done before deletion!
				delete panel;
			}
			else
				panel->free(true);
		}
	}
	if( free_this ) {
		children->clear();
	}
	if( this->modal_child != NULL && free_this ) {
		modal_child = NULL;
	}
}

void PanelPage::drawPopups() {
	if( mobile_ui ) {
		return;
	}
	/*if( owner != NULL )
    {
        // DEBUG
        int sx = (int)((owner->getLeft() + offset_x - tolerance) * scale_width);
        int sy = (int)((owner->getTop() + offset_y - tolerance) * scale_height);
        screen->fillRect(sx, sy, (this->w+2*tolerance)*scale_width, (this->h+2*tolerance)*scale_height, 255, 0, 255);
    }*/
	// popup text
	int m_x = 0, m_y = 0;
	screen->getMouseCoords(&m_x, &m_y);
	PanelPage *panel = this;
	const char *lmb_text = panel->getInfoLMB();
	const char *rmb_text = panel->getInfoRMB();
	//const char *bmb_text = panel->getInfoBMB();
	if( lmb_text != NULL || rmb_text != NULL /*|| bmb_text != NULL*/ ) {
		if( panel->isHelpTextOn() && panel->mouseOver(m_x, m_y) ) {
			if( !popup_item ) {
				popup_item = true;
				popup_x = (int)(m_x/scale_width);
				popup_y = (int)(m_y/scale_height);
				/*popup_x *= scale_width;
				popup_y *= scale_height;*/
				popup_x = (int)(popup_x*scale_width);
				popup_y = (int)(popup_y*scale_height);
			}
			{
				int n_texts = 0;
				/*const char *text[3] = {NULL, NULL, NULL};
				int mice_indx[3] = {-1, -1, -1};*/
				const char *text[3] = {NULL, NULL};
				int mice_indx[3] = {-1, -1};
				if( rmb_text != NULL ) {
					text[n_texts] = rmb_text;
					mice_indx[n_texts] = 1;
					n_texts++;
				}
				if( lmb_text != NULL ) {
					text[n_texts] = lmb_text;
					mice_indx[n_texts] = 0;
					n_texts++;
				}
				/*if( bmb_text != NULL ) {
					text[n_texts] = bmb_text;
					mice_indx[n_texts] = 2;
					n_texts++;
				}*/
				int w = letters_small[0]->getScaledWidth();
				int h = letters_small[0]->getScaledHeight();
				int off_x = (int)(popup_x/scale_width + 24);
				int gap_left = 20;
				int gap_right = 4;
				int gap_y = 4;
				int between_lines_y = 2;
				int between_texts_y = 4;
				bool one_line[3];
				int n_lines[3];
				int total_lines = 0;
				int max_wid = 0;
				for(int j=0;j<n_texts;j++) {
					int this_max_wid = 0;
					textLines(&n_lines[j], &this_max_wid, text[j]);
					if( this_max_wid > max_wid )
						max_wid = this_max_wid;
					one_line[j] = n_lines[j] == 1;
					if( one_line[j] )
						n_lines[j] = 2;
					total_lines += n_lines[j];
				}

				int rect_x = (int)(off_x * scale_width);
				int rect_y = (int)(popup_y - gap_y * scale_height);
				int rect_w = (int)(( max_wid * w + gap_left + gap_right ) * scale_width);
				int rect_h = (int)(( 2 * gap_y + h * total_lines +
					between_lines_y * ( total_lines - 1 ) +
					( between_texts_y - between_lines_y ) * ( n_texts - 1 ) ) * scale_height);
				if( rect_x + rect_w >= default_width_c * scale_width ) {
					off_x = (int)(default_width_c - 8 - rect_w/scale_width);
					rect_x = (int)(off_x * scale_width);
					// also adjust y
					int new_y = (int)(panel->getBottom() * scale_height);
					if( new_y + rect_h >= default_height_c * scale_height ) {
						new_y = (int)(panel->getTop() * scale_height - rect_h);
					}
					popup_y += new_y - rect_y;
					rect_y = new_y;
				}
				else if( rect_y + rect_h >= default_height_c * scale_height ) {
					int new_y = (int)(( default_height_c - 1 ) * scale_height - rect_h);
					popup_y += new_y - rect_y;
					rect_y = new_y;
				}
				//SDL_FillRect(screen->getSurface(), &rect, col_outline);
				rect_x++;
				rect_y++;
				rect_w -= 2;
				rect_h -= 2;
#if SDL_MAJOR_VERSION == 1
				Image *fill_rect = Image::createBlankImage(rect_w, rect_h, 24);
				fill_rect->fillRect(0, 0, rect_w, rect_h, 0, 0, 128);
				fill_rect->convertToDisplayFormat();
				fill_rect->drawWithAlpha(rect_x, rect_y, 160);
				delete fill_rect;
#else
				screen->fillRectWithAlpha(rect_x, rect_y, rect_w, rect_h, 0, 0, 128, 160);
#endif

				int py = (int)(popup_y/scale_height);
				for(int j=0;j<n_texts;j++) {
					icon_mice[mice_indx[j]]->draw(off_x + 4, py, true);
					Image::write(off_x + gap_left, py + (one_line[j] ? h/2 : 0), letters_small, text[j], Image::JUSTIFY_LEFT, true);
					py += n_lines[j] * h + between_texts_y;
				}
			}
		}
		else
			popup_item = false;
	}
	else
		popup_item = false;


	for(unsigned int i=0;i<children->size();i++) {
		PanelPage *panel = children->at(i);
		panel->drawPopups();
	}
}

void PanelPage::draw() {
	for(unsigned int i=0;i<children->size();i++) {
		PanelPage *panel = children->at(i);
		panel->draw();
	}

	this->drawPopups();
}

bool PanelPage::mouseOver(int m_x,int m_y) {
	if( enabled &&
        m_x >= ( this->getLeft() - tolerance ) * scale_width &&
        m_x < ( this->getLeft() + w + tolerance ) * scale_width &&
        m_y >= ( this->getTop() - tolerance ) * scale_height &&
        m_y < ( this->getTop() + h + tolerance ) * scale_height ) {
			return true;
	}
	return false;
}

void PanelPage::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	// mouseOver check disabled, as PanelPages currently have 0 width and height
	/*if( !mouseOver(m_x, m_y) ) {
	return;
	}*/
	if( this->modal_child != NULL ) {
		this->modal_child->input(m_x, m_y, m_left, m_middle, m_right, click);
        return;
	}
	for(unsigned int i=0;i<children->size();i++) {
		PanelPage *panel = children->at(i);
		panel->input(m_x, m_y, m_left, m_middle, m_right, click);
	}
}

/*Button::Button(int x,int y,Image *image) : PanelPage(x, y) {
init_button();
this->image = image;
this->w = image->getScaledWidth();
this->h = image->getScaledHeight();
}

Button::Button(int x,int y,Image *image,char *infoLMB) : PanelPage(x, y, infoLMB) {
init_button();
this->image = image;
this->w = image->getScaledWidth();
this->h = image->getScaledHeight();
}

Button::Button(int x,int y,int w, int h,Image *image) : PanelPage(x, y) {
init_button();
this->w = w;
this->h = h;
this->image = image;
}*/

Button::Button(int x,int y,const char *text,Image *font[]) : PanelPage(x, y) {
	//init_button();
	/*strncpy(this->text, text, GUI_MAX_STRING);
	this->text[GUI_MAX_STRING] = '\0';*/
	this->text = text;
	this->font = font;
	this->w = font[0]->getScaledWidth() * this->text.length();
	this->h = font[0]->getScaledHeight();
	if( mobile_ui ) {
		this->tolerance += 4;
		this->h += 8; // useful for Android, where touches often seem to register lower than I seem to expect
	}
}

Button::~Button() {
}

/*void Button::init_button() {
this->image = NULL;
*this->text = '\0';
this->font = NULL;
}

void Button::draw() {
if( enabled && image != NULL )
image->draw(owner->getLeft() + offset_x, owner->getTop() + offset_y, true);
else if( enabled && *this->text != '\0' ) {
Image::write(owner->getLeft() + offset_x, owner->getTop() + offset_y, this->font, this->text, Image::JUSTIFY_LEFT, true);
}
}*/

void Button::draw() {
	if( enabled ) {
        //LOG("write: %s\n", this->text.c_str());
       /*{
            // DEBUG
            int sx = (int)((owner->getLeft() + offset_x - tolerance) * scale_width);
            int sy = (int)((owner->getTop() + offset_y - tolerance) * scale_height);
            screen->fillRect(sx, sy, (this->w+2*tolerance)*scale_width, (this->h+2*tolerance)*scale_height, 255, 0, 255);
        }*/
        Image::writeMixedCase(owner->getLeft() + offset_x, owner->getTop() + offset_y, this->font, this->font, numbers_yellow, this->text.c_str(), Image::JUSTIFY_LEFT, true);
	}
	PanelPage::draw();
}

ImageButton::ImageButton(int x,int y,const Image *image) : PanelPage(x, y) {
	ASSERT(image != NULL);
	this->image = image;
    /*this->has_alpha = false;
    this->alpha = 255;*/
	this->w = image->getScaledWidth();
	this->h = image->getScaledHeight();
}

ImageButton::ImageButton(int x,int y,const Image *image,const char *infoLMB) : PanelPage(x, y, infoLMB) {
	ASSERT(image != NULL);
	this->image = image;
    /*this->has_alpha = false;
    this->alpha = 255;*/
    this->w = image->getScaledWidth();
	this->h = image->getScaledHeight();
}

ImageButton::ImageButton(int x,int y,int w, int h,const Image *image) : PanelPage(x, y) {
	ASSERT(image != NULL);
	this->image = image;
    /*this->has_alpha = false;
    this->alpha = 255;*/
    this->w = w;
	this->h = h;
}

ImageButton::ImageButton(int x,int y,int w, int h,const Image *image,const char *infoLMB) : PanelPage(x, y, infoLMB) {
	ASSERT(image != NULL);
	this->image = image;
    /*this->has_alpha = false;
    this->alpha = 255;*/
    this->w = w;
	this->h = h;
}

ImageButton::~ImageButton() {
}

void ImageButton::draw() {
    if( enabled && image != NULL ) {
        //LOG("imagebutton: %d, %d\n", owner->getLeft() + offset_x, owner->getTop() + offset_y);
        /*{
            // DEBUG
            int sx = (int)((owner->getLeft() + offset_x - tolerance) * scale_width);
            int sy = (int)((owner->getTop() + offset_y - tolerance) * scale_height);
            screen->fillRect(sx, sy, (this->w+2*tolerance)*scale_width, (this->h+2*tolerance)*scale_height, 255, 0, 255);
        }*/
        /*if( has_alpha ) {
            image->drawWithAlpha(owner->getLeft() + offset_x, owner->getTop() + offset_y, alpha);
        }
        else {
            image->draw(owner->getLeft() + offset_x, owner->getTop() + offset_y, true);
        }*/
        image->draw(owner->getLeft() + offset_x, owner->getTop() + offset_y, true);
    }
	PanelPage::draw();
}

CycleButton::CycleButton(int x,int y,char *texts[],int n_texts,Image *font[]) : PanelPage(x, y) {
	ASSERT( n_texts > 0 );
	this->n_texts = n_texts;
	this->texts = new char *[n_texts];
	int max_len = 0;
	for(int i=0;i<n_texts;i++) {
		int len = strlen(texts[i]);
		if( len > max_len )
			max_len = len;
		this->texts[i] = new char[ len + 1 ];
		strncpy(this->texts[i], texts[i], len);
		(this->texts[i])[len] = '\0';
	}
	this->font = font;
	this->w = font[0]->getScaledWidth() * max_len;
	this->h = font[0]->getScaledHeight();
	this->active = 0;
	this->tolerance *= 2;
}

CycleButton::~CycleButton() {
	for(int i=0;i<n_texts;i++) {
		delete [] (texts[i]);
	}
	delete [] texts;
}

void CycleButton::draw() {
	if( enabled ) {
       /*{
            // DEBUG
            int sx = (int)((owner->getLeft() + offset_x - tolerance) * scale_width);
            int sy = (int)((owner->getTop() + offset_y - tolerance) * scale_height);
            screen->fillRect(sx, sy, (this->w+2*tolerance)*scale_width, (this->h+2*tolerance)*scale_height, 255, 0, 255);
        }*/
		Image::write(owner->getLeft() + offset_x, owner->getTop() + offset_y, this->font, this->texts[ this->active ], Image::JUSTIFY_LEFT, true);
	}
	PanelPage::draw();
}

void CycleButton::setActive(int active) {
	ASSERT( active >= 0 && active < n_texts );
	this->active = active;
}

void CycleButton::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	if( this->mouseOver(m_x, m_y) && m_left && click ) {
		registerClick();
		this->active++;
		if( this->active == this->n_texts )
			this->active = 0;
	}
	PanelPage::input(m_x, m_y, m_left, m_middle, m_right, click);
}

MultiPanel::MultiPanel(int n_pages, int x, int y) : PanelPage(x, y) {
	int i;
	//this->n_pages = n_pages;
	//this->pages = new PanelPage *[n_pages];
	for(i=0;i<n_pages;i++) {
		/*this->pages[i] = new PanelPage(0, 0);
		this->add( this->pages[i] );*/
		this->add( new PanelPage(0, 0) );
		//this->get(i)->owner = this;
	}
	this->c_page = 0;

}

MultiPanel::~MultiPanel() {
	this->free(true);
	//delete [] this->pages;
}

/*void MultiPanel::free() {
for(int i=0;i<this->nChildren();i++)
this->get(i)->free();
}
*/
void MultiPanel::addToPanel(int page,PanelPage *panel) {
	//this->pages[page]->add(panel);
	this->get(page)->add(panel);
}

/*void MultiPanel::drawPopups() {
this->get(this->c_page)->drawPopups();
}*/

void MultiPanel::draw() {
	//this->pages[this->c_page]->draw();
	this->get(this->c_page)->draw();
}

void MultiPanel::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	//this->pages[this->c_page]->input(m_x, m_y, m_b);
	this->get(this->c_page)->input(m_x, m_y, m_left, m_middle, m_right, click);
}

