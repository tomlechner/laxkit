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
//    Copyright (C) 2016 by Tom Lechner
//

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>


#include <lax/interfaces/textonpathinterface.h>
#include <lax/interfaces/interfacemanager.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/units.h>
#include <lax/utf8utils.h>
#include <lax/laxutils.h>
#include <lax/fontdialog.h>
#include <lax/language.h>


//You need this if you use any of the Laxkit stack templates in lax/lists.h
#include <lax/lists.cc>


using namespace Laxkit;
using namespace LaxFiles;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//-------------------------- TextOnPath --------------------------------------
/*! \class TextOnPath
 *
 * Holds info about an attachment of text to a PathsData.
 */


TextOnPath::TextOnPath()
{
	baseline_type=FROM_Offset;
	baseline=0;
	start_offset=-.5; //actual distance along path from start of path
	end_offset=-1;
	path_length = -1;
	pathdirection=0;

	rotation=0;

	start=end=-1;
	text=NULL;
	paths=NULL;
	path=NULL;
	offsetpath=NULL;
	pathcontext=NULL; //local copy, deleted in destructor

	scale_correction=1; //because harfbuzz/cairo font computations suck when font size is below single digits
	font=NULL;
	color=new Color; //what about multicolor?
    direction=-1;
	language=NULL;
	script=NULL;


	dumpflags=0; //&1 = save cache

	cachetime=0;
	numglyphs=0;
	needtorecache=1;
}

TextOnPath::~TextOnPath()
{
	delete[] text;
	if (paths) paths->dec_count(); //path is assumed to be part of paths??
	if (offsetpath) delete offsetpath;
	if (font) font->dec_count();
	if (color) color->dec_count();
	if (pathcontext) delete pathcontext;

	delete[] language;
	delete[] script;
}

int TextOnPath::Text(const char *newtext, int nstart, int nend)
{
	makestr(text, newtext);
	start=nstart;
	if (start<0) start=0;
	if (nend<0) nend=strlen(text);
	end=nend;
	needtorecache=1;
	return 0;
}

/*! Reallocate cache to be able to hold newn glpyhs.
 *
 * Does not change data stored within the arrays.
 */
int TextOnPath::Reallocate(int newn)
{
	if (newn<numglyphs) return 0;

	numglyphs=newn;
	glyphs.Allocate(newn);
	while (glyphs.n<newn) {
		if (glyphs.e[glyphs.n]==NULL) glyphs.e[glyphs.n] = new OnPathGlyph;
		glyphs.n++;
	}

	return 0;
}

void TextOnPath::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
    Attribute att;
    dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

Attribute *TextOnPath::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
    if (!att) att=new Attribute;

    if (what==-1) {
		 //dump description
        att->push("baseline_type","path #or offset|stroke|otherstroke");
        att->push("baseline",".5em #number for how much to offset text from baseline_type");
        att->push("","");

		return att;
    }


	if (baseline_type==FROM_Path) att->push("baseline_type", "path");
	else if (baseline_type==FROM_Offset) att->push("baseline_type", "offset");
	else if (baseline_type==FROM_Stroke) att->push("baseline_type", "stroke");
	else if (baseline_type==FROM_Other_Stroke) att->push("baseline_type", "otherstroke");

	UnitManager *um=GetUnitManager();
	char scratch[100];
	const char *u=um->UnitName(baseline_units);
	sprintf(scratch, "%.10g %s", baseline, u ? u : "");
    att->push("baseline", scratch);

    att->push("start_offset", start_offset);
    att->push("end_offset", end_offset);
	att->push("rotation", rotation);

	if (text) {
		att->push("text", text);
		att->push("text_start", start);
		att->push("text_end",   end);
	}

	if (paths) {
		att->push("pathobject", paths->Id());
		att->push("pathindex", pathindex);
	}

	if (dumpflags&1) {
		//for (int c=0; c<numglyphs; c++) {
		//	***
		//}
	}

	return att;
} 

void TextOnPath::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
   char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"text")) {
			makestr(text, value);

        } else if (!strcmp(name,"text_start")) {
			IntAttribute(value, &start);
			
        } else if (!strcmp(name,"text_end")) {
			IntAttribute(value, &end);
			
        } else if (!strcmp(name,"baseline")) {
			DoubleAttribute(value, &baseline);

        } else if (!strcmp(name,"start_offset")) {
            DoubleAttribute(value, &start_offset);

        } else if (!strcmp(name,"end_offset")) {
            DoubleAttribute(value, &end_offset);
			
        } else if (!strcmp(name,"rotation")) {
            DoubleAttribute(value, &rotation);

        } else if (!strcmp(name,"font")) {
			// ***
			//LaxFont *newfont = fontmanager->dump_in_font(att->attributes.e[c]);
			//Font(newfont);

        } else if (!strcmp(name,"path")) {
			// ***
		}
	}
}

int TextOnPath::Font(Laxkit::LaxFont *newfont)
{
	if (newfont == font) return 0;
	if (!newfont) return 1;
	if (font) font->dec_count();
	font = newfont;
	font->inc_count();
	//if (font->textheight()<5) scale_correction = 50;
	needtorecache=1;
	return 0;
}

int TextOnPath::UseThisPath(PathsData *newpaths, int path_index)
{
	if (paths!=newpaths) {
		if (paths) paths->dec_count();
		paths = newpaths;
		paths->inc_count();
	}

	pathindex = path_index;
	if (pathindex<0) pathindex=0;

	path = paths->GetPath(pathindex);

	needtorecache=1;
	return 0;
}

// *** maybe have this be held in LaxFont??, just keeps live copies of freetype + harfbuzz fonts
//class TextCache
//{
//  public:
//	
//};

/*! Make the baseline be newbaseline, of if diff then make it oldbaseline+newbaseline.
 * Returns the new value of baseline.
 */
double TextOnPath::Baseline(double newbaseline, bool diff)
{
	if (diff) baseline+=newbaseline;
	else baseline=newbaseline;
	needtorecache=1;
	return baseline;
}

/*! Change baseline, but also change offset so that the constant point stays near the same
 * text after the baseline change.
 *
 * This assumes curvature is not so severe, allowing constant to still be actually closest point
 * after new baseline.
 */
double TextOnPath::Baseline(double newbaseline, bool diff, flatpoint constant)
{
	if (diff) baseline+=newbaseline;
	else baseline=newbaseline;

	double disttopath,  distalongpath,  tdist;
	double disttopath2, distalongpath2, tdist2;
	//double mag = Getmag();

	Remap(); //just in case, since we need offsetpath to be updated
	flatpoint closest = offsetpath->ClosestPoint(constant, &disttopath, &distalongpath, &tdist);
	needtorecache=1;
	Remap();
	closest = offsetpath->ClosestPoint(constant, &disttopath2, &distalongpath2, &tdist2);
	StartOffset(distalongpath2-distalongpath+start_offset, false);

	return baseline;
}

double TextOnPath::StartOffset(double newoffset, bool diff)
{
	if (diff) start_offset += newoffset;
	else start_offset=newoffset;

	if (offsetpath->IsClosed()) {
		Remap(); //just in case, since we need offsetpath to be updated
		double len=offsetpath->Length(0,-1);
		while (start_offset>=len)  start_offset-=len;
		while (start_offset<=-len) start_offset+=len;
	}

	needtorecache=1;
	return start_offset;
}

int TextOnPath::PathDirection(int newdir)
{
	pathdirection=newdir;
	if (pathdirection>3) pathdirection=0;
	if (pathdirection<0) pathdirection=3;
	needtorecache=1;
	return pathdirection;
}

/*! Assuming path is updated, align the text to it.
 */
int TextOnPath::Remap()
{
	if (!needtorecache) return 0;

	if (offsetpath) {
		delete offsetpath;
		offsetpath=NULL;
	}

	offsetpath = path->duplicate();
	if (baseline) {
		offsetpath->SetOffset(baseline, true);
	}
	offsetpath->ApplyOffset();
	path_length = offsetpath->Length(0,-1);

	if (end-start<=0) { //ok to have just a bunch of spaces
		textpathlen=0;
		needtorecache=0;
		cachetime=time(NULL);
		return 0;
	}


	 //set up freetype and hb fonts
	FontManager *fontmanager = InterfaceManager::GetDefault()->GetFontManager();

	FT_Face ft_face;
	FT_Library *ft_library = fontmanager->GetFreetypeLibrary();

	FT_New_Face (*ft_library, font->FontFile(), 0, &ft_face);
	FT_Set_Char_Size (ft_face, scale_correction*font->Msize()*64, scale_correction*font->Msize()*64, 0, 0);

	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (ft_face, NULL);


	 //figure out direction, language, and script
	hb_segment_properties_t seg_properties;


	 //Figure out direction, if any
	hb_direction_t dir = HB_DIRECTION_INVALID;
	if (direction==LAX_LRTB || direction==LAX_LRBT) dir=HB_DIRECTION_LTR;
	else if (direction==LAX_RLTB || direction==LAX_RLBT) dir=HB_DIRECTION_RTL;
	else if (direction==LAX_BTLR || direction==LAX_BTRL) dir=HB_DIRECTION_BTT;
	else if (direction==LAX_TBLR || direction==LAX_TBRL) dir=HB_DIRECTION_TTB;
	// else need to guess direction


	 //Figure out language, if any
	hb_language_t hblang = NULL;
	//const char *str=fontmanager->LanguageString(language);
	//if (language>=0 && str) hblang = hb_language_from_string(str, strlen(str));
	if (language) hblang = hb_language_from_string(language, strlen(language));
	//hblang = hb_language_get_default; 


	 //Figure out script, if any
	hb_script_t hbscript = HB_SCRIPT_UNKNOWN;
	//str=fontmanager->ScriptString(language);
	//if (script>=0 && *str) hbscript = hb_script_from_string(str, strlen(str));
	if (script) hbscript = hb_script_from_string(script, strlen(script));

//	const char *nonblank=NULL;
//	for (int c=0; c<lines.n; c++) {
//		if (!isblank(lines.e[c])) nonblank=lines.e[c];
//	}


	 //now figure out lines as necessary


	 //create buffer and add text
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create ();
	hb_buffer_add_utf8 (hb_buffer, text+start, end-start, 0, -1);


	 //set direction, language, and script
	if (dir==HB_DIRECTION_INVALID || hblang==NULL || hbscript==HB_SCRIPT_UNKNOWN) {
		 //at least one of these things we know, so we have to manually set again
		hb_buffer_guess_segment_properties (hb_buffer); //guesses direction, script, language 
		hb_buffer_get_segment_properties (hb_buffer, &seg_properties);

		if (dir!=HB_DIRECTION_INVALID)   seg_properties.direction = dir;      else dir      = seg_properties.direction;
		if (hblang!=NULL)                seg_properties.language  = hblang;   else hblang   = seg_properties.language;
		if (hbscript!=HB_SCRIPT_UNKNOWN) seg_properties.script    = hbscript; else hbscript = seg_properties.script;

		hb_buffer_set_segment_properties (hb_buffer, &seg_properties);

	} else {
		 //we know them all so...
		seg_properties.direction = dir;
		seg_properties.language  = hblang;
		seg_properties.script    = hbscript;
		hb_buffer_set_segment_properties (hb_buffer, &seg_properties);
	}


	 //set features
	hb_face_t *hb_face;
	hb_face = hb_ft_face_create_referenced(ft_face); 

	//*** foreach (feature in feature_string) set for hb_font
	//hb_ot_layout_language_get_feature_tags()
	//
	unsigned int lang_index=0;
	unsigned int script_index=0;
	hb_tag_t script_tag1, script_tag2;
	hb_ot_tags_from_script(hbscript, &script_tag1, &script_tag2);
	hb_ot_layout_table_find_script (hb_face, HB_OT_TAG_GSUB, script_tag1, &script_index);

	hb_tag_t lang_tag = hb_ot_tag_from_language (hblang);
	hb_ot_layout_script_find_language (hb_face, HB_OT_TAG_GSUB, script_index, lang_tag, &lang_index);

	unsigned int count = 80;
	hb_tag_t myResult[count];
	//hb_ot_layout_table_get_feature_tags(hb_font_get_face(hb_font), HB_OT_TAG_GSUB, 0, &count, myResult)
	//hb_ot_layout_table_get_feature_tags(hb_font_get_face(hb_font), HB_OT_TAG_GPOS, 0, &count, myResult)
	//
	//
	//- hb_ot_layout_table_get_script_tags() to get all scripts for this font;
	//- for each script index get all languages supported for this script with 
	//hb_ot_layout_script_get_language_tags();
	//- at this point it's two dimensional - (script index, language index), 
	//now get feature set for this pair with:

	hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, script_index, lang_index, 0, &count, myResult);
	DBG cerr <<"for font :"<<font->FontFile()<<":"<<endl;
	DBG cerr <<" All the GSUB features for cur lang+script:  ";
	char tagstr[5]; tagstr[4]='\0';
	for (int cc=0; cc<(int)count; cc++) {
		hb_tag_to_string(myResult[cc], tagstr);
		DBG cerr <<tagstr<<"  ";
	}
	DBG cerr << endl;
	hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GPOS, script_index, lang_index, 0, &count, myResult);
	DBG cerr <<" All the GPOS features for cur lang+script:  ";
	for (int cc=0; cc<(int)count; cc++) {
		hb_tag_to_string(myResult[cc], tagstr);
		DBG cerr <<tagstr<<"  ";
	}
	DBG cerr << endl;


	  //Shape it!
	hb_shape (hb_font, hb_buffer, NULL, 0);


	 // get positioning
	 //
	//struct hb_glyph_info_t {
	//  hb_codepoint_t codepoint;
	//  hb_mask_t      mask;
	//  uint32_t       cluster;
	//}
	//
	//struct hb_glyph_position_t {
	//  hb_position_t  x_advance;
	//  hb_position_t  y_advance;
	//  hb_position_t  x_offset; //shift from current position based on advances
	//  hb_position_t  y_offset;
	//}
	//
	//hb_segment_properties_t {
	//  hb_direction_t  direction;
	//  hb_script_t     script;
	//  hb_language_t   language;
	//}
	//
	//cairo_glyph_t {
	//  unsigned long        index;
	//  double               x;
	//  double               y;
	//} 
	//

	int nglyphs     = hb_buffer_get_length (hb_buffer); //glyphs wide
	hb_glyph_info_t *info    = hb_buffer_get_glyph_infos (hb_buffer, NULL);     //points to inside hb_buffer
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL); //points to inside hb_buffer

	 //reallocate glyphs, pos, rotation if necessary
	if (glyphs.Allocated() < nglyphs) {
		Reallocate(nglyphs);
	}
	numglyphs=nglyphs;

	double current_x = 0;
	double current_y = 0;
	double width     = 0; 
	double widthy    = 0;

	OnPathGlyph *glyph;

	for (int i = 0; i < numglyphs; i++) {
		glyph = glyphs.e[i];

		glyph->index     = info[i].codepoint;
		glyph->cluster   = info[i].cluster;
		glyph->x_advance = pos[i].x_advance / 64. / scale_correction;
		glyph->y_advance = pos[i].y_advance / 64. / scale_correction;
		glyph->x         =   current_x + pos[i].x_offset / 64. / scale_correction;
		glyph->y         = -(current_y + pos[i].y_offset / 64. / scale_correction);
		glyph->numchars  = get_num_chars(text+start, end-start, glyph->cluster, i==numglyphs-1 ? end-start : glyphs.e[i+1]->cluster);

		width     += glyph->x_advance;
		widthy    += glyph->y_advance;

		current_x += glyph->x_advance;
		current_y += glyph->y_advance;

		DBG fprintf(stderr, "index: %4d   cluster: %2u   at: %f, %f\n", glyph->index, glyph->cluster, glyph->x, glyph->y);
		//DBG cerr <<"index: "<<g[i].index<<"  cluster: "<<g[i].cluster<<"  at: "<<g[i].x<<","<<g[i].y<<endl;
	}

	DBG cerr <<endl;

	textpathlen = width;
	needtorecache = false;
	

	 //cleanup
	hb_buffer_destroy (hb_buffer); 
	hb_font_destroy (hb_font);
	FT_Done_Face (ft_face);


	 //now we have glyphs laid out along a straight line,
	 //we need to apply it to the actual line
	double d = start_offset;
	flatpoint point, tangent; 

	for (int i = 0; i < numglyphs; i++) {
		if (pathdirection%2==0) offsetpath->PointAlongPath(d, 1, &point, &tangent);
		else { offsetpath->PointAlongPath(-d, 1, &point, &tangent); tangent = -tangent; }

		glyphs.e[i]->position = point;
		glyphs.e[i]->rotation = atan2(tangent.y, tangent.x);

		d += glyphs.e[i]->x_advance / 72;
	}

	cachetime=time(NULL);
	return 0;
}


int TextOnPath::CharLen()
{
	return end-start;
}

int TextOnPath::DeleteChar(int pos,int after, int *newpos)
{
	if (after) { //del
		if (pos < end) {
			 //delete char within a line
			const char *p=utf8fwd(text+pos+1, text, text+end); 
			int cl=p-(text+pos);
			memmove(text+pos, text+pos+cl, strlen(text)-(cl+pos)+1);
			end-=cl;
		}

	} else { //bksp
		if (pos>start) {
			const char *p=utf8back(text+pos-1, text+start, text+start+(end-start));
			int cl=(text+pos)-p;
			memmove(text+pos-cl, text+pos, strlen(text)-pos+1);
			pos-=cl;
			end-=cl;
		}
	}

	needtorecache=1;
	if (newpos) *newpos=pos;
	return 0;
}

int TextOnPath::DeleteSelection(int fpos, int tpos, int *newpos)
{
	if (fpos<start || fpos>=end) fpos=end;
	if (tpos<start || tpos>=end) tpos=end;
	if (fpos==tpos) { *newpos=fpos; return 0; }

	 //make from be before to
	if (tpos<fpos) { int tt=tpos; tpos=fpos; fpos=tt; }

	 //remove text 
	memmove(text+fpos, text+tpos, strlen(text)-tpos+1);

	end-=tpos-fpos;
	*newpos=fpos;
	needtorecache=1;
	return 0;
}

int TextOnPath::InsertChar(unsigned int ch, int pos, int *newpos)
{
	if (pos<start || pos>=end) pos=end;

	char utf8[10];
	int cl=utf8encode(ch,utf8);
	utf8[cl]='\0';

	char *nline=new char[strlen(text)+cl+1];
	*nline='\0';
	if (pos) {
		strncpy(nline,text,pos);
		nline[pos]='\0';
	}
	strcat(nline,utf8);
	strcat(nline,text+pos);

	delete[] text;
	text=nline;
	end+=cl;
	pos+=cl;
	*newpos=pos;

	needtorecache=1; 
	return 0;
}

int TextOnPath::InsertString(const char *txt,int len, int pos, int *newpos)
{
	if (pos<start || pos>=end) pos=end;
	if (len<0) len=strlen(txt);
	if (!txt || !len) { *newpos=pos; return 0; }
	
	insertnstr(text, txt,len, pos);

	*newpos =pos;
	needtorecache=1;
	return 0;
}

SomeData *TextOnPath::ConvertToPaths(bool use_clones, Laxkit::RefPtrStack<SomeData> *clones_to_add_to)
{
	DBG cerr << " *** need to implement TextOnPath::ConvertToPaths()!!!"<<endl;
	return NULL;
}


//-------------------------- TextOnPathInterface --------------------------------------

/*! \class TextOnPathInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


TextOnPathInterface::TextOnPathInterface(anInterface *nowner, int nid)
 : anInterface(nowner,nid,NULL), pathinterface(0,NULL)
{
	textonpath_style=0;
	allow_edit_path=true;

	poc=NULL;
	textonpath=NULL;
	paths=NULL;

	caretpos=-1;
	showdecs=1;
	needtodraw=1;
	firsttime=1;

	hover_type=TPATH_None;

	sc=NULL; //shortcut list, define as needed in GetShortcuts()
}

TextOnPathInterface::~TextOnPathInterface()
{
	if (textonpath) textonpath->dec_count();
	if (sc) sc->dec_count();
	if (paths) paths->dec_count();
	if (poc) delete poc; 
}

const char *TextOnPathInterface::whatdatatype()
{ 
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *TextOnPathInterface::Name()
{ return _("Text On Path Tool"); }


//! Return new TextOnPathInterface.
/*! If dup!=NULL and it cannot be cast to TextOnPathInterface, then return NULL.
 */
anInterface *TextOnPathInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new TextOnPathInterface(NULL,id);
	else if (!dynamic_cast<TextOnPathInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int TextOnPathInterface::UseThis(anObject *nobj, unsigned int mask)
{ // ***

	if (!nobj) return 1;

    if (textonpath && dynamic_cast<LineStyle *>(nobj)) { // make all selected points have this color
        DBG cerr <<"TextOnPath new color stuff"<< endl;
        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);

        if (nlinestyle->mask&GCForeground) {
            textonpath->color->screen.red  =nlinestyle->color.red;
            textonpath->color->screen.green=nlinestyle->color.green;
            textonpath->color->screen.blue =nlinestyle->color.blue;
            textonpath->color->screen.alpha=nlinestyle->color.alpha;

            needtodraw=1;
        }

        needtodraw=1;
        return 1;
    }


	return 0;
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int TextOnPathInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int TextOnPathInterface::InterfaceOff()
{
	//Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void TextOnPathInterface::Clear(SomeData *d)
{
	if (textonpath) textonpath->dec_count();
	textonpath=NULL;
}

void TextOnPathInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

Laxkit::MenuInfo *TextOnPathInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo;

	//if (!menu->n()) menu->AddSep(_("Offset type"));
	menu->AddItem(_("Baseline from path"), TextOnPath::FROM_Path, LAX_ISTOGGLE|(textonpath->baseline_type==TextOnPath::FROM_Path ? LAX_CHECKED : 0));
	menu->AddItem(_("Baseline from offset"), TextOnPath::FROM_Path, LAX_ISTOGGLE|(textonpath->baseline_type==TextOnPath::FROM_Offset ? LAX_CHECKED : 0));
	menu->AddItem(_("Baseline from stroke"), TextOnPath::FROM_Path, LAX_ISTOGGLE|(textonpath->baseline_type==TextOnPath::FROM_Stroke ? LAX_CHECKED : 0));
	menu->AddItem(_("Baseline from other stroke"), TextOnPath::FROM_Path, LAX_ISTOGGLE|(textonpath->baseline_type==TextOnPath::FROM_Other_Stroke ? LAX_CHECKED : 0));
	menu->AddItem(_("Use envelope for size"), TextOnPath::FROM_Envelope, LAX_ISTOGGLE|(textonpath->baseline_type==TextOnPath::FROM_Envelope ? LAX_CHECKED : 0));
	//menu->AddItem(_("Use envelope to stretch"), TPATH_UseStretchEnvelope);
	menu->AddSep();
	menu->AddItem(_("Convert to path"), TPATH_ConvertToPath);

	return menu;
}

int TextOnPathInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{

	DBG cerr <<"TextOnPathInterface::Event() with message "<<(mes?mes:"??")<<endl;

    if (!strcmp(mes, "newfont")) {
        const StrsEventData *s=dynamic_cast<const StrsEventData*>(e_data);
        if (!s) return 1;

        if (!textonpath) return 0;

        double size=strtod(s->strs[2], NULL);
        if (size<=0) size=1e-4;

        LaxFont *newfont=dynamic_cast<LaxFont*>(s->object);
        if (newfont) textonpath->Font(newfont);
        //else data->Font(s->strs[3], s->strs[0], s->strs[1], size); //file, family, style, size

        //DBG cerr <<"------------ new font size a,d,h, fs: "<<data->font->ascent()<<", "<<data->font->descent()<<", "<<data->font->textheight()<<"   "<<data->fontsize<<endl;

        needtodraw=1;

        return 0;

	} else if (!strcmp(mes, "PathInterface")) {
		DBG cerr <<" ***** need to update textonpath!"<<endl;
		textonpath->needtorecache=1;
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i =s->info2; //id of menu item

        if (i==TPATH_ConvertToPath) {
			 // ***
			
		//} else if (i==) {
		}

		return 0; 
	}

	return 1; //event not absorbed
}

int TextOnPathInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (firsttime) {
		//--------testing vvvvv
		if (textonpath==NULL) {
			textonpath=new TextOnPath;
			textonpath->Text("Testing one two three!!!");
			LaxFont *font = anXApp::app->defaultlaxfont->duplicate();
			font->Resize(36);
			textonpath->Font(font);
			textonpath->color->screen.red=65535;
			font->dec_count();

			paths = new PathsData;
			paths->style|=PathsData::PATHS_Ignore_Weights;
			paths->moveTo(flatpoint(1,4));
			paths->curveTo(flatpoint(1,6), flatpoint(4,6), flatpoint(4,4));
			paths->append(flatpoint(4,2), POINT_TOPREV);
			paths->append(flatpoint(1,2), POINT_TONEXT);
			paths->close();
			//-------------
			//paths = new PathsData;
			//paths->style|=PathsData::PATHS_Ignore_Weights;
			//paths->moveTo(flatpoint(1,1));
			//paths->curveTo(flatpoint(2,1), flatpoint(3,1), flatpoint(4,1));
			//paths->curveTo(flatpoint(4,2), flatpoint(4,3), flatpoint(4,4));
			//paths->curveTo(flatpoint(3,4), flatpoint(2,4), flatpoint(1,4));
			//paths->append(flatpoint(1,3), POINT_TOPREV);
			//paths->append(flatpoint(1,2), POINT_TONEXT);
			//paths->close();

			textonpath->UseThisPath(paths, 0);
			caretpos=0;
		}
		//--------testing ^^^^^

		firsttime=0;
	}

	if (!textonpath) return 0;

	if (textonpath->needtorecache) textonpath->Remap();


	dp->NewFG(0.0,0.0,1.0);
	if (showdecs) pathinterface.DrawDataDp(dp, paths, NULL,NULL,0);


	if (textonpath->color) dp->NewFG(textonpath->color);

	 //draw each glyph one by one, since it is assumed they are all at odd angles




	 // draw glyphs
	//long curpos = textonpath->start;
	flatpoint pp;

	dp->font(textonpath->font,textonpath->font->textheight()/72);
	OnPathGlyph *glyph;
	GlyphPlace **glyphs = (GlyphPlace**)(textonpath->glyphs.e);

	for (int c=0; c<textonpath->numglyphs; c++) {
	//for (int c=0; c<1; c++) {
		glyph = textonpath->glyphs.e[c];

		 //set position and rotation, and write out glyph
		dp->PushAxes();
		dp->ShiftReal(glyph->position.x, glyph->position.y);
		dp->YAxis(-dp->YAxis());
		pp=dp->realtoscreen(0,0);
		dp->Rotate(-glyph->rotation, pp.x,pp.y);

		//DBG pp=dp->realtoscreen(flatpoint(0,0));
		//DBG cerr <<"  glyphrect ll: "<<pp.x<<", "<<pp.y<<endl;
		//DBG pp=dp->realtoscreen(flatpoint(glyph->x_advance, textonpath->font->Msize()));
		//DBG cerr <<"  glyphrect ll: "<<pp.x<<", "<<pp.y<<endl;

		//dp->NewFG(0.0,0.0,1.0);
		//dp->drawrectangle(0,0, glyph->x_advance/72, textonpath->font->Msize()/72, 1);

		dp->NewFG(textonpath->color->screen.red/65535.,textonpath->color->screen.green/65535.,textonpath->color->screen.blue/65535.,textonpath->color->screen.alpha/65535.);
		//dp->NewFG(1.0,0.0,0.0);
		dp->glyphsout(0,0, NULL, glyphs+c, 1, LAX_BASELINE|LAX_LEFT);
		dp->PopAxes();

		//dp->drawrectangle(textonpath->glyphs[c].x,textonpath->glyphs[c].y, textonpath->glyphs[c].x_advance, textonpath->font->Msize(), 1);
	}


	if (showdecs) {
		dp->LineWidthScreen(1);
		dp->NewFG(0.0,0.0,1.0);

		flatpoint tangent,tv;
		textonpath->offsetpath->PointAlongPath(hovert, 0, &pp, &tangent);
		tv=transpose(tangent);
		//double angle = angle_full(tangent, lasthover-pp);
		//if (angle<0) tv=-tv;
		if (textonpath->pathdirection%2==1) tv=-tv;
		tv.normalize();
		tangent.normalize();
		double th=textonpath->font->Msize()/72;
		//----------
		dp->drawpoint(pp, 3, 0);
		dp->drawpoint(pp+tv*(th), 3, 0);
		dp->drawpoint(pp+tv*(1.5*th), 3, 0);
		dp->drawpoint(pp+tv*(-th/2), 3, 0);
		//----------
		 //draw start offset modifier
		if (hover_type==TPATH_Offset) {
			dp->moveto(pp +     th*tv -   2*th*tangent);
			dp->lineto(pp +     th*tv +   2*th*tangent);
			dp->lineto(pp + 2  *th*tv +     th*tangent);
			dp->lineto(pp + 1.5*th*tv +     th*tangent);
			dp->lineto(pp + 1.5*th*tv -     th*tangent);
			dp->lineto(pp + 2  *th*tv -     th*tangent);
			dp->closed();
			dp->NewFG(0.0,0.0,1.0,.25);
			dp->fill(0);

		} else if (hover_type==TPATH_Baseline) {
			dp->moveto (pp - .5*th*tv - th*tangent);
			dp->curveto(pp - th*tangent, pp + th*tangent, pp - .5*th*tv + th*tangent);
			dp->curveto(pp - th*tv + th*tangent, pp - th*tv - th*tangent, pp - .5*th*tv - th*tangent);
			dp->closed();
			dp->NewFG(.0,.0,.0,.25);
			dp->fill(0);
		}


		//dp->LineWidthScreen(1);
		//dp->NewFG(0.0,0.0,1.0);
		//textonpath->path->PointAlongPath(hoveralongpath, 1, &pp, NULL);
		//DBG cerr <<"hover point computed: "<<pp.x<<','<<pp.y<<endl;
		//dp->drawpoint(pp, 15, 0);

		//dp->NewFG(0.0,1.0,0.0);
		//textonpath->path->PointAlongPath(hovert, 0, &pp, NULL);
		//dp->drawpoint(pp, 15, 0);
	}

	return 0;
}

int TextOnPathInterface::scan(int x,int y,unsigned int state, double *alongpath, double *alongt, double *distto)
{
	if (!textonpath) return TPATH_None;

	flatpoint p=screentoreal(x,y);
	lasthover=p;

	flatpoint closest, tangent, point;
	double disttopath, distalongpath, tdist;
	//double mag = Getmag();

	closest = textonpath->offsetpath->ClosestPoint(p, &disttopath, &distalongpath, &tdist);
	disttopath = sqrt(disttopath);

	textonpath->offsetpath->PointAlongPath(tdist, 0, &point, &tangent);
	double angle = angle_full(tangent, p-point);
	if (angle<0) disttopath=-disttopath;
	if (textonpath->pathdirection%2==1) disttopath=-disttopath;

	if (distto) *distto=disttopath;
	*alongpath = distalongpath;
	*alongt = tdist;
	hovert = tdist;

	DBG cerr << " --------------textonpath p:"<<p.x<<','<<p.y<<"  disttopath: "<<disttopath<<"  distalong: "<<distalongpath<<"  tdist: "<<tdist<<endl;


	//if (distalongpath>=start_offset && distalongpath<=end_offset) {
	double th=textonpath->font->Msize()/72;
	//double dd = (disttopath - textonpath->baseline)/th; 
	double dd = (disttopath)/th; 

	if (distalongpath >= textonpath->start_offset) {

		//DBG char str[200];
		//DBG sprintf(str, "d:%f  bl:%f  th:%f nn:%f", disttopath, textonpath->baseline, th, dd);
		//DBG PostMessage(str);

		if (dd >= -.5 && dd <= 0) {
			return TPATH_Baseline;
		} 
		if (dd >= 0 && dd <= 1) {
			return TPATH_Text;
		}
		if (dd >= 1 && dd <= 1.5) {
			return TPATH_Offset;
		}
//		if (disttopath >= -th/2 && disttopath <= 0) {
//			return TPATH_Baseline;
//		} 
//		if (disttopath >= 0 && disttopath <= th) {
//			return TPATH_Text;
//		}
//		if (disttopath >= th && disttopath <= th*2) {
//			return TPATH_Offset;
//		}
	}

	DBG char str[200];
	DBG sprintf(str, "d:%f  bl:%f  th:%f nn:%f", disttopath, textonpath->baseline, th, dd);
	DBG PostMessage(str);


	return TPATH_None;
}

int TextOnPathInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"TextOnPathInterface::LBDown().."<<endl;


    if (textonpath && count==2) {
        app->rundialog(new FontDialog(NULL, "Font",_("Font"),ANXWIN_REMEMBER, 10,10,700,700,0, object_id,"newfont",0,
                    NULL, NULL, textonpath->font->textheight(), //data->fontfamily, data->fontstyle, data->fontsize,
                    NULL, //sample text
                    textonpath->font, true
                    ));
        buttondown.up(d->id,LEFTBUTTON);
        return 0;
    } 


	double along, alongt;
	int hover = scan(x,y,state, &along, &alongt, NULL);
	hover_type=hover;

	buttondown.down(d->id,LEFTBUTTON,x,y, hover);



	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int TextOnPathInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"TextOnPathInterface::LBUp().."<<endl;

	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}



int TextOnPathInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		double alongpath, alongt;
		int hover=scan(x,y,state, &alongpath, &alongt, NULL);
		hoveralongpath = alongpath;

		DBG cerr <<" TextOnPath hover: "<<hover<<endl;

		if (hover!=hover_type) {
			hover_type = hover;

			if (hover_type==TPATH_Baseline)    PostMessage("baseline");
			else if (hover_type==TPATH_Offset) PostMessage("offset");
			else if (hover_type==TPATH_Text)   PostMessage("text");
		}

		needtodraw=1;
		return 1;
	}

	if (!buttondown.isdown(d->id, LEFTBUTTON)) return 0;

	//else deal with mouse dragging...

	int lx,ly, hover;
	buttondown.move(d->id, x,y, &lx,&ly);
	buttondown.getextrainfo(d->id, LEFTBUTTON, &hover);

	double alongpath, alongt, distto;
	double alongpath2, alongt2, distto2;
	scan( x, y,state, &alongpath,  &alongt,  &distto);
	scan(lx,ly,state, &alongpath2, &alongt2, &distto2);

	if ((hover==TPATH_Baseline || hover==TPATH_Offset) && (state&ControlMask)) hover=TPATH_BaseAndOff;

	if (hover==TPATH_Baseline || hover==TPATH_BaseAndOff) {
		textonpath->Baseline(distto-distto2, true, screentoreal(x,y));
		needtodraw=1;
	}
	if (hover==TPATH_Offset || hover==TPATH_BaseAndOff) {
		textonpath->start_offset += (alongpath-alongpath2);
		textonpath->needtorecache=1;
		needtodraw=1;

	} else if (hover==TPATH_Text) {
	}
	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int TextOnPathInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1; //wheel up ignored
}

int TextOnPathInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1; //wheel down ignored
}


int TextOnPathInterface::send()
{
	// ***

//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"TextOnPathInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int TextOnPathInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
		//deal with various modified keys...
	}

	if (ch==LAX_Esc && child) {
		RemoveChild();
		needtodraw=1;
		return 0;

	}
	
	if (!textonpath) return 1;
	
	if (ch==LAX_Del) { // delete
		textonpath->DeleteChar(caretpos,1, &caretpos);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Bksp) { // backspace
		textonpath->DeleteChar(caretpos,0, &caretpos);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Home) {
		caretpos=0;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_End) {
		caretpos=textonpath->CharLen();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Left) { // left
		if (caretpos<0) caretpos=0;
		else if (caretpos>0) {
			caretpos--;
			caretpos=utf8back_index(textonpath->text+textonpath->start, caretpos, textonpath->end-textonpath->start);
			needtodraw=1;
		}
		return 0;

	} else if (ch==LAX_Right) { // right
		const char *line = textonpath->text+textonpath->start;
		if (line[caretpos]) {
			caretpos++;
			const char *pos = utf8fwd(line+caretpos, line, line+strlen(line));
			caretpos = pos-line;
			if (caretpos>textonpath->end-textonpath->start) caretpos=textonpath->end-textonpath->start;
			needtodraw=1;
		}
		return 0;

//	} else if (ch==LAX_Up) { // up
//		caretline--;
//		if (caretline<0) caretline=0;
//		if (caretpos>(int)strlen(textonpath->lines.e[caretline]))
//			caretpos=strlen(textonpath->lines.e[caretline]);
//		caretpos=utf8back_index(textonpath->lines.e[caretline], caretpos, strlen(textonpath->lines.e[caretline]));
//		needtodraw=1;
//		return 0;
//
//	} else if (ch==LAX_Down) { // down
//		caretline++;
//		if (caretline>=textonpath->lines.n) caretline=textonpath->lines.n-1;
//		if (caretpos>(int)strlen(textonpath->lines.e[caretline]))
//			caretpos=strlen(textonpath->lines.e[caretline]);
//		caretpos=utf8back_index(textonpath->lines.e[caretline], caretpos, strlen(textonpath->lines.e[caretline]));
//		needtodraw=1;
//		return 0;
//
//	} else if (ch==LAX_Enter) {
//		textonpath->InsertChar('\n',caretline,caretpos,&caretline,&caretpos);
//		needtodraw=1;
//		return 0;

	} else if (ch>=32 && ch<0xff00 && (state&(ControlMask|AltMask|MetaMask))==0) {
		 // add character to text
		if (caretpos<0) caretpos=0;

		textonpath->InsertChar(ch,caretpos,&caretpos);
		needtodraw=1;
		return 0;

	} else {
		 //default shortcut processing

		if (!sc) GetShortcuts();
		int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
		if (action>=0) {
			return PerformAction(action);
		}
	}
	

	return 1; //key not dealt with, propagate to next interface
}

int TextOnPathInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	return 1; //key not dealt with
}

//! Remove a child interface. In this case,a PathInterface.
/*! Redefine to change the path color from green back to control color.
 */
int TextOnPathInterface::RemoveChild()
{
    int c=anInterface::RemoveChild();
    if (!textonpath) return c;

	textonpath->needtorecache=1;

    viewport->postmessage("");

    return c;
}

Laxkit::ShortcutHandler *TextOnPathInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;


    sc=new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);

    sc->Add(TPATH_Kern,           'k',ControlMask,0,           "Kern",           _("Decrease kern"),NULL,0);
    sc->Add(TPATH_KernR,          'K',ShiftMask|ControlMask,0, "KernR",          _("Increase kern"),NULL,0);
    sc->Add(TPATH_BaselineUp,     'b',ControlMask,0,           "BaselineUp",     _("Move baseline up"),NULL,0);
    sc->Add(TPATH_BaselineDown,   'B',ShiftMask|ControlMask,0, "BaselineDown",   _("Move baseline up"),NULL,0);
    sc->Add(TPATH_OffsetInc,      'l',ControlMask,0,           "OffsetInc",      _("Move line start forward"),NULL,0);
    sc->Add(TPATH_OffsetDec,      'L',ShiftMask|ControlMask,0, "OffsetDec",      _("Move line start backward"),NULL,0);
    sc->Add(TPATH_EditPath,       'p',ControlMask,0,           "EditPath",       _("Edit the path"),NULL,0);
    sc->Add(TPATH_ConvertToPath,  'P',ShiftMask|ControlMask,0, "ConvertToPath",  _("Convert to path object"),NULL,0);
    sc->Add(TPATH_ToggleDirection,'D',ShiftMask|ControlMask,0, "ToggleDirection",_("Toggle basic direction of text"),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int TextOnPathInterface::PerformAction(int action)
{
	if (action==TPATH_EditPath) {
		if (child) return 0;
		if (!paths) return 0;

		pathinterface.Dp(dp);
		pathinterface.pathi_style=PATHI_One_Path_Only|PATHI_Esc_Off_Sub|PATHI_Two_Point_Minimum|PATHI_Path_Is_M_Real;
		pathinterface.Setting(PATHI_No_Weights, textonpath->baseline_type==TextOnPath::FROM_Envelope ? 1 : 0);
		pathinterface.primary=1;

		//VObjContext voc;
		//voc.SetObject(aligninfo->path);
		//pathinterface->UseThisObject(&voc);//copies voc
		pathinterface.UseThis(paths);

		child=&pathinterface;
		pathinterface.owner=this;
		pathinterface.inc_count();
		viewport->Push(&pathinterface,-1,0);
		needtodraw=1;

		viewport->postmessage(_("Press Enter or Esc to finish editing"));
		return 0;

	} else if (action==TPATH_OffsetDec) {
		textonpath->start_offset-=textonpath->font->Msize()/2/72;
		textonpath->needtorecache=1;
		needtodraw=1;
		return 0;

	} else if (action==TPATH_OffsetInc) {
		textonpath->start_offset+=textonpath->font->Msize()/2/72;
		textonpath->needtorecache=1;
		needtodraw=1;
		return 0;

	} else if (action==TPATH_BaselineUp) {
		textonpath->Baseline(textonpath->font->Msize()/2/72, true);
		char str[strlen(_("Baseline %.10g"))+20];
		sprintf(str, _("Baseline %.10g"), textonpath->baseline);
		PostMessage(str);
		needtodraw=1;
		return 0;

	} else if (action==TPATH_BaselineDown) {
		textonpath->Baseline(-textonpath->font->Msize()/2/72, true);
		char str[strlen(_("Baseline %.10g"))+20];
		sprintf(str, _("Baseline %.10g"), textonpath->baseline);
		PostMessage(str);
		needtodraw=1;
		return 0;

	} else if (action==TPATH_ToggleDirection) {
		textonpath->PathDirection(textonpath->PathDirection()+1);
		PostMessage(_("Direction flipped."));
		needtodraw=1;
		return 0;

	//	return 0;
	}

	return 1;
}


} // namespace LaxInterfaces
