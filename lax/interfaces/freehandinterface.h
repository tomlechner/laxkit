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
//    Copyright (C) 2004-2007,2011,2014 by Tom Lechner
//
#ifndef _LAX_FREEHANDINTERFACE_H
#define _LAX_FREEHANDINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/coordinate.h>

namespace LaxInterfaces { 


enum FreehandEditorStyles {
	FREEHAND_Coordinates     =(1<<0), //Construct Coordinate points (not implemented)
	FREEHAND_Flatpoints      =(1<<1), //Create a list of flatpoints (not implemented)
	FREEHAND_Raw_Path        =(1<<2), //Create a straight polyline PathsData with all input points
	FREEHAND_Poly_Path       =(1<<3), //Create a simplified polyline PathsData, approximated within a threshhold
	FREEHAND_Bez_Path        =(1<<4), //Create a bezier PathsData based on a simplified polyline
	FREEHAND_Bez_Outline     =(1<<5), //Create a bezier PathsData that is the outline of pressure sensitive line
	FREEHAND_Bez_Weighted    =(1<<6), //Create a bezier PathsData with PathWeightNode markers
	
	FREEHAND_Color_Mesh      =(1<<7), //Create a ColorPatchData using pressure and a gradient
	FREEHAND_Double_Mesh     =(1<<8), //Create a ColorPatchData where the gradient is mirrored about the middle of the line
	FREEHAND_Grid_Mesh       =(1<<9), //Create a PatchData
	FREEHAND_Path_Mesh       =(1<<10), //Create a PatchData based on a bezier weighted path

	FREEHAND_Mesh            =((1<<7)|(1<<8)|(1<<9)|(1<<10)), //mask for any mesh
	FREEHAND_All_Types       =((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9)|(1<<10)),

	FREEHAND_Till_Closed     =(1<<11), //mouse down drag out a line, up and clicking adds points
	FREEHAND_Notify_All_Moves=(1<<12), //send events to owner upon every move
	FREEHAND_Lock_Type       =(1<<13),
	FREEHAND_Remove_On_Up    =(1<<14), //remove the interface from viewport when button up

	FREEHAND_MAX
};

class RawPoint {
  public:
	flatpoint p;
	int flag;
	clock_t time;
	double pressure;
	double tiltx,tilty;
	RawPoint() { time=0; pressure=0; tiltx=tilty=0; flag=0; }
	RawPoint(flatpoint pp) { p=pp; time=0; pressure=0; tiltx=tilty=0; flag=0; }
};

typedef Laxkit::PtrStack<RawPoint> RawPointLine;

class FreehandInterface : public anInterface
{
  protected:
	char showdecs;
	LineStyle linestyle;
	Laxkit::ShortcutHandler *sc;
	clock_t ignore_clock_t;

	int findLine(int id);

	virtual int send(int i);
	virtual void sendObject(LaxInterfaces::SomeData *tosend, int i);
	virtual void RecurseReduce(RawPointLine *l, int start, int end, double epsilon);
	virtual void RecurseReducePressure(RawPointLine *l, int start, int end, double epsilon);
	virtual RawPointLine *Reduce(int i, double epsilon);
	virtual RawPointLine *ReducePressure(int i, double epsilon);
	virtual Coordinate *BezApproximate(RawPointLine *l);

  public:
	unsigned int freehand_style;
	double brush_size;
	int ignore_tip_time; //milliseconds of final input to ignore, as pressure produces ugly ends
	double smooth_pixel_threshhold;
	Laxkit::ScreenColor linecolor;
	Laxkit::ScreenColor pointcolor;

	Laxkit::PtrStack<RawPointLine> lines;
	Laxkit::NumStack<int> deviceids;

	FreehandInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~FreehandInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Freehand"; }
	virtual const char *Name();
	virtual const char *whattype() { return "FreehandInterface"; }
	virtual const char *whatdatatype() { return NULL; } // is creation only
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int IgnoreTipTime(int milliseconds);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	//virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual unsigned int SendType(unsigned int type);

};

} // namespace LaxInterfaces

#endif

