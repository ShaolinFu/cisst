/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: svlTrackerMSBruteForce.cpp 618 2009-07-31 16:39:42Z bvagvol1 $

  Author(s):  Balazs Vagvolgyi
  Created on: 2007

  (C) Copyright 2006-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#include <cisstStereoVision/svlTrackerMSBruteForce.h>

//#define __DEBUG_TRACKER

#define __NO_TMP     0
#define __NEW_TMP    1
#define __HAS_TMP    2


/*************************/
/*** Helper functions ****/
/*************************/

inline unsigned int sqrt_uint64(unsigned long long value)
{
	unsigned int a, g = 0;
	unsigned int bshft = 31;
	unsigned int b = 1 << 31;
    unsigned long long temp;
    union { unsigned int ui[2]; long long ll; } c;

    do {
        a = g + g + b;
        if (bshft) {
            c.ui[0] = a << bshft;
            c.ui[1] = a >> (32 - bshft);
            temp = c.ll;
        }
        else temp = a;

		if (value >= temp) {     
			g += b;
			value -= temp;
		}
		b >>= 1;
	} while (bshft --);

	return g;
}

inline unsigned int sqrt_uint32(unsigned int value)
{
    unsigned int a, g = 0;
    unsigned int bshft = 15;
    unsigned int b = 1 << bshft;

    do {
        a = (g + g + b) << bshft;
        if (value >= a) {
            g += b;
            value -= a;
        }
        b >>= 1;
    } while (bshft --);

    return g;
}


/*************************************/
/*** svlTrackerMSBruteForce class ****/
/*************************************/

svlTrackerMSBruteForce::svlTrackerMSBruteForce() :
    svlImageTracker(),
    TargetsAdded(false),
    TemplateRadiusRequested(3),
    WindowRadiusRequested(6),
    Metric(svlSAD),
    Scale(1),
    OrigTmpltWeight(255),
    LowerScale(0),
    LowerScaleImage(0),
    TrajectoryFilter(0.0),
    TrajectoryFilterInv(1.0)
{
}

svlTrackerMSBruteForce::~svlTrackerMSBruteForce()
{
    Release();
}

void svlTrackerMSBruteForce::SetParameters(svlErrorMetric metric,
                                           unsigned int templateradius,
                                           unsigned int windowradius,
                                           unsigned int scales,
                                           unsigned char tmplupdweight,
                                           double trajfilter)
{
    Metric = metric;
    WindowRadiusRequested = std::max(windowradius, 1u);
    OrigTmpltWeight = 255 - tmplupdweight;
    TrajectoryFilter = trajfilter;
    TrajectoryFilterInv = 1.0 - TrajectoryFilter;

    if (Initialized) {
        if (LowerScale) {
            LowerScale->SetParameters(Metric,
                                      TemplateRadiusRequested / 2,
                                      WindowRadiusRequested / 2,
                                      Scale - 1,
                                      255 - OrigTmpltWeight,
                                      TrajectoryFilter);
        }
    }
    else {
        TemplateRadiusRequested = std::max(templateradius, 1u);

        if (scales < 1) scales = 1;
        else if (scales > 5) scales = 5;
        Scale = scales;
    }
}

int svlTrackerMSBruteForce::SetTarget(unsigned int targetid, const svlTarget2D & target)
{
    if (targetid >= Targets.size()) return SVL_FAIL;

    if (LowerScale) {
        svlTarget2D ls_target;
        ls_target.used    = target.used;
        ls_target.conf    = target.conf;
        ls_target.pos.x   = (target.pos.x + 1) >> 1;
        ls_target.pos.y   = (target.pos.y + 1) >> 1;

        LowerScale->SetTarget(targetid, ls_target);
    }

    return svlImageTracker::SetTarget(targetid, target);
}

int svlTrackerMSBruteForce::Initialize()
{
    if (Width < 1 || Height < 1) return SVL_FAIL;

    Release();

    unsigned int i, templatesize;
    const unsigned int targetcount = Targets.size();

#ifdef __DEBUG_TRACKER
        std::stringstream __name;
        __name << "Scale " << Scale;
        ScaleName = __name.str();
#endif

    if (Scale > 1) {
        // creating lower scale
        LowerScale = new svlTrackerMSBruteForce();
        // half the image size, scale decremented recursively
        LowerScale->SetParameters(Metric,
                                  TemplateRadiusRequested / 2,
                                  WindowRadiusRequested / 2,
                                  Scale - 1,
                                  255 - OrigTmpltWeight,
                                  TrajectoryFilter);
        // same target count
        LowerScale->SetTargetCount(targetcount);
        // half the image size
        LowerScale->SetImageSize(Width / 2, Height / 2);
        // half the work area
        svlRect roi;
        roi.left   = ROI.left   / 2;
        roi.right  = ROI.right  / 2;
        roi.top    = ROI.top    / 2;
        roi.bottom = ROI.bottom / 2;
        LowerScale->SetROI(roi);
        // initialize
        LowerScale->Initialize();

        // create image for the lower scale
        LowerScaleImage = new svlSampleImageRGB;
        LowerScaleImage->SetSize(Width / 2, Height / 2);

        // modify current parameters for multiscale processing + add some margin
        TemplateRadius = std::max(TemplateRadiusRequested / 2, 2u);
        WindowRadius = 1;
    }
    else {
        // coarsest scale so go by the original parameters
        TemplateRadius = std::max(TemplateRadiusRequested, 2u);
        WindowRadius = std::max(WindowRadiusRequested, 2u);
    }

    templatesize = TemplateRadius * 2 + 1;
    templatesize *= templatesize * 3;

    Templates.SetSize(targetcount);
    OrigTemplates.SetSize(targetcount);
    for (i = 0; i < targetcount; i ++) {
        OrigTemplates[i] = new unsigned char[templatesize];
        Templates[i] = new unsigned char[templatesize];
    }

    OrigTemplateConf.SetSize(targetcount);
    OrigTemplateConf.SetAll(__NO_TMP);

    MatchMap.SetSize(WindowRadius * 2 + 1, WindowRadius * 2 + 1);

    TargetsAdded = false;
    Initialized = true;

    return SVL_OK;
}

void svlTrackerMSBruteForce::ResetTargets()
{
    TargetsAdded = false;
    OrigTemplateConf.SetAll(__NO_TMP);
    if (LowerScale) LowerScale->ResetTargets();
}

int svlTrackerMSBruteForce::PreProcessImage(svlSampleImage & image, unsigned int videoch)
{
    if (!Initialized) return SVL_FAIL;

    // pre-processing image
    if (Scale > 1) {

        // shirinking image for the lower scales recursively
#if (CISST_SVL_HAS_OPENCV == ON)
        cvResize(image.IplImageRef(videoch), LowerScaleImage->IplImageRef(), CV_INTER_AREA);
#else // CISST_SVL_HAS_OPENCV
        ShrinkImage(image.GetUCharPointer(videoch), LowerScaleImage->GetUCharPointer());
#endif // CISST_SVL_HAS_OPENCV

        LowerScale->PreProcessImage(*LowerScaleImage);
    }

    return SVL_OK;
}

int svlTrackerMSBruteForce::Track(svlSampleImage & image, unsigned int videoch)
{
    if (!Initialized) return SVL_FAIL;

    const int leftborder   = ROI.left   + TemplateRadius;
    const int topborder    = ROI.top    + TemplateRadius;
    const int rightborder  = ROI.right  - TemplateRadius;
    const int bottomborder = ROI.bottom - TemplateRadius;
    const unsigned int targetcount = Targets.size();
    const unsigned int scalem1 = Scale - 1;
    int xpre, ypre, x, y;
    svlTarget2D target;
    unsigned char conf;
    unsigned int i;


    // Call lower scales recursively
    if (LowerScale) LowerScale->Track(*LowerScaleImage);


    // Acquire target templates if possible
    for (i = 0; i < targetcount; i ++) {
        if (!Targets[i].used) {
            Targets[i].visible = false;
            continue;
        }

        // Determine target visibility
        x = Targets[i].pos.x;
        y = Targets[i].pos.y;
        if (x < leftborder || x >= rightborder || y < topborder  || y >= bottomborder) {
            Targets[i].visible = false;
            Targets[i].conf    = 0;
            // Skip target if not visible
            continue;
        }
        Targets[i].visible = true;

        // Check if this scale already has a template
        if (OrigTemplateConf[i] == __NO_TMP) {

            // Update this scale's template with the
            // new position estimated by the tracker
            // filter
            CopyTemplate(image.GetUCharPointer(videoch),
                         OrigTemplates[i],
                         Targets[i].pos.x - TemplateRadius,
                         Targets[i].pos.y - TemplateRadius);
            CopyTemplate(image.GetUCharPointer(videoch),
                         Templates[i],
                         Targets[i].pos.x - TemplateRadius,
                         Targets[i].pos.y - TemplateRadius);

            // Set template flag
            OrigTemplateConf[i] = __NEW_TMP;
        }
        else {
            if (LowerScale) {
                // Scale up the tracking results from the
                // lower scale and use that as position
                LowerScale->GetTarget(i, target);
                Targets[i].used    = target.used;
                Targets[i].conf    = target.conf;
                Targets[i].pos.x   = target.pos.x * 2 + 1;
                Targets[i].pos.y   = target.pos.y * 2 + 1;
            }
        }
    }


    // Track targets
    for (i = 0; i < targetcount; i ++) {

        // Skip non-visible targets
        if (!Targets[i].visible) continue;

        // Skip targets with just-acquired templates
        if (OrigTemplateConf[i] == __NEW_TMP) {
            OrigTemplateConf[i] =  __HAS_TMP;
            continue;
        }

        // template matching + updating coordinates
        xpre = Targets[i].pos.x;
        ypre = Targets[i].pos.y;

        if (Scale == 1) {
            if (Metric == svlSAD) {
                MatchTemplateSAD(image.GetUCharPointer(videoch), Templates[i], xpre, ypre);
                GetBestMatch(x, y, Targets[i].conf, false);
            }
            else if (Metric == svlSSD) {
                MatchTemplateSSD(image.GetUCharPointer(videoch), Templates[i], xpre, ypre);
                GetBestMatch(x, y, Targets[i].conf, false);
            }
            else if (Metric == svlNCC) {
                MatchTemplateNCC(image.GetUCharPointer(videoch), Templates[i], xpre, ypre);
                GetBestMatch(x, y, Targets[i].conf, true);
            }
            else return SVL_FAIL;

            Targets[i].pos.x = x + xpre;
            Targets[i].pos.y = y + ypre;

            // trajectory filtering
            Targets[i].pos.x = static_cast<int>(TrajectoryFilter * xpre +
                                                TrajectoryFilterInv * Targets[i].pos.x);
            Targets[i].pos.y = static_cast<int>(TrajectoryFilter * ypre +
                                                TrajectoryFilterInv * Targets[i].pos.y);
        }
        else {
            if (Metric == svlSAD) {
                MatchTemplateSAD(image.GetUCharPointer(videoch), Templates[i], xpre, ypre);
                GetBestMatch(x, y, conf, false);
            }
            else if (Metric == svlSSD) {
                MatchTemplateSSD(image.GetUCharPointer(videoch), Templates[i], xpre, ypre);
                GetBestMatch(x, y, conf, false);
            }
            else if (Metric == svlNCC) {
                MatchTemplateNCC(image.GetUCharPointer(videoch), Templates[i], xpre, ypre);
                GetBestMatch(x, y, conf, true);
            }
            else return SVL_FAIL;

            Targets[i].conf = (static_cast<int>(Targets[i].conf) * scalem1 + conf) / Scale;
            Targets[i].pos.x = x + xpre;
            Targets[i].pos.y = y + ypre;
        }

#ifdef __DEBUG_TRACKER
        cvNamedWindow(ScaleName.c_str(), CV_WINDOW_AUTOSIZE); 
        cvShowImage(ScaleName.c_str(), image.IplImageRef(videoch));
        cvWaitKey(1);
#endif

        // update templates with new tracking results
        UpdateTemplate(image.GetUCharPointer(videoch),
                       OrigTemplates[i],
                       Templates[i],
                       Targets[i].pos.x - TemplateRadius,
                       Targets[i].pos.y - TemplateRadius);
    }

    return SVL_OK;
}

void svlTrackerMSBruteForce::Release()
{
    unsigned int i;

    for (i = 0; i < Templates.size();     i ++) delete [] Templates[i];
    for (i = 0; i < OrigTemplates.size(); i ++) delete [] OrigTemplates[i];

    Templates.SetSize(0);
    OrigTemplates.SetSize(0);

    if (LowerScale) {
        // deletes all the lower scales recursively
        delete LowerScale;
        LowerScale = 0;
    }
    if (LowerScaleImage) {
        delete LowerScaleImage;
        LowerScaleImage = 0;
    }

    Initialized = false;
}

void svlTrackerMSBruteForce::CopyTemplate(unsigned char* img, unsigned char* tmp, unsigned int left, unsigned int top)
{
    const unsigned int imstride = Width * 3;
    const unsigned int tmpheight = TemplateRadius * 2 + 1;
    const unsigned int tmpwidth = tmpheight * 3;
    unsigned char *input = img + imstride * top + left * 3;

    // copy data
    for (unsigned int j = 0; j < tmpheight; j ++) {
        memcpy(tmp, input, tmpwidth);
        input += imstride;
        tmp += tmpwidth;
    }
}

void svlTrackerMSBruteForce::UpdateTemplate(unsigned char* img, unsigned char* origtmp, unsigned char* tmp, unsigned int left, unsigned int top)
{
    if (OrigTmpltWeight == 255) {
        unsigned int tmplsize = TemplateRadius * 2 + 1;
        tmplsize *= tmplsize * 3;
        memcpy(tmp, origtmp, tmplsize);
        return;
    }

    if (OrigTmpltWeight == 0) {
        CopyTemplate(img, tmp, left, top);
        return;
    }

    const unsigned int imstride = Width * 3;
    const unsigned int tmpheight = TemplateRadius * 2 + 1;
    const unsigned int tmpwidth = tmpheight * 3;
    const unsigned int endstride = imstride - tmpwidth;
    const unsigned int origweight = OrigTmpltWeight;
    const unsigned int newweight = 255 - origweight;
    unsigned char *input = img + imstride * top + left * 3;
    unsigned int i, j;

    // update template
    for (j = 0; j < tmpheight; j ++) {
        for (i = 0; i < tmpwidth; i ++) {
            *tmp = static_cast<unsigned char>((origweight * (*origtmp) + newweight * (*input)) >> 8);
            input ++;
            origtmp ++;
            tmp ++;
        }
        input += endstride;
    }
}

void svlTrackerMSBruteForce::MatchTemplateSAD(unsigned char* img, unsigned char* tmp, int x, int y)
{
    const unsigned int imgstride = Width * 3;
    const unsigned int tmpheight = TemplateRadius * 2 + 1;
    const unsigned int tmppixcount = tmpheight * tmpheight;
    const unsigned int tmpstride = imgstride - tmpheight * 3;
    const unsigned int winsize = WindowRadius * 2 + 1;
    const unsigned int imgwinstride = imgstride - winsize * 3;
    const int imgwidth = static_cast<int>(Width);
    const int imgheight = static_cast<int>(Height);

    int k, l, sum, ival, hfrom, vfrom;
    int* map = MatchMap.Pointer();
    unsigned char *timg, *ttmp;
    unsigned int i, j, v, h;

    if (x < static_cast<int>(TemplateRadius)) x = TemplateRadius;
    else if (x >= static_cast<int>(Width - TemplateRadius)) x = Width - TemplateRadius - 1;
    if (y < static_cast<int>(TemplateRadius)) y = TemplateRadius;
    else if (y >= static_cast<int>(Height - TemplateRadius)) y = Height - TemplateRadius - 1;

    hfrom = x - TemplateRadius - WindowRadius;
    vfrom = y - TemplateRadius - WindowRadius;

    k = vfrom * imgstride + hfrom * 3;
    if (k > 0) img += k;
    else img -= -k;
    
    for (v = 0, l = vfrom; v < winsize; v ++, l ++) {
        if (l >= 0 && l < imgheight) {

            for (h = 0, k = hfrom; h < winsize; h ++, k ++) {
                if (k >= 0 && k < imgwidth) {

                    // match in current position
                    timg = img; ttmp = tmp;
                    sum = 0;
                    for (j = 0; j < tmpheight; j ++) {
                        for (i = 0; i < tmpheight; i ++) {
                            ival = (static_cast<int>(*timg) - *ttmp); timg ++; ttmp ++;
                            ival < 0 ? sum -= ival : sum += ival;
                            ival = (static_cast<int>(*timg) - *ttmp); timg ++; ttmp ++;
                            ival < 0 ? sum -= ival : sum += ival;
                            ival = (static_cast<int>(*timg) - *ttmp); timg ++; ttmp ++;
                            ival < 0 ? sum -= ival : sum += ival;
                        }
                        timg += tmpstride;
                    }
                    sum /= tmppixcount;

                    *map = sum + 1; map ++;

                    #ifdef __DEBUG_TRACKER
                        int __res = sum / 30;
                        if (__res > 255) __res = 255;
                        img[0] = img[1] = img[2] = __res;
                    #endif

                }
                else {

                    *map = 0; map ++;

                }

                img += 3;
            }
            img += imgwinstride;

        }
        else {

            memset(map, 0, winsize * sizeof(int)); map += winsize;
            img += imgstride;

        }
    }
}

void svlTrackerMSBruteForce::MatchTemplateSSD(unsigned char* img, unsigned char* tmp, int x, int y)
{
    const unsigned int imgstride = Width * 3;
    const unsigned int tmpheight = TemplateRadius * 2 + 1;
    const unsigned int tmppixcount = tmpheight * tmpheight;
    const unsigned int tmpstride = imgstride - tmpheight * 3;
    const unsigned int winsize = WindowRadius * 2 + 1;
    const unsigned int imgwinstride = imgstride - winsize * 3;
    const int imgwidth = static_cast<int>(Width);
    const int imgheight = static_cast<int>(Height);

    int k, l, sum, ival, hfrom, vfrom;
    int* map = MatchMap.Pointer();
    unsigned char *timg, *ttmp;
    unsigned int i, j, v, h;

    if (x < static_cast<int>(TemplateRadius)) x = TemplateRadius;
    else if (x >= static_cast<int>(Width - TemplateRadius)) x = Width - TemplateRadius - 1;
    if (y < static_cast<int>(TemplateRadius)) y = TemplateRadius;
    else if (y >= static_cast<int>(Height - TemplateRadius)) y = Height - TemplateRadius - 1;

    hfrom = x - TemplateRadius - WindowRadius;
    vfrom = y - TemplateRadius - WindowRadius;

    k = vfrom * imgstride + hfrom * 3;
    if (k > 0) img += k;
    else img -= -k;
    
    for (v = 0, l = vfrom; v < winsize; v ++, l ++) {
        if (l >= 0 && l < imgheight) {

            for (h = 0, k = hfrom; h < winsize; h ++, k ++) {
                if (k >= 0 && k < imgwidth) {

                    // match in current position
                    timg = img; ttmp = tmp;
                    sum = 0;
                    for (j = 0; j < tmpheight; j ++) {
                        for (i = 0; i < tmpheight; i ++) {
                            ival = (static_cast<int>(*timg) - *ttmp); timg ++; ttmp ++;
                            sum += ival * ival;
                            ival = (static_cast<int>(*timg) - *ttmp); timg ++; ttmp ++;
                            sum += ival * ival;
                            ival = (static_cast<int>(*timg) - *ttmp); timg ++; ttmp ++;
                            sum += ival * ival;
                        }
                        timg += tmpstride;
                    }
                    sum /= tmppixcount;

                    *map = sum + 1; map ++;

                    #ifdef __DEBUG_TRACKER
                        int __res = sqrt_uint32(sum / 3) * 10;
                        if (__res > 255) __res = 255;
                        img[0] = img[1] = img[2] = __res;
                    #endif
                    
                }
                else {

                    *map = 0; map ++;

                }

                img += 3;
            }
            img += imgwinstride;

        }
        else {

            memset(map, 0, winsize * sizeof(int)); map += winsize;
            img += imgstride;

        }
    }
}

void svlTrackerMSBruteForce::MatchTemplateNCC(unsigned char* img, unsigned char* tmp, int x, int y)
{
    const unsigned int imgstride = Width * 3;
    const unsigned int tmpheight = TemplateRadius * 2 + 1;
    const unsigned int tmpwidth = tmpheight * 3;
    const unsigned int winsize = WindowRadius * 2 + 1;
    const unsigned int imgwinstride = imgstride - winsize * 3;
    const int imgwidth_m1 = static_cast<int>(Width) - 1;
    const int imgheight_m1 = static_cast<int>(Height) - 1;

    int i, j, k, l, sum, hfrom, vfrom;
    int tmpxfrom, tmpxto, tmpyfrom, tmpyto;
    int tmpstride, tmprowcount, tmpcolcount, tmppixcount;
    int xoffs, yoffs, ioffs;
    int mi1, mi2, mi3, mt1, mt2, mt3;
    int di1, di2, di3, dt1, dt2, dt3;
    int di, dt, cr1, cr2, cr3;
    int* map = MatchMap.Pointer();
    unsigned char *timg, *ttmp;
    unsigned int v, h;

    hfrom = x - TemplateRadius - WindowRadius;
    vfrom = y - TemplateRadius - WindowRadius;

    k = vfrom * imgstride + hfrom * 3;
    if (k > 0) img += k;
    else img -= -k;

    tmpxfrom = tmpyfrom = 0;
    tmpxto = tmpyto = tmpheight;
    tmppixcount = tmpheight * tmpheight;

    // Compute template means 
    ttmp = tmp; mt1 = mt2 = mt3 = 0;
    for (j = tmpyfrom; j < tmpyto; j ++) {
        for (i = tmpxfrom; i < tmpxto; i ++) {
            mt1 += *ttmp; ttmp ++;
            mt2 += *ttmp; ttmp ++;
            mt3 += *ttmp; ttmp ++;
        }
    }
    mt1 /= tmppixcount; mt2 /= tmppixcount; mt3 /= tmppixcount;

    // Compute template standard deviations
    ttmp = tmp; dt1 = dt2 = dt3 = 0;
    for (j = tmpyfrom; j < tmpyto; j ++) {
        for (i = tmpxfrom; i < tmpxto; i ++) {
            dt = static_cast<int>(*ttmp) - mt1; dt1 += dt * dt; ttmp ++;
            dt = static_cast<int>(*ttmp) - mt2; dt2 += dt * dt; ttmp ++;
            dt = static_cast<int>(*ttmp) - mt3; dt3 += dt * dt; ttmp ++;
        }
    }
    dt1 = sqrt_uint32(dt1); dt2 = sqrt_uint32(dt2); dt3 = sqrt_uint32(dt3);
    if (dt1 == 0) dt1 = 1; if (dt2 == 0) dt2 = 1; if (dt3 == 0) dt3 = 1;

    for (v = 0, l = vfrom; v < winsize; v ++, l ++) {

        yoffs = 0;
        tmpyfrom = l;
        if (tmpyfrom < 0) {
            tmpyfrom = 0;
            yoffs = -tmpyfrom;
        }
        tmpyto = l + tmpheight - 1;
        if (tmpyto > imgheight_m1) {
            tmpyto = imgheight_m1;
        }
        tmprowcount = tmpyto - tmpyfrom + 1;

        if (tmprowcount > 0) {

            for (h = 0, k = hfrom; h < winsize; h ++, k ++) {

                xoffs = 0;
                tmpxfrom = k;
                if (tmpxfrom < 0) {
                    tmpxfrom = 0;
                    xoffs = -tmpxfrom;
                }
                tmpxto = k + tmpheight - 1;
                if (tmpxto > imgwidth_m1) {
                    tmpxto = imgwidth_m1;
                }
                tmpcolcount = tmpxto - tmpxfrom + 1;

                if (tmpcolcount > 0) {

                    tmpstride = imgstride - tmpcolcount * 3;
                    tmppixcount = tmprowcount * tmpcolcount;

                    xoffs *= 3;
                    ioffs = yoffs * imgstride + xoffs;

                    // Compute image means
                    timg = img + ioffs;
                    mi1 = mi2 = mi3 = 0;
                    for (j = tmpyfrom; j <= tmpyto; j ++) {
                        for (i = tmpxfrom; i <= tmpxto; i ++) {
                            mi1 += *timg; timg ++;
                            mi2 += *timg; timg ++;
                            mi3 += *timg; timg ++;
                        }
                        timg += tmpstride;
                    }
                    mi1 /= tmppixcount; mi2 /= tmppixcount; mi3 /= tmppixcount;

                    // Compute image standard deviations and correlations
                    timg = img + ioffs;
                    ttmp = tmp + yoffs * tmpwidth + xoffs;
                    cr1 = cr2 = cr3 = 0;
                    di1 = di2 = di3 = 0;
                    for (j = tmpyfrom; j <= tmpyto; j ++) {
                        for (i = tmpxfrom; i <= tmpxto; i ++) {
                            di = static_cast<int>(*timg) - mi1; di1 += di * di; timg ++;
                            dt = static_cast<int>(*ttmp) - mt1;                 ttmp ++;
                            cr1 += di * dt;
                            di = static_cast<int>(*timg) - mi2; di2 += di * di; timg ++;
                            dt = static_cast<int>(*ttmp) - mt2;                 ttmp ++;
                            cr2 += di * dt;
                            di = static_cast<int>(*timg) - mi3; di3 += di * di; timg ++;
                            dt = static_cast<int>(*ttmp) - mt3;                 ttmp ++;
                            cr3 += di * dt;
                        }
                        timg += tmpstride;
                    }
                    di1 = sqrt_uint32(di1); di2 = sqrt_uint32(di2); di3 = sqrt_uint32(di3);

                    if (di1 != 0) sum  = (cr1 << 8) / (di1 * dt1); else sum  = (cr1 << 8);
                    if (di2 != 0) sum += (cr2 << 8) / (di2 * dt2); else sum += (cr2 << 8);
                    if (di3 != 0) sum += (cr3 << 8) / (di3 * dt3); else sum += (cr3 << 8);

                    *map = sum + 1; map ++;

                    #ifdef __DEBUG_TRACKER
                        int __res = sum / 10 + 128;
                        if (__res > 255) __res = 255;
                        img[0] = img[1] = img[2] = __res;
                    #endif
                    
                }
                else {

                    *map = 0; map ++;

                }

                img += 3;
            }
            img += imgwinstride;

        }
        else {

            memset(map, 0, winsize * sizeof(int)); map += winsize;
            img += imgstride;

        }
    }
}

void svlTrackerMSBruteForce::GetBestMatch(int &x, int &y, unsigned char &conf, bool higherbetter)
{
    const int size = WindowRadius * 2 + 1;
    const int size2 = size * size;
    int i, j, t, avrg, stdev, best, best_x = 0, best_y = 0;
    int* map = MatchMap.Pointer();

    // Compute average match and best match
    avrg = 0;
    if (higherbetter) {
        best = 0x80000000;
        for (j = 0; j < size; j ++) {
            for (i = 0; i < size; i ++) {
                t = *map; map ++;

                if (t > best) {
                    best = t;
                    best_x = i;
                    best_y = j;
                }

                avrg += t;
            }
        }
    }
    else {
        best = 0x7FFFFFFF;
        for (j = 0; j < size; j ++) {
            for (i = 0; i < size; i ++) {
                t = *map; map ++;

                if (t < best) {
                    best = t;
                    best_x = i;
                    best_y = j;
                }

                avrg += t;
            }
        }
    }
    avrg /= size2;
    x = best_x - WindowRadius;
    y = best_y - WindowRadius;

    // Compute standard deviation
    stdev = 0;
    map = MatchMap.Pointer();
    for (i = 0; i < size2; i ++) {
        t = *map - avrg; map ++;
        stdev += t * t;
    }
    stdev /= size2;
    if (stdev > 0) stdev = sqrt_uint32(stdev);
    else stdev = 1;

    // Compute confidence
    if (best > avrg) best -= avrg;
    else best = avrg - best;
    if (stdev > 0) best = (best << 6) / stdev;
    else best = 0;
    if (best < 0) best = 0;
    else if (best > 255) best = 255;
    conf = static_cast<unsigned char>(best);
}

void svlTrackerMSBruteForce::ShrinkImage(unsigned char* src, unsigned char* dst)
{
    const unsigned int smw = Width / 2;
    const unsigned int smh = Height / 2;
    const unsigned int lgstride = Width * 3;
    const unsigned int lgstride2 = lgstride * 2;

    unsigned char *srcln1, *srcln2, *src1, *src2;
    unsigned int i, j, r, g, b;

    srcln1 = src;
    srcln2 = src + lgstride;

    // update template
    for (j = 0; j < smh; j ++) {
        src1 = srcln1;
        src2 = srcln2;
        for (i = 0; i < smw; i ++) {
            r = *src1 + *src2;
            src1 ++; src2 ++;
            g = *src1 + *src2;
            src1 ++; src2 ++;
            b = *src1 + *src2;
            src1 ++; src2 ++;

            r += *src1 + *src2;
            src1 ++; src2 ++;
            g += *src1 + *src2;
            src1 ++; src2 ++;
            b += *src1 + *src2;
            src1 ++; src2 ++;

            *dst = r >> 2;
            dst ++;
            *dst = g >> 2;
            dst ++;
            *dst = b >> 2;
            dst ++;
        }
        srcln1 += lgstride2;
        srcln2 += lgstride2;
    }
}

