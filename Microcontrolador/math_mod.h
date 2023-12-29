////////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2003 Custom Computer Services            ////
//// This source code may only be used by licensed users of the CCS C   ////
//// compiler.  This source code may only be distributed to other       ////
//// licensed users of the CCS C compiler.  No other use, reproduction  ////
//// or distribution is permitted without written permission.           ////
//// Derivative programs created using this software in object code     ////
//// form are not restricted in any way.                                ////
////////////////////////////////////////////////////////////////////////////
////                                                                    ////
//// History:                                                           ////
////  * 9/20/2001 :  Improvments are made to sin/cos code.              ////
////                 The code now is small, much faster,                ////
////                 and more accurate.                                 ////
////                                                                    ////
////////////////////////////////////////////////////////////////////////////

#ifndef MATH_H
#define MATH_H



//////////////////// Exponential and logarithmic functions ////////////////////


/************************************************************/

float const pl[4] = {0.45145214, -9.0558803, 26.940971, -19.860189};
float const ql[4] = {1.0000000,  -8.1354259, 16.780517, -9.9300943};

#define LN2 0.6931471806

////////////////////////////////////////////////////////////////////////////
//	float log(float x)
////////////////////////////////////////////////////////////////////////////
// Description : returns the the natural log of x
// Date : N/A
//
float log(float x)
{
   float y, res, r, y2;
   signed n;
   #ifdef _ERRNO
   if(x <0)
   {
      errno=EDOM;
   }
   if(x ==0)
   {
      errno=ERANGE;
      return(0);
   }
   #endif
   y = x;

   if (y != 1.0)
   {
      *(&y) = 0x7E;

      y = (y - 1.0)/(y + 1.0);

      y2=y*y;

      res = pl[0]*y2 + pl[1];
      res = res*y2 + pl[2];
      res = res*y2 + pl[3];

      r = ql[0]*y2 + ql[1];
      r = r*y2 + ql[2];
      r = r*y2 + ql[3];

      res = y*res/r;

      n = *(&x) - 0x7E;

      if (n<0)
         r = -(float)-n;
      else
         r = (float)n;

      res += r*LN2;
   }

   else
      res = 0.0;

   return(res);
}


#define LN10 2.30258509

////////////////////////////////////////////////////////////////////////////
//	float log10(float x)
////////////////////////////////////////////////////////////////////////////
// Description : returns the the log base 10 of x
// Date : N/A
//
float log10(float x)
{
   float r;

   r = log(x);
   r = r/LN10;
   return(r);
}
