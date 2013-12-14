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
#ifndef _LAX_TRANSFORMMATH_H
#define _LAX_TRANSFORMMATH_H

#include <lax/vectors.h>

//-------------------- Affine Transform Utilities ---------------------

namespace Laxkit {

class Affine
{
  protected:
	double _m[6];

  public:
	Affine();
	Affine(const double *m);
	Affine(const Affine &m);
	Affine(double xx,double xy,double yx,double yy,double tx,double ty);
	Affine &operator=(Affine &m);
	Affine &operator*=(Affine const &m);
	Affine operator*(Affine const m);
	virtual ~Affine();
	virtual void setIdentity();
	virtual bool isIdentity();
	virtual void setRotation(double angle);
	virtual void setScale(double sx,double sy);
	virtual void setBasis(flatpoint o, flatpoint x,flatpoint y);
	virtual void setBasics(double x,double y,double sx,double sy,double angle,double shear);
	virtual void getBasics(double *x,double *y,double *sx,double *sy,double *angle,double *shear);

	virtual void Translate(flatvector d);
	virtual void Rotate(double angle);
	virtual void Rotate(double angle, flatpoint around_point);
	virtual void RotateScale(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2);
	virtual void Stretch(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2);
	virtual void AnchorShear(flatpoint anchor1, flatpoint anchor2, flatpoint anchor3, flatpoint newanchor3);
	virtual void Scale(double s);
	virtual void Scale(flatpoint o, double s);
	virtual void Scale(flatpoint o, double sx,double sy);
	virtual void Scale(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2);
	virtual void FlipH();
	virtual void FlipV();
	virtual void Flip(flatpoint f1,flatpoint f2);
	virtual void Multiply(Affine &m);
	virtual void PreMultiply(Affine &m);

	virtual Affine Inversion();
	virtual void Invert();
	virtual flatpoint transformPoint(flatpoint p);
	virtual flatpoint transformPointInverse(flatpoint p);
	virtual flatpoint transformVector(flatpoint p);

	virtual const double *m() const { return _m; }
	virtual void   m(const double *mm);
	virtual void   m(double xx,double xy,double yx,double yy,double tx,double ty);
	virtual double m(int c) { return _m[c]; }
	virtual void   m(int c,double v) { _m[c]=v; }
	virtual void Unshear(int preserve_x, int normalize);

	virtual flatpoint origin() { return flatpoint(_m[4],_m[5]); }
	virtual void      origin(flatpoint o) { _m[4]=o.x; _m[5]=o.y; }
	virtual flatpoint xaxis() { return flatpoint(_m[0],_m[1]); }
	virtual void      xaxis(flatpoint x) { _m[0]=x.x; _m[1]=x.y; }
	virtual flatpoint yaxis() { return flatpoint(_m[2],_m[3]); }
	virtual void      yaxis(flatpoint y) { _m[2]=y.x; _m[3]=y.y; }
};

void dumpctm(const double *d);
int is_degenerate_transform(double *m);
double *transform_invert(double *result,const double *m);
double *transform_mult(double *result,const double *a,const double *b);
double *transform_identity(double *result);
double *transform_rotate(double *m, double angle);
double *transform_from_basis(double *result,flatpoint o,flatpoint x,flatpoint y);
void transform_to_basis(double *m,flatpoint *o,flatpoint *x,flatpoint *y);
double *transform_from_basics(double *result,double x,double y,double sx,double sy,double angle,double shear);
void transform_to_basics(double *m,double *x,double *y,double *sx,double *sy,double *angle,double *shear);
double *transform_set(double *m,double a,double b,double c,double d,double x0,double y0);
void transform_copy(double *dest,const double *src);
flatpoint transform_point_inverse(const double *m,flatpoint p);
flatpoint transform_point(const double *m,flatpoint p);
flatpoint transform_point(const double *m,double x,double y);
flatpoint transform_vector(const double *m,flatpoint p);

double *transform_from_3x3_fixed(double *result,int M[3][3]);
void transform_to_3x3_fixed(int M[3][3],double *m);

double *svgtransform(const char *v, double *m);

} // namespace Laxkit

#endif
