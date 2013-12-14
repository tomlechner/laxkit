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

#include <lax/bezutils.h>
#include <lax/transformmath.h>


#include <iostream>
using namespace std;
#define DBG 


/*! \file
 * \todo  should probably look into lib2geom
 */


namespace Laxkit {


//------------------------------- Bez utils -------------------------------------------
//! Update a bounding box to include the given bezier segment.
/*! \ingroup math
 * This assumes that p has already been figured in bbox, and bbox is 
 *   already a suitable bbox. That allows easy bounds checking per segment
 *   without checking the endpoints more than once.
 *
 * If extrema is not null, it should be a double[5]. It gets filled with the t parameters
 * corresponding to the extrema of the bez segment (where the tangent to the curve is
 * either vertical or horizontal), where the segment from p to q 
 * corresponds to t=[0,1]. Repeated extrema are only included once.
 *  
 *  Returns the number of extrema (up to 4).
 *  
 * \todo *** something is fishy with quad, occasionally finds way out extreme??
 */
int bez_bbox(flatpoint p,flatpoint c,flatpoint d,flatpoint q,DoubleBBox *bbox,double *extrema)//extrema=NULL
{
	 // check max min for endpoint q, assume p was done in last 
	 // check, or prior to calling here
	bbox->addtobounds(q);

	 // check max/min at extrema
	 // b =                a1 *t^3 +             a2 *t^2 +        a3 *t + p
	 // b = (-p + 3c -3d + q) *t^3 + (3p - 6c + 3d) *t^2 + (-3p + 3c)*t + p
	 // b'=(-3p + 9c -9d + 3q)*t^2 + (6p - 12c + 6d)*t   + (-3p + 3c)
	 // b'=              3*a1 *t^2 +           2*a2 *t   +        a3
	flatpoint a1=-p + 3*c -3*d + q,
			a2=3*p - 6*c + 3*d,
			a3=-3*p + 3*c,
			bp;
	double t1,t;
	int extt=0;
	
	 // ----------Check x extrema
	 // The curve can be constant, linear, quadratic, cubic.
	 // thus, only need to check extrema for quad and cubic
	if (a1.x==0 && a2.x!=0) { // is quadratic, must deal with sep. from cubic because div by 0
		t1=-a3.x/2/a2.x;
		if (t1>=0 && t1<=1) { // found 1 extrema in range
			if (extrema) extrema[extt++]=t1;
			bp=(a2*t1 + a3)*t1 + p; // find the bez point
			DBG cerr <<"x quad ext:"<<t1<<" at:"<<bp.x<<','<<bp.y<<endl;
			bbox->addtobounds(bp);
		}
	} else if (a1.x!=0) { // is full cubic, otherwise is just a straight line
		t=4*a2.x*a2.x - 4*(3*a1.x)*a3.x;  // t=b^2-4*a*c
		//t=p.x*(q.x-d.x)+c.x*c.x+d.x*d.x-c.x*(d.x+q.x); //t=b^2-4*a*c (radical in quadratic)
		if (t>=0) { // one extrema
			t=sqrt(t);
			t1=(-2*a2.x+t)/(2*3*a1.x); // t1= (-b + sqrt(b^2-4*a*c))/2a
			DBG cerr <<"x ext:"<<t1;
			if (t1>=0 && t1<=1) { // found 1 extrema in range
				if (extrema) extrema[extt++]=t1;
				bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
				DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
				bbox->addtobounds(bp);
			}
			DBG else cerr <<endl;
			if (t) { // if is not a double root
				t1=(-2*a2.x-t)/(2*3*a1.x); // t1= (-b - sqrt(b^2-4*a*c))/2a
				DBG cerr <<"x2 ext:"<<t1;
				if (t1>=0 && t1<=1) { // found another x extrema in range
					if (extrema) extrema[extt++]=t1;
					bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
					DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
					bbox->addtobounds(bp);
				}
				DBG else cerr <<endl;
			}
		} // else no extrema
	}

	 // --------Check y extrema
	if (a1.y==0 && a2.y!=0) { // is quadratic, must deal with sep. from cubic because div by 0
		t1=-a3.y/2/a2.y;
		if (t1>=0 && t1<=1) { // found 1 extrema in range
			if (extrema) extrema[extt++]=t1;
			bp=(a2*t1 + a3)*t1 + p; // find the bez point
			DBG cerr <<"y quad ext:"<<t1<<" at:"<<bp.x<<','<<bp.y<<endl;
			bbox->addtobounds(bp);
		}
	} else if (a1.y!=0) { // full cubic
		t=4*a2.y*a2.y - 4*3*a1.y*a3.y;  // t=b^2-4*a*c
		if (t>=0) { // one extrema
			t=sqrt(t);
			t1=(-2*a2.y+t)/(2*3*a1.y); // t1= (-b + sqrt(b^2-4*a*c))/2a
			DBG cerr <<"y ext:"<<t1;
			if (t1>=0 && t1<=1) {
				if (extrema) extrema[extt++]=t1;
				bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
				DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
				bbox->addtobounds(bp);
			}
			DBG else cerr <<endl;
			if (t) {
				t1=(-2*a2.y-t)/(2*3*a1.y); // t1= (-b - sqrt(b^2-4*a*c))/2a
				DBG cerr <<"y2 ext:"<<t1;
				if (t1>=0 && t1<=1) {
					if (extrema) extrema[extt++]=t1;
					bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
					DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
					bbox->addtobounds(bp);
				}
				DBG else cerr <<endl;
			}
		} // else no extrema
	}
	return extt;
}

//! Return the t parament for the point closest to p in the bezier segment p1,c1,c2,p2.
/*! \ingroup math
 *  This merely finds the least distance to p of any point at t=0,1/maxpoints,2/maxpoints,...,1.0.
 * 
 * maxpoints must somehow be chosen so that for each segment [t,t+1/maxpoints], its length
 * is no longer than the distance one is really looking for.
 *
 * If d_ret!=NULL, then return the minimum distance to the path in it.
 */
double bez_closest_point(flatpoint p, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int maxpoints,
		double *d_ret, double *dalong_ret, flatpoint *found)
{
	flatpoint bp,last;
	double d=1e+10,
		   dd;
	double da=0, dat=0;
	double at_t=1e+10,
		   t,dt,a1,a2,a3,a4;
	double start=0,end=1;
	dt=1/(double)maxpoints;
	last=p1;

	for (int recurse=0; recurse<2; recurse++) {
		for (t=start; t<=end; t+=dt) {
			a1=(1-t)*(1-t)*(1-t);
			a2=3*t*(1-t)*(1-t);
			a3=3*t*t*(1-t);
			a4=t*t*t;
			bp.x=(a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x);
			bp.y=(a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y);
			dd=(bp.x-p.x)*(bp.x-p.x)+(bp.y-p.y)*(bp.y-p.y);

			if (dalong_ret) { dat+=norm(bp-last); last=bp; }
			if (dd<d) { d=dd; at_t=t; da=dat; if (found) *found=bp; }
		}

		start=at_t-dt;
		end  =at_t+dt;
		dt=(end-start)/maxpoints;
	}

	if (d_ret) *d_ret=d;
	if (dalong_ret) *dalong_ret=da;
	return at_t;
}

//! Return the physical length of the segment, by approximating with npoints.
double bez_segment_length(flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int npoints)
{
	double d=0;
	double dt=1/(double)npoints;
	double lx,ly, x,y;
	double a1,a2,a3,a4;
	for (double t=0; t<=1.0; t+=dt) {
		a1=(1-t)*(1-t)*(1-t);
		a2=3*t*(1-t)*(1-t);
		a3=3*t*t*(1-t);
		a4=t*t*t;
		x=(a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x);
		y=(a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y);

		if (t!=0) d+=sqrt((x-lx)*(x-lx)+(y-ly)*(y-ly));
		lx=x;
		ly=y;
	}
	return d;
}

/*! For when you only need one intersection on one bezier segment.
 * Return 1 for hit found, or 0.
 *
 * This just calls the fuller bez_intersections() with appropriate settings.
 */
int bez_intersection(flatpoint p1,flatpoint p2, int isline,
					flatpoint bp1, flatpoint bc1, flatpoint bc2, flatpoint bp2,
					int resolution, flatpoint *point_ret, double *t_ret)
{
	flatpoint b[4];
	b[0]=bp1;
	b[1]=bc1;
	b[2]=bc2;
	b[3]=bp2;

	int hits=bez_intersections(p1,p2, isline, b,4, resolution, 0, point_ret,1, t_ret,1, NULL);
	if (!hits) *point_ret=flatpoint();

	return hits;
}

/*! Transform points to coordinate system where p1 is the origin, and p2 corresponds
 * to point (1,0). Then it is easy to find intersections through the segment, or
 * through the line going through p1 and p2, since that is wherever the new x axis is crossed.
 * 
 * Assumes points is an array structured as v-c-c-v-c-c...c-c-v. For closed paths, you must ensure
 * the final vertex is the same as the initial. So there should be n/3+1 vertices in the list.
 *
 * This is a kind of primitive approximation, based on sampling resolution number of points per
 * v-c-c-v bezier segment.
 *
 * Return value is number of hits actually parsed. If the whole path was not processed, then
 * endt is assigned the ending t, else it gets 0.
 */
int bez_intersections(flatpoint P1,flatpoint P2, int isline,
					  flatpoint *points, //!< array of v-c-c-v
					  int n,             //!< number of flatpoints in points
					  int resolution,    //!< how many linear segments to begin search for each segment
					  double startt,     //!< offset this many segments before searching
					  flatpoint *points_ret, //!< this must be allocated already
					  int np,            //!< number of points allocated in points_ret, return up to this many hits
					  double *t_ret,     //!< this must be allocated already (optional, can be NULL)
					  int nt,            //!< number of doubles allocated in t_ret
					  double *endt)      //!< t at which searching stopped (hit max of np)
{
	double m[6],mi[6];
	transform_from_basis(m, P1,P2-P1,transpose(P2-P1));
	transform_invert(mi,m);

	int numhits=0;
	flatpoint p1,c1,c2,p2;
	flatpoint t1,t2,v,p;

	double tsofar=0;
	double dt,t,tt,ttt, a1,a2,a3;
	dt=1.0/resolution;

	p1=transform_point(mi,points[0]);
	for (int c=0; c<n-1; p1=p2, c+=3) {
		p2=transform_point(mi,points[c+3]);
		while (startt>=1) { startt-=1; continue; } //resume where we might have left off

		c1=transform_point(mi,points[c+1]);
		c2=transform_point(mi,points[c+2]);

		t1=p1;
		for (t=startt; t<1.0; t+=dt) {
			tt=t*t;
			ttt=tt*t;
			a1=1-3*t+3*tt-  ttt;
			a2=  3*t-6*tt+3*ttt;
			a3=	  3*tt-3*ttt;
			t2.x= a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
			t2.y= a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

			if ((t1.y>0 && t2.y<=0) || (t1.y<=0 && t2.y>0)) {
				 //found a hit! now interpolate for further approximation of x axis cross.
				 //note that case where t1.y==t2.y==0 (where segment is ON x axis) does not qualify here
				p.y=0;
				p.x=t2.x-t2.y*(t2.x-t1.x)/(t2.y-t1.y);

				if (!isline && (p.x<0 || p.x>1)) ; //not a hit for segment intersect!
				else {
					 //add p to hit list!
					p=transform_point(m,p);//back to original coordinates
					points_ret[numhits]=p;
					if (t_ret) t_ret[numhits]=tsofar+t;
					numhits++;
					if (numhits>=np || (t_ret && numhits>=nt)) {
						 //hit maximum allocated hits
						if (endt) *endt=tsofar+t;
						return numhits;
					}
				}
			}
			t1=t2;
		} //loop over one v-c-c-v segment
		tsofar+=1;
	}

	if (endt) *endt=0;
	return numhits;
}

//! From a physical distance, return the corresponding t parameter value.
/*! Note that this is probably not very reliable for long segments.
 */
double bez_distance_to_t(double dist, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int resolution)
{
	double dd=0, ddd;
	double x,y, lx,ly;
	double t,tt,ttt, a1,a2,a3;
	double dt=1.0/(resolution-1);
	int recurse=0;
	double end=1.0;

	for (t=0; t<=end; t+=dt) {
		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;

		x=a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
		y=a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

		if (t>0) {
			ddd=sqrt((x-lx)*(x-lx)+(y-ly)*(y-ly));
			if (dd+ddd>dist) {
				recurse++;
				if (recurse>1) break;

				end=t;
				t-=dt;
				dt=dt/(resolution-1);
				continue;
			}
			dd+=ddd;
		}
		lx=x;
		ly=y;
	}
	return t;
}

//! From a t parameter, return the corresponding distance value.
/*! Note that this is probably not very reliable for long segments. It only checks
 * against resolution number of straight line segments.
 */
double bez_t_to_distance(double T, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int resolution)
{
	double dd=0;
	double x,y, lx,ly;
	double t,tt,ttt, a1,a2,a3;
	double dt=1.0/(resolution-1);
	int recurse=0;
	double end=1.0;

	for (t=0; t<=end+dt; t+=dt) {
		if (t>=T || t>1) {
			recurse++;
			if (recurse>2) return dd;

			end=t;
			t-=dt;
			dt/=resolution;
			continue;
		}

		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;

		x=a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
		y=a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

		if (t>0) dd+=sqrt((x-lx)*(x-lx)+(y-ly)*(y-ly));

		lx=x;
		ly=y;
	}
	return dd;
}

//! Return the numerical tangent at t.
/*! Note that this is NOT necessarily the visual tangent! If a control point is on the vertex,
 * then the tangent there is the null vector.
 */
flatpoint bez_tangent(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	double tt, a1,a2,a3,a4;
	tt=t*t;
	a1= -3 + 6*t -3*tt;
	a2=  3 -12*t +9*tt;
	a3=      6*t -9*tt;
	a4=           3*tt;

	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x),
					  (a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y)));
}

//! Return the visual tangent at t.
/*! If t>0 and t<1, then just return bez_tangent(). Otherwise, approximate a vector with a point just
 * off the path. If t<0 or t>1, then a null vector is returned.
 *
 * \todo this could use L'Hopital's rule, which says if two functions in this case x(t) and y(t) approach
 *   0, then x'(t) and y'(t) are such that x/y=x'/y' when limit x'/y' exists...
 *   
 */
flatpoint bez_visual_tangent(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	if (t>0 && t<1) return bez_tangent(t,p1,c1,c2,p2);

	if (t==0) {
		flatpoint pp=bez_point(.00001, p1,c1,c2,p2);
		return pp-p1;
	}

	if (t==1) {
		flatpoint pp=bez_point(.99999, p1,c1,c2,p2);
		return p2-pp;
	}

	return flatpoint(0,0);
}

//! Cut the bezier segment in two at t.
/*! points_ret must be an already allocated array of 5 points. It is filled with the
 * new found points as follows:
 * <pre>
 *   points_ret[0] = new handle of p1
 *   points_ret[1] = new tonext of new vertex
 *   points_ret[2]= new vertex
 *   points_ret[3]= new toprev of new vertex
 *   points_ret[4]= new handle of p2
 * </pre>
 */
void bez_subdivide(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, flatpoint *points_ret)
{
	flatpoint nv=bez_point(t,p1,c1,c2,p2);
	flatpoint nt=bez_tangent(t,p1,c1,c2,p2);

	points_ret[0]=p1+t*(c1-p1);
	points_ret[1]=nv-t*nt/3;
	points_ret[2]=nv;
	points_ret[3]=nv+t*nt/3;
	points_ret[4]=p2+(1-t)*(c2-p2);
}

//! Return the cubic bezier point at t. t==0 is p1, t==1 is p2.
/*! \ingroup math
 *  \todo *** make sure this is really optimized!
 */
flatpoint bez_point(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
//	---------
//	double a1,a2,a3;
//	a1=(1-t)*(1-t)*(1-t); //1 - 3*t + 3*t^2 - t^3
//	a2=3*t*(1-t)*(1-t);   //      t - 3*t^2 + t^3
//	a3=3*t*t*(1-t);       //            t^2 - t^3
//	a4=t*t*t;             //                  t^3 
//	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x),
//					  (a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y));
//	---------
	double tt,ttt, a1,a2,a3;
	tt=t*t;
	ttt=tt*t;
	a1=1-3*t+3*tt-  ttt;
	a2=  3*t-6*tt+3*ttt;
	a3=      3*tt-3*ttt;
	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x),
					  (a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y)));
}

//! Break down numsegs bezier segments to a polyline with resolution*numsegs points.
/*! If numsegs==1, then from_points is an array of points: v-c-c-v. Each additional
 * segment means that two control points and another vertex follow (-c-c-v).
 *
 * If to_points==NULL, then return a new flatpoint[numsegs*resolution];
 */
flatpoint *bez_points(flatpoint *to_points,int numsegs,flatpoint *from_points,int resolution)
{
	if (to_points==NULL) to_points=new flatpoint[resolution*numsegs];

	for (int c=0,i=0; c<numsegs; c++,i+=resolution) {
		bez_points(to_points+i,from_points+c*3,resolution,c==0?0:1);
	}
	return to_points;
}

//! Break down numsegs bezier segments to a polyline with resolution*numsegs points.
/*! from_points is a list of bezier vertices and control points. If atend==0, then the
 * array starts with a vertex, and that vertex's previous control point is at the end of 
 * from_points. Otherwise it starts with a control point, then a vertex.
 * numsegs is the number of bezier segments in from_points. Each segment is defined by
 * 4 points. If closed==1, then the final vertex connects to the first vertex (after 2 control
 * points). Note that an array c-v-c is valid, provided that isclosed==1 and atend==0.
 *
 * If to_points==NULL, then return a new flatpoint[numsegs*resolution];
 */
flatpoint *bez_points(flatpoint *to_points,int numsegs,flatpoint *from_points,int resolution,int isclosed,int atend)
{
	cout <<"*** imp bez_points!"<<endl;
	return NULL;
//	if (to_points==NULL) to_points=new flatpoint[resolution*numsegs];
//
//	for (int c=0,i=0; c<numsegs; c++,i+=resolution) {
//		bez_points(to_points+i,from_points+c*3,resolution,c==0?0:1);
//	}
//	return to_points;
}

//! Break down the bezier segment to a polyline with resolution points.
/*! \ingroup math
 * If ignorefirst, do not compute the first point. This allows code to call this repeatedly
 * without calculating vertices twice.
 *
 * If to_points!=NULL, it must have room for resolution number of points.
 * If to_points==NULL, then return a new flatpoint[resolution].
 * In either case, the generates points array is returned.
 *
 * from_points is an array of the 4 flatpoints of the bezier segment: v-c-c-v.
 *
 * \todo optimize me
 */
flatpoint *bez_points(flatpoint *to_points,flatpoint *from_points,int resolution,int ignorefirst)
{
	if (to_points==NULL) to_points=new flatpoint[resolution];
	
	double t,tt,ttt, a1,a2,a3, dt;
	dt=1.0/(resolution-1);
	DBG int i=0;
	for (int c=(ignorefirst?1:0); c<resolution; c++) {
		t=dt*c;
		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;
		to_points[c]=flatpoint((a1*from_points[0].x + a2*from_points[1].x + a3*from_points[2].x + ttt*from_points[3].x),
						 	   (a1*from_points[0].y + a2*from_points[1].y + a3*from_points[2].y + ttt*from_points[3].y));
		DBG i++;
	}
	//DBG cerr <<"bez_points made "<<i<<" points, res="<<resolution<<endl;
	return to_points;
}

//! Break down the bezier segment to a polyline with resolution points.
/*! \ingroup math
 * If ignorefirst, do not compute the first point. This allows code to call this repeatedly
 * without calculating vertices twice.
 *
 * If to_points!=NULL, it must have room for resolution number of points.
 * If to_points==NULL, then return a new flatpoint[resolution].
 * In either case, to_points is returned.
 *
 * p1,c1,c2,p2 define the bezier segment. This just puts the points in an array and calls
 * bezpoints(flatpoint *,flatpoint *,int,int).
 */
flatpoint *bez_points(flatpoint *to_points,
					  flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2,int resolution,int ignorefirst)
{
	flatpoint p[4];
	p[0]=p1;
	p[1]=c1;
	p[2]=c2;
	p[3]=p2;
	return bez_points(to_points,p,resolution,ignorefirst);
}

//! Return the distance p is from the bezier curve in points.
/*! \ingroup math
 * points is v-c-c-v-c-c-v-...-v, and n is the number of all points including control points.
 * So (n mod 3) must be 1.
 * 
 * Returns the shortest distance of p to the curve.
 * t_ret gets filled with the t index [0..1] within the segment starting at i_ret.
 * i_ret gets filled with the index in points of the vertex for the segment the point is just after.
 *  If there is no point within 1e+10 units of the curve, then i_ret gets -1 and 1e+10 is returned.
 *
 * This is a very rough approximation. Only checks against maxpoints sample points per segment.
 *
 * \todo I'm sure this could be optimized and improved somehow.
 */
double bez_near_point(flatpoint p,flatpoint *points,int n,
					  int maxpoints,
					  double *t_ret,int *i_ret)
{
	int c,at_c=-1;
	double d=1e+10,dd,
		   tt,t=-1;
	for (c=0; c<n-1; c+=3) {
		tt=bez_closest_point(p,points[c],points[c+1],points[c+2],points[c+3],maxpoints,&dd,NULL,NULL);
		if (dd<d) { d=dd; t=tt; at_c=c; }
	}
	if (i_ret) *i_ret=at_c;
	if (t_ret) *t_ret=t;
	return d;
}

//! Just like bez_near_point() but with a list of pointers to points, rather than directly at points.
/*! \ingroup math
 *
 * This makes it slightly easier to process bezier curves that are not stored as a simple array
 * of flatpoints.
 */
double bez_near_point_p(flatpoint p,flatpoint **points,int n,
					  int maxpoints,
					  double *t_ret,int *i_ret)
{
	int c,at_c=-1;
	double d=1e+10,dd,
		   tt,t=-1;
	for (c=0; c<n; c+=3) {
		tt=bez_closest_point(p,*points[c],*points[c+1],*points[c+2],*points[c+3],maxpoints,&dd,NULL,NULL);
		if (dd<d) { d=dd; t=tt; at_c=c; }
	}
	if (i_ret) *i_ret=at_c;
	if (t_ret) *t_ret=t;
	return d;
}

//! Return the winding number of p relative to the bezier curve in points. 0 means point is inside
/*! \ingroup math
 *  points is assumed to be: c-v-c-c-v-...-v-c, and n must be number of vertices. So there must be
 *  n*3 points in the array.
 *
 * This breaks down the curve into (number of vertices-1)*resolution points, and then finds
 * the winding number for the resulting polyline.
 */
int point_is_in_bez(flatpoint p,flatpoint *points,int n,int resolution)//res=20
{
	flatpoint pnts[resolution*n];
	bez_to_points(pnts,points,n,resolution,1);
	return point_is_in(p,pnts,n*resolution);
}

//! Decompose the bezier curve to a polyline.
/*! There are n vertices, and n*3 points in from_points. The first bezier segment is 
 * from_points[1]-from_points[2],from_points[3],from_points[4]. If closed is nonzero, then assume
 * the final 2 points and the first 2 points of from_points make up the final segment of the curve.
 *
 * to_points is a flatpoint[n*resolution] array. If to_points==NULL, then a new'd array is returned,
 * else be sure that to_points has enough space for n*resolution flatpoints.
 * 
 * If n<2, NULL is returned.
 */
flatpoint *bez_to_points(flatpoint *to_points,flatpoint *from_points,int n,int resolution,int closed)
{
	if (n<2) return NULL;
	if (!to_points) to_points=new flatpoint[resolution*n];
	int c;
	for (c=0; c<n-1; c++) {
		bez_points(to_points+c*resolution,from_points+c*3+1,resolution+1,(c>0));
	}
	 // final segment
	if (closed) bez_points(to_points+c*resolution,
						   from_points[c*3+1],from_points[c*3+2],from_points[0],from_points[1],resolution,1);
	return to_points;
}

//! Return an approximate circle, with numpoints control points, or 4 if numpoints<=1.
/*! Center at (x,y) with radius r.
 *
 * To make 2 vertex points lie \f$\theta\f$ degrees apart on a circle of radius r,
 * then the control rods will have length v:
 * \f[
 * 	v=\frac{4\:r}{3}\:\frac{2\;sin(\theta/2)-sin(\theta)}{1-cos(\theta)};
 * \f]
 *
 * The first point in the returned array is a control point for the second point in the array.
 * So points alternate handle-node-handle - handle-node-handle - ...
 * So there will be numpoints*3 points in the returned array.
 *
 * If points==NULL, then return a new flatpoint[3*numpoints], else it is assumed that pts has
 * at least 3*numpoints allocated.
 */
flatpoint *bez_circle(flatpoint *points, int numpoints, double x,double y,double r)
{
	if (numpoints<=1) numpoints=4;
	int n=numpoints*3;

	if (!points) points=new flatpoint[n];

	double theta=2*3.141592653589/(numpoints); //radians between control points
	double v=4*r*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta)); //length of control handle

	flatpoint center=flatpoint(x,y);
	double xx,yy;
	for (int c=0, i=0; c<numpoints; c++, i+=3) {
		xx=cos(c*theta);
		yy=sin(c*theta);
		points[i+1]=center + flatpoint(r*xx,r*yy);
		points[i  ]=points[i+1] + flatpoint(v*yy,-v*xx);
		points[i+2]=points[i+1] + flatpoint(-v*yy,v*xx);
	}

	return points;
}

//! Create an ellipse composed of numsegments bezier segments, or 4 if numsegments<=1.
/*! Start and end in radians. If start==end, then assume a full circle.
 */
flatpoint *bez_ellipse(flatpoint *points, int numsegments,
					   double x,double y,
					   double xr,double yr,
					   flatvector xaxis,flatvector yaxis,
					   double start_angle,double end_angle)
{
	if (numsegments<=1) numsegments=4;
	int n=numsegments*3;

	if (!points) points=new flatpoint[n];

	if (end_angle==start_angle) end_angle=start_angle+2*M_PI;
	double theta=(end_angle-start_angle)/(numsegments); //radians between control points
	double v=4*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta)); //length of control handle

	flatpoint center=flatpoint(x,y);
	double mm[6];
	transform_from_basis(mm, center,xr*xaxis,yr*yaxis);

	double xx,yy;
	for (int c=0, i=0; c<numsegments; c++, i+=3) {
		 //first find for unit circle
		xx=cos(start_angle+c*theta);
		yy=sin(start_angle+c*theta);

		points[i+1]=flatpoint(xx,yy); //the vertex
		points[i  ]=points[i+1] + flatpoint(v*yy,-v*xx); //previous control
		points[i+2]=points[i+1] + flatpoint(-v*yy,v*xx); //next control

		 //now map to ellipse
		points[i  ]=transform_point(mm,points[i  ]);
		points[i+1]=transform_point(mm,points[i+1]);
		points[i+2]=transform_point(mm,points[i+2]);
	}

	return points;
}


} // namespace Laxkit
