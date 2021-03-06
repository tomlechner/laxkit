//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006 by Tom Lechner
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
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
#ifndef _LAX_DUMP_H
#define _LAX_DUMP_H

#include <cstdio>
#include <lax/anobject.h>
#include <lax/attributes.h>
#include <lax/laxdefs.h>
#include <lax/errorlog.h>

namespace LaxFiles {


//------------------------------- DumpContext ---------------------------------
class DumpContext : public Laxkit::anObject
{
 public:
	int what;
	int zone; //app dependent, like 0 for document, 1 for project, 2 for component (for instance)
	unsigned long initiator; //object_id of top initiating object

	char *basedir;
	char subs_only;
	Laxkit::anObject *extra;

	Laxkit::ErrorLog *log;

	DumpContext();
	DumpContext(const char *nbasedir,char nsubs_only, unsigned long initer);
	virtual ~DumpContext();
};


//------------------------------- DumpUtility ---------------------------------
class DumpUtility
{
 public:
	virtual void       dump_out(FILE *f,int indent,int what,DumpContext *context) =0;
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context) { return NULL; }

	virtual void dump_in (FILE *f,int indent,int what,DumpContext *context,Attribute **att);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context) =0;

	virtual ~DumpUtility() {}
};



} // namespace LaxFiles

#endif


