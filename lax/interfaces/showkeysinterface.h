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
//    Copyright (C) 2014 by Tom Lechner
//
#ifndef _LAX_PRESSUREMAPINTERFACE_H
#define _LAX_PRESSUREMAPINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/coordinate.h>

namespace LaxInterfaces { 



class ShowKeysInterface : public anInterface
{
  protected:
	//int showdecs;
	//int device;
	//std::string device_name;

	virtual int DrawBox(const char *str,int x,int y);
  public:
	unsigned int showkeys_style;
	int placement;
	double ui_scale;
	int buttonmask;
	unsigned int currentkey, currentstate;

	ShowKeysInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~ShowKeysInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "ShowKeys"; }
	const char *Name();
	const char *whattype() { return "ShowKeysInterface"; }
	const char *whatdatatype() { return NULL; } // is creation only

	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

};

} // namespace LaxInterfaces

#endif

