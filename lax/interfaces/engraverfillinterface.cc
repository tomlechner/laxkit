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



#include <lax/interfaces/engraverfillinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/dumpcontext.h>
#include <lax/imagedialog.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/iconmanager.h>
#include <lax/laxutils.h>
#include <lax/colorsliders.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>
#include <lax/popupmenu.h>
#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/somedataref.h>

// *** DBG:
#include <lax/laximages-imlib.h>


#include <lax/lists.cc>
#include <lax/refptrstack.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//------------------------------------- Engraver identifiers ------------------------
enum EngraveControls {
	ENGRAVE_None=0,

	 //------------ panel ids...
	ENGRAVE_Panel           = PATCHA_MAX,
	ENGRAVE_Mode_Selection,
	ENGRAVE_Groups,

	 //--------------- point group
	ENGRAVE_Toggle_Group_List,
	ENGRAVE_Previous_Group,
	ENGRAVE_Next_Group,
	ENGRAVE_Group_Name,
	ENGRAVE_Group_List,
	ENGRAVE_Group_Linked,
	ENGRAVE_Group_Active,
	ENGRAVE_Group_Color,
	ENGRAVE_Delete_Group,
	ENGRAVE_New_Group,
	ENGRAVE_Merge_Group,
	ENGRAVE_Dup_Group, // <- is same as new?
	ENGRAVE_Group_Up,
	ENGRAVE_Group_Down,

	 //--------------- tracing  
	ENGRAVE_Trace_Box,
	ENGRAVE_Trace_Weight_Map,
	ENGRAVE_Trace_Once,
	ENGRAVE_Trace_Load,
	ENGRAVE_Trace_Clear,
	ENGRAVE_Trace_Continuous,
	ENGRAVE_Trace_Object,
	ENGRAVE_Trace_Opacity,
	ENGRAVE_Trace_Identifier,
	ENGRAVE_Trace_Curve,
	ENGRAVE_Trace_Move_Mesh,

	 //--------------- Dashes  
	ENGRAVE_Dashes,
	ENGRAVE_Dash_Same_As,
	ENGRAVE_Dash_Threshhold,
	ENGRAVE_Dash_Zero_Threshhold,
	ENGRAVE_Dash_Broken_Threshhold,
	ENGRAVE_Dash_Length,
	ENGRAVE_Dash_Seed,
	ENGRAVE_Dash_Random,
	ENGRAVE_Dash_Taper,
	ENGRAVE_Dash_Density,
	ENGRAVE_Dash_Caps,
	ENGRAVE_Dash_Join,

	 //--------------- Direction  
	ENGRAVE_Direction,
	ENGRAVE_Dir_Same_As,
	ENGRAVE_Dir_Type,
	ENGRAVE_Dir_Current,
	ENGRAVE_Dir_Paint,
	ENGRAVE_Dir_Create_From_Cur,
	ENGRAVE_Dir_From_Trace,
	ENGRAVE_Dir_Load_Normal,
	ENGRAVE_Dir_Load_Image,

	 //--------------- Spacing  
	ENGRAVE_Spacing,
	ENGRAVE_Spacing_Default,
	ENGRAVE_Spacing_Use_Map,
	ENGRAVE_Spacing_Map_File,
	ENGRAVE_Spacing_Same_As,
	ENGRAVE_Spacing_Preview,
	ENGRAVE_Spacing_Create_From_Cur,
	ENGRAVE_Spacing_Load,
	ENGRAVE_Spacing_Save,
	ENGRAVE_Spacing_Paint,

	 //------------tracing panel (some below in tool controls
	ENGRAVE_Tracing,
	ENGRAVE_Trace_Same_As,
	ENGRAVE_Trace_Thicken,
	ENGRAVE_Trace_Thin,
	ENGRAVE_Trace_Set,
	ENGRAVE_Trace_Using_type,
	ENGRAVE_Trace_Using,
	ENGRAVE_Trace_Apply,
	ENGRAVE_Trace_Remove,

	ENGRANE_Panel_MAX,

	 //------------on canvas tool controls:

	ENGRAVE_Orient,
	ENGRAVE_Orient_Spacing,
	ENGRAVE_Orient_Position,
	ENGRAVE_Orient_Direction,
	ENGRAVE_Orient_Type,
	ENGRAVE_Orient_Grow,

	 //------modes:
	EMODE_Controls,
	EMODE_Mesh, //dev note: EMODE_Mesh MUST be first in mode list for proper mouse over stuff
	EMODE_Thickness,
	EMODE_Orientation,
	EMODE_Freehand,
	EMODE_Blockout,
	EMODE_Drag, 
	EMODE_PushPull,
	EMODE_AvoidToward,
	EMODE_Twirl,
	EMODE_Turbulence,
	EMODE_Trace,
	EMODE_Direction,
	EMODE_Resolution, //change sample point distribution
	EMODE_MAX

};


//------------------------------------- LinePointCache ------------------------

/*! \class LinePointCache
 * A cache point for use by LinePoint and EngraverFillData.
 */

LinePointCache::LinePointCache(int ntype)
{
	type=ntype;
	weight=0;
	on=dashon=ENGRAVE_On;
	bt=0;

	original=NULL;
	next=NULL;
	prev=NULL;
}

LinePointCache::LinePointCache(LinePointCache *prevpoint)
{
	type=ENGRAVE_Original;
	weight=0;
	on=dashon=ENGRAVE_On;
	bt=0;

	original=NULL;
	next=NULL;
	prev=prevpoint;
	if (prevpoint) prevpoint->next=this;
}

//! Deletes next.
LinePointCache::~LinePointCache()
{
	if (prev) {
		prev->next=NULL;
		prev=NULL;
	}
	if (next) {
		next->prev=NULL;
		delete next;
	}
	if (original) original->cache=NULL;
}

LinePoint *LinePointCache::PrevOriginal()
{
	LinePointCache *lc=this;
	while (lc && !lc->original) lc=lc->prev;
	if (lc) return lc->original;
	return NULL;
}

//! Add an OPEN line segment right after *this.
void LinePointCache::Add(LinePointCache *np)
{
	LinePointCache *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	np->next=next;
	if (next) next->prev=np;

	pp->prev=this;
	next=pp;
}

//! Add an OPEN line segment right before *this.
void LinePointCache::AddBefore(LinePointCache *np)
{
	LinePointCache *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	pp->prev=prev;
	if (prev) prev->next=pp;

	prev=np;
	np->next=this;
}

/*! Place np at a proper point after this. If np->bt==this->t, place right after this.
 * Returns np.
 */
LinePointCache *LinePointCache::InsertAfter(LinePointCache *np)
{
	LinePointCache *p=this;
	double t=np->bt;

	DBG if (t<0) {
	DBG 	cerr <<" ***\n *** t=="<<t<<" is BAD!!!!!\n ***"<<endl;
	DBG     LinePointCache *cc=NULL; cerr <<"--force segfault--"<<cc->bt;
	DBG     exit(1);
	DBG }

	if (t>=1) {

		while (p->type!=ENGRAVE_Original && p->prev) p=p->prev;
		LinePoint *lp=p->original;
		while (t>=1 && lp->next) {
			t--;
			lp=lp->next;
		}
	}
	while (t > p->bt && p->next && p->next->type!=ENGRAVE_Original) {
		if (t<p->next->bt) break;
		p=p->next;
		if (p->type==ENGRAVE_Original) {
			t-=1;
			if (t<=0) break;
		}
	}
	p->Add(np);
	return np;
}


/*! Remove *this from the chain.
 * Returns this->prev. If this->prev does not exist, then return this->next.
 */
LinePointCache *LinePointCache::Detach()
{
	LinePointCache *p=prev;
	if (prev) {
		prev->next=next;
	} else {
		p=next;
	}
	if (this->next) this->next->prev=prev;

	next=prev=NULL;
	original=NULL;
	return p;
}


//------------------------------------- LinePoint ------------------------
/*! \class LinePoint
 * Kind of a temp node for EngraverFillData.
 *
 * Each line point is a definite sample point that lies on an engraver line.
 * Between each LinePoint can be any number of cached points, to flesh
 * out and delineate dashes, and sections that are off, but don't fall cleanly
 * on LinePoint boundaries.
 */

LinePoint::LinePoint()
{
	type=ENGRAVE_Original;
	bt=0;
	on=ENGRAVE_On;
	row=col=0;
	s=t=0;
	weight=1;
	weight_orig=1;
	spacing=-1;
	next=prev=NULL;
	needtosync=1;

	cache=NULL;
}

LinePoint::LinePoint(double ss, double tt, double ww)
{
	type=ENGRAVE_Original;
	bt=0;
	on=ENGRAVE_On;
	next=prev=NULL;
	s=ss;
	t=tt;
	weight_orig=ww;
	weight=ww;
	spacing=-1;
	needtosync=1;

	cache=NULL;
}

LinePoint::~LinePoint()
{
	if (prev) prev->next=NULL;
	if (next) {
		next->prev=NULL;
		delete next;
	}

	if (cache) delete cache;
}


void LinePoint::Clear()
{
	if (next) delete next;
	next=NULL;
}

//! Add an OPEN line segment right after *this.
void LinePoint::Add(LinePoint *np)
{
	LinePoint *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	np->next=next;
	if (next) next->prev=np;

	pp->prev=this;
	next=pp;
}

//! Add an OPEN line segment right before *this.
void LinePoint::AddBefore(LinePoint *np)
{
	LinePoint *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	pp->prev=prev;
	if (prev) prev->next=pp;

	prev=np;
	np->next=this;
}

/*! Create initial this->cache, which is simple mirror of current line.
 * If cache already exists, it is verified to correspond to current line.
 */
void LinePoint::BaselineCache()
{
	LinePoint *l=this, *start=this;
	LinePointCache *lc=NULL;

	if (!l->cache) {
		 //need to create new cache
		do {
			l->cache=new LinePointCache(ENGRAVE_Original);
			l->cache->original=l;
			if (lc) lc->Add(l->cache);

			l->cache->p=l->p;
			l->cache->weight=l->weight;
			l->cache->on=l->on;

			lc=l->cache;
			l=l->next;
		} while (l && l!=start);

		if (l && l==start) l->cache->AddBefore(l->prev->cache);

	} else {
		 //need to validate existing cache
		DBG cerr <<" *** LinePoint::BaselineCache(), must implement validate existing cache!"<<endl;
	}
}

/*! Update the bez handles for this point only.
 */
void LinePoint::UpdateBezHandles()
{
	LinePoint *pp, *nn;
	pp=(prev ? prev : this);
	nn=(next ? next : this);

	flatpoint v=nn->p - pp->p;
	v.normalize();

	double sx=norm(p - pp->p)*.333;
	bez_before = p - v*sx;

	sx=norm(nn->p - p)*.333;
	bez_after = p + v*sx;
}

/*! Copy over everything except next and prev.
 */
void LinePoint::Set(LinePoint *pp)
{
	s=pp->s;
	t=pp->t;
	row=pp->row;
	col=pp->col;
	weight=pp->weight;
	weight_orig=pp->weight_orig;
	on=pp->on;
	needtosync=pp->needtosync;
	p=pp->p;
}


//----------------------------- EngraverLineQuality -----------------------------------

/*! \class EngraverLineQuality
 * Class to hold info about how to render broken lines, caps, and custom mappings of
 * stored weights to actual widths.
 */

EngraverLineQuality::EngraverLineQuality()
{
	//dash_length=spacing*2;
	dash_length=2; //a multiple of group->spacing

	dash_density=0;
	dash_randomness=0;
	randomseed=0;
	zero_threshhold=0;
	broken_threshhold=0;
	dash_taper=0;

	indashcaps =0;
	outdashcaps=0;
	startcaps  =0;
	endcaps    =0;
}

EngraverLineQuality::~EngraverLineQuality()
{
}

EngraverLineQuality *EngraverLineQuality::duplicate()
{
	EngraverLineQuality *dup=new EngraverLineQuality();
	*dup=*this; //shallow copy ok so far!!
	return dup;
}

void EngraverLineQuality::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *EngraverLineQuality::dump_out_atts(Attribute *att,int what,Laxkit::anObject *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {

		att->push("dash_length",      "#This times group->spacing is the length dashes should be between breaks.");
		att->push("dash_randomness",  "#0 for regular spacing, up to 1 how much to randomize spacing.");
		att->push("zero_threshhold",  "#Weights below this value are considered off");
		att->push("broken_threshhold","#Weights below this value are rendered as broken lines.");
		att->push("dash_taper",       "#How much to shrink weight of dashes as zero weight approaches. 0 is all the way, 1 is not at all.");
		att->push("density",          "#The minimum dash density. 0 for all blank at 0, 1 for all solid at 0.");
		att->push("indashcaps",       "#Line cap at the inside start of a dash. Todo!");
		att->push("outdashcaps",      "#Line cap at the inside end of a dash. Todo!");
		att->push("startcaps",        "#Line cap at the start of a whole line. Todo!");
		att->push("endcaps",          "#Line cap at the end of a whole line. Todo!");
		return att;
	}

	char buffer[50];

	sprintf(buffer,"%.10g",dash_length);
	att->push("dash_length",buffer);

	sprintf(buffer,"%.10g",dash_randomness);
	att->push("dash_randomness",buffer);

	sprintf(buffer,"%.10g",zero_threshhold);
	att->push("zero_threshhold",buffer);

	sprintf(buffer,"%.10g",broken_threshhold);
	att->push("broken_threshhold",buffer);

	sprintf(buffer,"%.10g",dash_taper);
	att->push("dash_taper",buffer);

	sprintf(buffer,"%.10g",dash_density);
	att->push("density",buffer);

	sprintf(buffer,"%d",indashcaps);
	att->push("indashcaps",buffer);

	sprintf(buffer,"%d",outdashcaps);
	att->push("outdashcaps",buffer);

	sprintf(buffer,"%d",startcaps);
	att->push("startcaps",buffer);

	sprintf(buffer,"%d",endcaps);
	att->push("endcaps",buffer);

	return att;
}

void EngraverLineQuality::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"dash_length")) {
			DoubleAttribute(value,&dash_length, NULL);

		} else if (!strcmp(name,"dash_randomness")) {
			DoubleAttribute(value,&dash_randomness, NULL);

		} else if (!strcmp(name,"density")) {
			DoubleAttribute(value,&dash_density, NULL);

		} else if (!strcmp(name,"zero_threshhold")) {
			DoubleAttribute(value,&zero_threshhold, NULL);

		} else if (!strcmp(name,"broken_threshhold")) {
			DoubleAttribute(value,&broken_threshhold, NULL);

		} else if (!strcmp(name,"dash_taper")) {
			DoubleAttribute(value,&dash_taper, NULL);

		} else if (!strcmp(name,"indashcaps")) {
			IntAttribute(value,&indashcaps, NULL);

		} else if (!strcmp(name,"outdashcaps")) {
			IntAttribute(value,&outdashcaps, NULL);

		} else if (!strcmp(name,"startcaps")) {
			IntAttribute(value,&startcaps, NULL);

		} else if (!strcmp(name,"endcaps")) {
			IntAttribute(value,&endcaps, NULL);
		}
	}
}

/*! From a weight value, translate through the dash settings to get a new weight value.
 * It will differ from original weight when dash_taper!=0.
 *
 * If weight>=broken, return 2.
 * If weight<=zero, return 1.
 * Otherwise return 0.
 */
int EngraverLineQuality::GetNewWeight(double weight, double *weight_ret)
{
	if (weight>=broken_threshhold) { *weight_ret=weight; return 2; }
	if (weight<=zero_threshhold)   { *weight_ret=weight; return 1; } 

	 //now we figure out the length, gap placement, and width of a dash.
	 //taper means dash width varies from broken to zero+taper*(broken-zero)
	 //actual width varies from broken down to zero

	double a=(weight-zero_threshhold)/(broken_threshhold-zero_threshhold); //0..1, how long between zero and broken
	*weight_ret   = broken_threshhold*a + (dash_taper*(broken_threshhold-zero_threshhold)+zero_threshhold)*(1-a);
	return 0;
}



//--------------------------- NormalDirectionMap -----------------------------
    
NormalDirectionMap::NormalDirectionMap()
{   
    normal_map=NULL;
	data=NULL;
	width=height=0;
}

NormalDirectionMap::NormalDirectionMap(const char *file)
{   
    normal_map=load_image(file);
	data=NULL;
	width=height=0;

    if (normal_map) {
        width=normal_map->w();
        height=normal_map->h();
		unsigned char *dd=normal_map->getImageBuffer();
		data=new unsigned char[width*height*4];
		memcpy(data, dd, width*height*4);
		normal_map->doneWithBuffer(dd);
    }
} 

NormalDirectionMap::~NormalDirectionMap()
{
	delete[] data;
    if (normal_map) normal_map->dec_count();
}

void NormalDirectionMap::Clear()
{
	if (normal_map) normal_map->dec_count();
	normal_map=NULL;
	delete[] data;
	data=NULL;
	width=height=0;
}

/*! Calling with NULL just calls Clear().
 * If image loading fails, return 1.
 * Success returns 0.
 */
int NormalDirectionMap::Load(const char *file)
{
	if (file==NULL) {
		Clear();
		return 0;
	}

	LaxImage *img=load_image(file);
	if (!img) return 1;

	if (normal_map) normal_map->dec_count();
	normal_map=img;

	delete[] data;
	data=NULL;

	width=normal_map->w();
	height=normal_map->h();
	unsigned char *dd=normal_map->getImageBuffer();
	data=new unsigned char[width*height*4];
	memcpy(data, dd, width*height*4);
	normal_map->doneWithBuffer(dd);

	return 0;
}

flatpoint NormalDirectionMap::Direction(double x,double y)
{
    flatpoint p=m.transformPoint(flatpoint(x,y));

    if (p.x<0 || p.x>=width || p.y<0 || p.y>=height) return flatpoint(0,0);

    int i=((int)p.y*width+(int)p.x)*4;
    //cerr <<"p: "<<(int)p.x<<","<<(int)p.y<<" i:"<<i<<"  "<<endl;
    //p=flatpoint(data[i]-128,data[i+1]-128);
    //p=flatpoint(data[i+2]-128,data[i+3]-128);
    p=flatpoint((int)(data[i+1])-128,(int)(data[i+2])-128);

    return p;
}




//------------------------------------- EngraverPointGroup ------------------------

/*! \class EngraverPointGroup
 *
 * Info for groups of points in an EngraverFillData.
 *
 * Built in generator types are: linear, radial, spiral, circular
 */


/*! Default to linear. Warning: leaves trace==NULL!
 */
EngraverPointGroup::EngraverPointGroup()
{
	trace=NULL;
	dashes=NULL;
	numdashes=0;

	id=getUniqueNumber(); //the group number in LinePoint
	name=NULL;
	type=PGROUP_Linear; //what manner of lines
	type_d=0;   //parameter for type, for instance, an angle for spirals
	active=true;
	linked=false;

	spacing=.1;

	position.x=position.y=.5;
	direction.x=1;

	iorefs=NULL;
}

/*! Creates a unique new number for id if nid<0.
 */
EngraverPointGroup::EngraverPointGroup(int nid,const char *nname, int ntype, flatpoint npos, flatpoint ndir, double ntype_d,
										EngraverTraceSettings *newtrace)
{
	dashes=NULL;
	numdashes=0;

	trace=newtrace;
	if (trace) trace->inc_count();
	else trace=new EngraverTraceSettings();

	id=nid;
	if (id<0) id=getUniqueNumber(); //the group number in LinePoint
	name=newstr(nname);
	type=ntype; //what manner of lines
	type_d=ntype_d;   //parameter for type, for instance, an angle for spirals
	position=npos;
	direction=ndir;

	active=true;
	linked=false;

	spacing=.1;

	position.x=position.y=.5;
	direction.x=1;

	iorefs=NULL;
}

EngraverPointGroup::~EngraverPointGroup()
{
	if (trace) trace->dec_count();
	if (dashes) dashes->dec_count();
	delete[] name;
	delete[] iorefs;
}

/*! If keep_name, then do not copy id and name.
 * if link_trace or link_dash, then inc_count those objects instead of making a copy.
 */
void EngraverPointGroup::CopyFrom(EngraverPointGroup *orig, bool keep_name, bool link_trace, bool link_dash)
{
	if (keep_name) {
		id=orig->id;
		makestr(name,orig->name);
	}

	active   =orig->active;
	type     =orig->type;
	type_d   =orig->type_d;
	spacing  =orig->spacing;
	position =orig->position;
	direction=orig->direction;
	color    =orig->color;

	 //probably needs to be more complete here...
	if (trace) { trace->dec_count(); trace=NULL; }

	if (link_trace) { trace=orig->trace; if (trace) trace->inc_count(); }
	else if (orig->trace!=NULL) {
		trace=orig->trace->duplicate();
	}

	if (dashes) { dashes->dec_count(); dashes=NULL; }
	if (link_dash) { dashes=orig->dashes; if (dashes) dashes->inc_count(); }
	else if (orig->dashes) {
		dashes=new EngraverLineQuality;
		*dashes=*orig->dashes; //warning shallow copy!! so far that's ok....
	}


	LinePoint *pp, *lp;

	for (int c=0; c<orig->lines.n; c++) {
		pp=new LinePoint();
		pp->Set(orig->lines.e[c]);
		lines.push(pp);

		lp=orig->lines.e[c]->next;		
		while (lp) {
			pp->next=new LinePoint();
			pp->next->prev=pp;
			pp->next->Set(lp);
			pp=pp->next;
			lp=lp->next;
		}
	}
}

void EngraverPointGroup::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context,
						const char *sharetrace, const char *sharedash)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (what==-1) {
		fprintf(f,"%sid 1             #group id number\n", spc);
		fprintf(f,"%sname Name        #some name for \n", spc);

		fprintf(f,"%sactive yes       #yes|no, whether to use this group or not\n", spc);
		fprintf(f,"%slinked yes       #yes|no, whether to warp along with other linked groups\n", spc);
		fprintf(f,"%stype linear      #or radial, circular\n", spc);
		fprintf(f,"%sposition (.5,.5) #default origin for the pattern \n", spc);
		fprintf(f,"%sdirection (1,0)  #default direction for the pattern \n", spc);
		fprintf(f,"%scolor rgbaf(1.,1.,1.,1.)  #color of lines in this group\n", spc);
		fprintf(f,"%sspacing  .1      #default spacing, in object space, not s,t space \n", spc);
		fprintf(f,"%strace            #trace settings. If sharing with a previous group, use \"trace with: pgroup-name\" \n", spc);
		if (trace) trace->dump_out(f,indent+2,-1,context);
		else {
			EngraverTraceSettings t;
			t.dump_out(f,indent+2,-1,context);
		}
		fprintf(f,"%sdashes           #Dash settings\n", spc);
		if (dashes) dashes->dump_out(f,indent+2,-1,context);
		else {
			EngraverLineQuality d;
			d.dump_out(f,indent+2,-1,context);
		}

		fprintf(f,"%sline \\           #One for each defined line\n",spc);
		fprintf(f,"%s  group 3        #(optional) which point group this line belongs to. Must be first line! 0 means use default settings\n",spc);
		fprintf(f,"%s  (1.0,2.5) 2 on #Any number of points, 1 per line. Coordinate is within mesh. Format is: (x,y) weight on|off|end|start\n",spc);
		fprintf(f,"%s  ...\n",spc);
		return;
	}

	fprintf(f,"%sid %d\n", spc, id);
	if (!isblank(name)) fprintf(f,"%sname %s\n", spc, name);
	//fprintf(f,"%strace    #trace settings.. output TODO!! \n", spc);

	fprintf(f,"%sactive %s\n",spc, active?"yes":"no");
	fprintf(f,"%slinked %s\n",spc, linked?"yes":"no");

	const char *str="linear";
	if (type==PGROUP_Radial) str="radial";
	else if (type==PGROUP_Spiral) str="spiral";
	else if (type==PGROUP_Circular) str="circular";
	fprintf(f,"%stype %s\n", spc, str);
	fprintf(f,"%sposition (%.10g, %.10g)\n", spc,position.x,position.y);
	fprintf(f,"%sdirection (%.10g, %.10g)\n", spc,direction.x,direction.y);
	fprintf(f,"%sspacing  %.10g\n", spc, spacing);

	fprintf(f,"%scolor rgbaf(%.10g,%.10g,%.10g,%.10g)\n",spc, 
			color.red/65535.,
			color.green/65535.,
			color.blue/65535.,
			color.alpha/65535.);
	
	if (trace) {
		if (sharetrace) fprintf(f,"%strace with: %s\n",spc,sharetrace);
		else {
			fprintf(f,"%strace\n",spc);
			trace->dump_out(f,indent+2,what,context);
		}
	}

	if (dashes) {
		if (sharedash) fprintf(f,"%sdashes with: %s\n",spc,sharedash);
		else {
			fprintf(f,"%sdashes\n",spc);
			dashes->dump_out(f,indent+2,what,context);
		}
	}

	 //finally output the lines
	LinePoint *p;
	const char *ons, *dash;
	for (int c=0; c<lines.n; c++) {
		fprintf(f,"%sline \\ #%d\n",spc,c);

		p=lines.e[c];
		while (p) {
			if (p->on==ENGRAVE_Off) ons="off";
			else if (p->on==ENGRAVE_On) ons="on";
			else if (p->on==ENGRAVE_EndPoint) ons="end";
			else if (p->on==ENGRAVE_StartPoint) ons="start";
			fprintf(f,"%s  (%.10g, %.10g) %.10g %s\n",spc, p->s,p->t,p->weight, ons);

			p=p->next;
		}
		
		if (lines.e[c]->cache) {
			fprintf(f,"%slinecache \\ #%d\n",spc,c);
			LinePointCache *cc=lines.e[c]->cache;
			LinePointCache *ccstart=cc;
			const char *onsd;

			do {
				if (cc->on==ENGRAVE_Off) ons="off";
				else if (cc->on==ENGRAVE_On) ons="on";
				else if (cc->on==ENGRAVE_EndPoint) ons="end";
				else if (cc->on==ENGRAVE_StartPoint) ons="start";

				if (cc->dashon==ENGRAVE_Off) onsd="off";
				else if (cc->dashon==ENGRAVE_On) onsd="on";
				else if (cc->dashon==ENGRAVE_EndPoint) onsd="end";
				else if (cc->dashon==ENGRAVE_StartPoint) onsd="start";

				if (cc->type==ENGRAVE_Original) dash="orig";
				else if (cc->type==ENGRAVE_BlockStart ) dash="blockstart";
				else if (cc->type==ENGRAVE_BlockEnd   ) dash="blockend";
				else if (cc->type==ENGRAVE_VisualCache) dash="sample";
				else if (cc->type==ENGRAVE_EndDash    ) dash="dashend";
				else if (cc->type==ENGRAVE_StartDash  ) dash="dashstart";
				else dash="unknown";

				fprintf(f,"%s  %s %.10g (%.10g, %.10g) %.10g %s %s\n",spc, dash, cc->bt, cc->p.x,cc->p.y,cc->weight, ons, onsd);

				cc=cc->next;
			} while (cc && cc!=ccstart);
		}
	}

}

void EngraverPointGroup::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	const char *tracekey=NULL;
	const char *dashkey=NULL;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			IntAttribute(value, &id, NULL);

		} else if (!strcmp(name,"name")) {
			delete[] this->name;
			this->name=newstr(value);

		} else if (!strcmp(name,"active")) {
			active=BooleanAttribute(value);

		} else if (!strcmp(name,"linked")) {
			linked=BooleanAttribute(value);

		} else if (!strcmp(name,"type")) {
			if (!strcasecmp(value,"linear")) type=PGROUP_Linear;
			else if (!strcasecmp(value,"radial")) type=PGROUP_Radial;
			else if (!strcasecmp(value,"spiral")) type=PGROUP_Spiral;
			else if (!strcasecmp(value,"circular")) type=PGROUP_Circular;

		} else if (!strcmp(name,"position")) {
			FlatvectorAttribute(value,&position);

		} else if (!strcmp(name,"direction")) {
			FlatvectorAttribute(value,&direction);

		} else if (!strcmp(name,"color")) {
			SimpleColorAttribute(value, NULL, &color, NULL);

		} else if (!strcmp(name,"spacing")) {
			DoubleAttribute(value,&spacing, NULL);

		} else if (!strcmp(name,"dashes")) {
			if (value && strstr(value,"with:")==value) {
				dashkey=value+5;
				while (isspace(*dashkey)) dashkey++;
			}
			//if (dashes) dashes->dec_count();
			if (!dashes) dashes=new EngraverLineQuality();
			dashes->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"trace")) {
			if (value && strstr(value,"with:")==value) {
				tracekey=value+5;
				while (isspace(*tracekey)) tracekey++;
			}
			//if (trace) trace->dec_count();
			if (!trace) trace=new EngraverTraceSettings();
			trace->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"line")) {
			char *end_ptr=NULL;
			flatpoint v;
			int status;
			double w;
			int on=ENGRAVE_On;
			LinePoint *lstart=NULL, *ll=NULL;

			do { //one iteration for each point
				 //each line has: (x,y) weight on|off groupid

				while (isspace(*value)) value++;
				if (!strncmp(value,"cache",5)) {
					 //ignore cache values, they are regenerated
					value=strchr(value,'\n');
					if (!value) break;
					continue;
				}

				 //get (s,t) point
				status=FlatvectorAttribute(value, &v, &end_ptr);
				if (status==0) break;

				 //get weight
				value=end_ptr;
				status=DoubleAttribute(value, &w, &end_ptr);
				if (status==0) break;

				value=end_ptr;
				while (isspace(*value)) value++;
				if (*value=='o' && value[1]=='n') { on=ENGRAVE_On; value+=2; }
				else if (*value=='o' && value[1]=='f' && value[2]=='f') { on=ENGRAVE_Off; value+=3; }
				else if (*value=='e' && value[1]=='n' && value[2]=='d') { on=ENGRAVE_EndPoint; value+=3; }
				else if (*value=='s' && value[1]=='t' && value[2]=='a' && value[2]=='r' && value[2]=='t') { on=ENGRAVE_StartPoint; value+=5; }

				 //skip everything until the next line
				while (*value!='\0' && *value!='\n') value++;
				if (*value=='\n') value++;

				if (!lstart) { lstart=ll=new LinePoint(v.x,v.y, w); ll->on=on; }
				else {
					ll->next=new LinePoint(v.x,v.y, w);
					ll->next->prev=ll;
					ll->next->on=on;
					ll=ll->next;
				}

				while (isspace(*value)) value++;

			} while (*value!='\0');

			if (lstart) lines.push(lstart);
		}
	}
	if (!trace)  trace =new EngraverTraceSettings();
	if (!dashes) dashes=new EngraverLineQuality();

	if (isblank(this->name)) makestr(this->name,"Group");

	delete[] iorefs;
	if (tracekey) {
		appendstr(iorefs,"|trace:");
		appendstr(iorefs,tracekey);
	}
	if (dashkey) {
		appendstr(iorefs,"|dash:");
		appendstr(iorefs,dashkey);
	}
}


/*! Return 1 if a point is considered on by the criteria of the settings in dashes, else 0 if confirmed off.
 * Default is point must be on, and weight>=zero_threshhold.
 */
int EngraverPointGroup::PointOn(LinePoint *p)
{
	if (!p) return 0;
	if (p->on==ENGRAVE_Off) return 0;
	if (dashes && p->weight < dashes->zero_threshhold) return 0;

	if (p->on==ENGRAVE_EndPoint) return ENGRAVE_EndPoint;
	if (p->on==ENGRAVE_StartPoint) return ENGRAVE_StartPoint;
	return ENGRAVE_On;
}

/*! Return nonzero if a point is considered on by the criteria of the settings in dashes, else 0 if confirmed off.
 * Default is point must be dashon, and weight>=zero_threshhold.
 */
int EngraverPointGroup::PointOnDash(LinePointCache *p)
{
	if (!p) return 0;
	if (p->dashon==ENGRAVE_Off || p->on==ENGRAVE_Off) return 0;
	if (dashes && p->weight < dashes->zero_threshhold) return 0;

	if (p->dashon==ENGRAVE_EndPoint)   return ENGRAVE_EndPoint;
	if (p->dashon==ENGRAVE_StartPoint) return ENGRAVE_StartPoint;
	return ENGRAVE_On;
}

/*! Return nonzero if a point is considered on by the criteria of the settings in dashes, else 0 if confirmed off.
 * Default is point must be dashon, and weight>=zero_threshhold.
 */
int EngraverPointGroup::CachePointOn(LinePointCache *p)
{
	if (!p) return 0;
	if (p->dashon==ENGRAVE_Off || p->on==ENGRAVE_Off) return 0;
	if (dashes && p->weight < dashes->zero_threshhold) return 0;

	if (p->dashon==ENGRAVE_EndPoint)   return ENGRAVE_EndPoint;
	if (p->dashon==ENGRAVE_StartPoint) return ENGRAVE_StartPoint;
	return ENGRAVE_On;
}

void EngraverPointGroup::StripDashes()
{
	LinePointCache *cache, *start, *cc;

	for (int c=0; c<lines.n; c++) {
		start=cache=lines.e[c]->cache;

		if (!cache) continue;

		do {
			while (cache->next && (cache->next->type==ENGRAVE_EndDash || cache->next->type==ENGRAVE_StartDash)) {
				cc=cache->next;
				cc->Detach();
				delete cc;
			}

			if (cache->original) cache->weight=cache->original->weight;
			cache=cache->next;
		} while (cache && cache!=start);
	}
}

void EngraverPointGroup::UpdatePositionCache()
{
	DBG cerr <<" *** EngraverPointGroup::UpdatePositionCache() needs to be optimized"<<endl;
	
	UpdateBezCache();
	
	LinePointCache *cache, *start;
	LinePoint *l;

	for (int c=0; c<lines.n; c++) {
		start=cache=lines.e[c]->cache;

		if (!cache) continue;
		l=cache->original;

		do {
			if (cache->original) {
				l=cache->original;
				cache->p=l->p;
			} else {
				if (l->next) cache->p=bez_point(cache->bt, l->p, l->bez_after, l->next->bez_before, l->next->p);
			}

			cache=cache->next;
		} while (cache && cache!=start);
	}
}

/*! Update the bez handles and bez length of all points.
 */
void EngraverPointGroup::UpdateBezCache()
{
	LinePoint *p, *start;

	for (int c=0; c<lines.n; c++) {
		start=p=lines.e[c];

		if (!p) continue;

		do {
			p->UpdateBezHandles();
			if (p->prev) p->prev->length=bez_segment_length(p->prev->p, p->prev->bez_after, p->bez_before,p->p, 8);
			else p->length=0;
			if (!p->next) p->length=0;

			p=p->next;
		} while (p && p!=start);
	}
}

/*! Update any additional points added to the lines.
 *
 * Returns number of dashes.
 */
int EngraverPointGroup::UpdateDashCache()
{
	DBG cerr <<"UpdateDashCache..."<<endl;

	numdashes=0;
	if (!dashes || (dashes && dashes->zero_threshhold==0 && dashes->broken_threshhold<=dashes->zero_threshhold)) {
		int hascache=0;
		for (int c=0; c<lines.n; c++) {
			if (lines.e[c]->cache) hascache=1;
			else lines.e[c]->BaselineCache();
		}
		if (hascache) StripDashes();
		return 1;
	}


	LinePointCache *lc=NULL, *lcstart;
	LinePoint *l, *lnext, *lstart; 

	double broken =dashes->broken_threshhold;
	double zero   =dashes->zero_threshhold;
	double dashlen=dashes->dash_length*spacing; 

	if (broken<zero) broken=zero;

	double next[4];
	int nexton[4];
	int laston; //should always be either ENGRAVE_On or ENGRAVE_Off
	int nextmax;
	double lasts;
	double t, endt;
	double s;
	double weight,dashweight, ww;
	double maxs, maxt;
	int indash=0; //0=no, 1=on initial dash, 2=on gap, 3=on final dash
	int hastoend=0;

	if (dashes->randomseed>0) srandom(dashes->randomseed);

	PtrStack<LinePointCache> unused;
	unused.Allocate(10);

	for (int c=0; c<lines.n; c++) {
		lstart=l=lines.e[c];
		if (!l->cache) l->BaselineCache();
		lc=NULL;
		lcstart=NULL;
		t=endt=-1;
		s=-1;
		indash=0;
		laston=ENGRAVE_Off;
		lasts=0;

		 //special treatment when starting off line
		l->cache->dashon=l->cache->on=ENGRAVE_Off;
		laston=ENGRAVE_Off;

		do { //once per l
			lc=l->cache;
			lc->dashon=lc->on=laston;

			if (!l->next) break;
			lnext=l->next;


			 //remove old dash points for this segment
			lcstart=lc->next;
			while (lcstart && lcstart->type!=ENGRAVE_Original) {
				if (lcstart->type==ENGRAVE_EndDash || lcstart->type==ENGRAVE_StartDash) {
					unused.push(lcstart, 1); //1 means remove and flush will delete lcstart
					lcstart=lcstart->Detach();
				}
				lcstart=lcstart->next;
			}


			if (!indash) {
				if (lc->dashon==ENGRAVE_EndPoint || lc->dashon==ENGRAVE_Off)
					 laston=ENGRAVE_Off;
				else laston=ENGRAVE_On; 

				if (l->weight >= broken && lnext->weight >= broken) {
					 //this point and next are too thick for dashes, so skip. We should not be 
					 //indash if this is true
					lc->dashon=lc->on=ENGRAVE_On;
					l->cache->weight=l->weight;
					l=lnext;
					laston=ENGRAVE_On;
					lasts=0;
					// *** need to blot any other cache points to have the proper on
					continue;
				}

				if (l->weight <= zero && lnext->weight <= zero) {
					 //this point and next are too thin for anything, so skip
					lc->dashon=ENGRAVE_Off;
					l->cache->weight=l->weight;
					l=lnext;
					laston=ENGRAVE_Off;
					lasts=0;
					// *** need to blot any other cache points to have the proper on
					continue;
				}


				 //find starting point and metrics of dash. We need to have a weight value for the dash
				 //that ideally is somewhere between but not on zero or broken.
	
				if (l->weight <= broken) {
					 //starts thin. maybe gets thicker, maybe gets thinner
					 //dash will start right on l
					 
					if (l->weight <= zero) { //lnext->weight must be >zero here
						 //starts below zero, must start dash at zero threshhold
						t=(zero - l->weight)/(lnext->weight - l->weight);
						s=t*l->length;
						weight=(zero+lnext->weight)/2; //pick a weight that will produce some kind of dash
						if (weight>broken) weight=broken-.1*(broken-zero);

					} else {
						 //weight starts between zero and broken,
						 //next weight might be anything
						t=0;
						s=0;
						ww=(lnext->weight>broken ? broken : (lnext->weight<zero ? zero : lnext->weight));
						weight=(l->weight+ww)/2;
					}

				} else {
					 //current point is > broken, but next point is < broken, so actual threshhold is
					 //somewhere within l..lnext
					t=(l->weight - broken)/(l->weight - lnext->weight);
					s=t*l->length;
					weight=(broken+lnext->weight)/2;
					if (weight<zero) weight=zero+.05*(broken-zero); //arbitrarily pick a non-zero weight
				}


				 //now we figure out the length, gap placement, and width of a dash.
			     //taper means dash width varies from broken to zero+taper*(broken-zero)
				 //original width varies from broken down to zero
				EstablishDashMetrics(s,weight, next,nexton,nextmax, l,lc, laston,lasts, dashweight, dashlen, unused);

				indash=1; 
				numdashes++; 

			} //if !indash



			if (indash) {
				 //figure out if the dash has to end before the next point from the line
				 //being either too thick or too thin

				if (lnext->weight >= broken) {
					 //need to determine absolute end of dash creation when line gets too thick...
					maxt=(broken - l->weight)/(lnext->weight - l->weight);
					maxs=maxt*l->length;
					hastoend=1;

				} else if (lnext->weight <= zero) {
					 //need to determine absolute end of dash creation when line gets too thin...
					maxt=(l->weight - zero)/(l->weight - lnext->weight);
					maxs=maxt*l->length;
					hastoend=-1;

				} else if (!lnext->next) {
					 //end of the line! don't go beyond
					maxt=.9999;
					maxs=.9999*l->length;
					hastoend=-1;

				} else {
					maxs=-1;
					maxt=-1;
					hastoend=0;
				}

			}

			while (indash) {
				if (hastoend && maxs<=next[indash]) {
					 //line either gets too thick or too thin before dash ends

					if (hastoend<0) {
						 //line gets too thin, need to insert an end point maybe
						if (nexton[indash]==ENGRAVE_EndPoint) {
							lc=AddPoint(maxs, ENGRAVE_EndPoint, ENGRAVE_EndDash, dashweight,dashlen, l,lc, unused);
							laston=ENGRAVE_Off;
							lasts=maxs;

						} //else already off, nothing required!

					} else { //else line gets too thick, needs to become on...
						if (nexton[indash]==ENGRAVE_StartPoint) {
							lc=AddPoint(maxs, ENGRAVE_StartPoint, ENGRAVE_StartDash, dashweight,dashlen, l,lc, unused);
							laston=ENGRAVE_On;
							lasts=maxs;

						} //else already on, nothing required!
					}

					indash=0;
					break;

				} else { //does not havetoend
					 //dash continues, but maybe need to start a new dash/gap span...
		
					if (next[indash] < l->length) {
						 //dash part ends within segment, so must add a change point
						if (indash!=nextmax-1) {
							lc=AddPoint(next[indash], nexton[indash], -1, dashweight,dashlen, l,lc, unused);
							if (nexton[indash]==ENGRAVE_StartPoint) laston=ENGRAVE_On;
							else laston=ENGRAVE_Off;
						}

						indash++;
						if (indash==nextmax) { 
							 //reset dash metrics to start at next[indash-1]
							EstablishDashMetrics(next[nextmax-1],-1, next,nexton,nextmax,  l,lc, laston,lasts, dashweight, dashlen,unused);
							indash=1;
							numdashes++;
						}

					} else {
						 //current dash part ends after current segment
						 //need to advance l, need to break from this loop
						//gaplen and dashweight updated if necessary below, outside of this dash loop
						break;
					} 

				} //if dash didn't have to end
			} //while indash



			 //lastly, advance to next segment...
			l=l->next;
			lasts-=l->prev->length;

			if (indash) {
				 //we need to make sure all the stuff in next[] is still current for this next segment...
				for (int c=indash; c<nextmax; c++) next[c]-=l->prev->length;

				//now need to update dashweight, etc

				if (l->weight!=l->prev->weight) {
					 //need to remap dashlen, gaplen, dashonlen, dashweight
					dashes->GetNewWeight(l->weight, &dashweight);
					//dash metrics are reevaluated at each dash boundary, so just update weight here
				}

				l->cache->weight=dashweight;

			} else { //outside of dash, just update cache weight
				l->cache->weight=l->weight;
			}


		} while (l && l!=lstart);

		ApplyBlockout(lines.e[c]);
	} //foreach line

	DBG cerr <<"end UpdateDashCache: "<<numdashes<<endl;
	return numdashes;
}

/*! Install a proper LinePointCache, with all fields set to proper values.
 *
 * If type==-1, the make type be ENGRAVE_EndDash or ENGRAVE_StartDash, according to if on==ENGRAVE_EndPoint or ENGRAVE_StartPoint.
 */
LinePointCache *EngraverPointGroup::AddPoint(double s, int on, int type, double dashweight,double dashlen,
									LinePoint *l,LinePointCache *&lc, Laxkit::PtrStack<LinePointCache> &unused)
{
	LinePointCache *lcc;
	if (unused.n) {
		lcc=unused.pop();
	} else lcc=new LinePointCache(0);

	if (type==-1) {
		if (on==ENGRAVE_EndPoint) type=ENGRAVE_EndDash;
		else if (on==ENGRAVE_StartPoint) type=ENGRAVE_StartDash;
	}

	lcc->type=type;

	lcc->bt=s/l->length;
	lcc->p=bez_point(lcc->bt, l->p, l->bez_after, l->next->bez_before, l->next->p);
	lcc->weight=dashweight;
	lcc->dashon=on;
	lcc->on    =on;

	return lc->InsertAfter(lcc);
}

/*! Will insert a LinePointCache after lc if mandated by conflict with laston.
 *
 * \todo implement lasts so as to keep minimum dash lengths intact.
 */
void EngraverPointGroup::EstablishDashMetrics(double s,double weight, double *next,int *nexton,int &nextmax,
								LinePoint *l,LinePointCache *&lc, int &laston, double &lasts,
								double &dashweight, double &dashlen, Laxkit::PtrStack<LinePointCache> &unused)
{
	if (weight<0) weight=l->weight + s/l->length * (l->next->weight - l->weight);

	 //now we figure out the length, gap placement, and width of a dash.
     //taper means dash width varies from broken to zero+taper*(broken-zero)
	 //original width varies from broken down to zero
	//double a=(weight-zero)/(broken-zero); //0..1, how long between zero and broken
	//dashweight = broken*a + (taper*(broken-zero)+zero)*(1-a);

	double a=(weight - dashes->zero_threshhold)/(dashes->broken_threshhold - dashes->zero_threshhold); //0..1, how long between zero and broken
	dashweight   = dashes->broken_threshhold*a + (dashes->dash_taper*(dashes->broken_threshhold - dashes->zero_threshhold)+dashes->zero_threshhold)*(1-a);

	double dashonlen = dashlen * (dashes->dash_density + (1 - dashes->dash_density)*a);
	double gaplen    = dashlen-dashonlen;
	double gapstart  = dashlen/2 + dashonlen/2 + dashlen * (dashes->dash_randomness*random()/RAND_MAX);
	while (gapstart>=dashlen) gapstart-=dashlen;


	if (gapstart==0) {
		 // ---***  gap starts at beginning
		next[0]=s;
		nexton[0]=ENGRAVE_EndPoint;
		next[1]=s+gaplen;
		nexton[1]=ENGRAVE_StartPoint;
		next[2]=s+dashlen;
		nexton[2]=ENGRAVE_EndPoint;
		nextmax=3; 

	} else if (gapstart+gaplen==dashlen) {
		 // ***---  whole gap fills final portion
		next[0]=s;
		nexton[0]=ENGRAVE_StartPoint;
		next[1]=s+gapstart;
		nexton[1]=ENGRAVE_EndPoint;
		next[2]=s+dashlen;
		nexton[2]=ENGRAVE_StartPoint;
		nextmax=3; 

	} else if (gapstart+gaplen>dashlen) {
		 // --****---  gap runs past end of dash, so need to wrap around
		next[0]=s;
		nexton[0]=ENGRAVE_EndPoint;
		next[1]=s+gapstart+gaplen-dashlen;
		nexton[1]=ENGRAVE_StartPoint;
		next[2]=next[1]+dashonlen;
		nexton[2]=ENGRAVE_EndPoint;
		next[3]=s+dashlen;
		nexton[3]=ENGRAVE_StartPoint;
		nextmax=4; 

	} else {
		 // **----***  gap within dashlen, so solid portion wraps around
		next[0]=s;
		nexton[0]=ENGRAVE_StartPoint;
		next[1]=s+gapstart;
		nexton[1]=ENGRAVE_EndPoint;
		next[2]=s+gapstart+gaplen;
		nexton[2]=ENGRAVE_StartPoint;
		next[3]=s+dashlen;
		nexton[3]=ENGRAVE_EndPoint;
		nextmax=4; 
	}

	 //add initial dash point if necessary
	if (   (laston==ENGRAVE_Off && nexton[0]==ENGRAVE_StartPoint)
	    || (laston==ENGRAVE_On  && nexton[0]==ENGRAVE_EndPoint)
	   ) {

		lc=AddPoint(next[0], nexton[0], -1, dashweight,dashlen, l,lc, unused);
		laston = (nexton[0]==ENGRAVE_EndPoint ? ENGRAVE_Off : ENGRAVE_On);
		lasts  = next[0];
	}
}

/*! l must have cache and dashes already properly installed and processed.
 */
int EngraverPointGroup::ApplyBlockout(LinePoint *l)
{
	LinePoint *lstart=l;
	LinePointCache *lc;

	//lcstart=l->cache;
	//int laston=l->on;

	do {
		if (l->on==ENGRAVE_Off) l->cache->on=ENGRAVE_Off;
		if (!l->next) break;
		
		if (l->on==ENGRAVE_Off || l->next->on==ENGRAVE_Off) {
			lc=l->cache;
			while (lc!=l->next->cache) {
				lc->on=ENGRAVE_Off;
				lc=lc->next;
			}
		}
		
		
		l=l->next;
	} while (l && l!=lstart);

	return 0;
}

/*! If newdash is NULL, then dec_count() the old one and install a fresh default one.
 * Otherwise decs count on old, incs count on newdash.
 */
void EngraverPointGroup::InstallDashes(EngraverLineQuality *newdash, int absorbcount)
{
	if (!newdash) {
		if (dashes) dashes->dec_count();
		dashes=new EngraverLineQuality();
		return;
	}
	if (newdash==dashes) return;
	if (dashes) dashes->dec_count();
	dashes=newdash;
	if (!absorbcount) dashes->inc_count();

	//UpdateDashCache();
}

/*! If newtrace is NULL, then dec_count() the old one and install a fresh default one.
 * Otherwise decs count on old, incs count on newtrace.
 */
void EngraverPointGroup::InstallTraceSettings(EngraverTraceSettings *newtrace, int absorbcount)
{
	if (!newtrace) {
		if (trace) trace->dec_count();
		trace=new EngraverTraceSettings();
		return;
	}
	if (newtrace==trace) return;
	if (trace) trace->dec_count();
	trace=newtrace;
	if (!absorbcount) trace->inc_count();
}

/*! Provide a direction vector for specified point. This is used to grow lines
 * in EngraverFillData objects. If you need exact lines, you will want to use
 * LineFrom(), since building from Direction() here will introduce too many
 * rounding errors. For instance, you will never build exact circles only
 * from a direction field.
 */
flatpoint EngraverPointGroup::Direction(double s,double t)
{
	if (type==PGROUP_Linear) {
		return direction;

	} else if (type==PGROUP_Circular) {
		return transpose(flatpoint(s,t)-position);

	} else if (type==PGROUP_Radial) {
		return flatpoint(s,t)-position;

	} else if (type==PGROUP_Spiral) {
		return rotate(transpose(flatpoint(s,t)-position), type_d);
	}

	return flatpoint();
}

/*! Create a line extending from coordinate s,t.
 */
LinePoint *EngraverPointGroup::LineFrom(double s,double t)
{
	if (type==PGROUP_Linear) {

	} else if (type==PGROUP_Circular) {
	} else if (type==PGROUP_Radial) {
	} else if (type==PGROUP_Spiral) {
	}

	return NULL;
}

/*! fill in x,y = 0..1,0..1
 */
void EngraverPointGroup::Fill(EngraverFillData *data, double nweight)
{
	if (direction.isZero()) direction.x=1;

	lines.flush();

	if (type==PGROUP_Circular) {
		FillCircular(data,nweight);

	} else if (type==PGROUP_Radial) {
		FillRadial(data,nweight);

	} else if (type==PGROUP_Spiral) {
		//FillSpiral(data,nweight);

	} else { //default: if (type==PGROUP_Linear) {
		FillRegularLines(data,nweight);

	}

}

/*! spacing is an object distance (not in s,t space) to be used as the distance between line centers.
 * If spacing<0, then use 1/20 of the x or y dimension, whichever is smaller
 * If weight<0, then use spacing/10.
 * Inserts lines follow	a,<<<ing this->direction, which is in (s,t) space.
 */
void EngraverPointGroup::FillRegularLines(EngraverFillData *data, double nweight)
{
	double thisspacing=spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing=thisspacing;

	double weight=nweight;
	if (weight<=0) weight=thisspacing/10; //remember, weight is actual distance, not s,t!!
	thisspacing=spacing/data->getScaling(.5,.5,false);




	LinePoint *p;

	flatvector v=direction; //this is s,t space
	if (v.x<0) v=-v;
	v.normalize();
	v*=thisspacing;
	//flatvector vt=transpose(v);

	 //we need to find the s,t equivalent of spacing along direction
	double vv=thisspacing*thisspacing;

	double s_spacing= (v.y==0 ? -1 : fabs(vv/v.y));
	double t_spacing= (v.x==0 ? -1 : fabs(vv/v.x));

	//if (xsize>4) s_spacing/= xsize/3;
	//if (ysize>4) t_spacing/= ysize/3;

	//if (v.y<0) data->lines.push(new LinePoint(0,1,weight,id)); //push a (0,0) starter point
	//else       data->lines.push(new LinePoint(0,0,weight,id)); //push a (0,0) starter point

	 //starter points along y
	if (t_spacing>0) {
		double oy;
		if (v.y<0) oy=position.y-position.x*v.y/v.x;
		else oy=position.y-position.x*v.y/v.x;

		for (double yy=oy-t_spacing*floor(oy/t_spacing); yy<=1; yy+=t_spacing) {
			//if (v.y<0) data->lines.push(new LinePoint(0,1-yy, weight,id));
			if (v.y<0) lines.push(new LinePoint(0,yy, weight));
			else       lines.push(new LinePoint(0,yy, weight));
		}
	}

	 //starter points along x
	if (s_spacing>0) {
		double ox;
		if (v.y<0) ox=position.x+(1-position.y)*v.x/v.y;
		else ox=position.x-position.y*v.x/v.y;

		for (double xx=ox-s_spacing*floor(ox/s_spacing); xx<=1; xx+=s_spacing) {
			if (v.y<0) lines.push(new LinePoint(xx,1, weight));
			else       lines.push(new LinePoint(xx,0, weight));
		}
	}

	 //grow lines
	flatvector pp;
	for (int c=0; c<lines.n; c++) {
		p=lines.e[c];

		while (p->s>=0 && p->t>=0 && p->s<=1 && p->t<=1) {
			pp=flatpoint(p->s,p->t) + v;
			p->next=new LinePoint(pp.x, pp.y, weight);
			p->next->prev=p;
			p=p->next;
		}
	}
}

/*! If weight<0, then use spacing/10.
 */
void EngraverPointGroup::FillRadial(EngraverFillData *data, double nweight)
{
	double thisspacing=spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing=thisspacing;
		

	double weight=nweight;
	if (weight<=0) weight=spacing/10; //remember, weight is actual distance, not s,t!!
	thisspacing=spacing/data->getScaling(.5,.5,false);


	int numpoints=2*M_PI/thisspacing;
	if (numpoints<3) numpoints=3;

	LinePoint *p;
	flatpoint pp;
	flatpoint v;

	for (int c=0; c<numpoints; c++) {
		pp=position;
		p=new LinePoint(position.x, position.y, weight);
		lines.push(p);

		v=rotate(.05*direction/norm(direction),2*M_PI*c/numpoints);
		
		while (p->s>=0 && p->t>=0 && p->s<=1 && p->t<=1) {
			pp=flatpoint(p->s,p->t) + v;
			p->next=new LinePoint(pp.x, pp.y, weight);
			p->next->prev=p;
			p=p->next;
		}
	}
}

/*! If weight<0, then use spacing/10.
 */
void EngraverPointGroup::FillCircular(EngraverFillData *data, double nweight)
{
	double thisspacing=spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing=thisspacing;

	double weight=nweight;
	if (weight<=0) weight=spacing/10; //remember, weight is actual distance, not s,t!!
	thisspacing=spacing/data->getScaling(.5,.5,false);


	int numpoints=30;

	LinePoint *p;
	flatpoint pp;
	flatpoint v(thisspacing,0);
	double r=0, rr=0;
	r=rr=norm(flatpoint(0,0)-position);
	r=norm(flatpoint(1,0)-position);
	if (r>rr) rr=r;
	r=norm(flatpoint(0,1)-position);
	if (r>rr) rr=r;
	r=norm(flatpoint(1,1)-position);
	if (r>rr) rr=r;


	r=-thisspacing/2;
	LinePoint *sp=NULL;
	int first=-1;
	DBG int circle=0;

	while (r<rr) { //one loop per radius
		r+=thisspacing;
		numpoints=10+2*r*M_PI/thisspacing;
		sp=p=NULL;
		first=-1;

		DBG circle++;

		for (int c=0; c<=numpoints+1; c++) {
			pp=position + r*flatpoint(cos(2*M_PI*c/numpoints),sin(2*M_PI*c/numpoints));

			if (pp.x<0 || pp.x>1 || pp.y<0 || pp.y>1) {
				 //point out of bounds
				if (!sp) continue; //no current segment, so just skip

				//else we had a current segment, so we have to terminate
				sp=NULL;
				continue;
			}

			//now we have a point to add

			if (c==numpoints+1) {
				 //we have come full circle, the previous point was on
				if (first>=0 && sp) {
					 //first point of circle was on, so since this last point is also on,
					 //we need to connect first and last
					lines.e[first]->prev=p;
					p->next=lines.e[first];
					lines.e[first]=sp;
					if (sp->prev) sp->prev->next=NULL;
					sp->prev=NULL;
					if (first!=lines.n-1) {
						lines.e[lines.n-1]=NULL;
						lines.n--;
					}
				}
				first=-1;
				sp=NULL;
				break;
			}

			if (sp==NULL) {
				 //starting new segment
				p=new LinePoint(pp.x, pp.y, weight);
				lines.push(p);

				 //we need to remember the start of the circle, in case we need to
				 //connect the final segment of the circle to the first segment
				sp=p;
				if (c==0) first=lines.n-1;

			} else {
				p->next=new LinePoint(pp.x, pp.y, weight);
				p->next->prev=p;
				p=p->next;
			}

		} //foreach point in circle
	} //for each radius

}

/*! Class to aid growing lines.
 */
class StarterPoint
{
  public:
	flatpoint lastdir;
	int iteration;
	int piteration;
	int lineref;

	LinePoint *line, *first, *last;
	int dodir; //1 for add to +direction, 2 for add to -direction, 3 for both

	StarterPoint (flatpoint p, int indir, double weight,int groupid, int nlineref);
};

StarterPoint::StarterPoint(flatpoint p, int indir, double weight,int groupid, int nlineref)
{
	first=last=line=new LinePoint(p.x,p.y,weight);
	iteration=0;
	piteration=0;
	dodir=indir;
	lineref=nlineref;
}

/*! If growpoint_ret already has points in it, use those, don't create automatically along edges.
 */
void EngraverPointGroup::GrowLines(EngraverFillData *data,
									double resolution, 
									double defaultspace,  	ValueMap *spacingmap,
									double defaultweight,   ValueMap *weightmap, 
									flatpoint direction,    DirectionMap *directionmap,
									Laxkit::PtrStack<GrowPointInfo> *growpoint_ret,
									int iteration_limit)
{
	 //remove any old lines from same group
	lines.flush();


	if (directionmap==NULL) directionmap=this;

	//double dir_rotation=0;
	DoubleBBox bounds(0,data->xsize/3, 0,data->ysize/3);
	PtrStack<StarterPoint> generators;
	StarterPoint *g;
	flatpoint v=resolution*direction/norm(direction);
	flatpoint vv;
	double weight=defaultweight;
	double curspace=defaultspace/data->getScaling(.5,.5,false);
	resolution/=data->getScaling(.5,.5,false);

	double VEPSILON=1e-6;
	flatpoint p(bounds.minx,bounds.miny);
	flatpoint p2;


	 //create initial generators 

	if (growpoint_ret && growpoint_ret->n>0) {
		 //use supplied points
		for (int c=0; c<growpoint_ret->n; c++) {
			g=new StarterPoint(growpoint_ret->e[c]->p, growpoint_ret->e[c]->godir, weight, id, generators.n);
			g->line->p=data->getPoint(g->line->s, g->line->t, true);
			g->line->needtosync=0;
			generators.push(g,1);
		}

	} else {
		 //and add initial line points along each of the 4 edges of bounds

		 //start at origin, add points around bounds, starting with lower x edge
		while (p.x<bounds.maxx) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.y>0 && ((v.x>0 && p.x<bounds.maxx) || (v.x<0 && p.x>bounds.minx))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.y)<VEPSILON) { p.x=bounds.maxx; break; }

			p.x+=fabs(resolution*curspace/v.y);
		}

		 //find initial starter along maxx segment
		if (fabs(v.y)<VEPSILON) { //like horizontal lines
			p.x=bounds.maxx;
			p.y=bounds.miny+spacing;
		} else {
			if (fabs(v.x)<VEPSILON) {
				p.x=bounds.maxx; //need to skip traverse up maxx edge
				p.y=bounds.maxy;
			} else {
				p.y=bounds.miny + (-p.x+bounds.maxx)*v.y/v.x;
				while (p.y<bounds.miny) p.y+=fabs(spacing*v.y/v.x);
				p.x=bounds.maxx;
			}
		}

		 //cruise up maxx side
		while (p.y<bounds.maxy) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.x<0 && ((v.y>0 && p.y>bounds.miny) || (v.y<0 && p.y<bounds.maxy))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.x)<VEPSILON) { p.y=bounds.maxy; break; }

			p.y+=fabs(resolution*curspace/v.x);
		}

		 //find initial starter on maxy edge
		if (fabs(v.x)<VEPSILON) { //like horizontal lines
			p.x=bounds.maxx-spacing;
			p.y=bounds.maxy;
		} else {
			if (fabs(v.y)<VEPSILON) {
				p.x=bounds.minx; //need to skip traverse
				p.y=bounds.maxy;
			} else {
				p.x=bounds.maxx - (p.y-bounds.maxy)*v.x/v.y;
				while (p.x>bounds.maxx) p.x-=fabs(spacing*v.x/v.y);
				p.y=bounds.maxy;
			}
		}


		 //cruise down maxy side
		while (p.x>bounds.minx) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.y<0 && ((v.x>0 && p.x<bounds.maxx) || (v.x<0 && p.x>bounds.minx))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.y)<VEPSILON) { p.x=bounds.minx; break; }

			p.x-=fabs(resolution*curspace/v.y);
		}

		 //find initial starter on minx edge
		if (fabs(v.y)<VEPSILON) { //like horizontal lines
			p.x=bounds.minx;
			p.y=bounds.maxy-spacing;
		} else {
			if (fabs(v.x)<VEPSILON) {
				p.x=bounds.minx; //need to skip traverse down minx edge
				p.y=bounds.miny;
			} else {
				p.y=bounds.maxy + (-p.x+bounds.minx)*v.y/v.x;
				while (p.y>bounds.maxy) p.y-=fabs(spacing*v.y/v.x);
				p.x=bounds.minx;
			}
		}


		 //cruise down minx side
		while (p.y>bounds.miny) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.x>0 && ((v.y>=0 && p.y>bounds.miny) || (v.y<0 && p.y<bounds.maxy))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.x)<VEPSILON) { p.y=bounds.maxy; break; }

			p.y-=fabs(resolution*curspace/v.x);
		}

		for (int c=0; c<lines.n; c++) {
			lines.e[c]->p=data->getPoint(lines.e[c]->s,lines.e[c]->t, true);
			lines.e[c]->needtosync=0;
		}

	} //if adding starters along edges

	// 
	// Now all initial starter points positioned on edges,
	// we must grow them...
	//


	 //grow points until generators depleted
	int iteration=0;
	bool maybefill=true;
	double xx=bounds.minx; //for fill point search below
	double yy=bounds.miny;

	do {
		DBG cerr <<" iter: "<<iteration<<" g.n:"<<generators.n<<"  ";
		iteration++;
		if (iteration==iteration_limit) {
			DBG cerr <<"Warning! EngraverPointGroup GrowLines() hit iteration limit of: "<<iteration_limit<<endl;
			generators.flush();
			break;
		}

		 //----advance each generator according to directionmap
		for (int c=0; c<generators.n; c++) {
			g=generators.e[c];

			 //advance forward
			if (g->dodir&1) {
				if (directionmap) {
					v=directionmap->Direction(g->last->s, g->last->t);
					v*=resolution/norm(v);
				} //else v already set;

				p=flatpoint(g->last->s + v.x, g->last->t + v.y);
				p2=data->getPoint(p.x,p.y, true);
				if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
				if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

				g->last->Add(new LinePoint(p.x,p.y, weight));
				g->last=g->last->next;
				g->last->p=p2;
				g->last->needtosync=0;
			}

			 //advance backwards
		   	if (g->dodir&2) {
				if (directionmap) {
					v=directionmap->Direction(g->first->s, g->first->t);
					v*=resolution/norm(v);
				}

				p=flatpoint(g->first->s - v.x, g->first->t - v.y);
				p2=data->getPoint(p.x,p.y, true);
				if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
				if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

				g->first->AddBefore(new LinePoint(p.x,p.y, weight));
				g->first=g->first->prev;
				g->first->p=p2;
				g->first->needtosync=0;
			}
		}


		 //-----terminate lines now out of bounds
		 // *** todo: don't stretch so far out of bounds, interpolate to edge
		for (int c=generators.n-1; c>=0; c--) {
			g=generators.e[c];

			if (g->dodir&1) if (!bounds.boxcontains(g->last->s, g->last->t))  g->dodir&=~1;
			if (g->dodir&2) if (!bounds.boxcontains(g->first->s,g->first->t)) g->dodir&=~2;
			DBG cerr <<" gen dir:"<<g->dodir<<endl;

			if (g->dodir==0) {
				//all done with this generator! make sure the associated line starts at a point with no prev points
				if (g->first!=g->line) {
					int i=lines.findindex(g->line);
					lines.e[i]=g->first;
				}
				generators.remove(c);
			}

		}


		 //-----search for merges and splits
		for (int c=generators.n-1; c>=0; c--) {
			g=generators.e[c];


			if (g->dodir&1) {
				//search for g->last->p within curspace*.75 distance to any other sample points,
				//terminate if found

				 //find square of transformed spacing
				if (spacingmap) curspace=spacingmap->GetValue(g->last->p)/data->getScaling(g->last->s,g->last->t,true); //else spacing is constant
				double lsp=curspace*.95; //least space threshhold
				double ls2=lsp*lsp;
				double msp=curspace*1.5; //most space threshhold
				//double msp2=msp*msp;
				double ld=1e+10, d2;
				//LinePoint *lclosest=NULL;
				LinePoint *lp;
				//LinePoint *gg;
				p=g->last->p;

				for (int cc=0; cc<lines.n; cc++) {
					lp=lines.e[cc];
					while (lp->prev) lp=lp->prev;

					//gg=NULL;
					//if (cc==g->lineref) {
					//	 //is same line as current generator, need a special guard to not check against adjacent points to current points
					//	gg=g->last;
					//	for (int c2=0; c2<int(curspace/resolution)+1 && gg; c2++) gg=gg->prev; //skip at least a curspace worth of points
					//	if (!gg) continue;
					//}
					for ( ; lp; lp=lp->next) {
						//if (gg && gg==lp) break;
						if (lp==g->last) continue;

						if (lp->p.x < p.x-msp) continue; //skip out of range so we don't waste time on a lot of multiplications
						if (lp->p.x > p.x+msp) continue;
						if (lp->p.y < p.y-msp) continue;
						if (lp->p.y > p.y+msp) continue;

						d2=(lp->p.x - p.x)*(lp->p.x - p.x) + (lp->p.y - p.y)*(lp->p.y - p.y);  //norm2(lp->p - p);
						if ((cc==g->lineref && d2<resolution/2) || (cc!=g->lineref && d2<ld)) {
							//lclosest=lp;
							ld=d2;
						}
					}
				}

				if (ld<ls2) {
					//g->last->Add(new LinePoint(lclosest->s,lclosest->t, weight));
					//g->last=g->last->next;
					//g->last->p=lclosest->p;
					//g->last->needtosync=0;
					g->dodir&=~1;
				}
			} //search in next direction

			if (g->dodir&2) {
				//search for g->first->p within curspace*.75 distance to any other sample points
				//terminate if found

				if (spacingmap) curspace=spacingmap->GetValue(g->first->p)/data->getScaling(g->first->s,g->first->t,true); //else spacing is constant
				double lsp=curspace*.75; //least space threshhold
				double ls2=lsp*lsp;
				double msp=curspace*2; //most space threshhold
				//double msp2=msp*msp;
				double ld=1e+10, d2;
				//LinePoint *lclosest=NULL;
				LinePoint *lp;
				//LinePoint *gg;
				p=g->first->p;

				for (int cc=0; cc<lines.n; cc++) {
					lp=lines.e[cc];
					while (lp->next) lp=lp->next;

					//gg=NULL;
					//if (cc==g->lineref) {
					//	 //is same line as current generator, need a special guard to not check against adjacent points to current points
					//	gg=g->first;
					//	for (int c2=0; c2<int(curspace/resolution)+1 && gg; c2++) gg=gg->next; //skip at least a curspace worth of points
					//	if (!gg) continue;
					//}
					for ( ; lp; lp=lp->prev) {
						//if (gg && gg==lp) break;
						if (lp==g->first) continue;

						if (lp->p.x < p.x-msp) continue; //skip out of range so we don't waste time on a lot of multiplications
						if (lp->p.x > p.x+msp) continue;
						if (lp->p.y < p.y-msp) continue;
						if (lp->p.y > p.y+msp) continue;

						d2=(lp->p.x-p.x)*(lp->p.x-p.x) + (lp->p.y-p.y)*(lp->p.y-p.y);  //norm2(lp->p - p);
						if ((cc==g->lineref && d2<resolution/2) || (cc!=g->lineref && d2<ld)) {
							//lclosest=lp;
							ld=d2;
						}
					}
				}

				if (ld<ls2) {
					//g->first->AddBefore(new LinePoint(lclosest->s,lclosest->t, weight));
					//g->first=g->first->prev;
					//g->first->p=lclosest->p;
					//g->first->needtosync=0;
					g->dodir&=~2;
				}
			} //search in previous direction
			 
//			*** //if merging with an endpoint, move both endpoints to midpoint
//				//if merging against middle of line, collide into it, don't modify original line?

			if (g->dodir==0) {
				//all done with this generator! make sure the associated line starts at a point with no prev points
				if (g->first!=g->line) {
					int i=lines.findindex(g->line);
					lines.e[i]=g->first;
				}
				generators.remove(c);
			}
		} //foreach generator, merges and splits



		//*** //need to equalize somehow after merging and splitting


		 //-----no more generators, search for holes to fill! this might happen with specialized direction maps
		//add one generator per iteration
		if (generators.n==0 && maybefill) {
			//maybefill=false; //only fill in once
			DBG cerr <<" searching for empty space during iteration "<<iteration<<"..."<<endl;

			flatpoint closest;
			double d=1e+10;
			double d2, s2, s;
			LinePoint *lp;
			flatpoint p2;
			int maxline=lines.n;

			for ( ; xx<bounds.maxx; xx+=curspace) {

				if (yy>=bounds.maxy) yy=bounds.miny;
				for ( ; yy<bounds.maxy; yy+=curspace) {
					 //find closest point to xx,yy

					p=data->getPoint(xx,yy,true);
					if (spacingmap) curspace=spacingmap->GetValue(p)/data->getScaling(xx,yy,true); //else spacing is constant

					 //find square of transformed spacing
					s=curspace*2;
					s2=curspace*curspace*4;
					d=1e+10;

					for (int c=0; c<maxline; c++) {
						lp=lines.e[c];
						for ( ; lp; lp=lp->next) {
							//if (pb.boxcontains(lp->p)) {...

							if (lp->p.x<p.x-s) continue;
							if (lp->p.x>p.x+s) continue;
							if (lp->p.y<p.y-s) continue;
							if (lp->p.y>p.y+s) continue;

							d2=(lp->p.x-p.x)*(lp->p.x-p.x) + (lp->p.y-p.y)*(lp->p.y-p.y);  //norm2(lp->p - p);
							if (d2 < d) {
								closest.x=lp->s; closest.y=lp->t;
								d=d2;
							}
						}
					}

					if (d>s2) {
						 //nothing was very close
						if (weightmap)  weight = weightmap->GetValue(p); //else weight is constant
						g=new StarterPoint(flatpoint(xx,yy), 3, weight, id, generators.n);
						g->line->p=p;
						g->line->needtosync=0;
						lines.push(g->line);
						generators.push(g,1);
						if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(flatpoint(xx,yy),g->dodir));
						DBG cerr <<"Add fill point at "<<xx<<','<<yy<<endl;
						yy+=curspace;
						break;
					}
				}
				if (yy<bounds.maxy) break;
			}
			if (xx>=bounds.maxx && yy>=bounds.maxy) break;
		}
	} while (generators.n);


	 //Add lines to data
	LinePoint *lp, *ll;
	for (int c=0; c<lines.n; c++) {
		lp=lines.e[c];

		 //need to normalize all points
		ll=lp;
		while (ll) {
			ll->s=(ll->s-bounds.minx)/(bounds.maxx-bounds.minx);
			ll->t=(ll->t-bounds.miny)/(bounds.maxy-bounds.miny);
			if (ll->s>=1 || ll->t>=1 || ll->s<=0 || ll->t<=0) {
				ll->p=data->getPoint(ll->s,ll->t, false);
				ll->needtosync=0;
			}
			ll=ll->next;
		}
	}
}


//------------------------------------- EngraverFillData ------------------------


/*! \class EngraverFillData
 * \ingroup interfaces
 *
 * See EngraverFillInterface.
 */


EngraverFillData::EngraverFillData()
  : PatchData(0,0,1,1,1,1,0)
{
	usepreview=0;

	MakeDefaultGroup();
	
	maxx=maxy=1;
}

EngraverFillData::~EngraverFillData()
{
}

/*! Make sure there is at least one defined group.
 * If there are already groups, then do nothing.
 */
void EngraverFillData::MakeDefaultGroup()
{
	if (groups.n) return;

	EngraverPointGroup *group=new EngraverPointGroup;
	group->spacing=(maxy-miny)/20;
	group->id=0;
	group->color.red=0;
	group->color.green=0;
	group->color.blue=65535;
	group->color.alpha=65535;
	if (!group->trace)  group->trace =new EngraverTraceSettings();
	if (!group->dashes) group->dashes=new EngraverLineQuality();

	groups.push(group);
}

/*! Changes the name of group index which to be unique.
 *
 * Return 1 if name had to be changed, else 0.
 */
int EngraverFillData::MakeGroupNameUnique(int which)
{
	EngraverPointGroup *group=GroupFromIndex(which);

	if (!group->name) makestr(group->name,"Group");

	 //need to make a unique new name
	int c;
	int changed=0;

	do {
		for (c=0; c<groups.n; c++) {
			if (c==which) continue;
			if (!strcmp(group->name, groups.e[c]->name)) {
				char *str=increment_file(group->name);
				makestr(group->name,str);
				delete[] str;
				changed=1;
				break;
			}
		}
	} while (c!=groups.n);

	return changed;
}

/*! Return if the item type is shared with a previous group.
 */
int EngraverFillData::IsSharing(int what, int curgroup)
{
	if (groups.n==0 || curgroup<0) return -1;
	for (int c=0; c<groups.n; c++) {
		if (c==curgroup) continue;

		if (what==ENGRAVE_Tracing   && groups.e[c]->trace ==groups.e[curgroup]->trace ) return c;
		if (what==ENGRAVE_Dashes    && groups.e[c]->dashes==groups.e[curgroup]->dashes) return c;
		//if (what==ENGRAVE_Direction && groups.e[c]->==groups.e[curgroup]->) return c;
		//if (what==ENGRAVE_Spacing   && groups.e[c]->==groups.e[curgroup]->) return c;
	}
	return -1;
}


/*! Return point to group corresponding to the given id. 
 * If found and err_ret!=NULL, then set to 1.
 * If not found, then return pointer to the first group, or NULL if no groups, and set err_ret to 0.
 */
EngraverPointGroup *EngraverFillData::FindGroup(int id, int *err_ret)
{
	if (err_ret) *err_ret=1;
	for (int c=0; c<groups.n; c++) {
		if (groups.e[c]->id==id) return groups.e[c];
	}
	if (err_ret) *err_ret=0;
	return groups.n ? groups.e[0] : NULL;
}

/*! Return point to group corresponding to the given index in groups, or group 0 if index out of range.
 * Thus it will ALWAYS return non-null, provided groups.n>0.
 *
 * If found and err_ret!=NULL, then set to 1.
 * If index out of range, then return pointer to the first group, or NULL if no groups, and set err_ret to 0.
 */
EngraverPointGroup *EngraverFillData::GroupFromIndex(int index, int *err_ret)
{
	if (err_ret) *err_ret=1;
	if (index>=0 && index<groups.n) return groups.e[index];

	if (err_ret) *err_ret=0;
	return groups.n ? groups.e[0] : NULL;
}

void EngraverFillData::UpdatePositionCache()
{
	for (int c=0; c<groups.n; c++) {
		groups.e[c]->UpdatePositionCache();
	}
}

/*! Set the default spacing in group 0.
 */
double EngraverFillData::DefaultSpacing(double nspacing)
{
	double oldspacing=groups.e[0]->spacing;
	groups.e[0]->spacing=nspacing;
	return oldspacing;
}

SomeData *EngraverFillData::duplicate(SomeData *dup)
{
	EngraverFillData *p=dynamic_cast<EngraverFillData*>(dup);
	if (!p && !dup) return NULL; //was not EngraverFillData!

	//char set=1;
	if (!dup && somedatafactory) {
		dup=somedatafactory->newObject(LAX_ENGRAVERFILLDATA,this);
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			//set=0;
		}
		p=dynamic_cast<EngraverFillData*>(dup);
		p->groups.flush();
	} 
	if (!p) {
		p=new EngraverFillData();
		dup=p;
		p->groups.flush();
	}

	p->NeedToUpdateCache(0,0,-1,-1);

	for (int c=0; c<groups.n; c++) {
		EngraverPointGroup *group=new EngraverPointGroup;
		group->CopyFrom(groups.e[c], false, false, false);
		p->groups.push(group);
	}

	return PatchData::duplicate(dup);
}


/*! Make lines->p be the transformed s,t.
 */
void EngraverFillData::Sync(bool asneeded)
{
	LinePoint *l;
	EngraverPointGroup *group;

	for (int g=0; g<groups.n; g++) {
		group=groups.e[g];

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];

			while (l) {
				if (!asneeded || (asneeded && l->needtosync==1))
					l->p=getPoint(l->s,l->t, false); // *** note this is hideously inefficient, matrices are not cached with getPoint!!!
				l->needtosync=0;

				l=l->next;
			}
		}
	}
}

/*! Assume lines->p are accurate, and we need to map back to s,t mesh coordinates.
 */
void EngraverFillData::ReverseSync(bool asneeded)
{
	LinePoint *l;
	flatpoint pp;
	int in=0;
	EngraverPointGroup *group;

	for (int g=0; g<groups.n; g++) {
		group=groups.e[g];

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];

			while (l) {
				if (!asneeded || (asneeded && l->needtosync==2)) {
					pp=getPointReverse(l->p.x,l->p.y, &in); // *** note this is hideously inefficient
					if (in) {
						l->s=pp.x;
						l->t=pp.y;
						l->needtosync=0;
					}
				}

				l=l->next;
			}
		}
	}
}


/*! This is basically a flush of all points groups, and make default group be linear.
 *
 * spacing is an object distance (not in s,t space) to be used as the distance between line centers.
 * If spacing<0, then use 1/25 of the x or y dimension, whichever is smaller
 */
void EngraverFillData::FillRegularLines(double weight, double spacing)
{
	if (spacing<=0) {
		if (maxx-minx<maxy-miny) spacing=(maxx-minx)/20;
		else spacing=(maxy-miny)/20;
	}

	while (groups.n>1) groups.remove(-1);
	DefaultSpacing(spacing);

	groups.e[0]->type=EngraverPointGroup::PGROUP_Linear;
	groups.e[0]->Fill(this, -1);
	Sync(false);
}

void EngraverFillData::Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	PatchData::Set(xx,yy,ww,hh,nr,nc,stle);
	NeedToUpdateCache(0,0,-1,-1);
}

/*! \ingroup interfaces
 * Dump out an EngraverFillData.
 *
 * Something like:
 * <pre>
 *  filename blah.jpg
 *  iwidth 100
 *  iheight 100
 *  matrix 1 0 0 1 0 0
 *  xsize 4
 *  ysize 4
 *  points \
 *    1 2
 *    3 4
 *    5 6
 * </pre>
 * 
 * \todo *** assumes data is from filename. It shouldn't. It might be random buffer information.
 *
 * Calls PatchData::dump_out(f,indent,0,context), then puts out filename, and the
 * pixel dimensions of the image in filename.
 * If what==-1, then output a pseudocode mockup of the format. Otherwise
 * output the format as above.
 */
void EngraverFillData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (what==-1) {
		fprintf(f,"%smesh             #The mesh used to encase the lines\n",spc);
		PatchData::dump_out(f,indent+2,-1,context);

		fprintf(f,"%sgroup            #(1 or more)\n",spc);
		groups.e[0]->dump_out(f,indent+2,-1,context, NULL,NULL);

		return;
	}

	fprintf(f,"%smesh\n",spc);
	PatchData::dump_out(f,indent+2,what,context);

	const char *sharetrace=NULL, *sharedash=NULL;
	for (int c=0; c<groups.n; c++) {
		sharetrace=NULL;
		sharedash =NULL;

		for (int c2=0; c2<c; c2++) {
			if (groups.e[c2]->trace ==groups.e[c]->trace)  sharetrace=groups.e[c2]->name;
			if (groups.e[c2]->dashes==groups.e[c]->dashes) sharedash =groups.e[c2]->name;
		}

		fprintf(f,"%sgroup\n",spc);
		groups.e[c]->dump_out(f,indent+2,what,context, sharetrace,sharedash);
	}


	return;
}

//! Reverse of dump_out.
void EngraverFillData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;

	char *name;
	//char *value;
	int c;

	bool groups_flushed=false;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		//value=att->attributes.e[c]->value;

		if (!strcmp(name,"mesh")) {
			PatchData::dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"group")) {
			if (groups.n && !groups_flushed) {
				 //only flush groups when there are actual groups being read in
				groups.flush();
				groups_flushed=true;
			}
			EngraverPointGroup *group=new EngraverPointGroup;
			group->dump_in_atts(att->attributes.e[c],flag,context);
			groups.push(group);

		}
	}

	 //now we need to account for block sharing....
	char *s,*e;
	char type=0; //'t', 'd', 'D', 's'
	for (int c=0; c<groups.n; c++) {
		if (isblank(groups.e[c]->iorefs)) continue;

		s=groups.e[c]->iorefs+1;
		while (*s) {
			if (!strncmp(s,"trace:",6)) { type='t'; s+=6; }
			else if (!strncmp(s,"dash:",5)) { type='d'; s+=5; }
			else if (!strncmp(s,"direction:",10)) { type='D'; s+=10; }
			else if (!strncmp(s,"spacing:",8)) { type='s'; s+=8; }
			else type=0;

			e=strchr(s,'|');
			if (!e) e=s+strlen(s);

			if (type!=0) {
				for (int c2=0; c2<groups.n; c2++) {
					if (c2==c) continue;
					if (!strncmp(groups.e[c2]->name,s,e-s+1)) {
						if (type=='t') groups.e[c]->InstallTraceSettings(groups.e[c2]->trace, 0);
						if (type=='d') groups.e[c]->InstallDashes(groups.e[c2]->dashes, 0);
						//if (type=='D') groups.e[c]->InstallDirection(groups.e[c2]->direction);
						//if (type=='s') groups.e[c]->InstallSpacing(groups.e[c2]->spacing);
					} 
				}
			}
			s=e;
			if (*s=='|') s++;
		}
		
		delete[] groups.e[c]->iorefs;
		groups.e[c]->iorefs=NULL;
	}

	FindBBox();
	Sync(false);

	for (int c=0; c<groups.n; c++) {
		groups.e[c]->UpdateBezCache();
		groups.e[c]->UpdateDashCache();
	} 
}

/*! Return whether a point is considered on by the criteria of the settings in dashes.
 * Default is point must be on, and weight>=zero_threshhold.
 */
int EngraverFillData::PointOn(LinePoint *p, EngraverPointGroup *g)
{
	if (!p->on) return 0; 
	if (!groups.n) return 0;
	if (!g) g=groups.e[0]; 
	return g->PointOn(p);
}

/*! Creates a single PathsData as outline.
 * If whichgroup<0, then pack all groups. But be warned doing this might not respect separate colors
 */
PathsData *EngraverFillData::MakePathsData(int whichgroup)
{
	PathsData *paths=NULL;
    if (somedatafactory) 
		paths=dynamic_cast<PathsData*>(somedatafactory->newObject(LAX_PATHSDATA,NULL));
    else paths=new PathsData();
	paths->m(m());

	paths->line(0,-1,-1,&groups.e[0]->color); //set default paths color, maybe overridden by individual paths
	paths->fill(&groups.e[0]->color); //set default paths color, maybe overridden by individual paths
	// *** should we make it ordered so that all lines from same group are next to each other in paths??

	//currently, makes a PathsData with the outline of all the strokes...

	//Todo: does not currently handly 0 weight segments properly

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l;
	LinePointCache *lc, *lcstart;
	flatvector t, tp;
	flatvector p1,p2;
	EngraverPointGroup *group;

	int s=0, e=groups.n;
	if (whichgroup>=0 && whichgroup<groups.n) { s=e=whichgroup; }

	for (int g=s; g<=e; g++) {
		group=groups.e[g];

		if (!group->active) continue;

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];
			lc=l->cache;
			lcstart=lc;

			 //make points be a list of points:
			 //   2  4  6 
			 // 1 *--*--*--8   gets rearranged to: 1 2 4 6 8 3 5 7
			 //   3  5  7
			 //points 1 and 8 are cap point references, converted to rounded ends later
			do { //one loop per connected segment
				if (!group->PointOnDash(lc)) { lc=lc->next; continue; } //skip off points

				 //get tangent vector at lc
				if (lc->original!=NULL) {
					l=lc->original;
					if (l->next) tp=bez_visual_tangent(0, l->p,l->bez_after,l->next->bez_before,l->next->p);
					else if (l->prev) tp=bez_visual_tangent(1, l->prev->p,l->prev->bez_after,l->bez_before,l->p);
					else tp=flatpoint(1,0);

				} else {
					l=lc->PrevOriginal();
					tp=bez_visual_tangent(lc->bt, l->p,l->bez_after,l->next->bez_before,l->next->p);
				}

				tp.normalize(); 
				t=transpose(tp);

				if (points.n==0) {
					 //add a first point cap
					points.push(lc->p - lc->weight/2*tp);
				}

				 //add top and bottom points for l
				p1=lc->p + lc->weight/2*t;
				p2=lc->p - lc->weight/2*t;

				points.push(p1);
				points.push(p2);


				if (!lc->next || !group->PointOnDash(lc->next) || lc->dashon==ENGRAVE_EndPoint) {
					 //need to add a path!
		
					 //add last cap
					tp=points.e[points.n-1]-points.e[points.n-2];
					tp.normalize();
					tp=transpose(tp);
					points.push(lc->p + lc->weight/2*tp);


					 //convert to bez approximation
					 //make points2 be points rearranged according to outline
					points2.push(points.e[0]); //initial cap
					for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

					points2.push(points.e[points.n-1]); //final cap
					for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
					

					BezApproximate(points,points2);

					paths->moveTo(points.e[1]);
					for (int c2=1; c2<points.n; c2+=3) {
						paths->curveTo(points.e[c2+1], points.e[(c2+2)%points.n], points.e[(c2+3)%points.n]);
					}

					paths->close();

					points.flush_n();
					points2.flush_n();
				}

				lc=lc->next;
			} while (lc && lc!=lcstart);

		}
	}

	 //finally, install color for this line
	//paths->paths.e[paths->paths.n-1]->LineColor(&group->color);
	paths->fill(&group->color);
	paths->fillstyle->fillrule=LAXFILL_Nonzero;
	paths->line(0,-1,-1,&group->color);
	paths->linestyle->function=LAXOP_None;

	return paths;
}

void EngraverFillData::dump_out_svg(const char *file)
{
	FILE *f=fopen(file,"w");
	if (!f) return;

	setlocale(LC_ALL,"C");

	//powerstroke path effect goes in defs, BUT the outline is in the path->d,
	//the original path is in an attribute of the path...
	

	fprintf(f,  "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
				"<!-- Created with Inkscape (http://www.inkscape.org/) -->\n"
				"\n"
				"<svg\n"
				"   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
				"   xmlns:cc=\"http://creativecommons.org/ns#\"\n"
				"   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
				"   xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
				"   xmlns=\"http://www.w3.org/2000/svg\"\n"
				"   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n"
				"   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
				//"   width=\"%.10g\"\n"
				//"   height=\"%.10g\"\n"
				"   width=\"1000\"\n"
				"   height=\"1000\"\n"
				"   id=\"svg2\"\n"
				"   version=\"1.1\"\n"
				"   inkscape:version=\"0.48+devel r custom\"\n"
				"   viewBox=\"0 0 1000 1000\"\n"
				"   sodipodi:docname=\"test-powerstroke.svg\">\n"
				"  <defs id=\"defs4\">\n" 
					//1.1*(maxx-minx), 1.1*(maxy-miny)
			);


	// *** output path effect defs
	//    <inkscape:path-effect
	//       effect="powerstroke"
	//       id="path-effect4064"
	//       is_visible="true"
	//       offset_points="0,1 | 3.0017209,-52.700851 | 2.2287256,-40.548946 | 5,-2.3889435"
	//       sort_points="true"
	//       interpolator_type="CubicBezierJohan"
	//       interpolator_beta="0.2"
	//       start_linecap_type="round"
	//       linejoin_type="round"
	//       miter_limit="4"
	//       end_linecap_type="round" />


	fprintf(f,  "  </defs>\n");
	fprintf(f,  " <g\n"
				"     inkscape:label=\"Layer 1\"\n"
				"     inkscape:groupmode=\"layer\"\n"
				"     id=\"layer1\">\n");
	double s;
	if (maxx-minx>maxy-miny) s=900/(maxx-minx);
	else s=900/(maxy-miny);
	double neww=s*(maxx-minx);
	double newh=s*(maxy-miny);

	fprintf(f,  "  <g id=\"themesh\" "
	                 //"transform=\"matrix(1 0 0 -1 0 0)\" \n   >"
	                 "transform=\"matrix(%.10g 0 0 -%.10g %.10g %.10g)\" \n   >",
						s,s,
						//-(500-neww/2),miny*s+(500+newh/2) //-m(4),-m(5)
						(500-neww/2)-minx*s,miny*s+(500+newh/2) //-m(4),-m(5)
		         );
 
	// *** output paths
	//  <path d="....the full outline....."
	//        inkscape:path-effect="path-effect4064"
	//        inkscape:original-d="....original path...."

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l;
	LinePointCache *lc, *lcstart;
	flatvector t, tp;
	flatvector p1,p2;
	EngraverPointGroup *group;

	char stylestr[150];
	

	for (int g=0; g<groups.n; g++) {
		group=groups.e[g];

		sprintf(stylestr," style=\"fill:#%02x%02x%02x; fill-opacity:%.10g; stroke:none; stroke-opacity:%.10g;\" ",
						group->color.red>>8,group->color.green>>8,group->color.blue>>8,
						group->color.alpha/65535.,
						group->color.alpha/65535.
						);


		if (!group->active) continue;

		fprintf(f,"  <g>\n"); //encase each point group in its own svg group

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];
			lc=lcstart=l->cache;


			 //make points be a list of points:
			 //   2  4  6 
			 // 1 *--*--*--8   gets rearranged to: 1 2 4 6 8 3 5 7
			 //   3  5  7
			 //points 1 and 8 are cap point references, converted to rounded ends later
			do { //one loop per on segment
				if (!group->PointOnDash(lc)) { lc=lc->next; continue; }

				 //get tangent vector at lc
				if (lc->original!=NULL) {
					l=lc->original;
					if (l->next) tp=bez_visual_tangent(0, l->p,l->bez_after,l->next->bez_before,l->next->p);
					else if (l->prev) tp=bez_visual_tangent(1, l->prev->p,l->prev->bez_after,l->bez_before,l->p);
					else tp=flatpoint(1,0);

				} else {
					l=lc->PrevOriginal();
					tp=bez_visual_tangent(lc->bt, l->p,l->bez_after,l->next->bez_before,l->next->p);
				}

				tp.normalize(); 
				t=transpose(tp);

				if (points.n==0) {
					 //add a first point cap
					points.push(lc->p - lc->weight/2*tp);
				}

				 //add top and bottom points for lc
				p1=lc->p + lc->weight/2*t;
				p2=lc->p - lc->weight/2*t;

				points.push(p1);
				points.push(p2);

				if (!lc->next || !group->PointOnDash(lc->next) || lc->dashon==ENGRAVE_EndPoint) {
					 //end of segment, need to add a path to file!
		
					 //add last cap
					tp=points.e[points.n-1]-points.e[points.n-2];
					tp.normalize();
					tp=transpose(tp);
					points.push(lc->p + lc->weight/2*tp);


					 //convert to bez approximation
					 //make points2 be points rearranged according to outline
					points2.push(points.e[0]); //initial cap
					for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

					points2.push(points.e[points.n-1]); //final cap
					for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
					

					BezApproximate(points,points2);

					fprintf(f,"    <path %s d=\"", stylestr);
					fprintf(f,"M %f %f ", points.e[1].x,points.e[1].y);
					for (int c2=1; c2<points.n; c2+=3) {
						fprintf(f,"C %f %f %f %f %f %f ",
							points.e[c2+1].x,points.e[c2+1].y,
							points.e[(c2+2)%points.n].x,points.e[(c2+2)%points.n].y,
							points.e[(c2+3)%points.n].x,points.e[(c2+3)%points.n].y);
					}


					fprintf(f,"z \" />\n");

					points.flush();
					points2.flush();
				}

				lc=lc->next;
			} while (lc && lc!=lcstart);
		} //foreach line

		fprintf(f,"  </g>\n");
	} //foreach group


	fprintf(f,  "  </g>\n"
				" </g>\n"
				"</svg>\n");

	setlocale(LC_ALL,"");
	fclose(f);
}

/*! Add more sample points between all existing points.
 *
 * Approximate each line with a bezier curve, and grab the center of each segment.
 *
 * Afterwards, you will need to call ReverseSync(true).
 *
 * group is an index into groups, or -1 for default group.
 */
void EngraverFillData::MorePoints(int curgroup)
{
	NumStack<flatpoint> pts;
	flatpoint *bez=NULL;
	int n,i;
	LinePoint *ll;
	LinePoint *l;

	EngraverPointGroup *group;
	if (curgroup<0 || curgroup>=groups.n) group=groups.e[0];
	else group=groups.e[curgroup];

	for (int c=0; c<group->lines.n; c++) {
		l=group->lines.e[c];
		n=0;

		pts.flush();
		while (l) { pts.push(l->p); l=l->next; }

		if (3*pts.n>n) {
			if (bez) delete[] bez;
			bez=new flatpoint[3*pts.n];
			n=3*pts.n;
		}
		bez_from_points(bez, pts.e,pts.n);

		i=1;
		l=group->lines.e[c];
		while (l) {
			if (!l->next) break;
	
			ll=new LinePoint();
			ll->weight=(l->weight+l->next->weight)/2;
			ll->spacing=(l->spacing+l->next->spacing)/2;
			ll->s=(l->s+l->next->s)/2;
			ll->t=(l->t+l->next->t)/2; //just in case reverse map doesn't work
			ll->p=bez_point(.5, l->p, bez[i+1], bez[i+2], l->next->p);

			if (l->on==ENGRAVE_EndPoint) l->on=ENGRAVE_Off;
			else ll->on=(l->on || l->next->on ? ENGRAVE_On : ENGRAVE_Off);

			ll->next=l->next;
			l->next->prev=ll;
			l->next=ll;
			ll->prev=l;
			ll->needtosync=2;

			i+=3;
			l=l->next->next;
		}
	}
}

/*! points is a special list of sample points, meaning points that lie on the line.
 * This makes fauxpoints be a bezier list: c-p-c-c-p-c-...-c-p-c smoothly connecting all the points.
 *
 * points is assumed to be a list of top points, then of matched bottom points, so
 * points[0] is on top top points[points.n/2], and so on.
 * Currently only round caps are applied.
 */
void EngraverFillData::BezApproximate(Laxkit::NumStack<flatvector> &fauxpoints, Laxkit::NumStack<flatvector> &points)
{
	// There are surely better ways to do this. Not sure how powerstroke does it.
	// This is not simplied/optimized at all. Each point gets control points to smooth it out.
	// no fancy corner handling done yet

    fauxpoints.flush();

    flatvector v,p, pp,pn;
	flatvector opn, opp;


    double sx;
	//caps are at points index 0 and points.n/2
	
    for (int c=0; c<points.n; c++) {
        p=points.e[c];

		if (c==0) {
			 //on first cap, apply round
			opp=p+3*.5522*(points.e[points.n-1]-points.e[1])/2;
			opn=p+3*.5522*(points.e[1]-points.e[points.n-1])/2;

		} else if (c==points.n/2) {
			 //on final cap, apply round
			opp=p+3*.5522*(points.e[c-1]-points.e[c+1])/2;
			opn=p+3*.5522*(points.e[c+1]-points.e[c-1])/2;

		} else {
			if (c==1 || c==points.n/2+1) {
				pp=(p+points.e[points.n-c])/2;
				v=points.e[c-1]-pp;
				opp=p+3*.5522*v;
			} else opp=points.e[c-1];

			if (c==points.n-1 || c==points.n/2-1) {
				pp=(p+points.e[points.n-c])/2;
				v=points.e[(c+1)%points.n]-pp;
				opn=p+3*.5522*v;
			} else opn=points.e[c+1];
		}

        v=opn-opp;
        v.normalize();

        sx=norm(p-opp)*.333;
        pp=p - v*sx;

        sx=norm(opn-p)*.333;
        pn=p + v*sx;

        fauxpoints.push(pp);
        fauxpoints.push(p);
        fauxpoints.push(pn);

    }
}



//------------------------------ TraceObject -------------------------------

/*! \class TraceObject
 *
 * Class to hold information about a tracing source, for use in EngraverTraceSettings.
 */

TraceObject::TraceObject()
{
	type=TRACE_None;

	object=NULL;
	image_file=NULL;

	samplew=sampleh=0;
	trace_sample_cache=NULL;
	cachetime=0;

	 //black and white cache:
	tw=th=0; //dims of trace_ref_bw
	trace_ref_bw=NULL;
}

TraceObject::~TraceObject()
{
	if (object) object->dec_count();
	delete[] image_file;
	delete[] trace_sample_cache;
	delete[] trace_ref_bw;
}

/*! Note this is more for one time lookups. Not very efficient for mass lookups.
 * Returns -1 for point outside of trace object.
 *
 * trace_sample_cache MUST be set up properly. TRACE_Current is just pass through for p->weight.
 *
 * If transform!=NULL, then transform p->p by transform before using.
 */
double TraceObject::GetValue(LinePoint *p, double *transform)
{
	if (type==TRACE_Current) return p->weight;

	int x,y,i;
	int sample, samplea;
	flatpoint pp=p->p;
	if (transform) pp=transform_point(transform,pp);

	x=samplew*(pp.x-object->minx)/(object->maxx-object->minx);
	y=sampleh*(pp.y-object->miny)/(object->maxy-object->miny);

	if (x>=0 && x<samplew && y>=0 && y<sampleh) {
		i=4*(x+(sampleh-y)*samplew);

		samplea=trace_sample_cache[i+3];
		if (samplea==0) return -1; //transparent sample!

		sample=0.3*trace_sample_cache[i] + 0.59*trace_sample_cache[i+1] + 0.11*trace_sample_cache[i+2];
		if (sample>255) {
			sample=255;
		}

		return (255-sample)/255.; 
	}
	
	// else point outside sample area
	return -1;
}

void TraceObject::Install(TraceObjectType ntype, SomeData *obj)
{
	if (object && object!=obj) {
		object->dec_count();
		object=NULL;
	}

	type=ntype;
	object=obj;
	if (obj) obj->inc_count();


	 //make object_idstr be basically same as identifier
	if (type==TRACE_Current) {
		makestr(object_idstr, "current");

	} else if (type==TRACE_ImageFile) {
		makestr(object_idstr, "file");

	} else if (type==TRACE_LinearGradient || type==TRACE_RadialGradient) {
		makestr(object_idstr, "gradient");

	} else if (type==TRACE_Object) { 
		delete[] object_idstr;
		SomeDataRef *ref=dynamic_cast<SomeDataRef*>(object);
		object_idstr=new char[strlen(_("ref: %s"))+strlen(ref->thedata_id)+1];
		sprintf(object_idstr, _("ref: %s"),ref->thedata_id);
	}
}

void TraceObject::ClearCache(bool obj_too)
{
	delete[] trace_sample_cache;
	trace_sample_cache=NULL;
	samplew=sampleh=0;
	cachetime=0;

	if (obj_too) {
		object->dec_count();
		object=NULL;
	}
}

int TraceObject::NeedsUpdating()
{
	if (!trace_sample_cache) return 1;
	if (type==TRACE_Object) {
		if (!object) return 1;
		if (object->modtime>cachetime) return 1;
	}
	
	return 0;
}

/*! Calling this will always force a redrawing of the cache.
 * trace_sample_cache will only be reallocated if it is not currently large enough to hold data from object.
 *
 * viewport is unfortunately needed for a terrible hack to render random object references.
 */
int TraceObject::UpdateCache(ViewportWindow *viewport)
{
	//we need to render the trace object to a grayscale sample board
	

	//if (!trace_sample_cache) return; //cache already there
	if (!object) {
		ClearCache(false);
		return 0;
	}

	double w,h;
	w=object->maxx - object->minx;
	h=object->maxy - object->miny;
	if (w<500 && h<500) {
		if (w<h) {
			double a=w/h;
			h=500;
			w=h*a;
		} else {
			double a=h/w;
			w=500;
			h=500*a;
		}
	}


	Displayer *ddp=newDisplayer(NULL);
	ddp->CreateSurface((int)w,(int)h);

	 // setup ddp to have proper scaling...
	ddp->NewTransform(1.,0.,0.,-1.,0.,0.);
	//ddp->NewTransform(1.,0.,0.,1.,0.,0.);
	DoubleBBox bbox;
	bbox.addtobounds(object);
	ddp->SetSpace(bbox.minx,bbox.maxx,bbox.miny,bbox.maxy);
	ddp->Center(bbox.minx,bbox.maxx,bbox.miny,bbox.maxy);

	ddp->NewBG(255,255,255); // *** this should be the paper color for paper the page is on...
	ddp->NewFG(0,0,0,255);
	//ddp->m()[4]=0;
	//ddp->m()[5]=2*h;
	//ddp->Newmag(w/(bbox.maxx-bbox.minx));
	ddp->ClearWindow();


	viewport->DrawSomeData(ddp,object, NULL,NULL,0);
	ddp->EndDrawing();

	LaxImage *img=ddp->GetSurface();
	if (!img) {
		DBG cerr <<"could not render trace object"<<endl;
		return 1;
	}

	if (trace_sample_cache) {
		if (img->w()*img->h()>samplew*sampleh) {
			 //old isn't big enough to hold the new
			delete[] trace_sample_cache;
			trace_sample_cache=NULL;
		}
	}
	samplew=img->w();
	sampleh=img->h();
	if (trace_sample_cache==NULL) trace_sample_cache=new unsigned char[4*samplew*sampleh];

	unsigned char *data=img->getImageBuffer();
	memcpy(trace_sample_cache, data, 4*samplew*sampleh);
	img->doneWithBuffer(data);
	cachetime=time(NULL);

	// **** imlib only for DBG:
	LaxImlibImage *iimg=dynamic_cast<LaxImlibImage*>(img);
	imlib_context_set_image(iimg->image);
	imlib_image_set_format("png");
	imlib_save_image("trace.png");

	return 0;
}


//------------------------------ EngraverTraceSettings -------------------------------

/*! \class EngraverTraceSettings
 * Holds settings about tracing points from objects for EngraverFillData.
 */

EngraverTraceSettings::EngraverTraceSettings()
{
	continuous_trace=false; 
	group=-1;
	traceobj_opacity=1;
	tracetype=0;
	traceobject=NULL;
	identifier=NULL;
}

EngraverTraceSettings::~EngraverTraceSettings()
{
	if (traceobject) traceobject->dec_count();
	delete[] identifier;
}

EngraverTraceSettings *EngraverTraceSettings::duplicate()
{
	EngraverTraceSettings *dup=new EngraverTraceSettings;

	dup->continuous_trace=continuous_trace;
	dup->traceobject=traceobject;
	if (traceobject) traceobject->inc_count();

	dup->traceobj_opacity=traceobj_opacity;
	dup->tracetype=tracetype;
	makestr(dup->identifier, identifier);

	return dup;
}

/*! Creates brand new traceobject, dec_counts old.
 */
void EngraverTraceSettings::Install(TraceObject::TraceObjectType ntype, SomeData *obj)
{
	if (traceobject) traceobject->dec_count();
	traceobject=new TraceObject;
	traceobject->Install(ntype,obj);
}

/*! Remove traceobject if obj_too, else clear the object within traceobject.
 */
void EngraverTraceSettings::ClearCache(bool obj_too)
{
	delete[] identifier;
	identifier=NULL;

	if (traceobject) {
		if (obj_too) {
			traceobject->dec_count();
			traceobject=NULL;
		} else {
			traceobject->ClearCache(obj_too);
		}
	}
}

void EngraverTraceSettings::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *EngraverTraceSettings::dump_out_atts(Attribute *att,int what,Laxkit::anObject *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {
		att->push("curve","#The value to weight curve");
		att->push("view_opacity", "#Opacity of background reference");
		att->push("continuous true",   "#Whether to trace continuously");
		att->push("trace", "#What to trace from");
		return att;
	}

	char buffer[50];
	sprintf(buffer,"%.10g",traceobj_opacity);
	att->push("view_opacity", buffer);
	att->push("continuous", continuous_trace?"true":"false" );

	if (traceobject) {
		if (traceobject->type==TraceObject::TRACE_Current) {
			att->push("trace", "current");

		} else if (traceobject->type==TraceObject::TRACE_LinearGradient || traceobject->type==TraceObject::TRACE_RadialGradient) {
			att->push("trace", "gradient");
			traceobject->object->dump_out_atts(att->attributes.e[att->attributes.n-1], what, savecontext);

		} else if (traceobject->type==TraceObject::TRACE_ImageFile) {
			att->push("trace", "image");
			traceobject->object->dump_out_atts(att->attributes.e[att->attributes.n-1], what, savecontext); 

		} else if (traceobject->type==TraceObject::TRACE_Object) {
			att->push("trace", "object");
			traceobject->object->dump_out_atts(att->attributes.e[att->attributes.n-1], what, savecontext); 
		}
	}

	Attribute *att2=att->pushSubAtt("curve");
	value_to_weight.dump_out_atts(att2,what,savecontext);

	return att;
}

void EngraverTraceSettings::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"curve")) {
			value_to_weight.dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"view_opacity")) {
			DoubleAttribute(value,&traceobj_opacity, NULL);

		} else if (!strcmp(name,"continuous")) {
			continuous_trace=BooleanAttribute(value);

		} else if (!strcmp(name,"trace")) {
			if (!strcmp(value,"current")) {
				Install(TraceObject::TRACE_Current, NULL);
				makestr(identifier,NULL);

			} else if (!strcmp(value,"gradient")) {
				GradientData *grad=new GradientData;
				grad->dump_in_atts(att->attributes.e[c], flag,context);
				if (grad->IsRadial()) Install(TraceObject::TRACE_RadialGradient, grad);
				else Install(TraceObject::TRACE_LinearGradient, grad);
				grad->dec_count();

			} else if (!strcmp(value,"object")) {
				SomeDataRef *ref=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				ref->dump_in_atts(att->attributes.e[c], flag,context);
				Install(TraceObject::TRACE_Object, ref);
				makestr(identifier, traceobject->object_idstr);
				ref->dec_count();

			} else if (!strcmp(value,"image")) {
				ImageData *img=new ImageData;
				img->dump_in_atts(att->attributes.e[c], flag,context);

				delete[] identifier;  identifier=NULL;
				const char *bname=lax_basename(img->Filename());
				if (bname) {
					Install(TraceObject::TRACE_ImageFile, img);
					identifier=new char[strlen(_("img: %s"))+strlen(bname)+1];
					sprintf(identifier,_("img: %s"),bname);
				}

				img->dec_count();

			} else {
				makestr(identifier, value);
			}
		}
	}
}


////------------------------------ EngraverFillInterface -------------------------------


/*! \class EngraverFillInterface
 * \ingroup interfaces
 * \brief Interface for dealing with EngraverFillData objects.
 *
 * \todo *** select multiple datas to adjust. Mesh tinker only on one of them, touch up on many
 */

enum EngraveShortcuts {
	ENGRAVE_SwitchMode=PATCHA_MAX,
	ENGRAVE_SwitchModeR,
	ENGRAVE_ExportSvg,
	ENGRAVE_RotateDir,
	ENGRAVE_RotateDirR,
	ENGRAVE_SpacingInc,
	ENGRAVE_SpacingDec,
	ENGRAVE_ShowPoints,
	ENGRAVE_ShowPointsN,
	ENGRAVE_MorePoints,
	ENGRAVE_ToggleTrace,
	ENGRAVE_TogglePanel,
	ENGRAVE_ToggleGrow,
	ENGRAVE_ToggleWarp,
	ENGRAVE_ToggleDir,
	ENGRAVE_LoadDirectory,
	ENGRAVE_NextFill,
	ENGRAVE_PreviousFill,
	ENGRAVE_MAX
};

EngraverFillInterface::EngraverFillInterface(int nid,Displayer *ndp)
  : PatchInterface(nid,ndp),
	curvemapi(0,ndp)
{
	primary=1;
	//selection=NULL;

	showdecs=SHOW_Points|SHOW_Edges;
	drawrendermode=rendermode=3;
	recurse=0;
	edata=NULL;
	tracebox=NULL;

	current_group=0;
	default_spacing=1./20;
	turbulence_size=1;
	turbulence_per_line=false;

	show_points=0;
	submode=0;
	mode=controlmode=EMODE_Mesh;

	directionmap=NULL;

	curvemapi.owner=this;
	curvemapi.ChangeEditable(CurveMapInterface::YMax, 1);
	brush_radius=40;

	makestr(thickness.title,_("Brush Ramp"));
	thickness.SetSinusoidal(6);
	thickness.RefreshLookup();

	//DBG CurveWindow *ww=new CurveWindow(NULL,"curve","curve",0,0,0,400,400,0,NULL,0,NULL);
	//DBG ww->SetInfo(&thickness);
	//DBG app->addwindow(ww);

	whichcontrols=Patch_Coons;

	continuous_trace=false;
	show_panel=true;
	show_trace=false;
	grow_lines=false;
	always_warp=true;
	show_direction=false;

	eventobject=0;
	eventgroup=-1;

	lasthoverindex=lasthoverdetail=-1;
	lasthovercategory=ENGRAVE_None;
	lasthover=ENGRAVE_None;

	IconManager *iconmanager=IconManager::GetDefault();
	modes.AddItem(_("Mesh mode"),       iconmanager->GetIcon("EngraverMesh"),        EMODE_Mesh         );
	modes.AddItem(_("Thickness"),       iconmanager->GetIcon("EngraverThickness"),   EMODE_Thickness    );
	modes.AddItem(_("Blockout"),        iconmanager->GetIcon("EngraverKnockout"),    EMODE_Blockout     );
	modes.AddItem(_("Drag mode"),       iconmanager->GetIcon("EngraverDrag"),        EMODE_Drag         );
	modes.AddItem(_("Push/pull"),       iconmanager->GetIcon("EngraverPushPull"),    EMODE_PushPull     );
	modes.AddItem(_("Avoid"),           iconmanager->GetIcon("EngraverAvoid"),       EMODE_AvoidToward  );
	modes.AddItem(_("Twirl"),           iconmanager->GetIcon("EngraverTwirl"),       EMODE_Twirl        );
	modes.AddItem(_("Turbulence"),      iconmanager->GetIcon("EngraverTurbulence"),  EMODE_Turbulence   );
	//modes.AddItem(_("Resolution"),      iconmanager->GetIcon("EngraverResolution"), EMODE_Resolution   );
	modes.AddItem(_("Orientation"),     iconmanager->GetIcon("EngraverOrientation"), EMODE_Orientation  );
	modes.AddItem(_("Freehand"),        iconmanager->GetIcon("EngraverFreehand"),    EMODE_Freehand     );
	modes.AddItem(_("Trace adjustment"),iconmanager->GetIcon("EngraverTrace"),       EMODE_Trace        );
	//modes.AddItem(_("Direction"),       iconmanager->GetIcon("EngraverDirection"), EMODE_Direction    );

	fgcolor.rgbf(0.,0.,0.);
	bgcolor.rgbf(1.,1.,1.);
}

//! Empty destructor.
EngraverFillInterface::~EngraverFillInterface() 
{
	DBG cerr<<"-------"<<whattype()<<","<<" destructor"<<endl;
	//if (selection) selection->dec_count();
}

const char *EngraverFillInterface::Name()
{ return _("Engraver Fill Tool"); }

anInterface *EngraverFillInterface::duplicate(anInterface *dup)//dup=NULL;
{
	EngraverFillInterface *dupp;
	if (dup==NULL) dupp=new EngraverFillInterface(id,NULL);
	else dupp=dynamic_cast<EngraverFillInterface *>(dup);
	if (!dupp) return NULL;

	dupp->recurse=recurse;
	dupp->rendermode=rendermode;
	return PatchInterface::duplicate(dupp);
}

//! Return new local EngraverFillData
PatchData *EngraverFillInterface::newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	EngraverFillData *ndata=NULL;
	if (somedatafactory) {
		ndata=dynamic_cast<EngraverFillData *>(somedatafactory->newObject(LAX_ENGRAVERFILLDATA));
	} 
	if (!ndata) ndata=new EngraverFillData();//creates 1 count

	ndata->MakeGroupNameUnique(0);
	ndata->groups.e[0]->InstallTraceSettings(default_trace.duplicate(),1);
	//if (!ndata->groups.e[0]->trace)  ndata->groups.e[0]->trace =new EngraverTraceSettings;
	if (!ndata->groups.e[0]->dashes) ndata->groups.e[0]->dashes=new EngraverLineQuality;

	ndata->Set(xx,yy,ww,hh,nr,nc,stle);
	//ndata->renderdepth=-recurse;
	//ndata->xaxis(3*ndata->xaxis());
	//ndata->yaxis(3*ndata->yaxis());
	//ndata->origin(flatpoint(xx,yy));
	//ndata->xaxis(flatpoint(1,0)/Getmag()*100);
	//ndata->yaxis(flatpoint(0,1)/Getmag()*100);

	ndata->DefaultSpacing((ndata->maxy-ndata->miny)/20);
	ndata->style|=PATCH_SMOOTH;
	ndata->groups.e[0]->Fill(ndata, 1./dp->Getmag());
	ndata->Sync(false);
	ndata->FindBBox();

	return ndata;
}

//! id==4 means make recurse=ndata.
int EngraverFillInterface::UseThis(int id,int ndata)
{
	if (id!=4) return PatchInterface::UseThis(id,ndata);
	char blah[100];
	if (id==4) { // recurse depth
		if (ndata>0) {
			recurse=ndata;
			sprintf(blah,_("New Recurse Depth %d: "),recurse);
			PostMessage(blah);
			if (rendermode>0) needtodraw=1; 
		}
		return 1;
	}
	return 0;
}

int EngraverFillInterface::UseThisObject(ObjectContext *oc)
{
	int c=PatchInterface::UseThisObject(oc);
	edata=dynamic_cast<EngraverFillData *>(data);
	return c;
}

/*! Accepts EngraverFillData, or LineStyle for color.
 *
 * Returns 1 to used it, 0 didn't
 *
 */
int EngraverFillInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask) // assumes not use local
{
    if (!nobj) return 0;

	if (dynamic_cast<EngraverFillData *>(nobj)) { 
		return PatchInterface::UseThis(nobj,mask);

	} else if (dynamic_cast<LineStyle *>(nobj) && edata) {
		if (!edata) return 1;
        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);

		if (current_group>=0) {
			EngraverPointGroup *group=edata->groups.e[current_group];
			group->color=nlinestyle->color;
			needtodraw=1;
		}
        return 1;
	}


	return 0;
}

void EngraverFillInterface::deletedata()
{
	PatchInterface::deletedata();
	edata=NULL;
}


//! Flush curpoints.
int EngraverFillInterface::InterfaceOff()
{
	PatchInterface::InterfaceOff();
	curvemapi.SetInfo(NULL);
    return 0;
}


/*! Returns the panel item (x,y) is over, or ENGRAVE_None.
 * category will get set with one of the main section tags of the panel.
 * That is ENGRAVE_Groups, ENGRAVE_Tracing, ENGRAVE_Dashes, ENGRAVE_Direction,
 * or ENGRAVE_Spacing.
 */
int EngraverFillInterface::scanPanel(int x,int y, int *category, int *index_ret, int *detail_ret)
{
	if (!panelbox.boxcontains(x,y)) {
		DBG cerr <<"not in panel"<<endl;
		*category=ENGRAVE_None;
		return ENGRAVE_None;
	}
	DBG cerr <<"in panel"<<endl;

	*category=ENGRAVE_Panel;
	x-=panelbox.minx;
	y-=panelbox.miny;

	MenuItem *item, *item2;
	for (int c=0; c<panel.n(); c++) {
		item=panel.e(c);
		if (item->pointIsIn(x,y)) {
			if (item->id==ENGRAVE_Mode_Selection) {
				int i=(x-item->x)*modes.n()/item->w;
				if (i>=0 && i<modes.n()) return modes.e(i)->id;
			}

			if (item->id==ENGRAVE_Tracing && item->isOpen()) {
				// ******** TEMPORARY! vv
				if (tracebox->pointIsIn(x,y)) {
					double th=dp->textheight();
					double pad=2;

					*category=ENGRAVE_Panel;

					if(y<tracebox->y+2+th*4/3) {
						if (x<(tracebox->x+tracebox->w/2))
							return ENGRAVE_Trace_Once;
						else return ENGRAVE_Trace_Continuous;
					}
					if (y>(tracebox->y+tracebox->h)-pad-th) return ENGRAVE_Trace_Identifier;
					if (y>(tracebox->y+tracebox->h)-pad-2*th) return ENGRAVE_Trace_Opacity;
					if (x>tracebox->x+1.5*th && y<(tracebox->y+tracebox->h)-pad-2*th) return ENGRAVE_Trace_Curve;
					return ENGRAVE_Trace_Box;
				}
				// ******** TEMPORARY! ^^
			}

			if (item->hasSub() && item->isOpen()) {
				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					if (item2->pointIsIn(x,y)) {
						if (item2->id==ENGRAVE_Group_List) {
							double th=dp->textheight();
							//int n=NumGroupLines();
							int index=(y-item2->y)/th;
							int detail=(x-item2->x)/th;
							if (detail>3) detail=3;
							if (index_ret) *index_ret=index;
							if (detail_ret) *detail_ret=detail;
						}
						return item2->id;
					}
				}
			}
			 //in blank space of the section, or on title head
			return item->id;
		}
	}
	 //else on blank area of entire panel
	return ENGRAVE_Panel;

}

/*! Scan the panel or for other onscreen controls.
 */
int EngraverFillInterface::scanEngraving(int x,int y, int *category, int *index_ret, int *detail_ret)
{
	*category=0;

	if (show_panel) {
	    if (panelbox.boxcontains(x,y)) {
			return scanPanel(x,y,category,index_ret,detail_ret);
		}


		if (mode==EMODE_Trace) {
			flatpoint p=screentoreal(x,y); //p is in edata->parent space
			if (edata && edata->pointin(p)) return ENGRAVE_Trace_Move_Mesh;

			//Affine a=edata->GetTransformToContext(true, 0);
			//p=a.transformPoint(p);
			p=dp->screentoreal(x,y);

			EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

			if (trace->traceobject && trace->traceobject->object && trace->traceobject->object->pointin(p)) {
				return ENGRAVE_Trace_Object;
			}
		}
	}

	if (mode==EMODE_Orientation && edata) {
		 //note: need to coordinate with DrawOrientation
		*category=ENGRAVE_Orient;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		flatpoint p(x,y);
		flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
		center=realtoscreen(edata->transformPoint(center));

		int size=30;
		//double thick=.25;

		flatpoint dir=group->direction/norm(group->direction)*.05;
//		flatpoint xx= realtoscreen(edata->transformPoint(edata->getPoint(.5+dir.x,.5+dir.y)))
//					- realtoscreen(edata->transformPoint(edata->getPoint(.5,      .5      )));
		flatpoint xx= realtoscreen(edata->transformPoint(edata->getPoint(.5+dir.x,.5+dir.y, false)))
					- realtoscreen(edata->transformPoint(edata->getPoint(.5,      .5      , false)));
		xx=xx/norm(xx)*size;
		flatpoint yy=-transpose(xx);

		flatpoint pp;
		pp.x=(p-center)*xx/norm2(xx);
		pp.y=(p-center)*yy/norm2(yy);

		DBG cerr <<"orient pp:"<<pp.x<<','<<pp.y<<endl;
		if (pp.y>.5 && pp.y<1.5 && pp.x>-.5 && pp.x<.5) return ENGRAVE_Orient_Spacing;
		if (pp.y<.5 && pp.y>-.3) {
			if (pp.x>1.5 && pp.x<2.5) return ENGRAVE_Orient_Direction;
			if (pp.x<=1.5 && pp.x>-.5) return ENGRAVE_Orient_Position;
		}

		return ENGRAVE_None;
	}


	return ENGRAVE_None;
}

/*! oc->obj must be castable to an EngraverFillData or a PathsData.
 */
int EngraverFillInterface::AddToSelection(ObjectContext *oc)
{
    if (!oc || !oc->obj) return -2;
    if (   !dynamic_cast<EngraverFillData*>(oc->obj)
		&& !dynamic_cast<PathsData*>(oc->obj)) return -3; //not a usable object!

	if (!selection) selection=new Selection();
    return selection->AddNoDup(oc,-1);
}

//! Catch a double click to pop up an ImageDialog.
int EngraverFillInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	lasthover= scanEngraving(x,y, &lasthovercategory, &lasthoverindex, &lasthoverdetail);

	 //catch trace box overlay first
	if (lasthovercategory==ENGRAVE_Panel && panelbox.boxcontains(x,y)) {

		if ((state&LAX_STATE_MASK)!=0) lasthover=ENGRAVE_Panel;

		if (lasthover==ENGRAVE_Trace_Curve) {
			double pad=2;
			double th=dp->textheight();
			curvemapi.Dp(dp);
			EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
			curvemapi.SetInfo(&trace->value_to_weight);

			DoubleBBox box;
			box.minx=panelbox.minx + tracebox->x+2*th+2*pad;
			box.miny=panelbox.miny + tracebox->y+4./3*th;
			box.maxx=panelbox.minx + (tracebox->x+tracebox->w)-pad;
			box.maxy=panelbox.miny + (tracebox->y+tracebox->h)-pad-2*th-1.5*th;
			curvemapi.SetupRect(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny);

			child=&curvemapi;
			child->inc_count();
			if (curvemapi.LBDown(x,y,state,count,d)==1) {
				child->dec_count();
				child=NULL;
				lasthover=ENGRAVE_Trace_Box;
			}

			needtodraw=1;
			return 0;
		}

		buttondown.down(d->id,LEFTBUTTON,x,y,lasthover,lasthovercategory);
		controlmode=EMODE_Controls;
		needtodraw=1;

		if (lasthover==ENGRAVE_Trace_Opacity) {
			double pad=2;
			x-=panelbox.minx;
			y-=panelbox.miny;

			EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

			trace->traceobj_opacity=(x-(tracebox->x+pad))/(tracebox->w-2*pad);
			if (trace->traceobj_opacity<0) group->trace->traceobj_opacity=0;
			else if (trace->traceobj_opacity>1) group->trace->traceobj_opacity=1;

			DBG cerr << " *** need to implement actual trace object opacity"<<endl;
			needtodraw=1;
		}

		return 0;
	}

	if (child) return 1;

	if (mode==EMODE_Direction) {
		if (count==2 || !directionmap) {
			PerformAction(ENGRAVE_LoadDirectory);
			return 0;
		}

		buttondown.down(d->id,LEFTBUTTON,x,y,lasthover);
		return 0;
	}

	if (mode==EMODE_Trace) {
		 //we haven't clicked on the tracing box, so search for images to grab..
		//RectInterface *rect=new RectInterface(0,dp);
		//rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
		//dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
		//rect->owner=this;
        //rect->UseThis(&base_cells,0);
        //child=rect;
        //AddChild(rect,0,1);

		if (lasthover==ENGRAVE_Trace_Object || lasthover==ENGRAVE_Trace_Move_Mesh) {
			buttondown.down(d->id,LEFTBUTTON,x,y,lasthover);
			controlmode=EMODE_Controls;
			needtodraw=1;
			return 0;
		}

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

		if (!trace->traceobject) {
			SomeData *obj=NULL;
			ObjectContext *oc=NULL;
			int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
			if (c>0) obj=oc->obj;

			if (obj && obj!=edata) {
				 //set up proxy object
				SomeDataRef *ref=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				ref->Set(obj, false);
				ref->flags|=SOMEDATA_KEEP_ASPECT;
				double m[6]; //,m2[6],m3[6];
				viewport->transformToContext(m,oc,0,1);
				//viewport->transformToContext(m2,poc,0,1);//of current mesh
				//transform_invert(m3,m2);
				//transform_mult(m2,m,m3);
				//ref->m(m2);
				ref->m(m);

				trace->Install(TraceObject::TRACE_Object, ref);

				trace->ClearCache(false);
				delete[] trace->identifier;
				trace->identifier=new char[strlen(_("ref: %s"))+strlen(ref->thedata_id)+1];
				sprintf(trace->identifier,_("ref: %s"),ref->thedata_id);

				if (data->UsesPath()) ActivatePathInterface();
				needtodraw=1;
			}
		}


		return 0;
	}

	if (mode==EMODE_Freehand) {
		FreehandInterface *freehand=new FreehandInterface(this,-1,dp);
		freehand->freehand_style=FREEHAND_Path_Mesh|FREEHAND_Remove_On_Up;
		viewport->Push(freehand,-1,0);
		freehand->LBDown(x,y,state,count,d);
		if (child) RemoveChild();
		child=freehand;
		return 0;
	}
	
	if (!edata) ChangeMode(EMODE_Mesh);

	if (mode==EMODE_Mesh) {
		EngraverFillData *olddata=edata;
		int c=PatchInterface::LBDown(x,y,state,count,d);
		if (!edata && data) {
			edata=dynamic_cast<EngraverFillData*>(data);
			AddToSelection(poc);
		}
		if (edata!=olddata) UpdatePanelAreas();
		return c;
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  || mode==EMODE_AvoidToward
		  ) {
		if (count==2 && (state&LAX_STATE_MASK)==ShiftMask) {
			 //edit brush ramp

			 // *** in future, should be on symmetric brush ramp editing
			CurveMapInterface *ww=new CurveMapInterface(-1,dp,_("Brush Ramp"));
			ww->SetInfo(&thickness);

			child=ww;
            ww->owner=this;
			int pad=(dp->Maxx-dp->Minx)*.1;
			ww->SetupRect(dp->Minx+pad,dp->Miny+pad, dp->Maxx-dp->Minx-2*pad,dp->Maxy-dp->Miny-2*pad);
            viewport->Push(ww,-1,0);
			submode=0;
			return 0;
		}
		buttondown.down(d->id,LEFTBUTTON, x,y, mode);
		return 0;
	}

	if (mode==EMODE_Orientation) {
		if (lasthover==ENGRAVE_Orient_Position ||
			lasthover==ENGRAVE_Orient_Direction ||
			lasthover==ENGRAVE_Orient_Spacing)
		  {
		  buttondown.down(d->id,LEFTBUTTON, x,y, lasthover);
		}
		return 0;
	}

	return 0;
}
	
int EngraverFillInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (child) {
		if (child==&curvemapi) { //to be here, curvemapi must have taken the lbdown
			child->LBUp(x,y,state,d);
			child->dec_count();
			child=NULL;
			needtodraw=1;
			return 0;
		}
	}

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;


	 //catch trace box overlay first
	int over=0;
	int category=0;
	buttondown.getextrainfo(d->id,LEFTBUTTON, &over,&category);

	if (category==ENGRAVE_Panel) {

		int dragged=buttondown.up(d->id,LEFTBUTTON);
		controlmode=mode;

		if (lasthover!=over) return 0;
		//so all these that follow are clicking down and up over same area..

		 //open or collapse a whole section..
		if ( dragged<10
			  && (over==ENGRAVE_Tracing
			   || over==ENGRAVE_Direction
			   || over==ENGRAVE_Spacing
			   || over==ENGRAVE_Dashes)) {
			MenuItem *item=NULL;
			for (int c=0; c<panel.n(); c++) {
				item=panel.e(c);
				if (item->id==over) {
					if (item->isOpen()) item->Close();
					else item->Open();
					UpdatePanelAreas();
				}
				
			}
			needtodraw=1;
			return 0;
		}

		if (dragged<10 && over==ENGRAVE_Toggle_Group_List) {
			MenuItem *item=panel.findid(ENGRAVE_Group_List);
			if (item->isOpen()) item->Close(); else item->Open();
			UpdatePanelAreas();
			needtodraw=1;
			return 0;
		}

		if (dragged<10 && over==ENGRAVE_Group_List) {
			EngraverFillData *obj;
			int gindex=-1;
			obj=GroupFromLineIndex(lasthoverindex, &gindex);

			if (!obj) return 0;

			if (lasthoverdetail==0) {
				 //active
				obj->groups.e[gindex]->active=!obj->groups.e[gindex]->active;

			} else if (lasthoverdetail==1) {
				 //linked
				obj->groups.e[gindex]->linked=!obj->groups.e[gindex]->linked;

			} else if (lasthoverdetail==2) {
				 //color
				anXWindow *w=new ColorSliders(NULL,"New Color","New Color",
							   ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS,
							   0,0,200,400,0,
							   NULL,object_id,"newcolor",
							   LAX_COLOR_RGB,1./255,
							   obj->groups.e[gindex]->color.red/65535.,
							   obj->groups.e[gindex]->color.green/65535.,
							   obj->groups.e[gindex]->color.blue/65535.,
							   obj->groups.e[gindex]->color.alpha/65535.
							);
				eventgroup=gindex;
				eventobject=edata->object_id;
				app->addwindow(w);

			} else if (lasthoverdetail==3) {
				 //name/make current
				if (obj!=edata) {
					 //make current
					int i=selection->ObjectIndex(obj);
					UseThisObject(selection->e(i));
					current_group=0;
				} 

				if (gindex>=0) {
					if (gindex<obj->groups.n) {
						if (current_group!=gindex) {
							current_group=gindex;
						} else {
							MenuItem *item=panel.findid(over);
							double th=dp->textheight();
							LineEdit *le= new LineEdit(viewport,"Rename",_("Rename group"),
														LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE|ANXWIN_OUT_CLICK_DESTROYS|ANXWIN_HOVER_FOCUS,
														item->x+panelbox.minx+3*th,panelbox.miny+item->y+((y-item->y)/th)*th,
														item->w,th,
														   4, //border
														   NULL,object_id,"renamegroup",
														   obj->groups.e[gindex]->name);
							eventgroup=gindex;
							eventobject=obj->object_id;
							le->padx=le->pady=dp->textheight()*.1;
							le->SetSelection(0,-1);
							app->addwindow(le);

							return 0;
						}
					}
				}
			}

			needtodraw=1;
			return 0;
		} //if not dragged and Group_List

		if (over>=EMODE_Mesh && over<EMODE_MAX) {
			ChangeMode(over);
			needtodraw=1;
			return 0;
		}


		//----- some options don't require an edata...
		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);

		if (over==ENGRAVE_Trace_Identifier) {
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
			if (!trace->identifier) {
				app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
			} else {
				if (trace->traceobject) trace->ClearCache(true);
				makestr(trace->identifier, NULL);
			}
		}

		if (!edata) return 0;

		 //------------group
		if (over==ENGRAVE_Group_Active) {
			//PerformAction(ENGRAVE_ToggleActive);
			if (group) group->active=!group->active;
			needtodraw=1;
			return 0;

		} else if (over==ENGRAVE_Group_Linked) {
			if (group) group->linked=!group->linked;
			needtodraw=1;
			return 0;

		} else if (over==ENGRAVE_Group_Color) {
            anXWindow *w=new ColorSliders(NULL,"New Color","New Color",
						   ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS,
						   0,0,200,400,0,
                           NULL,object_id,"newcolor",
                           LAX_COLOR_RGB,1./255,
                           group->color.red/65535.,
                           group->color.green/65535.,
                           group->color.blue/65535.,
                           group->color.alpha/65535.
						);
			app->addwindow(w);

			eventgroup=current_group;
			eventobject=edata->object_id;
			return 0;

		} else if (over==ENGRAVE_Group_Name) {
			MenuItem *item=panel.findid(over);
            LineEdit *le= new LineEdit(viewport,"Rename",_("Rename group"),
                                        LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE|ANXWIN_OUT_CLICK_DESTROYS|ANXWIN_HOVER_FOCUS,
                                        item->x+panelbox.minx,item->y+panelbox.miny,
										item->w,item->h,
										   4, //border
                                           NULL,object_id,"renamegroup",
                                           group->name);
            le->padx=le->pady=dp->textheight()*.1;
            le->SetSelection(0,-1);
            app->addwindow(le);

			eventgroup=current_group;
			eventobject=edata->object_id;
			return 0;

		} else if (over==ENGRAVE_Previous_Group) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group<=0) current_group=edata->groups.n-1;
			else current_group--;
			needtodraw=1;
			return 0;
			
		} else if (over==ENGRAVE_Next_Group) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group>=edata->groups.n-1) current_group=0;
			else current_group++;
			needtodraw=1;
			return 0; 
			
		} else if (over==ENGRAVE_Group_Down) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group==0) return 0;
			edata->groups.swap(current_group,current_group-1);
			current_group--;
			PostMessage(_("Moved down."));
			needtodraw=1;
			return 0; 

		} else if (over==ENGRAVE_Group_Up) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group==edata->groups.n-1) return 0;
			edata->groups.swap(current_group,current_group+1);
			current_group++;
			PostMessage(_("Moved up."));
			needtodraw=1;
			return 0; 

		} else if (over==ENGRAVE_New_Group) {
			 //create a duplicate of the current group, add after current
			if (!edata) return 0;
			EngraverPointGroup *newgroup=new EngraverPointGroup;
			newgroup->CopyFrom(group, false,false,false);
			makestr(newgroup->name,group->name);
			if (!newgroup->name) makestr(newgroup->name,"Group");

			edata->groups.push(newgroup,1,current_group+1);
			current_group++;
			if (current_group>=edata->groups.n) current_group=edata->groups.n-1;
			edata->MakeGroupNameUnique(current_group);
			UpdatePanelAreas();
			needtodraw=1;
			PostMessage(_("Group added."));
			return 0;
			
		} else if (over==ENGRAVE_Delete_Group) {
			if (current_group>=0 && edata->groups.n>1) {
				EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
				if (curvemapi.GetInfo()==&cur->trace->value_to_weight) curvemapi.SetInfo(NULL);

				edata->groups.remove(current_group);
				current_group--;
				if (current_group<0) current_group=0;
				UpdatePanelAreas();
				needtodraw=1;
				PostMessage(_("Group deleted."));
			}
			return 0;
			
		} else if (over==ENGRAVE_Merge_Group) { 
			// *** need popup menu to select other group
			cerr << " *** need to implement ENGRAVE_Merge_Group!"<<endl;
			PostMessage("*** unimplemented!! ***");
			return 0;


		 //------------tracing
		} else if (over==ENGRAVE_Trace_Same_As) {
			MenuInfo *menu=GetGroupMenu(ENGRAVE_Tracing, current_group);

	       if (menu) app->rundialog(new PopupMenu("Share Group","Share Group", 0,
                                     0,0,0,0,1,
                                     object_id,"sharetrace",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Trace_Continuous) {
			if (!group->trace) group=edata->GroupFromIndex(-1); //shouldn't happen, trace should always be nonnull..

			continuous_trace=!continuous_trace;
			group->trace->continuous_trace=continuous_trace;
			if (continuous_trace) Trace();

		} else if (over==ENGRAVE_Trace_Once) {
			Trace();

		} else if (over==ENGRAVE_Trace_Load) {
			needtodraw=1;
			app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));

		 //------------dashes
		} else if (over==ENGRAVE_Dash_Same_As) {
			//int sharing=IsSharing(ENGRAVE_Dashes, current_group);
			MenuInfo *menu=GetGroupMenu(ENGRAVE_Dashes, current_group);

	       if (menu) app->rundialog(new PopupMenu("Share Group","Share Group", 0,
                                     0,0,0,0,1,
                                     object_id,"sharedash",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Dash_Length) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%.10g",group->dashes->dash_length);
				LineEdit *le= new LineEdit(viewport,"Dash maximum length",_("Dash Length"),
											LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE|ANXWIN_OUT_CLICK_DESTROYS|ANXWIN_HOVER_FOCUS,
											item->x+panelbox.minx,item->y+panelbox.miny,
											item->w,item->h,
											   4, //border
											   NULL,object_id,"dashlength",
											   str);
				le->padx=le->pady=dp->textheight()*.1;
				le->SetSelection(0,-1);
				app->addwindow(le);
			}
			return 0;

		} else if (over==ENGRAVE_Dash_Seed) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%d",group->dashes->randomseed);
				LineEdit *le= new LineEdit(viewport,"Random Seed",_("Random Seed"),
											LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE|ANXWIN_OUT_CLICK_DESTROYS|ANXWIN_HOVER_FOCUS,
											item->x+panelbox.minx,item->y+panelbox.miny,
											item->w,item->h,
											   4, //border
											   NULL,object_id,"dashseed",
											   str);
				le->padx=le->pady=dp->textheight()*.1;
				le->SetSelection(0,-1);
				app->addwindow(le);
			}
			return 0;

		} else if (over==ENGRAVE_Spacing_Default) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%.10g",group->spacing);
				LineEdit *le= new LineEdit(viewport,"Default spacing",_("Default spacing"),
											LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE|ANXWIN_OUT_CLICK_DESTROYS|ANXWIN_HOVER_FOCUS,
											item->x+panelbox.minx,item->y+panelbox.miny,
											item->w,item->h,
											   4, //border
											   NULL,object_id,"defaultspacing",
											   str);
				le->padx=le->pady=dp->textheight()*.1;
				le->SetSelection(0,-1);
				app->addwindow(le);
			}
			return 0;

		} else {
			//PostMessage("*** unimplemented!! ***"); //some are just labels...

		}

		needtodraw=1;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		PatchInterface::LBUp(x,y,state,d);
		if (!edata && data) edata=dynamic_cast<EngraverFillData*>(data);
		if (edata && always_warp) edata->Sync(false);
		//if (continuous_trace) Trace(); ...done in move
		return 0;
	}

	if (mode==EMODE_Freehand) {
//		if (child) {
//			RemoveChild();
//			needtodraw=1;
//		}
		return 0;
	}

	if (mode==EMODE_Direction) {
		buttondown.up(d->id,LEFTBUTTON);
		return 0;
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  || mode==EMODE_AvoidToward
		  ) {
		buttondown.up(d->id,LEFTBUTTON);

		if ( mode==EMODE_Drag
		  || mode==EMODE_Turbulence
		  || mode==EMODE_PushPull
		  || mode==EMODE_AvoidToward
		  || mode==EMODE_Twirl)
			edata->ReverseSync(true);

		 //...done in move:
		//if (continuous_trace && dragged && (mode==EMODE_Thickness)) continuous_trace=false;
		//if (continuous_trace) Trace();

		needtodraw=1;
	}

	buttondown.up(d->id,LEFTBUTTON);

	return 0;
}

int EngraverFillInterface::NumGroupLines()
{
	EngraverFillData *obj; 
	int n=0;
	
	if (!selection) {
		if (edata) return edata->groups.n+1;
		return 1;
	}

	for (int g=0; g<selection->n(); g++) {
		obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;
		n++;
		n+=obj->groups.n;;
	}

	return n;
}

EngraverFillData *EngraverFillInterface::GroupFromLineIndex(int i, int *gi)
{
	if (!selection) {
		if (i==0) {
			*gi=-1;
		}
		return edata;
	}

	EngraverFillData *obj; 
	
	for (int g=0; g<selection->n(); g++) {
		obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;
		if (i==0) {
			*gi=-1;
			return obj;
		}

		i--;
		if (i<obj->groups.n) {
			*gi=i;
			return obj;
		}
		i-=obj->groups.n;
	}

	*gi=-1;
	return NULL;
}


Laxkit::MenuInfo *EngraverFillInterface::GetGroupMenu(int what, int current)
{
	if (!edata || edata->groups.n<=1) return NULL;

	MenuInfo *menu=new MenuInfo();
	menu->AddSep(_("Use from"));
	int shared;
	int numshared=1;
	EngraverFillData *obj;
	
	for (int g=0; g<selection->n(); g++) {
		obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;
		menu->AddSep(obj->Id());

		for (int c=0; c<edata->groups.n; c++) {
			if (c==current) continue;

			shared=0;
			if (what==ENGRAVE_Tracing     && edata->groups.e[c]->trace ==edata->groups.e[current]->trace ) shared=1;
			else if (what==ENGRAVE_Dashes && edata->groups.e[c]->dashes==edata->groups.e[current]->dashes) shared=1;
			//else if (what==ENGRAVE_Direction && edata->groups.e[c]->direction==edata->groups.e[current]->direction) sharing=1;
			//else if (what==ENGRAVE_Spacing   && edata->groups.e[c]->spacing  ==edata->groups.e[current]->spacing)   sharing=1;
			
			if (shared) numshared++;

			menu->AddItem(edata->groups.e[c]->name, c, LAX_ISTOGGLE|(shared ? LAX_CHECKED : 0),
							0, NULL);
		}
	}

	if (numshared>1) {
		menu->AddSep();
		menu->AddItem(_("New"),-2);
	}

	return menu;
}

const char *EngraverFillInterface::ModeTip(int mode)
{
	if (mode==EMODE_Mesh         ) return _("Mesh mode");
	if (mode==EMODE_Thickness    ) return _("Thickness, shift for brush size, control to thin");
	if (mode==EMODE_Blockout     ) return _("Blockout mode, shift for brush size, control to turn on");
	if (mode==EMODE_Drag         ) return _("Drag mode, shift for brush size");
	if (mode==EMODE_PushPull     ) return _("Push or pull. Shift for brush size");
	if (mode==EMODE_AvoidToward  ) return _("Avoid or pull toward. Shift for brush size");
	if (mode==EMODE_Twirl        ) return _("Twirl, Shift for brush size");
	if (mode==EMODE_Turbulence   ) return _("Turbulence, randomly push sample points");
	if (mode==EMODE_Resolution   ) return _("Resolution. Add or remove sample points");
	if (mode==EMODE_Orientation  ) return _("Orientation mode");
	if (mode==EMODE_Freehand     ) return _("Freehand mode");
	if (mode==EMODE_Trace        ) return _("Trace adjustment mode");
	if (mode==EMODE_Direction    ) return _("Direction adjustment mode");

	return "";
}

void EngraverFillInterface::ChangeMessage(int forwhich)
{
	if (forwhich==ENGRAVE_Trace_Once) PostMessage(_("Trace once"));
	else if (forwhich==ENGRAVE_Trace_Load) PostMessage(_("Load an image to trace"));
	else if (forwhich==ENGRAVE_Trace_Continuous) PostMessage(_("Toggle continuous tracing"));
	else if (forwhich==ENGRAVE_Trace_Opacity) PostMessage(_("Trace object opacity"));
	else if (forwhich==ENGRAVE_Trace_Identifier) {
		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
		if (trace->identifier) PostMessage(_("Click to remove trace object"));
		else PostMessage(_("Click to load an image to trace"));

	} else if (forwhich==ENGRAVE_Orient_Direction) PostMessage(_("Drag to change direction"));
	else if (forwhich==ENGRAVE_Orient_Position) PostMessage(_("Drag to change position"));
	else if (forwhich==ENGRAVE_Orient_Spacing) PostMessage(_("Drag to change spacing"));
	else if (forwhich>=EMODE_Mesh && forwhich<EMODE_MAX) {
		PostMessage(ModeTip(forwhich));

	} else {
		MenuItem *item=panel.findid(forwhich);
		if (item && !isblank(item->name)) {
			PostMessage(item->name);
			return;
		}
	}

	if (forwhich==ENGRAVE_None) PostMessage(" ");
}

int EngraverFillInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (child) {
		if (child==&curvemapi) {
			 //to be here, curvemapi must have taken the lbdown
			child->MouseMove(x,y,state,d);
			if (continuous_trace) Trace();

			needtodraw=1;
			return 0;
		}

		 //if some other child, assume we let it just operate
		 //except for mesh path manipulations
		if (!(mode==EMODE_Mesh && data && data->UsesPath())) return 1;
	}

	 //smooth out hoverdir hint for EMODE_AvoidToward
	for (int c=0; c<9; c++) hdir[c]=hdir[c+1];
	//hdir[3].x=x-hover.x;
	//hdir[3].y=y-hover.y;
	//hoverdir=hdir[0]+hdir[1]+hdir[2]+hdir[3];
	hdir[9].x=x;
	hdir[9].y=y;
	hoverdir=hdir[9]-hdir[0];
	hover.x=x;
	hover.y=y;

	if (!buttondown.any()) {
		 //update lasthover
		int category=0, index=-1, detail=-1;
		int newhover= scanEngraving(x,y, &category, &index, &detail);
		if (newhover!=lasthover || index!=lasthoverindex || detail!=lasthoverdetail) {
			lasthover=newhover;
			lasthovercategory=category;
			lasthoverindex=index;
			lasthoverdetail=detail;
			ChangeMessage(lasthover);
			needtodraw=1;
		}
		DBG cerr <<"eng lasthover: "<<lasthover<<"  index:"<<index<<"  detail"<<detail<<endl;
	}


	if (controlmode==EMODE_Controls) {
		if (buttondown.any()) {
			int over=0,lx,ly;
			int overcat=0;
			double th=dp->textheight();
			double pad=th/2;

			buttondown.getextrainfo(d->id,LEFTBUTTON, &over,&overcat);
			buttondown.move(d->id,x,y, &lx,&ly);

			EngraverPointGroup *group=(edata?edata->GroupFromIndex(current_group):NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

			if (over== ENGRAVE_Trace_Object) {
				if ((state&LAX_STATE_MASK)==ControlMask) {
					 //scale trace object
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					for (int c=0; c<4; c++) {
						trace->traceobject->object->m(c,trace->traceobject->object->m(c)*s);
					}

				} else {
					flatpoint p=dp->screentoreal(x,y) - dp->screentoreal(lx,ly);
					trace->traceobject->object->origin(trace->traceobject->object->origin()+p);
				}
				if (continuous_trace) Trace();
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Move_Mesh) {
				flatpoint p=screentoreal(x,y)-screentoreal(lx,ly);
				edata->origin(edata->origin()+p);

				if (continuous_trace) Trace();
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Opacity) {
				double pad=2;
				//x-=panelbox.minx;
				//y-=panelbox.miny;

				double z=(x-(panelbox.minx+pad))/(panelbox.maxx-panelbox.minx-2*pad);
				if (z<0) z=0; else if (z>1) z=1;
				trace->traceobj_opacity=z;
				needtodraw=1;

			} else if (over==ENGRAVE_Panel) {
				if ((state&LAX_STATE_MASK)==ControlMask) {
					 //scale up box...
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					panelbox.maxx=panelbox.minx+s*(panelbox.maxx-panelbox.minx);
					panelbox.maxy=panelbox.miny+s*(panelbox.maxy-panelbox.miny);
					UpdatePanelAreas();

				} else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
					 //scale horizontally
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					panelbox.maxx=panelbox.minx+s*(panelbox.maxx-panelbox.minx);
					UpdatePanelAreas();
					
				} else {
					 //move box
					panelbox.minx+=x-lx;
					panelbox.maxx+=x-lx;
					panelbox.miny+=y-ly;
					panelbox.maxy+=y-ly;
				}
				needtodraw=1;

			} else if (over==ENGRAVE_Dash_Zero_Threshhold || over==ENGRAVE_Dash_Broken_Threshhold
					|| over==ENGRAVE_Dash_Random || over==ENGRAVE_Dash_Taper || over==ENGRAVE_Dash_Density) {

				if (group) {
					double z=(x-(panelbox.minx+pad))/(panelbox.maxx-panelbox.minx-2*pad);
					if (z<0) z=0; else if (z>1) z=1;

					if (over==ENGRAVE_Dash_Broken_Threshhold)
						group->dashes->broken_threshhold=z*group->spacing;

					else if (over==ENGRAVE_Dash_Zero_Threshhold)
						group->dashes->zero_threshhold=z*group->spacing;

					else if (over==ENGRAVE_Dash_Random)
						group->dashes->dash_randomness=z;

					else if (over==ENGRAVE_Dash_Density)
						group->dashes->dash_density=z;

					else group->dashes->dash_taper=z;

					UpdateDashCaches(group->dashes);
					needtodraw=1;
				}
				return 0;

			} else if (over==ENGRAVE_Dash_Length) {
				int dx=x-lx;
				if (dx>0) {
					group->dashes->dash_length*=1.+.01*dx;
				} else {
					if (dx<-50) dx=-50;
					group->dashes->dash_length*=1/(1-.01*dx);
				}

				UpdateDashCaches(group->dashes);
				needtodraw=1;

				return 0;

			} else if (over==ENGRAVE_Dash_Seed) {
				int dx=(x-lx);
				if (labs(dx)>2) dx/=2;
				group->dashes->randomseed+=dx;
				if (group->dashes->randomseed<0) group->dashes->randomseed=0;

				UpdateDashCaches(group->dashes);
				needtodraw=1;

				return 0;

			} else if (over==ENGRAVE_Spacing_Default) {
				int dx=x-lx;
				if (dx>0) {
					group->spacing*=1.+.01*dx;
				} else {
					if (dx<-50) dx=-50;
					group->spacing*=1/(1-.01*dx);
				}

				edata->Sync(false);
				edata->UpdatePositionCache();
				group->UpdateDashCache();
				if (continuous_trace) Trace();

				needtodraw=1;
				return 0;

			}
		}
		return 0;
	}

	if (mode==EMODE_Freehand && !child) {
		needtodraw=1;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		PatchInterface::MouseMove(x,y,state,d);
		if (buttondown.any()) {
			if (always_warp && curpoints.n>0) {
				edata->Sync(false);
				edata->UpdatePositionCache();

				if (grow_lines) {
					EngraverPointGroup *group=edata->GroupFromIndex(current_group);
					growpoints.flush();
					group->GrowLines(edata,
									 group->spacing/3,     //resolution
									 group->spacing, NULL, //spacing map
									 .01, NULL,             //weight map
									 group->direction,group, //directionmap
									 &growpoints,
									 1000 //iteration limit
									);
				}
			}

			if (continuous_trace) Trace();
		}

		return 0;

	}

	if (mode==EMODE_Orientation) {
		if (!buttondown.any()) return 0;

		int lx,ly;
		int over;
		buttondown.getextrainfo(d->id,LEFTBUTTON, &over);
		buttondown.move(d->id,x,y, &lx,&ly);

		flatpoint  p=edata->transformPointInverse(screentoreal( x, y));
		flatpoint op=edata->transformPointInverse(screentoreal(lx,ly));
		flatpoint d=screentoreal( x, y)-screentoreal(lx,ly);
		//flatpoint md=p-op;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (over==ENGRAVE_Orient_Direction) {
			flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
			double angle=angle_full(op-center,p-center,0);
			group->direction=rotate(group->direction,angle,0);

		} else if (over==ENGRAVE_Orient_Spacing) {
			flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
			double r1=norm( p-center);
			double r2=norm(op-center);
			group->spacing*=r1/r2;

		} else if (over==ENGRAVE_Orient_Position) {
			flatpoint pp=edata->getPoint(group->position.x,group->position.y, false);
			pp+=d;
			int status;
			pp=edata->getPointReverse(pp.x,pp.y, &status);
			if (status==1) group->position=pp;
		}

		if (grow_lines) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			growpoints.flush();
			group->GrowLines(edata,
							 group->spacing/3,
							 group->spacing, NULL,
							 .01, NULL,
							 group->direction,group,
							 &growpoints,
							 1000 //iteration limit
							);
		} else group->Fill(edata, 1./dp->Getmag());

		edata->Sync(false);
		edata->UpdatePositionCache();
		group->UpdateDashCache();
		if (continuous_trace) Trace();

		needtodraw=1;
		return 0;
	}
	
	if (    mode==EMODE_Thickness
		 || mode==EMODE_Blockout
		 || mode==EMODE_Turbulence
		 || mode==EMODE_Drag
		 || mode==EMODE_PushPull
		 || mode==EMODE_AvoidToward
		 || mode==EMODE_Twirl
		 ) {

		if (!buttondown.any()) {
			needtodraw=1;
			return 0;
		}

		int lx,ly;
		buttondown.move(d->id,x,y, &lx,&ly);

		if ((state&LAX_STATE_MASK)==ShiftMask) {
			 //change brush size
			brush_radius+=(x-lx)*2;
			if (brush_radius<5) brush_radius=5;
			needtodraw=1;

		} else {
			flatvector m=screentoreal(x,y);
			m=transform_point_inverse(edata->m(),m); //center of brush
			flatvector m2=screentoreal(x+brush_radius,y);
			m2=transform_point_inverse(edata->m(),m2); //on edge of brush radius


			double rr=norm2(m2-m); //radius of brush in object coordinates
			double d, a;
			LinePoint *l;
			flatpoint dv=m-transform_point_inverse(edata->m(),screentoreal(lx,ly));
			flatpoint pp;
			double nearzero=.001; // *** for when tracing makes a value at 0, thicken makes it this

			EngraverPointGroup *group;
			int recachewhich; //1 for thickness change, 2 for blockout change, 4 for position change
			
			for (int g=0; g<edata->groups.n; g++) {
				group=edata->groups.e[g];
				if (!group->linked && g!=current_group) continue;

				recachewhich=0;

				for (int c=0; c<group->lines.n; c++) {
					l=group->lines.e[c];
					while (l) {
						d=norm2(l->p - m); //distance point to brush center

						if (d<rr) { //point is within...

							if (mode==EMODE_Thickness) {
								a=sqrt(d/rr);
								a=thickness.f(a);

								if ((state&LAX_STATE_MASK)==ControlMask) {
									 //thin
									a=1-a*.05;
								} else {
									 //thicken
									a=1+a*.05;
									if (l->weight<=0) l->weight=nearzero;
								}
								l->weight*=a;
								if (l->cache) l->cache->weight*=a;
								recachewhich|=1;

							} else if (mode==EMODE_Blockout) {
								if ((state&LAX_STATE_MASK)==ControlMask) 
									l->on=ENGRAVE_On;
								else l->on=ENGRAVE_Off;
								recachewhich|=2;

							} else if (mode==EMODE_Drag) {
								a=sqrt(d/rr);
								a=thickness.f(a);
								l->p+=dv*a; //point without mesh
								l->needtosync=2;
								recachewhich|=4;

							} else if (mode==EMODE_Turbulence) {
								a=sqrt(d/rr);
								a=thickness.f(a);
								l->p+=rotate(dv,drand48()*2*M_PI);
								l->needtosync=2;
								recachewhich|=4;

							} else if (mode==EMODE_PushPull) {
								a=sqrt(d/rr);
								a=thickness.f(a);
								pp=(l->p-m)*.03;

								if ((state&LAX_STATE_MASK)==ControlMask) {
									l->p-=pp*a*d/rr;
								} else {
									l->p+=pp*a;
								}
								l->needtosync=2;
								recachewhich|=4;

							} else if (mode==EMODE_AvoidToward) {
								a=sqrt(d/rr);
								a=thickness.f(a);

								flatvector vt=transpose(hoverdir);
								vt.normalize();
								vt*=.01*a*((l->p-m)*vt > 0 ? 1 : -1);

								if ((state&LAX_STATE_MASK)==ControlMask) {
									l->p-=vt;
								} else {
									l->p+=vt;
								}
								l->needtosync=2;
								recachewhich|=4;

							} else if (mode==EMODE_Twirl) {
								a=sqrt(d/rr);
								a=thickness.f(a);

								if ((state&LAX_STATE_MASK)==ControlMask) {
									l->p=m+rotate(l->p-m,a*.1);
								} else {
									l->p=m+rotate(l->p-m,-a*.1);
								}
								l->needtosync=2;
								recachewhich|=4;
							}
						}

						l=l->next;
					}//foreach point in line
				} //foreach line

				if (recachewhich&1) { //thickness change
					group->UpdateDashCache();
				} else if (recachewhich&2) { //blockout change
					group->UpdateDashCache();
				} else if (recachewhich&4) { //position change
					group->UpdatePositionCache();
				}
			} //foreach relevant group
			needtodraw=1;

			if (continuous_trace && (mode==EMODE_Thickness)) continuous_trace=false;
			if (continuous_trace) Trace();
		} //if distortion mode, not brush adjust mode

		return 0;
	}

	return 0;
}

int EngraverFillInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (show_panel && panelbox.boxcontains(x,y) && (state&ControlMask)!=0) {
		double w=panelbox.maxx-panelbox.minx;
		double h=panelbox.maxy-panelbox.miny;
		double rx=(x-panelbox.minx)/w;
		double ry=(y-panelbox.miny)/h;
		w*=1.1;
		h*=1.1;
		panelbox.minx=x-rx*w;
		panelbox.maxx=x+(1-rx)*w;
		panelbox.miny=y-ry*h;
		panelbox.maxy=y+(1-ry)*h;

		UpdatePanelAreas();
		needtodraw=1;
		return 0;
	}
	return 1;
}

int EngraverFillInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (show_panel && panelbox.boxcontains(x,y) && (state&ControlMask)!=0) {
		double w=panelbox.maxx-panelbox.minx;
		double h=panelbox.maxy-panelbox.miny;
		double rx=(x-panelbox.minx)/w;
		double ry=(y-panelbox.miny)/h;
		w*=1/1.1;
		h*=1/1.1;
		panelbox.minx=x-rx*w;
		panelbox.maxx=x+(1-rx)*w;
		panelbox.miny=y-ry*h;
		panelbox.maxy=y+(1-ry)*h;

		UpdatePanelAreas();
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Checks for EngraverFillData, then calls PatchInterface::DrawData(ndata,a1,a2,info).
int EngraverFillInterface::DrawData(Laxkit::anObject *ndata,anObject *a1,anObject *a2,int info) // info=0
{
	if (!ndata || dynamic_cast<EngraverFillData *>(ndata)==NULL) return 1;

	EngraverFillData *ee=edata;
	edata=dynamic_cast<EngraverFillData *>(ndata);

	int  tshow_points   =show_points;  
	bool tshow_direction=show_direction;
	bool tshow_panel    =show_panel;
	bool tshow_trace    =show_trace;

	show_points   =false;  
	show_direction=false;
	show_panel    =false;
	show_trace    =false;

	int c=PatchInterface::DrawData(ndata,a1,a2,info);
	edata=ee;

	show_points   =tshow_points;  
	show_direction=tshow_direction;
	show_panel    =tshow_panel;
	show_trace    =tshow_trace;

	return c;
}

int EngraverFillInterface::Refresh()
{
	if (!needtodraw) return 0;


	 //draw the trace object if necessary
	EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
	EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

	if (trace->traceobject && trace->traceobj_opacity>.5 // **** .5 since actual opacity not working
			&& (mode==EMODE_Trace || show_trace)) {
	//if (trace->traceobject && trace->traceobj_opacity>0 // **** .5 since actual opacity not working
	//		&& (mode==EMODE_Trace || show_trace)) {

		Affine a;
		if (edata) a=edata->GetTransformToContext(true, 0);//supposed to be inverse from edata to base real
		//dp->setSourceAlpha(trace->traceobj_opacity);
		dp->PushAndNewTransform(a.m());
		dp->PushAndNewTransform(trace->traceobject->object->m());
		viewport->DrawSomeData(trace->traceobject->object, NULL,NULL,0);
		//dp->setSourceAlpha(1);
		dp->PopAxes();
		dp->PopAxes();

	} else if (trace->traceobject) {
		 //draw outline
		dp->NewFG(.9,.9,.9);
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

		Affine a;
		if (edata) a=edata->GetTransformToContext(true, 0);
		SomeData *o=trace->traceobject->object;
		if (o) {
			dp->PushAndNewTransform(a.m());
			dp->moveto(transform_point(o->m(),flatpoint(o->minx,o->miny)));
			dp->lineto(transform_point(o->m(),flatpoint(o->maxx,o->miny)));
			dp->lineto(transform_point(o->m(),flatpoint(o->maxx,o->maxy)));
			dp->lineto(transform_point(o->m(),flatpoint(o->minx,o->maxy)));
			dp->closed();
			dp->stroke(0);
			dp->PopAxes();
		}
	}

	if (show_direction) {
		DirectionMap *map=directionmap;
		if (!map && edata) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			map=group;
		}

		if (map) {
			dp->DrawScreen();
			dp->NewFG(.6,.6,1.);
			dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

			//double s=1;
			int step=20;
			int win_w=dp->Maxx-dp->Minx;
			int win_h=dp->Maxy-dp->Miny;
			int ww=win_w - 2*step;
			int hh=win_h - 2*step;


			flatpoint v;
			double vv;
			flatpoint p,p2;

			for (int x=win_w/2-ww/2; x<win_w/2+ww/2; x+=step) {
				for (int y=win_h/2-hh/2; y<win_h/2+hh/2; y+=step) {
					p.x=x-(win_w/2-ww/2);
					p.y=y-(win_h/2-hh/2);
					p=dp->screentoreal(p.x,p.y);

					//DBG cerr <<int(xx)<<','<<int(yy)<<"  ";
					v=map->Direction(p.x,p.y);
					p2=dp->realtoscreen(p+v);
					v=p2-flatpoint(x,y);
					vv=norm(v);
					if (vv>step*.8) v*=step*.8/vv;
					if (vv<step*.5) v*=step*.5/vv;


					dp->drawarrow(flatpoint(x,y), v, 0, 1, 2, 3);
				}
			}
			dp->DrawReal();
		}
	}

	if (mode==EMODE_Freehand && !child) {
		 //draw squiggly lines near mouse
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->NewFG(0.,0.,1.);
		dp->DrawScreen();
		double s=10;
		for (int c=-1; c<2; c++) {
			dp->moveto(hover-flatpoint(2*s,c*s));
			dp->curveto(hover-flatpoint(s*1.5,c*s+5), hover+flatpoint(-s/2,-c*s+s/2), hover+flatpoint(0,-c*s));
			dp->stroke(0);
		}
		dp->DrawReal();
		needtodraw=0;
	}

	if (!edata) {
		if (show_panel) DrawPanel();
		needtodraw=0;
		return 0;
	}



	 //----draw the actual lines
	LinePoint *l;
	//LinePoint *last=NULL;
	LinePointCache *lc, *lcstart, *clast=NULL;

	double mag=dp->Getmag();
	double lastwidth, neww;
	double tw;
	flatpoint lp,v;


	for (int g=0; g<edata->groups.n; g++) {
		group=edata->groups.e[g];
		if (!group->active) continue;
		if (!group->lines.n) {
			DBG cerr <<" *** WARNING! engraver group #"<<g<<"is missing lines!"<<endl;
			continue;
		}
		
		if (!group->lines.e[0]->cache) {
			group->UpdateBezCache();
			group->UpdateDashCache();
		} 

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];
			lc=lcstart=l->cache;
			clast=NULL;
			lastwidth=-1;

			dp->NewFG(&group->color);

			do { //one loop per on segment
				if (!group->PointOnDash(lc)) { lc=lc->next; continue; } //advance to first on point

				if (!clast) {
					 //establish a first point of a visible segment
					clast=lc;
					lastwidth=lc->weight*mag;
					dp->LineAttributes(lastwidth,LineSolid,LAXCAP_Round,LAXJOIN_Round);

					if (lc->on==ENGRAVE_EndPoint || !lc->next || !group->PointOnDash(lc->next)) {
						 //draw just a single dot
						dp->drawline(clast->p,clast->p);
						lc=lc->next;
						clast=NULL;
						continue;
					}
					lc=lc->next;
				}

				neww=lc->weight*mag;
				if (neww!=lastwidth) {
					lp=clast->p;
					v=(lc->p-clast->p)/9.;
					for (int c2=1; c2<10; c2++) {
						 //draw 10 mini segments, each of same width to approximate the changing width
						tw=lastwidth+c2/9.*(neww-lastwidth);
						dp->LineAttributes(tw,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						dp->drawline(lp+v*(c2-1), lp+v*c2);
					}

					lastwidth=neww;

				} else {
					dp->drawline(clast->p,lc->p);
				}

				if (lc->on==ENGRAVE_EndPoint) clast=NULL;
				else clast=lc;
				lc=lc->next;
				if (lc && !group->PointOnDash(lc)) clast=NULL;

			} while (lc && lc->next && lc!=lcstart);
//			-------------------LinePoint:
//			while (l) { //one loop per on segment
//				if (!group->PointOn(l)) { l=l->next; continue; } //advance to first on point
//
//				if (!last) {
//					 //establish a first point of a visible segment
//					last=l;
//					lastwidth=l->weight*mag;
//					dp->LineAttributes(lastwidth,LineSolid,LAXCAP_Round,LAXJOIN_Round);
//
//					if (l->on==ENGRAVE_EndPoint || !l->next || !group->PointOn(l->next)) {
//						 //draw just a single dot
//						dp->drawline(last->p,last->p);
//						l=l->next;
//						last=NULL;
//						continue;
//					}
//					l=l->next;
//				}
//
//				neww=l->weight*mag;
//				if (neww!=lastwidth) {
//					lp=last->p;
//					v=(l->p-last->p)/9.;
//					for (int c2=1; c2<10; c2++) {
//						 //draw 10 mini segments, each of same width to approximate the changing width
//						tw=lastwidth+c2/9.*(neww-lastwidth);
//						dp->LineAttributes(tw,LineSolid,LAXCAP_Round,LAXJOIN_Round);
//						dp->drawline(lp+v*(c2-1), lp+v*c2);
//					}
//
//					lastwidth=neww;
//
//				} else {
//					dp->drawline(last->p,l->p);
//				}
//
//				if (l->on==ENGRAVE_EndPoint) last=NULL;
//				else last=l;
//				l=l->next;
//				if (l && !group->PointOn(l)) last=NULL;
//			}
//			-------------------

			if (show_points) {
				 //show little red dots for all the sample points
				flatpoint pp;
				l=group->lines.e[c];
				dp->NewFG(1.,0.,0.);
				int p=1;
				char buffer[50];
				while (l) {
					dp->drawpoint(l->p, 2, 1);
					if (show_points&2) {
						sprintf(buffer,"%d,%d",c,p);
						dp->textout(l->p.x,l->p.y, buffer,-1, LAX_BOTTOM|LAX_HCENTER);
						p++;
					}
					l=l->next;
				}
			}
		} //foreach line
	} //foreach group


	 //draw other tool decorations
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	 //draw outline of mesh
	if (data->npoints_boundary) {
		dp->NewFG(150,150,150);
		dp->LineAttributes(1,LineSolid,linestyle.capstyle,linestyle.joinstyle);
		dp->drawbez(data->boundary_outline,data->npoints_boundary/3,1,0);
	}

	if (mode==EMODE_Freehand) {
		 //awaiting a click down
		if (!child && show_panel) DrawPanel();
		needtodraw=0;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		if (showdecs) {
			dp->DrawScreen();
			if (always_warp) {
				dp->NewFG(0.,.78,0.);
				dp->textout(0,0, "Warp",-1, LAX_TOP|LAX_LEFT);
			} else {
				dp->NewFG(.9,0.,0.);
				dp->textout(0,0, "Don't Warp",-1, LAX_TOP|LAX_LEFT);
			}
			dp->DrawReal();
		}

		PatchInterface::Refresh();
		if (dynamic_cast<PathInterface*>(child)) {
			 //due to defered refreshing, so we can draw path between mesh and panel
			PathInterface *pathi=dynamic_cast<PathInterface*>(child);
			pathi->needtodraw=1;
			pathi->Setting(PATHI_Defer_Render, false);
			pathi->Refresh();
			pathi->Setting(PATHI_Defer_Render, true);
		}


	} else if (mode==EMODE_Orientation) {
		 //draw a burin
		DrawOrientation(lasthover);

		if (grow_lines && growpoints.n) {
			dp->DrawScreen();

			flatpoint p;
			for (int c=0; c<growpoints.n; c++) {
				p=growpoints.e[c]->p;
				p=edata->getPoint(p.x,p.y, false);
				p=dp->realtoscreen(p);

				if (growpoints.e[c]->godir&1) dp->NewFG(255,100,100); //should use activate/deactivate colors
				else dp->NewFG(0,200,0);
				dp->drawthing(p.x+5,p.y, 5,5, 1, THING_Triangle_Right);

				if (growpoints.e[c]->godir&2) dp->NewFG(255,100,100); //should use activate/deactivate colors
				else dp->NewFG(0,200,0);
				dp->drawthing(p.x-5,p.y, 5,5, 1, THING_Triangle_Left);
			}

			dp->DrawReal();
		}

	} else if ((mode==EMODE_Thickness
			|| mode==EMODE_Blockout
			|| mode==EMODE_Drag
			|| mode==EMODE_Turbulence
			|| mode==EMODE_PushPull
		    || mode==EMODE_AvoidToward
		    || mode==EMODE_Twirl)
			&& lasthovercategory!=ENGRAVE_Panel
			) {


		dp->DrawScreen();

		 //set colors
		dp->NewFG(.5,.5,.5,1.);
		if (mode==EMODE_Thickness) {
			dp->LineAttributes(2,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			dp->NewFG(.5,.5,.5,1.);

		} else if (mode==EMODE_Turbulence) dp->NewFG(.5,.5,.5,1.);

		else if (   mode==EMODE_Drag
				 || mode==EMODE_PushPull
				 || mode==EMODE_Twirl) {
			if (submode==2) dp->NewFG(.5,.5,.5); //brush size change
			else dp->NewFG(0.,0.,.7,1.);

		} else if (mode==EMODE_Blockout) { //blockout
			if (submode==1) dp->NewFG(0,200,0);
			else if (submode==2) dp->NewFG(.5,.5,.5);
			else dp->NewFG(255,100,100);
		}

		 //draw main circle
		if (mode==EMODE_Turbulence) {
			 //draw jagged circle
			double xx,yy, r;
			for (int c=0; c<30; c++) {
				r=1 + .2*drand48()-.1;
				xx=hover.x + brush_radius*r*cos(c*2*M_PI/30);
				yy=hover.y + brush_radius*r*sin(c*2*M_PI/30);
				dp->lineto(xx,yy);
			}
			dp->closed();
			dp->stroke(0);

		} else if (mode==EMODE_Twirl) {
			int s=1;
			if (submode==1) s=-1;
			for (int c=0; c<10; c++) {
				dp->moveto(hover.x+brush_radius*cos(s*c/10.*2*M_PI), hover.y+brush_radius*sin(s*c/10.*2*M_PI));
				dp->curveto(flatpoint(hover.x+brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+brush_radius*sin(s*(c+1)/10.*2*M_PI)),
							flatpoint(hover.x+.85*brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+.85*brush_radius*sin(s*(c+1)/10.*2*M_PI)),
							flatpoint(hover.x+.85*brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+.85*brush_radius*sin(s*(c+1)/10.*2*M_PI)));
				dp->stroke(0);
			}

		} else if (mode==EMODE_PushPull) {
			dp->drawpoint(hover.x,hover.y, brush_radius,0);

			dp->LineAttributes(1,LineOnOffDash, LAXCAP_Butt, LAXJOIN_Miter);
			if (submode==1) dp->drawpoint(hover.x,hover.y, brush_radius*.85,0);
			else dp->drawpoint(hover.x,hover.y, brush_radius*1.10,0);
			dp->LineAttributes(1,LineSolid, LAXCAP_Butt, LAXJOIN_Miter);

		} else {
			 //draw plain old circle
			dp->drawpoint(hover.x,hover.y, brush_radius,0);
		}

		 //draw circle decorations
		if (mode==EMODE_Drag) {
			dp->drawarrow(hover,flatpoint(brush_radius/4,0), 0,1,2,3);
			dp->drawarrow(hover,flatpoint(-brush_radius/4,0), 0,1,2,3);
			dp->drawarrow(hover,flatpoint(0,brush_radius/4), 0,1,2,3);
			dp->drawarrow(hover,flatpoint(0,-brush_radius/4), 0,1,2,3);

		} else if (mode==EMODE_Blockout) {
			dp->drawpoint(hover.x,hover.y, brush_radius*.85,0); //second inner circle

		} else if (mode==EMODE_AvoidToward) {
			flatpoint vt(-hoverdir.y,hoverdir.x);
			vt.normalize();
			vt*=brush_radius/2;

			if (submode==1) {
				dp->drawarrow(hover+3*vt,-vt, 0,1,2,3);
				dp->drawarrow(hover-3*vt, vt, 0,1,2,3);
			} else {
				dp->drawarrow(hover+vt,vt, 0,1,2,3);
				dp->drawarrow(hover-vt,-vt, 0,1,2,3);
			}
		}

		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		if (submode==2) { //brush size change arrows
			dp->drawarrow(hover+flatpoint(brush_radius+10,0), flatpoint(20,0), 0, 20, 1, 3);
			dp->drawarrow(hover-flatpoint(brush_radius+10,0), flatpoint(-20,0), 0, 20, 1, 3);
		}

		dp->DrawReal();
	}

	if (show_panel) DrawPanel();

	needtodraw=0;
	return 0;
}

/*! Called from Refresh, draws the orientation handle.
 */
void EngraverFillInterface::DrawOrientation(int over)
{
	 //note: need to coordinate with scanPanel()

	EngraverPointGroup *group=edata->GroupFromIndex(current_group);

	 //draw a burin
	flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
	center=dp->realtoscreen(center);

	int size=30;
	double thick=.25;

	flatpoint dir=group->direction/norm(group->direction)*.05;
	flatpoint xx=dp->realtoscreen(edata->getPoint(.5+dir.x,.5+dir.y, false)) - dp->realtoscreen(edata->getPoint(.5,.5, false));
	xx=xx/norm(xx)*size;
	flatpoint yy=-transpose(xx);

	dp->DrawScreen();

	 //draw shaft of burin
	dp->moveto(center + yy);
	dp->lineto(center + yy/2);
	dp->curveto(center + .225*yy, center + .225*xx, center+xx/2);
	dp->lineto(center +2*xx);
	dp->lineto(center + (2-thick)*xx + thick*yy);
	dp->lineto(center + 2*thick*xx   + thick*yy);
	dp->curveto(center + (1.5*thick)*xx + thick*yy, center + thick*xx+1.5*thick*yy, center + thick*xx + 2*thick*yy);
	dp->lineto(center + thick*xx + yy);
	dp->closed();

	dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	if (over==ENGRAVE_Orient_Position) dp->NewFG(.8,.8,.8); else dp->NewFG(1.,1.,1.);
	dp->fill(1);
	dp->NewFG(0.,0.,.6);
	dp->stroke(0);

	 //draw knob of burin
	if (over==ENGRAVE_Orient_Spacing)  dp->NewBG(.8,.8,.8); else dp->NewBG(1.,1.,1.);
	dp->drawpoint(center+thick/2*xx+yy, norm(xx)/3, 2);
	
	if (over==ENGRAVE_Orient_Direction) {
		 //draw rotate indicator
		dp->NewFG(0.,0.,.6);
		dp->moveto(center + 2*xx + thick*yy);
		dp->curveto(center + (2+thick/2)*xx + thick/2*yy,
					center + (2+thick/2)*xx - thick/2*yy,
					center + 2*xx - thick*yy);

		dp->moveto(center + 2*xx + .9*thick*yy);
		dp->lineto(center + 2*xx + thick*yy);
		dp->lineto(center + 2.1*xx + thick*yy);

		dp->moveto(center + 2*xx - .9*thick*yy);
		dp->lineto(center + 2*xx - thick*yy);
		dp->lineto(center + 2.1*xx - thick*yy);

		dp->stroke(0);
	}

	dp->DrawReal();
}

void EngraverFillInterface::DrawNumInput(double pos,int type,int hovered, double x,double y,double w,double h, const char *text)
{
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
 	
	if (hovered)  dp->NewFG(0.,0.,0.); 
	else dp->NewFG(.5,.5,.5);

	double th=dp->textheight();

	if (text) {
		//if (pos<.5) dp->textout(x+w,y+h/2, text,-1, LAX_RIGHT|LAX_VCENTER);
		//else dp->textout(x,y+h/2, text,-1, LAX_LEFT|LAX_VCENTER);
		
		dp->textout(x+w-th-th/2,y+h/2, text,-1, LAX_RIGHT|LAX_VCENTER);
	}

	char str[20];
	if (type==0) sprintf(str,"%.2g", pos);
	else sprintf(str,"%d", (int)pos);
	dp->textout(x+th+th/2,y+h/2, str,-1, LAX_LEFT|LAX_VCENTER);

    dp->drawthing(x+th*.9,y+h/2, th*.3,th*.3, 0, THING_Triangle_Left);
    dp->drawthing(x+w-th*.9,y+h/2, th*.3,th*.3, 0, THING_Triangle_Right);
}

void EngraverFillInterface::DrawSlider(double pos,int hovered, double x,double y,double w,double h, const char *text)
{
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
 	
	dp->NewFG(.5,.5,.5);

	double ww=(text ? dp->textextent(text,-1, NULL,NULL) : 0);
	if (text) {
		//if (pos<.5) dp->textout(x+w,y+h/2, text,-1, LAX_RIGHT|LAX_VCENTER);
		//else dp->textout(x,y+h/2, text,-1, LAX_LEFT|LAX_VCENTER);
		
		dp->textout(x+w/2,y+h/2, text,-1, LAX_CENTER);
	}
	if (hovered)  dp->NewFG(0.,0.,0.); 

	if (ww>0) {
		dp->drawline(x, y+h/2, x+w/2-ww/2, y+h/2);
		dp->drawline(x+w/2+ww/2, y+h/2, x+w, y+h/2);
	} else dp->drawline(x, y+h/2, x+w, y+h/2);
	dp->drawpoint(flatpoint(x + h/4 + pos*(w-h/2),y+h/2), h*.4, 1);
}

/*! Draw the range of line widths in a strip, as for a curve map control.
 * Assumes already in dp->DrawScreen mode.
 */
void EngraverFillInterface::DrawLineGradient(double minx,double maxx,double miny,double maxy, int groupnum, int horizontal)
{
	double num=15;
	double sp=(maxy - miny)/num; //one unit
	if (horizontal) sp=(maxx-minx)/num;

	EngraverPointGroup *group=edata ? edata->GroupFromIndex(groupnum) : NULL;

	dp->NewFG(&fgcolor);

	double pos, rr,t, w, a, p, gapw;
	double zero   =(group ? group->dashes->zero_threshhold  /group->spacing : 0);
	double broken =(group ? group->dashes->broken_threshhold/group->spacing : 0);
	double density=(group ? group->dashes->dash_density : 0);
	double sw;

	for (int c=0; c<num; c++) {
		pos=c/num;
		if (pos<zero) continue;


		if (group && pos*group->spacing<group->dashes->broken_threshhold) {
			 //draw dashed
			rr    =group->dashes->dash_randomness;
			t     =group->dashes->dash_taper;

			a=(pos-zero)/(broken-zero);
			w=broken*a + (t*(broken-zero)+zero)*(1-a);

			a=density + (1-density)*a;
			gapw=1-a;
			p=.5 + (rr*random()/RAND_MAX - rr/2);

			sw=sp*w;
			if (horizontal) {
				gapw*=(maxy-miny);
				p*=maxy-miny-gapw;
				dp->drawrectangle(minx + (maxx-minx)*pos, miny,         sw>=1?sw:1, p, 1);
				dp->drawrectangle(minx + (maxx-minx)*pos, miny+p+gapw,  sw>=1?sw:1, (maxy-miny)-(p+gapw), 1);

			} else {
				gapw*=(maxx-minx);
				p*=maxx-minx-gapw;
				//dp->drawrectangle(minx,        miny + (maxy-miny)*pos,  maxx-minx,                    1+sp*w, 1);
				dp->drawrectangle(minx,        maxy - (maxy-miny)*pos,  p,                    sw>=1?sw:1, 1);
				dp->drawrectangle(minx+p+gapw, maxy - (maxy-miny)*pos,  (maxx-minx)-(p+gapw), sw>=1?sw:1, 1);
			}

		} else {
			 //draw solid
			if (horizontal) {
				dp->drawrectangle(minx + (maxx-minx)*pos, miny, 1+sp*pos,maxy-miny, 1);
			} else {
				dp->drawrectangle(minx, maxy - (maxy-miny)*pos, maxx-minx,sp*pos, 1);
			}
		}
	}

	//dp->drawrectangle(minx-1,miny-1, maxx-minx+2,maxy-miny+2,0);
}

/*! Draw a continuous tone gradient in a strip, as for a curve map control.
 * Assumes already in dp->DrawScreen mode.
 */
void EngraverFillInterface::DrawShadeGradient(double minx,double maxx,double miny,double maxy)
{
	ScreenColor col;

	dp->LineAttributes(2, LineSolid, CapButt, JoinMiter);
	for (int c=minx; c<maxx; c+=2) {
		dp->NewFG(coloravg(rgbcolor(0,0,0),rgbcolor(255,255,255), 1-(c-minx)/(maxx-minx)));
		dp->drawline(c,miny, c,maxy);
	}

	//dp->drawrectangle(minx-1,miny-1, maxx-minx+2,maxy-miny+2,0);
}

/*! Define the panel, and update positions within it.
 */
void EngraverFillInterface::UpdatePanelAreas()
{
	DBG cerr <<" EngraverFillInterface::UpdatePanelAreas()..."<<endl;

	if (!panel.n()) {
		 //need to define all the panel areas
		panel.AddItem("Mode Selection",  ENGRAVE_Mode_Selection);

		//--------------- Group selection ---------------
		 // < >  _Group_Name__  [del] [new]
		 // [new] [delete]  Active: 0  [color]
		panel.AddItem("Group Selection",  ENGRAVE_Groups);
		panel.SubMenu();
		  panel.AddItem("Toggle list" ,    ENGRAVE_Toggle_Group_List);
		  panel.AddItem("Previous Group",  ENGRAVE_Previous_Group);
		  panel.AddItem("Next Group",      ENGRAVE_Next_Group);
		  panel.AddItem("Group List",      ENGRAVE_Group_List);
		  panel.AddItem("Group Name",      ENGRAVE_Group_Name);
		  panel.AddItem("Active",          ENGRAVE_Group_Active);
		  panel.AddItem("Linked",          ENGRAVE_Group_Linked);
		  panel.AddItem("Color",           ENGRAVE_Group_Color);
		  panel.AddItem("Delete Group",    ENGRAVE_Delete_Group);
		  panel.AddItem("New Group",       ENGRAVE_New_Group);
		  panel.AddItem("Merge Group",     ENGRAVE_Merge_Group);
		  panel.AddItem("Move Group Up",   ENGRAVE_Group_Up);
		  panel.AddItem("Move Group Down", ENGRAVE_Group_Down);
		panel.EndSubMenu();
		panel.e(panel.n()-1)->state|=LAX_OPEN; //this always needs to be open
		MenuItem *grouplist=panel.findid(ENGRAVE_Group_List);
		grouplist->Open();

		//--------------- tracing  ---------------
		panel.AddItem("Tracing",    ENGRAVE_Tracing);
		panel.SubMenu();
		  panel.AddItem("Trace Same as",    ENGRAVE_Trace_Same_As);
		  panel.AddItem("Trace Curve",      ENGRAVE_Trace_Curve);
		  panel.AddItem("Thicken",          ENGRAVE_Trace_Thicken);
		  panel.AddItem("Thin",             ENGRAVE_Trace_Thin);
		  panel.AddItem("Set",              ENGRAVE_Trace_Set);
		  panel.AddItem("Using type",       ENGRAVE_Trace_Using_type);
		  panel.AddItem("Using",            ENGRAVE_Trace_Using);
		  panel.AddItem("Apply",            ENGRAVE_Trace_Apply); //<- same as Once?
		  panel.AddItem("Remove",           ENGRAVE_Trace_Remove);
		  panel.AddItem("Opacity",          ENGRAVE_Trace_Opacity);
		  panel.AddItem("Trace Once",       ENGRAVE_Trace_Once);
		  panel.AddItem("Trace Continuous", ENGRAVE_Trace_Continuous);
		panel.EndSubMenu();
		panel.e(panel.n()-1)->state|=LAX_OPEN;

		//--------------- Dashes  ---------------
		panel.AddItem("Dashes",     ENGRAVE_Dashes);
		panel.SubMenu();
		  panel.AddItem("Dash same as",    ENGRAVE_Dash_Same_As);
		  //panel.AddItem("Threshhold",    ENGRAVE_Dash_Threshhold);
		  panel.AddItem("Zero Threshhold", ENGRAVE_Dash_Zero_Threshhold);
		  panel.AddItem("Dash Threshhold", ENGRAVE_Dash_Broken_Threshhold);
		  panel.AddItem("Random",          ENGRAVE_Dash_Random);
		  panel.AddItem("Taper",           ENGRAVE_Dash_Taper);
		  panel.AddItem("Minimum density", ENGRAVE_Dash_Density);
		  panel.AddItem("Dash Length",     ENGRAVE_Dash_Length);
		  panel.AddItem("Random Seed",     ENGRAVE_Dash_Seed);
		  panel.AddItem("Caps",            ENGRAVE_Dash_Caps);
		  panel.AddItem("Join",            ENGRAVE_Dash_Join);
		  panel.EndSubMenu();

		  //--------------- Direction  ---------------
		  panel.AddItem("Direction",  ENGRAVE_Direction);
		  panel.SubMenu();
		  panel.AddItem("Dir same as",  ENGRAVE_Dir_Same_As);
		  panel.AddItem("Type",         ENGRAVE_Dir_Type);
		  panel.AddItem("Current",      ENGRAVE_Dir_Current);
		  panel.AddItem("Paint...",     ENGRAVE_Dir_Paint);
		  panel.AddItem("Create map from current",      ENGRAVE_Dir_Create_From_Cur);
		  panel.AddItem("Generate from trace object",   ENGRAVE_Dir_From_Trace);
		  panel.AddItem("Load normal map",              ENGRAVE_Dir_Load_Normal);
		  panel.AddItem("Load and generate from image", ENGRAVE_Dir_Load_Image);
		  panel.EndSubMenu();

		  //--------------- Spacing  ---------------
		  panel.AddItem("Spacing",    ENGRAVE_Spacing);
		  panel.SubMenu();
		  panel.AddItem("Default spacing",      ENGRAVE_Spacing_Default);
		  panel.AddItem("Use spacing map",      ENGRAVE_Spacing_Use_Map);
		  panel.AddItem("Use spacing map",      ENGRAVE_Spacing_Map_File);
		  panel.AddItem("Spacing same as",      ENGRAVE_Spacing_Same_As);
		  panel.AddItem("Preview",              ENGRAVE_Spacing_Preview);
		  panel.AddItem("Create from current",  ENGRAVE_Spacing_Create_From_Cur);
		  panel.AddItem("Load..",               ENGRAVE_Spacing_Load);
		  panel.AddItem("Save..",               ENGRAVE_Spacing_Save);
		  panel.AddItem("Paint..",              ENGRAVE_Spacing_Paint);
		  panel.EndSubMenu();

		  //--------------- Selection?  ---------------
		  // //selection management, 1 line for each selection, click x to remove?
		  //panel.AddItem("Spacing",    ENGRAVE_Spacing);
	}

	double th=dp->textheight();
	double pad=th/2;

	//now update positions
	if (!panelbox.validbounds()) {
		panelbox.minx=panelbox.miny=10;
		panelbox.maxx=8*1.5*th;
		panelbox.maxy=10*1.5*th+th+4./3*th+th+pad;
	}

	MenuItem *item, *item2;
	int y=pad;
	int pw=panelbox.boxwidth();
	//int ph=panelbox.boxheight();
	//int px=panelbox.minx;

	for (int c=0; c<panel.n(); c++) {
		item=panel.e(c);
		item->y=y;

		if (item->id==ENGRAVE_Mode_Selection) {
			item->x=pad;  item->y=y;  item->w=pw-2*pad;
			if (modes.e(0)->image) item->h=modes.e(0)->image->h(); else item->h=th;
			y+=item->h+pad;

		} else if (item->id==ENGRAVE_Groups) {
			item->x=pad; item->y=y; item->w=pw-2*pad;  item->h=th;

			item2=panel.findid(ENGRAVE_Group_List);
			int lh=0;
			bool showlist=false;
			if (item2->isOpen()) {
				lh=th*NumGroupLines();
				if (lh==0) lh=th;
				showlist=true;
			} else lh=th;
			item->h+=lh;

			int nww=5;

			//< > Groupname  active linked color
			//=   +  -  ^  v  ->
			//-------------
			//--visible - objectid----
			//active color linked   group 1
			//active color linked   group 2
			//active color linked   group 3
			//+  -  ^  v  dup

			for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
				item2=item->GetSubmenu()->e(c2);

				//----first line
				if (!showlist) {
					if (item2->id==ENGRAVE_Previous_Group) {
						item2->x=pad;  item2->y=y;  item2->w=th; item2->h=th;

					} else if (item2->id==ENGRAVE_Next_Group) {
						item2->x=pad+th;  item2->y=y;  item2->w=th; item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Name) {
						item2->x=pad+2*th;  item2->y=y;  item2->w=pw-2*pad-5*th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Active) {
						item2->x=pw-pad-3*th;  item2->y=y;  item2->w=th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Linked) {
						item2->x=pw-pad-2*th;  item2->y=y;  item2->w=th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Color) {
						item2->x=pw-pad-th;  item2->y=y;  item2->w=th;  item2->h=th;
					}

				} else {
					if (       item2->id==ENGRAVE_Previous_Group
							|| item2->id==ENGRAVE_Next_Group
							|| item2->id==ENGRAVE_Group_Name
							|| item2->id==ENGRAVE_Group_Active
							|| item2->id==ENGRAVE_Group_Linked
							|| item2->id==ENGRAVE_Group_Color) 
						item2->w=item2->h=0;
				}


				//----group list, if any
				if (item2->id==ENGRAVE_Group_List) {
					if (showlist) {
						item2->x=pad; item2->y=y; item2->w=pw-2*pad; item2->h=lh;
					} else {
						item2->x=pad; item2->y=y+th; item2->w=-1; item2->h=-1;
					}


					//----second line
				} else if (item2->id==ENGRAVE_Toggle_Group_List) {
					item2->x=pad;     item2->y=y+lh;  item2->w=th; item2->h=th;

				} else if (item2->id==ENGRAVE_New_Group) {
					item2->x=th+pad;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				} else if (item2->id==ENGRAVE_Delete_Group) {
					item2->x=th+pad+item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				} else if (item2->id==ENGRAVE_Group_Down) {
					item2->x=th+pad+2*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				} else if (item2->id==ENGRAVE_Group_Up) {
					item2->x=th+pad+3*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				//} else if (item2->id==ENGRAVE_Duplicate_Group) {
				//	item2->x=th+pad+4*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww, item2->h=th;

				//} else if (item2->id==ENGRAVE_Merge_Group) {
				//	item2->x=th+pad+5*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww, item2->h=th;

				}
			}
			y+=item->h; 

		} else if (item->id==ENGRAVE_Tracing) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!item->isOpen()) {
				item->h=th;
				DBG cerr <<" ---tracing section is CLOSED"<<endl;

			} else {
				DBG cerr <<" ---tracing section is OPEN"<<endl;

				int hasgroups = (edata && edata->groups.n>1 ? 1 : 0);
				item->h=item->w + 5*th + (hasgroups?th:0); //always square?

				// ...

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					//----first line
					if (item2->id==ENGRAVE_Trace_Same_As) {
						if (hasgroups) {
							item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
						} else {
							item2->x=pad;  item2->y=y+1*th;  item2->w=0;  item2->h=0; 
						}

					} else if (item2->id==ENGRAVE_Trace_Curve) {
						item2->x=pad;  item2->y=y+(hasgroups?2:1)*th;  item2->w=pw-2*pad;  item2->h=item->w+4*th; 
						tracebox=item2;
					}
				}
			}

			y+=item->h;

		} else if (item->id==ENGRAVE_Dashes) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!(item->state&LAX_OPEN)) item->h=th;
			else {
				int hasgroups = (edata && edata->groups.n>1 ? 1 : 0);

				item->h=8*th+pad+(hasgroups ? th : 0);
				// ...
				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					 //----first line
					if (item2->id==ENGRAVE_Dash_Same_As) {
						if (hasgroups) {
							item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
						} else {
							item2->x=pad;  item2->y=y+1*th;  item2->w=0;  item2->h=0; 
						}

					} else if (item2->id==ENGRAVE_Dash_Broken_Threshhold) {
						item2->x=pad;  item2->y=y+1*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Zero_Threshhold) {
						item2->x=pad;  item2->y=y+2*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Random) {
						item2->x=pad;  item2->y=y+3*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Taper) {
						item2->x=pad;  item2->y=y+4*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Density) {
						item2->x=pad;  item2->y=y+5*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Length) {
						item2->x=pad;  item2->y=y+6*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Seed) {
						item2->x=pad;  item2->y=y+7*th+(hasgroups ? th : 0);  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Caps) {
						// *** todo!
					} else if (item2->id==ENGRAVE_Dash_Join) {
						// *** todo!
					}
				}
			}

			y+=item->h;

		} else if (item->id==ENGRAVE_Spacing) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!(item->state&LAX_OPEN)) item->h=th;
			else {
				item->h=2*th;

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					if (item2->id==ENGRAVE_Spacing_Default) {
						item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
					}
				}
			}

			y+=item->h;

		} else if (item->id==ENGRAVE_Direction) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!(item->state&LAX_OPEN)) item->h=th;
			else {
				item->h=2*th;

				// ...
			}

			y+=item->h;

		}
	} 

	panelbox.maxy=panelbox.miny+y+pad;

}

void EngraverFillInterface::DrawPanel()
{
	if (panel.n()==0) UpdatePanelAreas();

	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

	ScreenColor col;
	coloravg(&col, &fgcolor,&bgcolor, .9);
	dp->NewBG(&col);
	dp->NewFG(coloravg(fgcolor.Pixel(),bgcolor.Pixel(), .5));
	dp->drawrectangle(panelbox.minx,panelbox.miny, panelbox.maxx-panelbox.minx,panelbox.maxy-panelbox.miny, 2);
	dp->NewFG(&fgcolor);

	double th=dp->textheight();
	double pad=th/2;
	int ix,iy,iw,ih;
	int i2x,i2y,i2w,i2h;
	unsigned long hcolor=coloravg(fgcolor.Pixel(),bgcolor.Pixel(), .8);

	EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);

	MenuItem *item, *item2;
	for (int c=0; c<panel.n(); c++) {
		item=panel.e(c);
		ix=item->x+panelbox.minx;
		iy=item->y+panelbox.miny;
		iw=item->w;
		ih=item->h;
		
		if (item->id==ENGRAVE_Mode_Selection) {
			int which=mode;
			if (lasthover>=EMODE_Mesh && lasthover<EMODE_MAX) which=lasthover;

			for (int c2=0; c2<modes.n(); c2++) {
				item2=modes.e(c2);

				if (which==item2->id) {
					if (item2->image) {
						int imw=item2->image->w(), imh=item2->image->h();
						double imxmin=ix+imw/2, imxmax=ix+iw-imw/2;
						double dist=(imxmax-imxmin)/(modes.n()-1); //regular distance between midpoints
						int sw=dist;
						if (sw>item2->image->h()) sw=item2->image->h();

						 //display smaller icons next to it with this one enlarged
						for (int c3=0; c3<c2; c3++) { //the ones to the left
							dp->imageout(modes.e(c3)->image, imxmin+c3*dist-sw/2, iy+ih/2+sw/2, sw,-sw);
						}
						for (int c3=modes.n()-1; c3>c2; c3--) { //the ones to the right
							dp->imageout(modes.e(c3)->image, imxmin+c3*dist-sw/2, iy+ih/2+sw/2, sw,-sw);
						}

						 //finally display actual one...
						dp->NewFG(coloravg(fgcolor.Pixel(),bgcolor.Pixel(),.6));
						dp->NewBG(coloravg(fgcolor.Pixel(),bgcolor.Pixel(),.9));
						dp->drawrectangle(imxmin+c2*dist-imw/2, iy+ih/2+imh/2, imw,-imh, 2);
						dp->imageout(item2->image, imxmin+c2*dist-imw/2, iy+ih/2+imh/2, imw,-imh);

					} else dp->textout(ix+iw/2,iy+ih/2, modes.e(c2)->name,-1, LAX_CENTER);

					break;
				}
			}
			dp->NewFG(&fgcolor);

		} else if (item->id==ENGRAVE_Groups) {

			int ww;
			for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
				item2=item->GetSubmenu()->e(c2);
				i2x=item2->x+panelbox.minx;
				i2y=item2->y+panelbox.miny;
				i2w=item2->w;
				i2h=item2->h;
				ww=i2w; if (i2h<ww) ww=i2h;

				if (lasthover==item2->id) { //highlight hovered
					dp->NewFG(hcolor);
					dp->drawrectangle(i2x, i2y, i2w, i2h, 1);
					dp->NewFG(&fgcolor);
				}

				if (item2->id==ENGRAVE_Toggle_Group_List) {
					dp->drawrectangle(i2x+i2w/5,i2y+i2h*3/16,   i2w*.6,i2h/8, 1);
					dp->drawrectangle(i2x+i2w/5,i2y+i2h*7/16,   i2w*.6,i2h/8, 1);
					dp->drawrectangle(i2x+i2w/5,i2y+i2h*11/16,  i2w*.6,i2h/8, 1);

				} else if (item2->id==ENGRAVE_Previous_Group) {
					ww=i2w*.4;
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,ww, 0, THING_Triangle_Left);

				} else if (item2->id==ENGRAVE_Next_Group) {
					ww=i2w*.4;
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,ww, 0, THING_Triangle_Right);

				} else if (item2->id==ENGRAVE_Group_Name) {
					if (group && i2h>0) dp->textout(i2x+pad, i2y+i2h/2, 
							(isblank(group->name) ? "(unnamed)" : group->name) ,-1,
							LAX_VCENTER|LAX_LEFT);

				} else if (item2->id==ENGRAVE_Group_Active && i2h>0) {
					DrawThingTypes thing= (group && group->active) ? THING_Open_Eye : THING_Closed_Eye;
					ww=i2w*.4;
					dp->NewBG(1.,1.,1.);
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,-ww, 2, thing);

				} else if (item2->id==ENGRAVE_Group_Linked && i2h>0) {
					ww=i2w*.25;
					unsigned long color=(group && group->linked ? rgbcolor(0,200,0) : rgbcolor(255,100,100) );
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,-ww, THING_Circle, color,color);

				} else if (item2->id==ENGRAVE_Group_Color && i2h>0) {
					if (group) {
						dp->NewFG(&group->color);
						dp->drawrectangle(i2x, i2y, i2w, i2h, 1);
					}
					dp->NewFG(&fgcolor);
					dp->drawrectangle(i2x, i2y, i2w, i2h, 0);

				} else if (item2->id==ENGRAVE_Group_List && item2->isOpen()) {
					EngraverFillData *obj; 
					EngraverPointGroup *group2;

					dp->NewFG(&bgcolor);
					dp->drawrectangle(i2x,i2y,i2w,i2h,1);
					dp->NewFG(&fgcolor);

					int oncurobj=0;
					int yy=i2y;
					int ii=0;
					double hhh;

					DBG cerr<<"lasthoverindex="<<lasthoverindex<<endl;
					//for (int o=0; o<selection->n(); o++) {
						//obj=dynamic_cast<EngraverFillData *>(selection->e(o));
						//if (!obj) { ii++; continue; }
						obj=edata;
						if (obj) {
							if (obj==edata) oncurobj=1; else oncurobj=0;

							 //object header:  [eye] Object id
							ww=th*.4;
							hhh=1.0;
							if (lasthoverindex==ii || oncurobj) {
								 //highlight for current object
								ScreenColor col;
								if (lasthoverindex==ii) hhh-=.1;
								if (oncurobj) hhh-=.2;
								coloravg(&col, &fgcolor,&bgcolor, hhh);
								dp->NewFG(&col);
								dp->drawrectangle(i2x,i2y+ii*th, i2w,th,1);
								dp->NewFG(&fgcolor);
							}
							//dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,-ww, 2, obj->Visible() ? THING_Open_Eye : THING_Closed_Eye);
							dp->textout(i2x+th,yy+th/2, obj->Id(),-1, LAX_LEFT|LAX_VCENTER);
							yy+=th;

							ii++;

							 //the object's groups
							 // [eye] [color] [linked] [sharing]  group name
							for (int g=0; g<obj->groups.n; g++) {
								int xx=i2x;
								group2=obj->groups.e[g];

								hhh=1.0;
								if (lasthoverindex==ii || group==group2) {
									 //highlight bg for current group
									ScreenColor col;
									if (lasthoverindex==ii) hhh-=.1;
									if (group==group2) hhh-=.2;
									coloravg(&col, &fgcolor,&bgcolor, hhh);
									dp->NewFG(&col);
									dp->drawrectangle(i2x,i2y+ii*th, i2w,th,1);
									dp->NewFG(&fgcolor);
								}
								
								 //active eye
								dp->drawthing(xx+th/2,yy+th/2, ww,-ww, 2,  group2->active ? THING_Open_Eye : THING_Closed_Eye);
								xx+=th;


								 //linked
								unsigned long color;
								double rr=.25; //radius of inner linked circle
								if (oncurobj && g==current_group && group2->active && !group2->linked) {
									 //draw green override circle over current group linked thing regardless of actual color
									color=rgbcolor(0,200,0);
									dp->drawthing(xx+th/2,yy+th/2, th*.4,-th*.4, THING_Circle, color,color);
									rr=.2;
								}
								if (group2->linked && !group2->active) {
									//draw red override circle, since group is not modifiable while invisible
									color=rgbcolor(255,100,100);
									dp->drawthing(xx+th/2,yy+th/2, th*.4,-th*.4, THING_Circle, color,color);
									rr=.2;
								}
								 //now draw actual linked state
								color=(group2 && group2->linked ? rgbcolor(0,200,0) : rgbcolor(255,100,100) );
								dp->drawthing(xx+th/2,yy+th/2, th*rr,-th*rr, THING_Circle, color,color);
								dp->NewFG(&fgcolor);
								xx+=th;


								 //color rectangle
								dp->NewFG(&group2->color);
								dp->drawrectangle(xx+th*.1, yy+th*.1, th*.8, th*.8, 1);	
								dp->NewFG(&fgcolor);
								dp->drawrectangle(xx+th*.1, yy+th*.1, th*.8, th*.8, 0);	
								xx+=th;

								 //name
								dp->textout(xx, yy+th/2, 
									(isblank(group2->name) ? "(unnamed)" : group2->name) ,-1,
									LAX_VCENTER|LAX_LEFT);

								yy+=th;
								ii++;
							}
						}
					//}

				} else if (item2->id==ENGRAVE_New_Group) {
					dp->textout(i2x+i2w/2, i2y+i2h/2, "+",-1, LAX_CENTER);

				} else if (item2->id==ENGRAVE_Delete_Group) {
					dp->textout(i2x+i2w/2, i2y+i2h/2, "-",-1, LAX_CENTER);

				} else if (item2->id==ENGRAVE_Group_Down) {
					ww*=.8;
					dp->drawthing(i2x+i2w/2, i2y+i2h/2, ww/2,ww/2, 1, THING_Arrow_Down);

				} else if (item2->id==ENGRAVE_Group_Up) {
					ww*=.8;
					dp->drawthing(i2x+i2w/2, i2y+i2h/2, ww/2,ww/2, 1, THING_Arrow_Up);

				} else if (item2->id==ENGRAVE_Merge_Group) {
					ww*=.8;
					dp->drawthing(i2x+i2w/2, i2y+i2h/2, ww/2,ww/2, 1, THING_Arrow_Right);

				} else if (item2->id==ENGRAVE_Toggle_Group_List) {
					ww*=.8; 
					dp->drawline(i2x+i2w/4, i2y+i2h*3/4, i2x+3*i2w/4,i2y+i2h*3/4);
					dp->drawline(i2x+i2w/4, i2y+i2h/2, i2x+3*i2w/4,i2y+i2h/2);
					dp->drawline(i2x+i2w/4, i2y+i2h/4, i2x+3*i2w/4,i2y+i2h/4);

				}
			}

		} else if (item->id==ENGRAVE_Tracing) {
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);

			if (item->isOpen()) { 

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (item2->id==ENGRAVE_Trace_Same_As && item2->w>0) {
						if (lasthover==item2->id) { //highlight
							dp->NewFG(hcolor);
							dp->drawrectangle(i2x, i2y, i2w, i2h, 1);
							dp->NewFG(&fgcolor);
						}

						int sharing=IsSharing(ENGRAVE_Tracing, current_group);
						if (sharing>=0) {
							dp->textout(i2x+i2w/2,i2y+i2h/2, "(Shared)",-1, LAX_CENTER);
						} else dp->textout(i2x+i2w/2,i2y+i2h/2, "(Not shared)",-1, LAX_CENTER);

					} else if (item2->id==ENGRAVE_Trace_Curve) {
						DrawTracingTools(item2);
					}
				}

			}

		} else if (item->id==ENGRAVE_Dashes) {
			dp->NewFG(&fgcolor);
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);
			if (item->isOpen()) {
				//dp->textout(ix+iw/2,iy+th+(ih-th)/2, "Todo!",-1,LAX_CENTER);

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (item2->id==ENGRAVE_Dash_Same_As && i2w>0) {
						if (lasthover==item2->id) { //highlight
							dp->NewFG(hcolor);
							dp->drawrectangle(i2x, i2y, i2w, i2h, 1);
							dp->NewFG(&fgcolor);
						}

						int sharing=IsSharing(ENGRAVE_Dashes, current_group);
						if (sharing>=0) {
							dp->textout(i2x+i2w/2,i2y+i2h/2, "(Shared)",-1, LAX_CENTER);
							//int x=dp->textout(i2x,i2y+i2h/2, "With: ",-1, LAX_VCENTER|LAX_LEFT);
							//dp->textout(i2x+x,i2y+i2h/2, edata->groups.e[sharing]->name,-1, LAX_VCENTER|LAX_LEFT);
						} else dp->textout(i2x+i2w/2,i2y+i2h/2, "(Not shared)",-1, LAX_CENTER);

					} else if (item2->id==ENGRAVE_Dash_Broken_Threshhold) {
						 //draw this to span broken and zero areas
						DrawLineGradient(i2x,i2x+i2w, i2y+pad/4,i2y+i2h*2-pad/2, current_group, 1);

						int hh=i2h*.4;
						if (group) {
							unsigned long col=fgcolor.Pixel();
							if (lasthover==ENGRAVE_Dash_Broken_Threshhold) col=rgbcolorf(.5,.5,.5);
							dp->drawthing(i2x+i2w*group->dashes->broken_threshhold/group->spacing,
									i2y+hh/2, pad*2/3.,hh*.6, THING_Triangle_Down, bgcolor.Pixel(),col); 

							col=bgcolor.Pixel();
							if (lasthover==ENGRAVE_Dash_Zero_Threshhold) col=rgbcolorf(.5,.5,.5);
							dp->drawthing(i2x+i2w*group->dashes->zero_threshhold/group->spacing,
									i2y+i2h+i2h-hh/2, pad/2.,hh/2, THING_Triangle_Up, fgcolor.Pixel(),col);
						}

					} else if (item2->id==ENGRAVE_Dash_Zero_Threshhold) {
						 // this is draw with ENGRAVE_Dash_Broken_Threshhold

					} else if (item2->id==ENGRAVE_Dash_Random) {
						DrawSlider((group ? group->dashes->dash_randomness : .5), lasthover==ENGRAVE_Dash_Random,
								i2x,i2y,i2w,i2h, _("Random"));

					} else if (item2->id==ENGRAVE_Dash_Density) {
						DrawSlider((group ? group->dashes->dash_density : .5), lasthover==ENGRAVE_Dash_Density,
								i2x,i2y,i2w,i2h, _("Density"));

					} else if (item2->id==ENGRAVE_Dash_Taper) {
						DrawSlider((group ? group->dashes->dash_taper : .5), lasthover==ENGRAVE_Dash_Taper,
								i2x,i2y,i2w,i2h, _("Taper"));

					} else if (item2->id==ENGRAVE_Dash_Length) {
						DrawNumInput((group ? group->dashes->dash_length : 2), 0, lasthover==ENGRAVE_Dash_Length,
								i2x,i2y,i2w,i2h, _("Length"));

					} else if (item2->id==ENGRAVE_Dash_Seed) {
						DrawNumInput((group ? group->dashes->randomseed : -1), 1, lasthover==ENGRAVE_Dash_Seed,
								i2x,i2y,i2w,i2h, _("Seed"));

					} else if (item2->id==ENGRAVE_Dash_Caps) {
						// *** todo!
					} else if (item2->id==ENGRAVE_Dash_Join) {
						// *** todo!
					}
				}
			}

		} else if (item->id==ENGRAVE_Spacing) {
			dp->NewFG(&fgcolor);
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);
			if (item->isOpen()) {

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (item2->id==ENGRAVE_Spacing_Default) {
						DrawNumInput((group ? group->spacing : default_spacing), 0, lasthover==ENGRAVE_Spacing_Default,
								i2x,i2y,i2w,i2h, NULL);

					}
				}
			}

		} else if (item->id==ENGRAVE_Direction) {
			dp->NewFG(&fgcolor);
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);
			if (item->isOpen()) {
				dp->textout(ix+iw/2,iy+th+(ih-th)/2, "Todo!",-1,LAX_CENTER);
			}

		}
	} 

	dp->DrawReal();
}

/*! Return if the item type is shared with another group.
 * It will return  the first index starting from 0 that it is sharing with.
 * Else -1 for not sharing with anyone.
 */
int EngraverFillInterface::IsSharing(int what, int curgroup)
{
	if (!edata || !edata->groups.n || curgroup<0) return -1;
	return edata->IsSharing(what,curgroup);
}

/*! Draw one of the panel sections, just a line around a name, with a little triangle.
 */
void EngraverFillInterface::DrawPanelHeader(int open, int hover, const char *name, int x,int y,int w, int hh)
{
	int h=dp->textheight(); //hh is whole section height, we can ignore.. maybe one day draw box around whole section
	double ww=dp->textextent(name,-1,NULL,NULL);
	if (ww<w) {
		dp->drawline(x,y+h/2, x+(w-ww)/2,y+h/2);
		dp->drawline(x+w,y+h/2, x+w-(w-ww)/2,y+h/2);
	}
	dp->textout(x+w/2,y+h/2, name,-1, LAX_CENTER);
	dp->drawthing(x+w-h,y+h/2,h*.3,h*.3, open ? THING_Triangle_Down : THING_Triangle_Right,
			fgcolor.Pixel(),hover?(coloravg(fgcolor.Pixel(),bgcolor.Pixel(),.6)):bgcolor.Pixel());
}

void EngraverFillInterface::DrawTracingTools(Laxkit::MenuItem *item)
{
	// *** for identifier area, maybe have a drag out box for:
	//  ...  >  Load image
	//          Use object... (type in object name)
	//          Radial gradient
	//          Linear gradient
	//          + Additive
	//          - subtractive
	//          * absolute
	//
	

	double uiscale=1;
	double th=dp->textheight();
	double r=th*2/3;
	int pad=2;


	 //blank out trace controls rect
	ScreenColor col;
	coloravg(&col, &fgcolor,&bgcolor, .9);
	dp->NewFG(&col);
	//dp->drawrectangle(tracebox.minx,tracebox.miny, tracebox.maxx-tracebox.minx,tracebox.maxy-tracebox.miny, 1);

	DoubleBBox tbox;
	tbox.minx=item->x+panelbox.minx;
	tbox.miny=item->y+panelbox.miny;
	tbox.maxx=item->x+panelbox.minx + item->w;
	tbox.maxy=item->y+panelbox.miny + item->h;

	
	EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
	EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

	coloravg(&col, &fgcolor,&bgcolor, .95);
	dp->NewFG(&col);
	if (lasthover==ENGRAVE_Trace_Once) {
		dp->drawrectangle(tbox.minx,tbox.miny, (tbox.maxx-tbox.minx)/2,2*r+pad, 1);

	} else if (lasthover==ENGRAVE_Trace_Continuous) {
		dp->drawrectangle(tbox.minx+(tbox.maxx-tbox.minx)/2,tbox.miny+pad, (tbox.maxx-tbox.minx)/2,2*r, 1);

	} else if (lasthover==ENGRAVE_Trace_Opacity) {
		dp->drawrectangle(tbox.minx+pad,tbox.maxy-pad-2*th, (tbox.maxx-tbox.minx),th, 1);

	} else if (lasthover==ENGRAVE_Trace_Identifier) {
		if (trace->identifier) {
			ScreenColor red;
			red.rgbf(1.,0.,0.);
			coloravg(&col,&bgcolor,&red,.1);
			dp->NewFG(&col);
		}
		dp->drawrectangle(tbox.minx+pad,tbox.maxy-pad-th, (tbox.maxx-tbox.minx),th, 1);
	}


	DoubleBBox box;
	box.setbounds(&tbox);

	dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);

	 //continuous trace circle
	if (continuous_trace) dp->NewFG(0,200,0); else dp->NewFG(255,100,100); //should be settings of activate/deactivate colors
	dp->drawellipse((tbox.minx+tbox.maxx)/2+th/2+r,pad+tbox.miny+r,
                        r*uiscale,r*uiscale,
                        0,2*M_PI,
                        0);

	 //single trace square
	if (lasthover==ENGRAVE_Trace_Once) dp->NewFG(0,200,0); else dp->NewFG(255,100,100);
	dp->drawrectangle((tbox.minx+tbox.maxx)/2-th/2-2*r, pad+tbox.miny, r*2,r*2, 0);


	 //draw opacity slider
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	dp->NewFG(.5,.5,.5); 
	dp->drawline(tbox.minx,tbox.maxy-pad-1.5*th, tbox.maxx, tbox.maxy-pad-1.5*th);
	dp->drawpoint(flatpoint(tbox.minx + trace->traceobj_opacity*(tbox.maxx-tbox.minx),tbox.maxy-pad-1.5*th), th/3, 1);

	dp->textout(tbox.minx,tbox.maxy-pad, 
			trace->identifier ? trace->identifier : "...",-1,
			LAX_LEFT|LAX_BOTTOM);



	box.miny+=2*r+2*pad;
	box.minx+=pad;
	box.maxx-=pad;
	box.maxy-=pad+2*th;
	int ww=2*th;
	DrawLineGradient(box.minx,box.minx+ww, box.miny,box.maxy-ww, current_group, 0);
	DrawShadeGradient(box.minx+ww,box.maxx, box.maxy-ww,box.maxy);

	box.minx+=ww;
	box.maxy-=ww; //now box is where the value to weight curve goes

	//if (child!=&curvemapi) {
		curvemapi.Dp(dp);
		if (group) curvemapi.SetInfo(&group->trace ->value_to_weight);
		else       curvemapi.SetInfo(&default_trace. value_to_weight);
		curvemapi.SetupRect(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny);
		curvemapi.needtodraw=1;
		curvemapi.Refresh();
		dp->DrawScreen();
	//}

}

int EngraverFillInterface::PerformAction(int action)
{
	if (action==PATCHA_RenderMode) {
		if (rendermode==0) rendermode=1;
		else if (rendermode==1) rendermode=2;
		else rendermode=0;

		if (rendermode==0) PostMessage(_("Render with grid"));
		else if (rendermode==1) PostMessage(_("Render with preview"));
		else if (rendermode==2) PostMessage(_("Render recursively"));

		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SwitchMode || action==ENGRAVE_SwitchModeR) {

		int i=modes.findIndex(mode);

		if (action==ENGRAVE_SwitchMode) {
			i++;
			if (i>=modes.n()) i=0;
		} else {
			i--;
			if (i<0) i=modes.n()-1;
		}

		ChangeMode(modes.e(i)->id);

		submode=0;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ExportSvg) {
		app->rundialog(new FileDialog(NULL,"Export Svg",_("Export engraving to svg"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"exportsvg",FILES_SAVE, "out.svg"));
		return 0;

	} else if (action==ENGRAVE_RotateDir || action==ENGRAVE_RotateDirR) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		group->direction=rotate(group->direction, (action==ENGRAVE_RotateDir ? M_PI/12 : -M_PI/12), 0);
		group->Fill(edata, 1./dp->Getmag());
		//edata->FillRegularLines(1./dp->Getmag(),edata->default_spacing);
		edata->Sync(false);
		if (continuous_trace) Trace();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SpacingInc || action==ENGRAVE_SpacingDec) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (action==ENGRAVE_SpacingInc) group->spacing*=1.1; else group->spacing*=.9;
		group->Fill(edata, 1./dp->Getmag());
		edata->Sync(false);
		if (continuous_trace) Trace();
		DBG cerr <<"new spacing: "<<group->spacing<<endl;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ShowPoints || action==ENGRAVE_ShowPointsN) {
		if (show_points) show_points=0;
		else if (action==ENGRAVE_ShowPoints) show_points=1;
 		else show_points=3;

		if (show_points&2) PostMessage(_("Show sample points with numbers"));
		else if (show_points&1) PostMessage(_("Show sample points"));
		else PostMessage(_("Don't show sample points"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_MorePoints) {
		edata->MorePoints(current_group);
		edata->ReverseSync(true);
		if (continuous_trace) Trace();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleTrace) {
		 // *** obsolete? maybe make it toggle continuous tracing?
		show_trace=!show_trace;
		if (show_trace) continuous_trace=false;
		if (show_trace) PostMessage(_("Show tracing controls"));
		else PostMessage(_("Don't show tracing controls"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_TogglePanel) {
		show_panel=!show_panel;
		if (show_panel) PostMessage(_("Show control panel"));
		else PostMessage(_("Don't show control panel"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleGrow) {
		grow_lines=!grow_lines;
		if (grow_lines) PostMessage(_("Grow lines after warp"));
		else PostMessage(_("Don't grow lines after warp"));

		if (grow_lines) {
			if (!edata) return 0;
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);

			growpoints.flush();
			group->GrowLines(edata,
							 group->spacing/3,
							 group->spacing, NULL,
							 .01, NULL,
							 group->direction,group,
							 &growpoints,
							 1000 //iteration limit
							);
			edata->Sync(true);
		}
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleDir) {
		show_direction=!show_direction;
		if (show_direction) PostMessage(_("Show direction map"));
		else PostMessage(_("Don't show direction map"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_LoadDirectory) {
		const char *file=NULL;
		if (directionmap && directionmap->normal_map && directionmap->normal_map->filename)
			file=directionmap->normal_map->filename;
		app->rundialog(new FileDialog(NULL,"Load normal",_("Load normal map for direction"),
						  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
						  object_id,"loadnormal",
						  FILES_OPEN_ONE|FILES_PREVIEW, 
						  file));
		return 0;

	} else if (action==ENGRAVE_ToggleWarp) {
		always_warp=!always_warp;
		if (always_warp) PostMessage(_("Always remap points when modifying mesh"));
		else PostMessage(_("Don't remap points when modifying mesh"));

		if (always_warp) {
			edata->ReverseSync(false);
		}
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_NextFill || action==ENGRAVE_PreviousFill) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (action==ENGRAVE_NextFill) {
			if      (group->type==EngraverPointGroup::PGROUP_Linear)   group->type=EngraverPointGroup::PGROUP_Radial;
			else if (group->type==EngraverPointGroup::PGROUP_Radial)   group->type=EngraverPointGroup::PGROUP_Circular;
			else if (group->type==EngraverPointGroup::PGROUP_Circular) group->type=EngraverPointGroup::PGROUP_Linear;
		} else {
			if      (group->type==EngraverPointGroup::PGROUP_Linear)   group->type=EngraverPointGroup::PGROUP_Circular;
			else if (group->type==EngraverPointGroup::PGROUP_Radial)   group->type=EngraverPointGroup::PGROUP_Linear;
			else if (group->type==EngraverPointGroup::PGROUP_Circular) group->type=EngraverPointGroup::PGROUP_Radial;
		}

		if (grow_lines) {
			growpoints.flush();
			group->GrowLines(edata,
							 group->spacing/3,
							 group->spacing, NULL,
							 .01, NULL,
							 group->direction,group,
							 &growpoints,
							 1000 //iteration limit
							);
			edata->Sync(true);
		} else {
			group->Fill(edata,-1);
			edata->Sync(false);
		}

		if      (group->type==EngraverPointGroup::PGROUP_Linear)   PostMessage(_("Linear fill"));
		else if (group->type==EngraverPointGroup::PGROUP_Radial)   PostMessage(_("Radial fill"));
		else if (group->type==EngraverPointGroup::PGROUP_Circular) PostMessage(_("Circular fill"));

		needtodraw=1;
		return 0;
	}


	return PatchInterface::PerformAction(action);
}

void EngraverFillInterface::UpdateDashCaches(EngraverLineQuality *dash)
{
	if (!edata) return;
	if (!selection) {
		AddToSelection(poc);
	}

	for (int g=0; g<selection->n(); g++) {
		EngraverFillData *obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;

		for (int c=0; c<obj->groups.n; c++) {
			if (obj->groups.e[c]->dashes==dash) obj->groups.e[c]->UpdateDashCache();
		}
	}
}

int EngraverFillInterface::Trace()
{
	if (!edata) return 1;

	EngraverPointGroup *group;

	for (int g=0; g<edata->groups.n; g++) {
		group=edata->groups.e[g];
		if (!group->active) continue;
		if (group->trace && !group->trace->continuous_trace) continue;

		if (!group->trace->traceobject) continue;


		 //update cache if necessary
		if (!group->trace->traceobject->trace_sample_cache || group->trace->traceobject->NeedsUpdating())
			group->trace->traceobject->UpdateCache(viewport);

		int samplew=group->trace->traceobject->samplew;
		int sampleh=group->trace->traceobject->sampleh;

		int x,y, i;
		int sample, samplea;
		double me[6],mti[6];
		unsigned char *rgb;
		flatpoint pp;
		double a;

		Affine aa=edata->GetTransformToContext(false, 0);//supposed to be from edata to base real
		SomeData *to=group->trace->traceobject->object;
		if (to) {
			transform_invert(mti,to->m());
			transform_mult(me, aa.m(),mti);
		}


		for (int c=0; c<group->lines.n; c++) {
			LinePoint *l=group->lines.e[c];

			while (l) {
				pp=transform_point(me,l->p);
				//pp=l->p;
				//pp=transform_point(mti,pp);

				if (to) {
					x=samplew*(pp.x-to->minx)/(to->maxx-to->minx);
					y=sampleh*(pp.y-to->miny)/(to->maxy-to->miny);

					if (x>=0 && x<samplew && y>=0 && y<sampleh) {
						i=4*(x+(sampleh-y)*samplew);
						rgb=group->trace->traceobject->trace_sample_cache+i;

						samplea=rgb[3];
						sample=0.3*rgb[0] + 0.59*rgb[1] + 0.11*rgb[2];
						if (sample>255) {
							sample=255;
						}

						a=(255-sample)/255.;
						a=group->trace->value_to_weight.f(a);
						l->weight=group->spacing*a; // *** this seems off
						l->on = samplea>0 ? ENGRAVE_On : ENGRAVE_Off;
					} else {
						l->weight=0;
						l->on=ENGRAVE_Off;
					}

				} else { //use current
					a=group->spacing * group->trace->value_to_weight.f(l->weight_orig/group->spacing);
					l->weight=a;
				}

				l=l->next;
			}
		} //each line

		group->UpdateDashCache();
	} //each group 

	needtodraw=1;
	return 0;
}

/*! Return old value of mode.
 * newmode is an id of an item in this->modes.
 */
int EngraverFillInterface::ChangeMode(int newmode)
{
	if (newmode==mode) return mode;

	int c=0;
	for (c=0; c<modes.n(); c++) {
		if (modes.e(c)->id==newmode) break;
	}
	if (c==modes.n()) return mode;

	int oldmode=mode;
	mode=newmode;

	if (mode==EMODE_Trace) { continuous_trace=false; }

	if (newmode!=EMODE_Mesh && data) {
		if (child && data->UsesPath()) RemoveChild();
	}

	if (newmode==EMODE_Mesh) {
		if (data && data->UsesPath()) ActivatePathInterface();
	}


	needtodraw=1;
	PostMessage(ModeTip(mode));
	return oldmode;

}

int EngraverFillInterface::ActivatePathInterface()
{
	int status=PatchInterface::ActivatePathInterface();
	if (dynamic_cast<PathInterface*>(child)) {
		dynamic_cast<PathInterface*>(child)->Setting(PATHI_Defer_Render, true);
	}
	return status;
}

int EngraverFillInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i     =s->info2; //id of menu item
		unsigned int interf=s->info4; //is curvemapi.object_id if from there

		DBG cerr <<"Engraver Event: i=="<<i<<"  interf="<<interf<<endl;
		
		if (interf==curvemapi.object_id) return curvemapi.Event(e_data,mes);
		if (i<PATCHA_MAX) return PatchInterface::Event(e_data,mes);

		if ( i==EMODE_Mesh
		  || i==EMODE_Thickness
		  || i==EMODE_Orientation
		  || i==EMODE_Freehand
		  || i==EMODE_Blockout
		  || i==EMODE_Drag 
		  || i==EMODE_PushPull
		  || i==EMODE_AvoidToward
		  || i==EMODE_Twirl
		  || i==EMODE_Turbulence
		  || i==EMODE_Trace
		  || i==EMODE_Resolution) {

			ChangeMode(i);
			return 0;
		}

		if (i==ENGRAVE_Trace_Load) {
			app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
			return 0;

		} else if (i==ENGRAVE_Trace_Clear) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			if (group->trace->traceobject) group->trace->ClearCache(true);
			return 0;
		}

		return 0;

	} else if (!strcmp(mes,"PathInterface")) {
        if (data) {
            data->UpdateFromPath();

			//if (always_warp && curpoints.n>0) {
			if (always_warp) {
				edata->Sync(false);
				edata->UpdatePositionCache();
			}

			if (continuous_trace) Trace();
            needtodraw=1;
        }
        return 0;

	} else if (!strcmp(mes,"dashlength")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		char *endptr=NULL;
		double d=group->dashes->dash_length=strtod(s->str, &endptr);
		if (endptr!=s->str) {
			group->dashes->dash_length=d;
			group->UpdateDashCache();
			needtodraw=1;
		}
 		return 0;

	} else if (!strcmp(mes,"dashseed")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		char *endptr=NULL;
		int i=strtol(s->str, &endptr, 10);
		if (endptr==s->str || i<=0) i=-1;

		group->dashes->randomseed=i;
		group->UpdateDashCache();
		needtodraw=1;

 		return 0;

	} else if (!strcmp(mes,"defaultspacing")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		char *endptr=NULL;
		double d=strtod(s->str, &endptr);
		if (endptr==s->str || d<=0) d=.1;
		group->spacing=d;

		edata->Sync(false);
		edata->UpdatePositionCache();
		group->UpdateDashCache();
		if (continuous_trace) Trace();

		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"newcolor")) {
 		//got a new color for current group
    	const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e_data);
        if (!ce) return 1;

         // apply message as new current color, pass on to viewport
         // (sent from color box)
        LineStyle linestyle;
        float max=ce->max;
        linestyle.color.red=  (unsigned short) (ce->channels[0]/max*65535);
        linestyle.color.green=(unsigned short) (ce->channels[1]/max*65535);
        linestyle.color.blue= (unsigned short) (ce->channels[2]/max*65535);
        if (ce->numchannels>3) linestyle.color.alpha=(unsigned short) (ce->channels[3]/max*65535);
        else linestyle.color.alpha=65535;

		EngraverFillData *obj=edata;
		SomeData *somedata;
		if (eventobject>0 && eventobject!=edata->object_id && selection) {
			for (int c=0; c<selection->n(); c++) {
				if (eventobject==selection->e(c)->obj->object_id) {
					somedata=selection->e(c)->obj;
					obj=dynamic_cast<EngraverFillData*>(somedata);
					break;
				}
			}
		}

		EngraverPointGroup *group=(obj ? obj->GroupFromIndex(eventgroup) : NULL);
		if (group) group->color=linestyle.color;

		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"renamegroup")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverFillData *obj=edata;
		if (eventobject>0 && eventobject!=edata->object_id && selection) {
			for (int c=0; c<selection->n(); c++) {
				if (eventobject==selection->e(c)->obj->object_id) {
					obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
					break;
				}
			}
		}

		EngraverPointGroup *group=(obj ? obj->GroupFromIndex(eventgroup) : NULL);
		makestr(group->name,s->str);
		obj->MakeGroupNameUnique(eventgroup);
		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"valuemap")) {
		 //in floating curve window, value map was changed...
		 // *** this needs to be changed to the on canvas interface
		if (!edata) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s) return 0;
		if (continuous_trace) Trace();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"exportsvg")) {
        if (!edata) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
        if (!s) return 1;
        if (!isblank(s->str)) {
			edata->dump_out_svg(s->str);
			PostMessage(_("Exported."));
		}
        return 0;

	} else if (!strcmp(mes,"loadimage")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) return 0;
		LaxImage *img=load_image(s->str);
		const char *bname=lax_basename(s->str);
		if (!img) {
			char buf[strlen(_("Could not load %s"))+strlen(bname)+1];
			sprintf(buf,_("Could not load %s"),bname);
			PostMessage(buf);
			return 0;
		}

		int sx=dp->Maxx-dp->Minx;
		int sy=dp->Maxy-dp->Miny;
		flatpoint p1=screentoreal(dp->Minx+sx*.1,dp->Miny+sy*.1);
		flatpoint p2=screentoreal(dp->Maxx-sx*.1,dp->Maxy-sy*.1);
		DoubleBBox box;
		box.addtobounds(p1);
		box.addtobounds(p2);

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

		if (trace->traceobject) {
			trace->ClearCache(true);
			//trace->traceobject->dec_count();
			//trace->traceobject=NULL;
		}
		ImageData *idata=new ImageData();
		idata->SetImage(img);
		img->dec_count();
		trace->Install(TraceObject::TRACE_ImageFile, idata);
		trace->traceobject->object->fitto(NULL,&box,50,50,2);
		traceobjects.push(trace->traceobject);

		trace->ClearCache(false);
		delete[] trace->identifier;
		trace->identifier=new char[strlen(_("img: %s"))+strlen(bname)+1];
		sprintf(trace->identifier,_("img: %s"),bname);

		continuous_trace=false;

		needtodraw=1;
		PostMessage(_("Image to trace loaded."));
		return 0;

	} else if (!strcmp(mes,"loadnormal")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) return 0;

		bool newmap=(directionmap?false:true);
		if (!directionmap) directionmap=new NormalDirectionMap();
		int status=directionmap->Load(s->str);
		if (status!=0) {
			if (newmap) { delete directionmap; directionmap=NULL; }

			const char *bname=lax_basename(s->str);
			char buf[strlen(_("Could not load %s"))+strlen(bname)+1];
			sprintf(buf,_("Could not load %s"),bname);
			PostMessage(buf);
			return 0;
		}

		 //fit into a box 80% the size of viewport
		int sx=dp->Maxx-dp->Minx;
		int sy=dp->Maxy-dp->Miny;
		flatpoint p1=screentoreal(dp->Minx+sx*.1,dp->Miny+sy*.1);
		flatpoint p2=screentoreal(dp->Maxx-sx*.1,dp->Maxy-sy*.1);
		DoubleBBox box;
		box.addtobounds(p1);
		box.addtobounds(p2);

		ImageData *idata=new ImageData();
		idata->SetImage(directionmap->normal_map);
		idata->fitto(NULL,&box,50,50,2);
		directionmap->m.set(*idata);
		directionmap->m.Invert();
		idata->dec_count();

		needtodraw=1;
		PostMessage(_("Normal map loaded."));
		return 0;

	} else if (!strcmp(mes,"sharedash")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i =s->info2; //id of menu item

		if (i==-2) { //new based on currently ref'd
			EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
			if (cur) {
				cur->InstallDashes(cur->dashes->duplicate(),1);
			}

		} else if (i>=0) { //share
			if (edata && i<edata->groups.n && i!=current_group) {
				EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
				EngraverPointGroup *with=edata->GroupFromIndex(i);
				if (cur && with && cur->dashes!=with->dashes) {
					cur->InstallDashes(with->dashes,0);
				}
			}
		}

		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"sharetrace")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i =s->info2; //id of menu item

		if (i==-2) { //new based on currently ref'd
			EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
			if (cur) {
				EngraverTraceSettings *dup=cur->trace->duplicate();
				cur->InstallTraceSettings(dup,1);
			}

		} else { //share
			if (edata && i<edata->groups.n && i!=current_group) {
				EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
				EngraverPointGroup *with=edata->GroupFromIndex(i);
				if (cur && with && cur->trace!=with->trace) {
					 //trace->value_to_weight is not fully ref counted, so we must beware
					if (curvemapi.GetInfo()==&cur->trace->value_to_weight) curvemapi.SetInfo(NULL);
					cur->InstallTraceSettings(with->trace,0);
				}
			}
		}

		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"FreehandInterface")) {
		 //got new freehand mesh

        const RefCountedEventData *s=dynamic_cast<const RefCountedEventData *>(e_data);
		if (!s) return 1;

		PatchData *patch=dynamic_cast<PatchData*>(const_cast<anObject*>(s->TheObject()));
		if (!patch) return 1;


		deletedata();
		EngraverFillData *newdata=dynamic_cast<EngraverFillData*>(newPatchData(0,0,1,1,1,1,PATCH_SMOOTH));
		newdata->m(patch->m());
		newdata->CopyMeshPoints(patch, true);
		DBG newdata->dump_out(stderr,0,0,NULL);

		//newdata->UpdateFromPath();
		newdata->FindBBox();
		newdata->DefaultSpacing(-1);
		newdata->groups.e[0]->Fill(newdata, 1./dp->Getmag());
		//newdata->UpdateDashCache();
		for (int c=0; c<newdata->groups.n; c++) {
			newdata->groups.e[c]->UpdateBezCache();
			newdata->groups.e[c]->UpdateDashCache();
		} 
		newdata->Sync(false);

		if (viewport) {
			ObjectContext *oc=NULL;
			viewport->NewData(newdata,&oc);

			if (oc) poc=oc->duplicate();
		}
		data=newdata;
		data->linestyle=linestyle;
		data->FindBBox();
		curpoints.flush();

		edata=dynamic_cast<EngraverFillData*>(data);

		AddToSelection(poc);
		ChangeMode(EMODE_Mesh);

		needtodraw=1;
		return 0;
    }

    return 1;
}


Laxkit::MenuInfo *EngraverFillInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (lasthover==ENGRAVE_Trace_Curve) {
		MenuInfo *m=curvemapi.ContextMenu(x,y,deviceid, menu);
		int oldn=m->n();
		for (int c=oldn; c<m->n(); c++) {
			m->e(c)->info=curvemapi.object_id;
		}
		return m;
	}

	if (!menu) menu=new MenuInfo();

	if (mode==EMODE_Mesh) {
		menu=PatchInterface::ContextMenu(x,y,deviceid,menu);
//		if (child) {
//			if (!strcmp(child->whattype(),"PathInterface")) {
//				int oldn=(menu ? menu->n() : 0);
//				menu=child->ContextMenu(x,y,deviceid, menu);
//				for (int c=oldn; c<menu->n(); c++) {
//					menu->e(c)->info=child->object_id;
//				}
//			}
//		}
	}

	if (menu->n()!=0) menu->AddSep(_("Engraver"));

	int category=0, index=-1, detail=-1;
	int where=scanEngraving(x,y, &category, &index, &detail);
	if (mode==EMODE_Trace
		 || where==ENGRAVE_Trace_Box
		 || where==ENGRAVE_Trace_Once
		 || where==ENGRAVE_Trace_Load
		 || where==ENGRAVE_Trace_Continuous) {
		menu->AddSep(_("Trace"));
		menu->AddItem(_("Load image to trace..."),ENGRAVE_Trace_Load, LAX_OFF);
		menu->AddItem(_("Clear trace object"), ENGRAVE_Trace_Clear, LAX_OFF);
	}

	menu->AddSep(_("Mode"));
	MenuItem *i;
	for (int c=0; c<modes.n(); c++) {
		i=modes.e(c);
		menu->AddItem(i->name, i->id, LAX_OFF|LAX_ISTOGGLE|(mode==i->id ? LAX_CHECKED : 0), 0);
	}

	return menu;
}


Laxkit::ShortcutHandler *EngraverFillInterface::GetShortcuts()
{
    if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

	PatchInterface::GetShortcuts();

	 //convert all patch shortcuts to EMODE_Mesh mode
	WindowAction *a;
	ShortcutDef *s;
	for (int c=0; c<sc->NumActions(); c++)   { a=sc->Action(c);   a->mode=EMODE_Mesh; }
	for (int c=0; c<sc->NumShortcuts(); c++) { s=sc->Shortcut(c); s->mode=EMODE_Mesh; }

	 //any mode shortcuts
	sc->Add(ENGRAVE_SwitchMode,  'm',0,0,          "SwitchMode",  _("Switch edit mode"),NULL,0);
	sc->Add(ENGRAVE_SwitchModeR, 'M',ShiftMask,0,  "SwitchModeR", _("Switch to previous edit mode"),NULL,0);
	sc->Add(ENGRAVE_ExportSvg,   'f',0,0,          "ExportSvg",   _("Export Svg"),NULL,0);
	sc->Add(ENGRAVE_RotateDir,   'r',0,0,          "RotateDir",   _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_RotateDirR,  'R',ShiftMask,0,  "RotateDirR",  _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_SpacingInc,  's',0,0,          "SpacingInc",  _("Increase default spacing"),NULL,0);
	sc->Add(ENGRAVE_SpacingDec,  'S',ShiftMask,0,  "SpacingDec",  _("Decrease default spacing"),NULL,0);
	sc->Add(ENGRAVE_ShowPoints,  'p',0,0,          "ShowPoints",  _("Toggle showing sample points"),NULL,0);
	sc->Add(ENGRAVE_ShowPointsN, 'p',ControlMask,0,"ShowPointsN", _("Toggle showing sample point numbers"),NULL,0);
	sc->Add(ENGRAVE_MorePoints,  'P',ControlMask|ShiftMask,0,"MorePoints",  _("Subdivide all lines to have more sample points"),NULL,0);
	sc->Add(ENGRAVE_TogglePanel, 'c',0,0,          "TogglePanel", _("Toggle showing control panel"),NULL,0);
	sc->Add(ENGRAVE_ToggleGrow,  'g',0,0,          "ToggleGrow",  _("Toggle grow mode"),NULL,0);
	sc->Add(ENGRAVE_ToggleWarp,  'w',0,0,          "ToggleWarp",  _("Toggle warping when modifying mesh"),NULL,0);
	sc->Add(ENGRAVE_ToggleDir,   'd',0,0,          "ToggleDir",   _("Toggle showing direction map"),NULL,0);
	sc->Add(ENGRAVE_LoadDirectory,     'd',ControlMask,0,"LoadDir",     _("Load a normal map for direction"),NULL,0);

	sc->Add(ENGRAVE_NextFill,     LAX_Left, 0,EMODE_Orientation,  "NextFillType",     _("Switch to next fill type"),NULL,0);
	sc->Add(ENGRAVE_PreviousFill, LAX_Right,0,EMODE_Orientation,  "PreviousFillType", _("Switch to previous fill type"),NULL,0);

	return sc;
}

int EngraverFillInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in EngraverFillInterface::CharInput"<<endl;
	
	//if (child) return 1;

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_AvoidToward
		  || mode==EMODE_Twirl
		  ) {

		if (ch==LAX_Control) {
			submode=1;
			//if (state&ShiftMask) submode=3;
			needtodraw=1;
			return 0;
		} else if (ch==LAX_Shift) {
			submode=2;
			//if (state&ControlMask) submode=3;
			needtodraw=1;
			return 0;
		}
	}

    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }


	if (mode==EMODE_Mesh) return PatchInterface::CharInput(ch,buffer,len,state,d);


	return 1;
}

int EngraverFillInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (child) return 1;

	if (mode==EMODE_Mesh) return PatchInterface::KeyUp(ch,state,d);

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_AvoidToward
		  || mode==EMODE_Twirl
		  ) {

		if (ch==LAX_Control || ch==LAX_Shift) {
			submode=0;
			needtodraw=1;
			return 0;
		}
	}

	return 1;
}


} // namespace LaxInterfaces

