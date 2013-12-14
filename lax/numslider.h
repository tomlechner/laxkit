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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_NUMSLIDER_H
#define _LAX_NUMSLIDER_H

#include <lax/itemslider.h>

namespace Laxkit {

#define NUMSLIDER_WRAP (1<<20)

class NumSlider : public ItemSlider
{
 protected:
	int lastitem;
	char *label;
	virtual void wraptoextent();
	virtual int getid(int i) { return curitem; }
	virtual int numitems() { return max-min+1; }
 public:
	int min,max;
	NumSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis,const char *nlabel,int nmin,int nmax,int cur=-10000);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int SelectPrevious();
	virtual int SelectNext();
	virtual int Select(int n);
	virtual void Refresh();
	virtual const char *Label(const char *nlabel);
	virtual int NewMin(int nmin) { return min=nmin; }
	virtual int NewMax(int nmax) { return max=nmax; } //*** doesn't do max>min checking
	virtual int NewMinMax(int nmin,int nmax) { max=nmax; return min=nmin; }
	virtual int Value() { return curitem; }
};

} // namespace Laxkit

#endif