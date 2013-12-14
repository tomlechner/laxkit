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
//    Copyright (C) 2004-2012 by Tom Lechner
//

#include <X11/Xlib.h>
#include <lax/interfaces/linestyle.h>
#include <lax/drawingdefs.h>

using namespace Laxkit;
using namespace LaxFiles;


namespace LaxInterfaces {

/*! \class LineStyle
 * \ingroup interfaces
 * \brief Store various characteristics of a line.
 *
 * width==0 is taken to be hairline. width<0 is taken to mean invisible.
 * Care should be taken to make sure that (red,green,blue) corresponds to color.
 *
 * See LaxCapStyle and LaxJoinStyle.
 * 
 * \todo int width; *** including invisible vs. hairline (==0)
 */


LineStyle::LineStyle()
{
	width=0;//hairline
	widthtype=1;//0 for screen width, 1 for real width
	color.red=color.green=0;
	color.blue=color.alpha=0xffff;
	capstyle=LAXCAP_Butt;
	joinstyle=JoinMiter; 
	function=LAXOP_Source;
	dotdash=0;
	mask=~0;
}

LineStyle::LineStyle(int r,int g,int b, int a, double w,int cap,int join,int dot,int func)
{
	color.red=r;
	color.green=g;
	color.blue=b;
	color.alpha=a;
	width=w;
	widthtype=1;
	capstyle=cap;
	joinstyle=join;
	dotdash=dot;
	function=func;
	mask=~0;
}

LineStyle::LineStyle(const LineStyle &l) 
{
	width=l.width;
	widthtype=l.widthtype;
	color=l.color;
	capstyle=l.capstyle;
	joinstyle=l.joinstyle;
	dotdash=l.dotdash; 
	function=l.function;
}

LineStyle &LineStyle::operator=(LineStyle &l) 
{
	width=l.width;
	widthtype=l.widthtype;
	color=l.color;
	capstyle=l.capstyle;
	joinstyle=l.joinstyle;
	dotdash=l.dotdash;
	function=l.function;
	return l;
}

//! Dump in.
void LineStyle::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"color")) {
			int i[4];
			if (IntListAttribute(value,i,4)==4) {
				color.red=i[0];
				color.green=i[1];
				color.blue=i[2];
				color.alpha=i[3];
			}
		} else if (!strcmp(name,"width")) {
			DoubleAttribute(value,&width);
		} else if (!strcmp(name,"capstyle")) {
			if (!strcmp(value,"round")) capstyle=LAXCAP_Round;
 			else if (!strcmp(value,"projecting")) capstyle=LAXCAP_Projecting;
			else capstyle=LAXCAP_Butt;
		} else if (!strcmp(name,"joinstyle")) {
			if (!strcmp(value,"round")) joinstyle=LAXJOIN_Round;
 			else if (!strcmp(value,"bevel")) joinstyle=LAXJOIN_Bevel;
			else joinstyle=LAXJOIN_Miter;
		} else if (!strcmp(name,"dotdash")) {
			IntAttribute(value,&dotdash);
		} else if (!strcmp(name,"function")) {
			IntAttribute(value,&function);
		} else if (!strcmp(name,"mask")) {
			ULongAttribute(value,&mask);
		}
	}
}

//! Set the color. Components are 0..0xffff.
void LineStyle::Color(int r,int g,int b,int a)
{
	color.red  =r;
	color.green=g;
	color.blue =b;
	color.alpha=a;
}

//! ***implement mask!! should only output the actually defined values?
/*! Does:
 * <pre>
 *  color 255 255 255 255
 *  capstyle round|miter|projecting
 *  joinstyle round|miter|bevel
 *  dotdash 1234
 *  function 1
 *  mask 2
 * </pre>
 * 
 * Ignores what. Uses 0 for it.
 */
void LineStyle::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%smask                   #what is active in this linestyle\n", spc);
		fprintf(f,"%scolor 10000 0 0 65535  #rgba in range [0..65535]\n",spc);
		fprintf(f,"%scapstyle round         #or miter, or projecting\n", spc);
		fprintf(f,"%sjoinstyle round        #or miter, or bevel\n",spc);
		fprintf(f,"%sdotdash 5              #an integer whose bits define an on-off pattern\n",  spc);
		fprintf(f,"%sfunction 3             #***mystery number saying how to combine the fill\n", spc);
		fprintf(f,"%swidth %.10g\n", spc,width);
		return;
	}
			
	const char *str;

	fprintf(f,"%smask %lu\n", spc,mask);
	fprintf(f,"%scolor %d %d %d %d\n",spc,color.red,color.green,color.blue,color.alpha);

	if (capstyle==LAXCAP_Butt) str="miter";
	else if (capstyle==LAXCAP_Round) str="round";
 	else if (capstyle==LAXCAP_Projecting) str="projecting";
    else str="?";
	fprintf(f,"%scapstyle %s\n", spc,str);

	if (joinstyle==LAXJOIN_Miter) str="miter";
	else if (joinstyle==LAXJOIN_Round) str="round";
 	else if (joinstyle==LAXJOIN_Bevel) str="bevel";
    else str="?";
	fprintf(f,"%sjoinstyle %d\n",spc,joinstyle);

	fprintf(f,"%sdotdash %d\n",  spc,dotdash);
	fprintf(f,"%sfunction %d\n", spc,function);
	fprintf(f,"%swidth %.10g\n", spc,width);
}

//! Return whether we will actually be drawing a stroke.
/*! Currently, this returns 1 for when function!=LAXOP_Dest and width>=0.
 */
int LineStyle::hasStroke()
{ return function!=LAXOP_Dest && width>=0; }



} // namespace LaxInterfaces
