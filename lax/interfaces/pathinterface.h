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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//
#ifndef _LAX_PATHINTERFACE_H
#define _LAX_PATHINTERFACE_H


#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/fillstyle.h>
#include <lax/screencolor.h>
#include <lax/dump.h>



namespace LaxInterfaces {


class PathOperator;

//These are added to Coordinate::flags, and devs should ensure they do not conflict with normal flags
#define BEZ_MASK           (255<<16)
#define BEZ_STIFF_EQUAL    (1<<16)
#define BEZ_STIFF_NEQUAL   (1<<17)
#define BEZ_NSTIFF_EQUAL   (1<<18)
#define BEZ_NSTIFF_NEQUAL  (1<<19)

 // NOTE that the FILL_* is basically an enum, not a bunch of flags
enum PathFillType {
	FILL_NONE,
	FILL_NONZERO,
	FILL_ZERO,
	FILL_EVEN_ODD,
	FILL_ODD_EVEN
};

//-------------------- Path ---------------------------

class Path : public LaxFiles::DumpUtility
{
 protected:
 public:
	static Laxkit::PtrStack<PathOperator> basepathops; 

	Coordinate *path; // path is not necessarily the head, but is a vertex
	LineStyle *linestyle; 

	Path();
	Path(Coordinate *np,LineStyle *nls=NULL);
	virtual ~Path();
	virtual Path *duplicate();

	 //building functions
	virtual void fixpath();
	virtual Coordinate *lastPoint(int v=0);
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual void append(flatpoint p,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual void append(Coordinate *coord);
	virtual void appendBezFromStr(const char *value);
	virtual void close();
	virtual void clear();

	 //info functions
	virtual int Intersect(flatpoint p1,flatpoint p2, int isline, double startt, flatpoint *pts,int ptsn, double *t,int tn);
	virtual int PointAlongPath(double t, int tisdistance, flatpoint *point, flatpoint *tangent);
	virtual flatpoint ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist);
	virtual int Reverse();
	virtual double Length(double tstart,double tend);
	virtual double distance_to_t(double distance, int *err);
	virtual double t_to_distance(double t, int *err);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


//-------------------- PathsData ---------------------------
	


class PathsData : virtual public SomeData
{
 protected:
 public:
	unsigned long style; // contains FILL_* for combining(?)
	Laxkit::PtrStack<Path> paths;
	LineStyle *linestyle; //!< This is the default line style for any paths that are added.
	FillStyle *fillstyle; //!< This is the fill style for the collection of paths

	PathsData(unsigned long ns=FILL_NONZERO);
	virtual ~PathsData();
	virtual const char *whattype() { return "PathsData"; }
	virtual void FindBBox();
	virtual SomeData *duplicate(SomeData *dup);

	virtual int line(double width,int cap=-1,int join=-1,Laxkit::ScreenColor *color=NULL);
	virtual int fill(Laxkit::ScreenColor *color);

	virtual int hasCoord(Coordinate *co);
	virtual int pathHasCoord(int pathindex,Coordinate *co);
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void append(flatpoint p,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void appendRect(double x,double y,double w,double h,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void close(int whichpath=-1);
	virtual void pushEmpty(int where=-1,LineStyle *nls=NULL);
	virtual void fixpath(int index);
//	virtual int AddAfter(Coordinate *afterwhich,flatpoint p); 
//	virtual int Delete(Coordinate *which); // returns num left in stack
	virtual void clear(int which=-1);
	virtual void ApplyTransform();
	virtual void MatchTransform(Affine &affine);
	virtual void MatchTransform(const double *mm);

	virtual flatpoint ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist, int *pathi);
	virtual int Intersect(int pathindex,flatpoint p1,flatpoint p2, int isline, double startt,flatpoint *pts,int ptsn, double *t,int tn);
	virtual int PointAlongPath(int pathindex, double t, int tisdistance, flatpoint *point, flatpoint *tangent);
	virtual Coordinate *GetCoordinate(int pathi, double t);
	virtual int ReversePath(int pathindex);
	virtual double Length(int pathi, double tstart,double tend);
	//virtual int NumPoints(int vertices=1);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


//-------------------- PathsData utility

PathsData *SvgToPathsData(PathsData *existingpath, const char *d,char **end_ptr);


//-------------------- PathOperator ---------------------------
	

class PathInterface;

class PathOperator : public Laxkit::anObject
{
 public:
	int id;

	PathOperator(int nid=-1);
	virtual ~PathOperator();
	virtual void dumpOut(FILE *f, int indent, SegmentControls *controls, int what, Laxkit::anObject *context) = 0;
	virtual void dumpIn(Coordinate *attachto, LaxFiles::Attribute *att, Laxkit::anObject *context) = 0;

	virtual void drawControls(Laxkit::Displayer *dp, int which, SegmentControls *controls,
							  int showdecs, PathInterface *pathi) = 0;

	 //return an interface that can manipulate relevant data
	virtual anInterface *getInterface(Laxkit::Displayer *dp,PathsData *data) = 0;

	 //return a thing id for yes, there is something at screen coordinate (x,y), or 0 for none
	virtual int scan(Laxkit::Displayer *dp,SegmentControls *ctl,double x,double y) = 0;

	 // Return a new point and associated points. It must return
	 // the most previous point in the group. This gets appended to whatever. 
	 // It must not return closed loops. By comparison, a new bez point would create 3
	 // points: control1--vertex--control2, and would return control1.
	 // The returned segment must start and end with a vertex.
	virtual Coordinate *newPoint(flatpoint p) = 0; // pop after/before, prefunit=0 means full unit

	virtual int ShiftPoint(Coordinate *pp,flatpoint d) = 0;
	virtual int Rotate(flatpoint o,double angle,Coordinate *pp) = 0;
	virtual int Scale(flatpoint o,double f,int contsrain, Coordinate *pp) = 0;

	//virtual int UseThis(***);
};
	


	
//-------------------- PathInterface ---------------------------

enum PathInterfaceSettings {
	PATHI_One_Path_Only    =(1<<0),
	PATHI_Two_Point_Minimum=(1<<1),
	PATHI_Path_Is_Screen   =(1<<2),
	PATHI_Path_Is_M_Screen =(1<<3),
	PATHI_Path_Is_Real     =(1<<4),
	PATHI_Path_Is_M_Real   =(1<<5),
	PATHI_Esc_Off_Sub      =(1<<6),
	PATHI_Plain_Click_Add  =(1<<7)
};

enum PathInterfaceActions {
	PATHIA_CurpointOnHandle,
	PATHIA_CurpointOnHandleR,
	PATHIA_Pathop,
	PATHIA_ToggleFillRule,
	PATHIA_ToggleFill,
	PATHIA_ToggleStroke,
	PATHIA_ColorFillOrStroke,
	PATHIA_RollNext,
	PATHIA_RollPrev,
	PATHIA_ToggleAddAfter,
	PATHIA_TogglePointType,
	PATHIA_Select,
	PATHIA_SelectInPath,
	PATHIA_Close,
	PATHIA_Decorations,
	PATHIA_StartNewPath,
	PATHIA_StartNewSubpath,
	PATHIA_Wider,
	PATHIA_Thinner,
	PATHIA_WidthStep,
	PATHIA_WidthStepR,
	PATHIA_MakeCircle,
	PATHIA_MakeRect,
	PATHIA_Reverse,
	PATHIA_Delete,
	PATHIA_Combine,
	PATHIA_ExtractPath,
	PATHIA_ExtractAll,
	PATHIA_Copy,
	PATHIA_Cut,
	PATHIA_Paste,

	PATHIA_MAX
};

class PathInterface : public anInterface
{
 protected:
	class SelectedPoint //prep for actions on multiple pathsdata objects
	{
	  public:
		ObjectContext *context;
		PathsData *paths;
		Coordinate *point;
		int pathindex;
		int pointindex;
	};

	 //PathInterface potentially non-local state
	int addmode;
	int editmode;
	LineStyle *linestyle,*defaultline;
	FillStyle *fillstyle,*defaultfill;

	 //other state
	int constrain;
	int addafter;
	int colortofill;
	double widthstep;
	
	Path *curpath;
	flatpoint curdirv;
	Coordinate *curdirp;
	Coordinate  defaulthoverp;
	Coordinate *curvertex; // curvertex points to the relevant vertex point, not to the last selected point
	Coordinate *lbfound;
	int lbselected;
	int drawhover;
	int hoverdevice;
	int hoverpointtype;
	flatpoint hoverpoint;
	flatpoint hoversegment[4];
	int lasth; //direction of toggling selected handle
	
	//Laxkit::PtrStack<PathOperator> pathops; 
	PathOperator *curpathop;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	
	virtual void clearSelection();
	virtual PathOperator *getPathOpFromId(int iid);
	virtual PathOperator *getPathOp(Coordinate *p);
	virtual void selectPoint(Coordinate *p,char flag);
	virtual void removeSegment(Coordinate *c);
	virtual Coordinate *scannear(Coordinate *p,char u,double radius=5);
	virtual void SetCurvertex(Coordinate *p, int path=-1);
	virtual void UpdateDir();
	virtual int ConnectEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);
	virtual int MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);

	virtual int shiftBezPoint(Coordinate *p,flatpoint d);
	virtual int shiftSelected(flatpoint d);
	virtual int scaleSelected(flatpoint center,double f,int constrain);
	virtual int rotateSelected(flatpoint center,double angle);

	virtual void Modified(int level);
	virtual void hoverMessage();
	virtual void drawNewPathIndicator(flatpoint p,int which);
 public:
	 // the following three comprise the default PathInterface settings.
	unsigned long controlcolor;
	unsigned long creationstyle;
	unsigned long pathi_style;
	
	Laxkit::PtrStack<Coordinate> curpoints; //*** how make this nondestructive/undoable changes?
	
	int showdecs;
	int verbose;
	PathsData *data;
	ObjectContext *poc;

	PathInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~PathInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup);
	virtual PathsData *newPathsData();

	 // from anInterface:
	virtual const char *IconId() { return "Path"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PathInterface"; }
	virtual const char *whatdatatype() { return "PathsData"; }
	virtual void Dp(Laxkit::Displayer *ndp);
	virtual int DrawDataDp(Laxkit::Displayer *ndp,SomeData *ndata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int DrawData(Laxkit::anObject *ndata, Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual ObjectContext *Context() { return poc; }
	virtual int Refresh();
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d=NULL);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
	
	//virtual void RegisterOp(PathOperator *apathop);
	virtual int ChangeCurpathop(int newiid);

	virtual int DeleteCurpoints();
	virtual int DeletePoint(Coordinate *p);
	virtual void deletedata();
	virtual Coordinate *scan(int x,int y,int pmask=0, int *pathindex=NULL);
	virtual Coordinate *scanEndpoints(int x,int y,int *pathindex,Coordinate *exclude);
	virtual int scanHover(int x,int y,unsigned int state);
	virtual flatpoint screentoreal(int x,int y);
	virtual flatpoint realtoscreen(flatpoint r);
	virtual int toggleclosed(int c=-1);
	virtual int AddPoint(flatpoint p);
	virtual void SetPointType(int newtype);
	virtual void SetPointType(Coordinate *v,int newtype);
	virtual void MakeCircle();
	virtual void MakeRect();
	virtual int CutNear(flatpoint hoverpoint);
};


} // namespace LaxInterfaces


#endif
