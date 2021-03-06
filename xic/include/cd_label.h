
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef CD_LABEL_H
#define CD_LABEL_H

//-------------------------------------------------------------------------
// Primitive label type
//-------------------------------------------------------------------------

struct hyList;

// label element
struct Label
{
    Label()
        {
            label = 0;
            x = y = 0;
            width = height = 0;
            xform = 0;
        }

    Label(hyList *str, int xx, int yy, int w, int h, int xf)
        {
            label = str;
            x = xx; y = yy;
            width = w;
            height = h;
            xform = xf;
        }

    void computeBB(BBox*);
    bool intersect(const BBox*, bool);

    static void TransformLabelBB(int, BBox*, Point**);
    static void InvTransformLabelBB(int, BBox*, Point*);

    hyList *label;
    int x, y, width, height;
    int xform;
};

// Code for xform field, see graphics.h
// bits  action
// 0-1   0-no rotation, 1-90, 2-180, 3-270
// 2     mirror y after rotation
// 3     mirror x after rotation and mirror y
// 4     shift rotation to 45, 135, 225, 315
// 5-6   horiz justification 00,11 left, 01 center, 10 right
// 7-8   vert justification 00,11 bottom, 01 center, 10 top
// 9-10  font number

#endif

