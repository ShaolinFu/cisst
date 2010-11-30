/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$
  
  Author(s):  Balazs Vagvolgyi
  Created on: 2010

  (C) Copyright 2006-2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#include <cisstStereoVision/svlDraw.h>
#include "svlDrawHelper.h"


/*****************************/
/*** svlDraw namespace *******/
/*****************************/

void svlDraw::Pixel(svlSampleImage* image,
                    unsigned int videoch,
                    svlPoint2D pos,
                    svlRGB color)
{
    Pixel(image, videoch, pos.x, pos.y, color.r, color.g, color.b);
}

void svlDraw::Pixel(svlSampleImage* image,
                    unsigned int videoch,
                    int x,
                    int y,
                    unsigned char r,
                    unsigned char g,
                    unsigned char b)
{
    if (!image || videoch >= image->GetVideoChannels() ||
        x < 0 || x >= static_cast<int>(image->GetWidth(videoch)) ||
        y < 0 || y >= static_cast<int>(image->GetHeight(videoch))) return;

    unsigned char* img = image->GetUCharPointer(videoch, x, y);

    *img = b; img ++;
    *img = g; img ++;
    *img = r;
}

void svlDraw::Rectangle(svlSampleImage* image,
                        unsigned int videoch,
                        svlRect rect,
                        svlRGB color,
                        bool fill)
{
    Rectangle(image, videoch, rect.left, rect.top, rect.right, rect.bottom, color.r, color.g, color.b, fill);
}

void svlDraw::Rectangle(svlSampleImage* image,
                        unsigned int videoch,
                        int left,
                        int top,
                        int right,
                        int bottom,
                        unsigned char r,
                        unsigned char g,
                        unsigned char b,
                        bool fill)
{
    if (!image || videoch >= image->GetVideoChannels()) return;

    const int width  = image->GetWidth(videoch);
    const int height = image->GetHeight(videoch);
    int i, j, fromx, fromy, tox, toy, endstride;
    unsigned char* img;

    if (fill) {
        fromx = left;
        if (fromx < 0) fromx = 0;
        fromy = top;
        if (fromy < 0) fromy = 0;
        tox = right;
        if (tox > width) tox = width;
        toy = bottom;
        if (toy > height) toy = height;

        img = image->GetUCharPointer(videoch, fromx, fromy);
        endstride = (width - (tox - fromx)) * 3;

        for (j = fromy; j < toy; j ++) {
            for (i = fromx; i < tox; i ++) {
                *img = b; img ++;
                *img = g; img ++;
                *img = r; img ++;
            }
            img += endstride;
        }
    }
    else {
        Line(image, videoch, left, top, right - 1, top, r, g, b);
        Line(image, videoch, left, top, left, bottom - 1, r, g, b);
        Line(image, videoch, right - 1, top, right - 1, bottom - 1, r, g, b);
        Line(image, videoch, left, bottom - 1, right - 1, bottom - 1, r, g, b);
    }
}

void svlDraw::Line(svlSampleImage* image,
                   unsigned int videoch,
                   svlPoint2D from,
                   svlPoint2D to,
                   svlRGB color)
{
    Line(image, videoch, from.x, from.y, to.x, to.y, color.r, color.g, color.b);
}

void svlDraw::Line(svlSampleImage* image,
                   unsigned int videoch,
                   int from_x,
                   int from_y,
                   int to_x,
                   int to_y,
                   unsigned char r,
                   unsigned char g,
                   unsigned char b)
{
    if (!image || videoch >= image->GetVideoChannels()) return;

    const int width  = image->GetWidth(videoch);
    const int height = image->GetHeight(videoch);
    unsigned char *tbuf, *img = image->GetUCharPointer(videoch);

    if (from_x == to_x && from_y == to_y) {
        if (from_x >= 0 && from_x < width &&
            from_y >= 0 && from_y < height) {
            tbuf = img + (from_y * width + from_x) * 3;
            *tbuf = b; tbuf ++;
            *tbuf = g; tbuf ++;
            *tbuf = r;
        }
        return;
    }

    int x, y, eps = 0;

    if (to_x <= from_x) {
        x = to_x;
        to_x = from_x;
        from_x = x;
        y = to_y;
        to_y = from_y;
        from_y = y;
    }

    int dx = to_x - from_x;
    int dy = to_y - from_y;

    y = from_y;
    x = from_x;

    if (dy > 0) {
        if (dx >= dy) {
            for (x = from_x; x <= to_x; x ++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    tbuf = img + (y * width + x) * 3;
                    *tbuf = b; tbuf ++;
                    *tbuf = g; tbuf ++;
                    *tbuf = r;
                }
                eps += dy;
                if ((eps << 1) >= dx) {
                    y ++;
                    eps -= dx;
                }
            }
        }
        else {
            for (y = from_y; y <= to_y; y ++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    tbuf = img + (y * width + x) * 3;
                    *tbuf = b; tbuf ++;
                    *tbuf = g; tbuf ++;
                    *tbuf = r;
                }
                eps += dx;
                if ((eps << 1) >= dy) {
                    x ++;
                    eps -= dy;
                }
            }
        }
    }
    else {
        if (dx >= abs(dy)) {
            for (x = from_x; x <= to_x; x ++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    tbuf = img + (y * width + x) * 3;
                    *tbuf = b; tbuf ++;
                    *tbuf = g; tbuf ++;
                    *tbuf = r;
                }
                eps += dy;
                if ((eps << 1) <= -dx) {
                    y --;
                    eps += dx;
                }
            }
        }
        else {
            for (y = from_y; y >= to_y; y --) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    tbuf = img + (y * width + x) * 3;
                    *tbuf = b; tbuf ++;
                    *tbuf = g; tbuf ++;
                    *tbuf = r;
                }
                eps += dx;
                if ((eps << 1) >= -dy) {
                    x ++;
                    eps -= -dy;
                }
            }
        }
    }
}

void svlDraw::Triangle(svlSampleImage* image,
                       unsigned int videoch,
                       svlPoint2D corner1,
                       svlPoint2D corner2,
                       svlPoint2D corner3,
                       svlRGB color,
                       svlDraw::Internals& internals)
{
    Triangle(image, videoch, corner1.x, corner1.y, corner2.x, corner2.y, corner3.x, corner3.y, color, internals);
}

void svlDraw::Triangle(svlSampleImage* image,
                       unsigned int videoch,
                       int x1, int y1,
                       int x2, int y2,
                       int x3, int y3,
                       svlRGB color,
                       svlDraw::Internals& internals)
{
    svlDrawHelper::TriangleInternals* triangledrawer = dynamic_cast<svlDrawHelper::TriangleInternals*>(internals.Get());
    if (triangledrawer == 0) {
        triangledrawer = new svlDrawHelper::TriangleInternals;
        internals.Set(triangledrawer);
    }
    if (!triangledrawer->SetImage(image, videoch)) return;

    triangledrawer->Draw(x1, y1, x2, y2, x3, y3, color);
}

void svlDraw::Poly(svlSampleImage* image,
                   unsigned int videoch,
                   const vctDynamicVectorRef<svlPoint2D> points,
                   svlRGB color,
                   unsigned int start)
{
    if (!image || videoch >= image->GetVideoChannels()) return;

    const unsigned int size = points.size();

    if (size < 1) return;
    if (size == 1) Pixel(image, videoch, points[0], color);

    unsigned int i, end = start + 1;

    for (i = 1; i < size; i ++) {
        if (end >= size) end = 0;
        Line(image, videoch, points[start], points[end], color);
        start = end;
        end ++;
    }
}

#if CISST_SVL_HAS_OPENCV

void svlDraw::Ellipse(svlSampleImage* image,
                      unsigned int videoch,
                      svlPoint2D center,
                      vctInt2 radii,
                      svlRGB color,
                      double from_angle,
                      double to_angle,
                      double rotation,
                      int thickness)
{
    if (!image || videoch >= image->GetVideoChannels()) return;

    cvEllipse(image->IplImageRef(videoch),
              cvPoint(center.x, center.y),
              cvSize(radii[0], radii[1]),
              rotation,
              from_angle,
              to_angle,
              cvScalar(color.r, color.g, color.b),
              thickness);
}

#else // CISST_SVL_HAS_OPENCV

void svlDraw::Ellipse(svlSampleImage* CMN_UNUSED(image),
                      unsigned int CMN_UNUSED(videoch),
                      svlPoint2D CMN_UNUSED(center),
                      vctInt2 CMN_UNUSED(radii),
                      svlRGB CMN_UNUSED(color),
                      double CMN_UNUSED(from_angle),
                      double CMN_UNUSED(to_angle),
                      double CMN_UNUSED(rotation),
                      int CMN_UNUSED(thickness))
{
    // To be implemented
}

#endif // CISST_SVL_HAS_OPENCV

void svlDraw::Crosshair(svlSampleImage* image,
                        unsigned int videoch,
                        svlPoint2D pos,
                        svlRGB color,
                        unsigned int radius)
{
    Crosshair(image, videoch, pos.x, pos.y, color.r, color.g, color.g, radius);
}

void svlDraw::Crosshair(svlSampleImage* image,
                        unsigned int videoch,
                        int x,
                        int y,
                        unsigned char r,
                        unsigned char g,
                        unsigned char b,
                        unsigned int radius)
{
    if (!image || videoch >= image->GetVideoChannels()) return;

    const int in_rad = radius / 3 + 1;
    svlPoint2D from, to;
    svlRGB color(r, g, b);

    from.Assign(x - radius, y);
    to.Assign(x - in_rad, y);
    Line(image, videoch, from, to, color);

    from.Assign(x + in_rad, y);
    to.Assign(x + radius, y);
    Line(image, videoch, from, to, color);

    from.Assign(x, y - radius);
    to.Assign(x, y - in_rad);
    Line(image, videoch, from, to, color);

    from.Assign(x, y + in_rad);
    to.Assign(x, y + radius);
    Line(image, videoch, from, to, color);
}

#if CISST_SVL_HAS_OPENCV

void svlDraw::Text(svlSampleImage* image,
                   unsigned int videoch,
                   svlPoint2D pos,
                   const std::string & text,
                   double fontsize,
                   svlRGB color)
{
    if (!image || videoch >= image->GetVideoChannels()) return;
    
    CvFont font;
    cvInitFont(&font,
               CV_FONT_HERSHEY_PLAIN,
               fontsize / SVL_OCV_FONT_SCALE,
               fontsize / SVL_OCV_FONT_SCALE,
               0, 1, 4);

    cvPutText(image->IplImageRef(videoch),
              text.c_str(),
              cvPoint(pos.x, pos.y),
              &font,
              cvScalar(color.r, color.g, color.b));
}

#else // CISST_SVL_HAS_OPENCV

void svlDraw::Text(svlSampleImage* CMN_UNUSED(image),
                   unsigned int CMN_UNUSED(videoch),
                   svlPoint2D CMN_UNUSED(pos),
                   const std::string & CMN_UNUSED(text),
                   double CMN_UNUSED(fontsize),
                   svlRGB CMN_UNUSED(color))
{
    // To be implemented
}

#endif // CISST_SVL_HAS_OPENCV

void svlDraw::Text(svlSampleImage* image,
                   unsigned int videoch,
                   int x,
                   int y,
                   const std::string & text,
                   double fontsize,
                   unsigned char r,
                   unsigned char g,
                   unsigned char b)
{
    Text(image, videoch, svlPoint2D(x, y), text, fontsize, svlRGB(r, g, b));
}


/********************************/
/*** svlDraw::Internals class ***/
/********************************/

svlDraw::Internals::Internals() :
    Ptr(0)
{
}

svlDraw::Internals::~Internals()
{
    Release();
}

svlDrawInternals* svlDraw::Internals::Get()
{
    return Ptr;
}

void svlDraw::Internals::Set(svlDrawInternals* ib)
{
    Release();
    Ptr = ib;
}

void svlDraw::Internals::Release()
{
    if (Ptr) delete Ptr;
    Ptr = 0;
}

