//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2004-2011,2013 by Tom Lechner
//




#include <lax/menubutton.h>
#include <lax/popupmenu.h>
#include <lax/drawingdefs.h>
#include <lax/laxutils.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


	
/*! \class MenuButton
 * \brief Simple class for a button pops up a menu instead of toggling itself in and out.
 *
 * \todo should probably refcount menuinfos
 */


/*! Does not call inc_count() on the image.
 *
 * \todo MENUBUTTON_DOWNARROW does faulty things
 */
MenuButton::MenuButton(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes, int nid, 
						MenuInfo *menu, int absorb,
						const char *nlabel,
						const char *filename,LaxImage *img,
						int npad,int ngap)
		: Button(parnt,nname,ntitle,nstyle&ANXWIN_MASK,xx,yy,ww,hh,brder,prev,nowner,nsendmes,nid,
					 nlabel,filename,img,npad,ngap)
{
	menubutton_style=nstyle&(~ANXWIN_MASK);
	menuinfo=menu;
	if (menu && !absorb) menu->inc_count();

	if (nstyle&MENUBUTTON_ICON_ONLY) setWinStyle(IBUT_ICON_ONLY, 1);
	else if (nstyle&MENUBUTTON_TEXT_ONLY) setWinStyle(IBUT_TEXT_ONLY, 1);
	else if (nstyle&MENUBUTTON_TEXT_ICON) setWinStyle(IBUT_TEXT_ICON, 1);
	else if (nstyle&MENUBUTTON_ICON_TEXT) setWinStyle(IBUT_ICON_TEXT, 1);

	if (nstyle&MENUBUTTON_TEXT_ONLY) labelstyle=LAX_TEXT_ONLY;
	else if (nstyle&MENUBUTTON_ICON_ONLY) labelstyle=LAX_ICON_ONLY;
	else if (nstyle&MENUBUTTON_ICON_TEXT) labelstyle=LAX_ICON_TEXT;
	else if (nstyle&MENUBUTTON_TEXT_ICON) labelstyle=LAX_TEXT_ICON;



	if (nstyle&MENUBUTTON_DOWNARROW) SetGraphic(THING_Triangle_Down,-1,-1);

	if (ww<2 || hh<2) WrapToExtent((ww<2?1:0)|(hh<2?2:0));
}


/*! Dec_count menuinfo.
 */
MenuButton::~MenuButton()
{
	if (menuinfo) menuinfo->dec_count();
}

/*! Return 0 for success, 1 for error.
 */
int MenuButton::SetMenu(MenuInfo *menu, int absorb)
{
	if (!menu) return 1;
	if (menuinfo) menuinfo->dec_count();
	menuinfo=menu;
	if (!absorb) menuinfo->inc_count();
	return 0;
}

//! Create the Popup MenuSelector. Called from LBDown().
void MenuButton::makePopup(int mouseid)
{
	if (!menuinfo || !menuinfo->n()) return;

//	MenuSelector *popup;
//	popup=new MenuSelector(NULL,menuinfo->name?menuinfo->name:"Button Popup",
//					menuinfo->title?menuinfo->title:"Button Popup",
//					ANXWIN_BARE|ANXWIN_HOVER_FOCUS,
//					0,0,0,0, 1, 
//					NULL,win_owner,win_sendthis, 
//					MENUSEL_ZERO_OR_ONE|MENUSEL_CURSSELECTS
//					 | MENUSEL_FOLLOW_MOUSE|MENUSEL_SEND_ON_UP
//					 | MENUSEL_GRAB_ON_MAP|MENUSEL_OUT_CLICK_DESTROYS
//					 | MENUSEL_CLICK_UP_DESTROYS|MENUSEL_DESTROY_ON_FOCUS_OFF
//					 | ((win_style&MENUBUTTON_LEFT)?MENUSEL_LEFT:0) 
//					 | ((win_style&MENUBUTTON_RIGHT)?MENUSEL_RIGHT:0)
//					 | ((win_style&MENUBUTTON_SEND_STRINGS)?MENUSEL_SEND_STRINGS:0),
//					menuinfo,0);
	PopupMenu *popup;
	popup=new PopupMenu(menuinfo->title?menuinfo->title:"Button Popup",
						menuinfo->title?menuinfo->title:"Button Popup",
						0,
						0,0,0,0, 1, 
						win_owner,win_sendthis, 
						mouseid,
						menuinfo,0,
						NULL,
						((menubutton_style&MENUBUTTON_SEND_STRINGS)?MENUSEL_SEND_STRINGS:0)
						 | ((menubutton_style&MENUBUTTON_LEFT)?MENUSEL_LEFT:0)
						 | ((menubutton_style&MENUBUTTON_RIGHT)?MENUSEL_RIGHT:0)
					   );

	popup->pad=pad;
	popup->Select(0);
//	popup->SetFirst(curitem,x,y); 
	popup->WrapToMouse(mouseid);
	app->rundialog(popup);
	mousein=0;
	needtodraw=1;

	LaxMouse *m=app->devicemanager->findMouse(mouseid);
	if (m) app->setfocus(popup,0,m->paired_keyboard);
}

//! Any left click down brings up the menu, rather than the default Button::LBDown().
int MenuButton::LBDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d)
{
	if (menubutton_style&MENUBUTTON_CLICK_CALLS_OWNER) {
		send(d->id,0);
	} else makePopup(d->id);
	return 0;
}


} // namespace Laxkit

