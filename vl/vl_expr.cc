
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
 * vl -- Verilog Simulator and Verilog support library.                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"

// set this to machine bit width
#define DefBits (8*(int)sizeof(int))


//---------------------------------------------------------------------------
//  Exports
//---------------------------------------------------------------------------

#define CX_INCR 100

// A vl_var factory
//
vl_var &
vl_new_var(CXmode cx)
{
    static int da_numused;
    static struct bl { vl_var block[CX_INCR]; bl *next; } *da_blocks;

    if (cx == CXclear) {
        da_numused = 0;
        while (da_blocks) {
            bl *b = da_blocks;
            da_blocks = b->next;
            delete b;
        }
        return ((vl_var&)*(vl_var*)0); // ignore this
    }
    if (da_numused == 0) {
        da_blocks = new bl;
        da_blocks->next = 0;
        da_numused = 1;
    }
    else if (da_numused == CX_INCR) {
        bl *b = new bl;
        b->next = da_blocks;
        da_blocks = b;
        da_numused = 1;
    }
    else
        da_numused++;
    return (da_blocks->block[da_numused - 1]);
}


//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

inline int max(int x, int y) { return (x > y ? x : y); }
inline int min(int x, int y) { return (x < y ? x : y); }

// Return status of i'th bit of integer
//
inline int
bit(int i, int pos)
{
    if ((i >> pos) & 1)  
        return (BitH);
    return (BitL);
}


// Return status of i'th bit of vl_time_t
//
inline int
bit(vl_time_t t, int pos)
{
    if ((t >> pos) & 1)  
        return (BitH);
    return (BitL);
}


// AND logical operation
//
inline int
op_and(int a, int b)
{
    if (a == BitH && b == BitH)
        return (BitH);
    if (a == BitL || b == BitL)
        return (BitL);
    return (BitDC);
}


// OR logical operation
//
inline int
op_or(int a, int b)
{
    if (a == BitH || b == BitH)
        return (BitH);
    if (a == BitL && b == BitL)
        return (BitL);
    return (BitDC);
}


// XOR logical operation
//
inline int
op_xor(int a, int b)
{
    if ((a == BitH && b == BitL) || (a == BitL && b == BitH))
        return (BitH);
    if ((a == BitH && b == BitH) || (a == BitL && b == BitL))
        return (BitL);
    return (BitDC);
}


static void
add_bits(char *s0, char *s1, char *s2, int w0, int w1, int w2,
    bool setc = false)
{
    int carry = setc ? 1 : 0;
    for (int i = 0; i < w0; i++) {
        int a = 0;
        if (i >= w1) {
            if (carry) {
                a = 1;
                carry = 0;
            }
        }
        else
            a = s1[i];

        int b = 0;
        if (i >= w2) {
            if (carry) {
                b = 1;
                carry = 0;
            }
        }
        else
            b = s2[i];

        if (a == BitDC || a == BitZ || b == BitDC || b == BitZ) {
            memset(&s0[i], BitDC, w0 - i);
            return;
        }
        int c = a + b + carry;
        s0[i] = (c & 1) ? BitH : BitL;
        carry = (c & 2) ? 1 : 0;
    }
}


//---------------------------------------------------------------------------
//  Arithmetic and logical operator overloads
//---------------------------------------------------------------------------

// Overload '*'
//
vl_var &
operator*(vl_var &data1, vl_var &data2)
{
    int w1 = 1;
    if (data1.data_type == Dbit)
        w1 = data1.bits.size;
    else if (data1.data_type == Dint)
        w1 = DefBits;
    else if (data1.data_type == Dtime)
        w1 = 8*sizeof(vl_time_t);

    int w2 = 1;
    if (data2.data_type == Dbit)
        w2 = data2.bits.size;
    else if (data2.data_type == Dint)
        w2 = DefBits;
    else if (data2.data_type == Dtime)
        w2 = 8*sizeof(vl_time_t);

    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dstring || data2.data_type == Dstring)
        d.setx(w1 + w2);
    else if ((data1.data_type == Dbit && data1.is_x()) ||
            (data2.data_type == Dbit && data2.is_x()))
        d.setx(w1 + w2);
    else if (data1.data_type == Dreal || data2.data_type == Dreal) {
        d.data_type = Dreal;
        d.u.r = (double)data1 * (double)data2;
    }
    else if (data1.data_type == Dtime || data2.data_type == Dtime) {
        d.data_type = Dtime;
        d.u.t = (vl_time_t)data1 * (vl_time_t)data2;
    }
    else {
        d.data_type = Dint;
        if (data1.data_type == Dbit || data2.data_type == Dbit)
            d.u.i = (int)((unsigned)data1 * (unsigned)data2);
        else
            d.u.i = (int)data1 * (int)data2;
    }
    return (d);
}


// Overload '/'
//
vl_var &
operator/(vl_var &data1, vl_var &data2)
{
    int w1 = 1;
    if (data1.data_type == Dbit)
        w1 = data1.bits.size;
    else if (data1.data_type == Dint)
        w1 = DefBits;
    else if (data1.data_type == Dtime)
        w1 = 8*sizeof(vl_time_t);

    int w2 = 1;
    if (data2.data_type == Dbit)
        w2 = data2.bits.size;
    else if (data2.data_type == Dint)
        w2 = DefBits;
    else if (data2.data_type == Dtime)
        w2 = 8*sizeof(vl_time_t);

    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dstring || data2.data_type == Dstring)
        d.setx(max(w1, w2));
    else if ((data1.data_type == Dbit && data1.is_x()) ||
            (data2.data_type == Dbit && data2.is_x()))
        d.setx(max(w1, w2));
    else if ((int)data2 == 0)
        d.setx(max(w1, w2));
    else if (data1.data_type == Dreal || data2.data_type == Dreal) {
        d.data_type = Dreal;
        d.u.r = (double)data1 / (double)data2;
    }
    else if (data1.data_type == Dtime || data2.data_type == Dtime) {
        d.data_type = Dtime;
        d.u.t = (vl_time_t)data1 / (vl_time_t)data2;
    }
    else {
        d.data_type = Dint;
        if (data1.data_type == Dbit || data2.data_type == Dbit)
            d.u.i = (int)((unsigned)data1 / (unsigned)data2);
        else
            d.u.i = (int)data1 / (int)data2;
    }
    return (d);
}


// Overload '%'
//
vl_var &
operator%(vl_var &data1, vl_var &data2)
{
    int w1 = 1;
    if (data1.data_type == Dbit)
        w1 = data1.bits.size;
    else if (data1.data_type == Dint)
        w1 = DefBits;
    else if (data1.data_type == Dtime)
        w1 = 8*sizeof(vl_time_t);

    int w2 = 1;
    if (data2.data_type == Dbit)
        w2 = data2.bits.size;
    else if (data2.data_type == Dint)
        w2 = DefBits;
    else if (data2.data_type == Dtime)
        w2 = 8*sizeof(vl_time_t);

    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dstring || data2.data_type == Dstring)
        d.setx(max(w1, w2));
    else if ((data1.data_type == Dbit && data1.is_x()) ||
            (data2.data_type == Dbit && data2.is_x()))
        d.setx(max(w1, w2));
    else if ((int)data2 == 0)
        d.setx(max(w1, w2));
    else if (data1.data_type == Dreal || data2.data_type == Dreal)
        d.setx(max(w1, w2));
    else if (data1.data_type == Dtime || data2.data_type == Dtime) {
        d.data_type = Dtime;
        d.u.t = (vl_time_t)data1 % (vl_time_t)data2;
    }
    else {
        d.data_type = Dint;
        if (data1.data_type == Dbit || data2.data_type == Dbit)
            d.u.i = (int)((unsigned)data1 % (unsigned)data2);
        else
            d.u.i = (int)data1 % (int)data2;
    }
    return (d);
}


// Overload '+' (binary)
//
vl_var &
operator+(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dstring || data2.data_type == Dstring)
        return (d);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint) {
            d.data_type = Dint;
            d.u.i = data1.u.i + data2.u.i;
        }
        else if (data2.data_type == Dbit)
            d.addb(data2, data1.u.i);
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.i + data2.u.t;
        }
        else if (data2.data_type == Dreal) {
            d.data_type = Dreal;
            d.u.r = data1.u.i + data2.u.r;
        }
    }
    else if (data1.data_type == Dbit) {
        if (data2.data_type == Dint)
            d.addb(data1, data2.u.i);
        else if (data2.data_type == Dbit)
            d.addb(data1, data2);
        else if (data2.data_type == Dtime)
            d.addb(data1, data2.u.t);
        else if (data2.data_type == Dreal) {
            if (data1.is_x())
                d.setx(1);
            else {
                d.data_type = Dreal;
                d.u.r = (double)data1 + data2.u.r;
            }
        }
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint) {
            d.data_type = Dtime;
            d.u.t = data1.u.t + data2.u.i;
        }
        else if (data2.data_type == Dbit)
            d.addb(data2, data1.u.t);
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.t + data2.u.t;
        }
        else if (data2.data_type == Dreal) {
            d.data_type = Dreal;
            d.u.r = data1.u.t + data2.u.r;
        }
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint) {
            d.data_type = Dreal;
            d.u.r = data1.u.r + data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                d.setx(1);
            else {
                d.data_type = Dreal;
                d.u.r = data1.u.r + (double)data2;
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dreal;
            d.u.r = data1.u.r + data2.u.t;
        }
        else if (data2.data_type == Dreal) {
            d.data_type = Dreal;
            d.u.r = data1.u.r + data2.u.r;
        }
    }
    return (d);
}


// Overload '-' (binary)
//
vl_var &
operator-(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dstring || data2.data_type == Dstring)
        return (d);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint) {
            d.data_type = Dint;
            d.u.i = data1.u.i - data2.u.i;
        }
        else if (data2.data_type == Dbit)
            d.subb(data1.u.i, data2);
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.i - data2.u.t;
        }
        else if (data2.data_type == Dreal) {
            d.data_type = Dreal;
            d.u.r = data1.u.i - data2.u.r;
        }
    }
    else if (data1.data_type == Dbit) {
        if (data2.data_type == Dint)
            d.subb(data1, data2.u.i);
        else if (data2.data_type == Dbit)
            d.subb(data1, data2);
        else if (data2.data_type == Dtime)
            d.subb(data1, data2.u.t);
        else if (data2.data_type == Dreal) {
            if (data1.is_x())
                d.setx(1);
            else {
                d.data_type = Dreal;
                d.u.r = (double)data1 - data2.u.r;
            }
        }
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint) {
            d.data_type = Dtime;
            d.u.t = data1.u.t - data2.u.i;
        }
        else if (data2.data_type == Dbit)
            d.subb(data1.u.t, data2);
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.t - data2.u.t;
        }
        else if (data2.data_type == Dreal) {
            d.data_type = Dreal;
            d.u.r = data1.u.t - data2.u.r;
        }
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint) {
            d.data_type = Dreal;
            d.u.r = data1.u.r - data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                d.setx(1);
            else {
                d.data_type = Dreal;
                d.u.r = data1.u.r - (double)data2;
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dreal;
            d.u.r = data1.u.r - data2.u.t;
        }
        else if (data2.data_type == Dreal) {
            d.data_type = Dreal;
            d.u.r = data1.u.r - data2.u.r;
        }
    }
    return (d);
}


// Overload '-' (unary)
//
vl_var &
operator-(vl_var &data1)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dint) {
        d.data_type = Dint;
        d.u.i = -data1.u.i;
    }
    if (data1.data_type == Dtime) {
        d.data_type = Dtime;
        d.u.t = -data1.u.t;
    }
    else if (data1.data_type == Dbit) {
        d.bits.set(DefBits);
        d.u.s = new char[DefBits];
        d.subb((int)0, data1);
    }
    else if (data1.data_type == Dreal) {
        d.data_type = Dreal;
        d.u.r = -data1.u.r;
    }
    return (d);
}


//
// Shift operators
//

// Overload '<<' for vl_var
//
vl_var &
operator<<(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data2.data_type == Dbit && data2.is_x())
        d.setx(data1.bits.size);
    else {
        int shift = (int)data2;
        if (shift < 0)
            shift = -shift;
        int bw;
        char *s = data1.bit_elt(0, &bw);
        d.data_type = Dbit;
        d.bits.set(bw + shift);
        d.u.s = new char[d.bits.size];
        for (int i = 0; i < d.bits.size; i++) {
            if (i >= shift)
                d.u.s[i] = s[i-shift];
            else
                d.u.s[i] = BitL;
        }
    }
    return (d);
}


// Overload '<<' for int
//
vl_var &
operator<<(vl_var &data1, int shift)
{
    vl_var &d = vl_new_var(CXalloc);
    if (shift < 0)
        shift = -shift;
    int bw;
    char *s = data1.bit_elt(0, &bw);
    d.data_type = Dbit;
    d.bits.set(bw);
    d.u.s = new char[d.bits.size];
    for (int i = 0; i < d.bits.size; i++) {
        if (i >= shift)
            d.u.s[i] = s[i-shift];
        else
            d.u.s[i] = BitL;
    }
    return (d);
}


// Overload '>>' for vl_var
//
vl_var &
operator>>(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data2.data_type == Dbit && data2.is_x())
        d.setx(data1.bits.size);
    else {
        int shift = (int)data2;
        if (shift < 0)
            shift = -shift;
        int bw;
        char *s = data1.bit_elt(0, &bw);
        d.data_type = Dbit;
        d.bits.set(bw);
        d.u.s = new char[d.bits.size];
        for (int i = 0; i < d.bits.size; i++) {
            if (i + shift < bw)
                d.u.s[i] = s[i+shift];
            else
                d.u.s[i] = BitL;
        }
    }
    return (d);
}


// Overload '>>' for int
//
vl_var &
operator>>(vl_var &data1, int shift)
{
    vl_var &d = vl_new_var(CXalloc);
    if (shift < 0)
        shift = -shift;
    int bw;
    char *s = data1.bit_elt(0, &bw);
    d.data_type = Dbit;
    d.bits.set(bw - shift);
    d.u.s = new char[d.bits.size];
    for (int i = 0; i < d.bits.size; i++) {
        if (i + shift < bw)
            d.u.s[i] = s[i+shift];
        else
            d.u.s[i] = BitL;
    }
    return (d);
}


//
// Equality operators
//

// In general, the overloaded functions are used only in vl_expr::eval(),
// so that the arguments are never Dconcat.  However, the equality
// operators may be used elsewhere for general equality testing, so that
// Dconcat handling is necessary.

// Overload '=='
//
vl_var &
operator==(vl_var &data1, vl_var &data2)
{
    if (data1.data_type == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = (ex.eval() == data2);
        ex.ux.mcat.var = 0;
        return (d);
    }
    if (data2.data_type == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = (data2 == ex.eval());
        ex.ux.mcat.var = 0;
        return (d);
    }

    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i == data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = BitL;
            int i, i1 = data1.u.i;
            for (i = 0; i < data2.bits.size; i++)
                if (bit(i1, i) != data2.u.s[i])
                    return (d);
            if (data2.bits.size != DefBits) {
                if (data2.bits.size > DefBits) {
                    for (i = DefBits; i < data2.bits.size; i++)
                        if (data2.u.s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data2.bits.size; i < DefBits; i++)
                        if (bit(i1, i) != BitL)
                            return (d);
                }
            }
            d.u.s[0] = BitH;
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1.u.i == data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = ((unsigned)data1.u.i == data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        if (data1.is_x())
            return (d);
        int i2;
        if (data2.data_type == Dint)
            i2 = data2.u.i;
        else if (data2.data_type == Dbit) {

            if (data2.is_x())
                return (d);
            d.u.s[0] = BitL;
            int wd = min(data1.bits.size, data2.bits.size);
            int i;
            for (i = 0; i < wd; i++)
                if (data1.u.s[i] != data2.u.s[i])
                    return (d);
            if (data1.bits.size != data2.bits.size) {
                if (data1.bits.size > data2.bits.size) {
                    for (i = data2.bits.size; i < data1.bits.size; i++)
                        if (data1.u.s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data1.bits.size; i < data2.bits.size; i++)
                        if (data2.u.s[i] != BitL)
                            return (d);
                }
            }
            d.u.s[0] = BitH;
            return (d);
        }
        else if (data2.data_type == Dtime)
            i2 = (int)data2.u.t;
        else if (data2.data_type == Dreal)
            i2 = (int)data2.u.r;
        else
            return (d);
        d.u.s[0] = BitL;
        int i;
        for (i = 0; i < data1.bits.size; i++)
            if (data1.u.s[i] != bit(i2, i))
                return (d);
        if (data1.bits.size != DefBits) {
            if (data1.bits.size > DefBits) {
                for (i = DefBits; i < data1.bits.size; i++)
                    if (data1.u.s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits.size; i < DefBits; i++)
                    if (bit(i2, i) != BitL)
                        return (d);
            }
        }
        d.u.s[0] = BitH;
        return (d);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t == (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = BitL;
            int i, i1 = (int)data1.u.t;
            for (i = 0; i < data2.bits.size; i++)
                if (bit(i1, i) != data2.u.s[i])
                    return (d);
            if (data2.bits.size != DefBits) {
                if (data2.bits.size > DefBits) {
                    for (i = DefBits; i < data2.bits.size; i++)
                        if (data2.u.s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data2.bits.size; i < DefBits; i++)
                        if (bit(i1, i) != BitL)
                            return (d);
                }
            }
            d.u.s[0] = BitH;
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t == data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t == data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r == data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = BitL;
            int i, i1 = (int)data1.u.r;
            for (i = 0; i < data2.bits.size; i++)
                if (bit(i1, i) != data2.u.s[i])
                    return (d);
            if (data2.bits.size != DefBits) {
                if (data2.bits.size > DefBits) {
                    for (i = DefBits; i < data2.bits.size; i++)
                        if (data2.u.s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data2.bits.size; i < DefBits; i++)
                        if (bit(i1, i) != BitL)
                            return (d);
                }
            }
            d.u.s[0] = BitH;
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r == data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r == data2.u.r ? BitH : BitL);
    }
    return (d);
}


// Overload '!='
//
vl_var &
operator!=(vl_var &data1, vl_var &data2)
{
    vl_var &d = (data1 == data2);
    if (d.u.s[0] == BitL)
        d.u.s[0] = BitH;
    else if (d.u.s[0] == BitH)
        d.u.s[0] = BitL;
    return (d);
}


// Case equal operation
//
vl_var &
case_eq(vl_var &data1, vl_var &data2)
{
    if (data1.data_type == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = case_eq(ex.eval(), data2);
        ex.ux.mcat.var = 0;
        return (d);
    }
    if (data2.data_type == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = case_eq(data2, ex.eval());
        ex.ux.mcat.var = 0;
        return (d);
    }

    if (data1.data_type == Dbit && data2.data_type == Dbit) {
        vl_var &d = vl_new_var(CXalloc);
        d.setx(1);
        d.u.s[0] = BitL;
        int wd = min(data1.bits.size, data2.bits.size);
        int i;
        for (i = 0; i < wd; i++)
            if (data1.u.s[i] != data2.u.s[i])
                return (d);
        if (data1.bits.size != data2.bits.size) {
            if (data1.bits.size > data2.bits.size) {
                for (i = data2.bits.size; i < data1.bits.size; i++)
                    if (data1.u.s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits.size; i < data2.bits.size; i++)
                    if (data2.u.s[i] != BitL)
                        return (d);
            }
        }
        d.u.s[0] = BitH;
        return (d);
    }
    else {
        vl_var &d = operator==(data1, data2);
        if (d.u.s[0] == BitDC)
            d.u.s[0] = BitL;
        return (d);
    }
}


// Casex equal operation
//
vl_var &
casex_eq(vl_var &data1, vl_var &data2)
{
    if (data1.data_type == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = casex_eq(ex.eval(), data2);
        ex.ux.mcat.var = 0;
        return (d);
    }
    if (data2.data_type == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = casex_eq(data2, ex.eval());
        ex.ux.mcat.var = 0;
        return (d);
    }

    if (data1.data_type == Dbit && data2.data_type == Dbit) {
        vl_var &d = vl_new_var(CXalloc);
        d.setx(1);
        d.u.s[0] = BitL;
        int wd = min(data1.bits.size, data2.bits.size);
        int i;
        for (i = 0; i < wd; i++) {
            if (data1.u.s[i] == BitDC || data2.u.s[i] == BitDC)
                continue;
            if (data1.u.s[i] == BitZ || data2.u.s[i] == BitZ)
                continue;
            if (data1.u.s[i] != data2.u.s[i])
                return (d);
        }
        if (data1.bits.size != data2.bits.size) {
            if (data1.bits.size > data2.bits.size) {
                for (i = data2.bits.size; i < data1.bits.size; i++)
                    if (data1.u.s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits.size; i < data2.bits.size; i++)
                    if (data2.u.s[i] != BitL)
                        return (d);
            }
        }
        d.u.s[0] = BitH;
        return (d);
    }
    else {
        vl_var &d = operator==(data1, data2);
        if (d.u.s[0] == BitDC)
            d.u.s[0] = BitL;
        return (d);
    }
}


// Casez equal operation
//
vl_var &
casez_eq(vl_var &data1, vl_var &data2)
{
    if (data1.data_type == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = casez_eq(ex.eval(), data2);
        ex.ux.mcat.var = 0;
        return (d);
    }
    if (data2.data_type == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = casez_eq(data2, ex.eval());
        ex.ux.mcat.var = 0;
        return (d);
    }

    if (data1.data_type == Dbit && data2.data_type == Dbit) {
        vl_var &d = vl_new_var(CXalloc);
        d.setx(1);
        d.u.s[0] = BitL;
        int wd = min(data1.bits.size, data2.bits.size);
        int i;
        for (i = 0; i < wd; i++) {
            if (data1.u.s[i] == BitZ || data2.u.s[i] == BitZ)
                continue;
            if (data1.u.s[i] != data2.u.s[i])
                return (d);
        }
        if (data1.bits.size != data2.bits.size) {
            if (data1.bits.size > data2.bits.size) {
                for (i = data2.bits.size; i < data1.bits.size; i++)
                    if (data1.u.s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits.size; i < data2.bits.size; i++)
                    if (data2.u.s[i] != BitL)
                        return (d);
            }
        }
        d.u.s[0] = BitH;
        return (d);
    }
    else {
        vl_var &d = operator==(data1, data2);
        if (d.u.s[0] == BitDC)
            d.u.s[0] = BitL;
        return (d);
    }
}


// Case not equal operation
//
vl_var &
case_neq(vl_var &data1, vl_var &data2)
{
    vl_var &d = case_eq(data1, data2);
    if (d.u.s[0] == BitL)
        d.u.s[0] = BitH;
    else
        d.u.s[0] = BitL;
    return (d);
}


//
// Logical operators
//

// Overload '&&'
//
vl_var &
operator&&(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i && data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.u.i;
            if (i1 && (x2 & Hmask))
                d.u.s[0] = BitH;
            else if (!i1 || (x2 & Lmask))
                d.u.s[0] = BitL;
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.i && data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.i && data2.u.r != 0.0 ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        int x1 = data1.bitset(); 
        int i2;
        if (data2.data_type == Dint)
            i2 = data2.u.i;
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            if ((x1 & Hmask) && (x2 & Hmask))
                d.u.s[0] = BitH;
            else if ((x1 & Lmask) || (x2 & Lmask))
                d.u.s[0] = BitL;
            return (d);
        }
        else if (data2.data_type == Dtime)
            i2 = data2.u.t != 0 ? 1 : 0;
        else if (data2.data_type == Dreal)
            i2 = data2.u.r != 0.0 ? 1 : 0;
        else
            return (d);
        if ((x1 & Hmask) && i2)
            d.u.s[0] = BitH;
        else if ((x1 & Lmask) || !i2)
            d.u.s[0] = BitL;
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t && data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.u.t != 0 ? 1 : 0;
            if (i1 && (x2 & Hmask))
                d.u.s[0] = BitH;
            else if (!i1 || (x2 & Lmask))
                d.u.s[0] = BitL;
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t && data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t && data2.u.r != 0.0 ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r != 0.0 && data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.u.r != 0.0 ? 1 : 0;
            if (i1 && (x2 & Hmask))
                d.u.s[0] = BitH;
            else if (!i1 || (x2 & Lmask))
                d.u.s[0] = BitL;
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r != 0.0 && data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r != 0.0 && data2.u.r != 0.0 ? BitH : BitL);
    }
    return (d);
}


// Overload '||'
//
vl_var &
operator||(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i || data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.u.i;
            if (i1 || (x2 & Hmask))
                d.u.s[0] = BitH;
            else if (!i1 && (x2 & Lmask))
                d.u.s[0] = BitL;
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.i || data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.i || data2.u.r != 0.0 ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        int x1 = data1.bitset(); 
        int i2;
        if (data2.data_type == Dint)
            i2 = data2.u.i;
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            if ((x1 & Hmask) || (x2 & Hmask))
                d.u.s[0] = BitH;
            else if ((x1 & Lmask) && (x2 & Lmask))
                d.u.s[0] = BitL;
            return (d);
        }
        else if (data2.data_type == Dtime)
            i2 = data2.u.t != 0 ? 1 : 0;
        else if (data2.data_type == Dreal)
            i2 = data2.u.r != 0.0 ? 1 : 0;
        else
            return (d);
        if ((x1 & Hmask) || i2)
            d.u.s[0] = BitH;
        else if ((x1 & Lmask) && !i2)
            d.u.s[0] = BitL;
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t || data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.u.t != 0 ? 1 : 0;
            if (i1 || (x2 & Hmask))
                d.u.s[0] = BitH;
            else if (!i1 && (x2 & Lmask))
                d.u.s[0] = BitL;
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t || data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t || data2.u.r != 0.0 ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r != 0.0 || data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.u.r != 0.0 ? 1 : 0;
            if (i1 || (x2 & Hmask))
                d.u.s[0] = BitH;
            else if (!i1 && (x2 & Lmask))
                d.u.s[0] = BitL;
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r != 0.0 || data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r != 0.0 || data2.u.r != 0.0 ? BitH : BitL);
    }
    return (d);
}


// Overload '!'
//
vl_var &
operator!(vl_var &data1)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint)
        d.u.s[0] = data1.u.i ? BitL : BitH;
    else if (data1.data_type == Dbit) {
        int x1 = data1.bitset();
        if (x1 & Hmask)
            d.u.s[0] = BitL;
        else if (x1 & Lmask)
            d.u.s[0] = BitH;
    }
    else if (data1.data_type == Dtime)
        d.u.s[0] = data1.u.t ? BitL : BitH;
    else if (data1.data_type == Dreal)
        d.u.s[0] = data1.u.r != 0.0 ? BitL : BitH;
    return (d);
}


// Reduction operation
//
vl_var &
reduce(vl_var &data1, int oper)
{
    int bw;
    char *s = data1.bit_elt(0, &bw);
    int xx = s[0];
    for (int i = 1; i < bw; i++) {
        switch (oper) {
        case UnandExpr:
        case UandExpr:
            xx = op_and(xx, s[i]);
            break;
        case UnorExpr:
        case UorExpr:
            xx = op_or(xx, s[i]);
            break;
        case UxnorExpr:
        case UxorExpr:
            xx = op_xor(xx, s[i]);
            break;
        default:
            xx = BitL;
            vl_error("(internal) in reduce, bad reduction op");
            data1.simulator->abort();
        }
    }
    if (oper == UnandExpr || oper == UnorExpr || oper == UxnorExpr) {
        if (xx == BitL)
            xx = BitH;
        else if (xx == BitH)
            xx = BitL;
    }
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    d.u.s[0] = xx;
    return (d);
}


//
// Relational operators
//

// Overload '<'
//
vl_var &
operator<(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i < data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1.u.i < (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1.u.i < data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.i < data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type == Dint)
            d.u.s[0] = ((unsigned)data1 < (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1 < (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1 < data2.u.t ? BitH : BitL);
        if (data2.data_type == Dreal)
            d.u.s[0] = ((unsigned)data1 < data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t < (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.t < (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t < data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t < data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r < data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.r < (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r < data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r < data2.u.r ? BitH : BitL);
    }
    return (d);
}


// Overload '<='
//
vl_var &
operator<=(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i <= data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1.u.i <= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1.u.i <= data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.i <= data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type == Dint)
            d.u.s[0] = ((unsigned)data1 <= (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1 <= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1 <= data2.u.t ? BitH : BitL);
        if (data2.data_type == Dreal)
            d.u.s[0] = ((unsigned)data1 <= data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t <= (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.t <= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t <= data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t <= data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r <= data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.r <= (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r <= data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r <= data2.u.r ? BitH : BitL);
    }
    return (d);
}


// Overload '>'
//
vl_var &
operator>(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i > data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1.u.i > (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1.u.i > data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.i > data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type == Dint)
            d.u.s[0] = ((unsigned)data1 > (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1 > (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1 > data2.u.t ? BitH : BitL);
        if (data2.data_type == Dreal)
            d.u.s[0] = ((unsigned)data1 > data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t > (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.t > (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t > data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t > data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r > data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.r > (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r > data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r > data2.u.r ? BitH : BitL);
    }
    return (d);
}


// Overload '>='
//
vl_var &
operator>=(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    d.setx(1);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.i >= data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1.u.i >= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1.u.i >= data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.i >= data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type == Dint)
            d.u.s[0] = ((unsigned)data1 >= (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = ((unsigned)data1 >= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = ((unsigned)data1 >= data2.u.t ? BitH : BitL);
        if (data2.data_type == Dreal)
            d.u.s[0] = ((unsigned)data1 >= data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.t >= (unsigned)data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.t >= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.t >= data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.t >= data2.u.r ? BitH : BitL);
    }
    else if (data1.data_type == Dreal) {
        if (data2.data_type == Dint)
            d.u.s[0] = (data1.u.r >= data2.u.i ? BitH : BitL);
        else if (data2.data_type == Dbit) {
            if (data2.is_x())
                return (d);
            d.u.s[0] = (data1.u.r >= (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type == Dtime)
            d.u.s[0] = (data1.u.r >= data2.u.t ? BitH : BitL);
        else if (data2.data_type == Dreal)
            d.u.s[0] = (data1.u.r >= data2.u.r ? BitH : BitL);
    }
    return (d);
}


//
// Bitwise logical operators
//

// Overload '&'
//
vl_var &
operator&(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint) {
            d.data_type = Dint;
            d.u.i = data1.u.i & data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data2.bits.size, DefBits));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data2.bits.size)
                    d.u.s[i] = op_and(data2.u.s[i], bit(data1.u.i, i));
                else
                    d.u.s[i] = BitL;
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.i & data2.u.t;
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dbit) {
        if (data2.data_type == Dint) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, DefBits));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data1.bits.size)
                    d.u.s[i] = op_and(data1.u.s[i], bit(data2.u.i, i));
                else
                    d.u.s[i] = BitL;
            }
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, data2.bits.size));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                int i1 = (i < data1.bits.size ? data1.u.s[i] : (int)BitL);
                int i2 = (i < data2.bits.size ? data2.u.s[i] : (int)BitL);
                d.u.s[i] = op_and(i1, i2);
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, 8*sizeof(vl_time_t)));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data1.bits.size)
                    d.u.s[i] = op_and(data1.u.s[i], bit(data2.u.t, i));
                else
                    d.u.s[i] = BitL;
            }
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint) {
            d.data_type = Dtime;
            d.u.t = data1.u.t & data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data2.bits.size, 8*sizeof(vl_time_t)));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data2.bits.size)
                    d.u.s[i] = op_and(data2.u.s[i], bit(data1.u.t, i));
                else
                    d.u.s[i] = BitL;
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.t & data2.u.t;
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dreal)
        d.setx(1);
    return (d);
}


// Overload '|'
//
vl_var &
operator|(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint) {
            d.data_type = Dint;
            d.u.i = data1.u.i | data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data2.bits.size, DefBits));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data2.bits.size)
                    d.u.s[i] = op_or(data2.u.s[i], bit(data1.u.i, i));
                else
                    d.u.s[i] = bit(data1.u.i, i);
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.i | data2.u.t;
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dbit) {
        if (data2.data_type == Dint) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, DefBits));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data1.bits.size)
                    d.u.s[i] = op_or(data1.u.s[i], bit(data2.u.i, i));
                else
                    d.u.s[i] = bit(data2.u.i, i);
            }
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, data2.bits.size));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                int i1 = (i < data1.bits.size ? data1.u.s[i] : (int)BitL);
                int i2 = (i < data2.bits.size ? data2.u.s[i] : (int)BitL);
                d.u.s[i] = op_or(i1, i2);
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, 8*sizeof(vl_time_t)));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data1.bits.size)
                    d.u.s[i] = op_or(data1.u.s[i], bit(data2.u.t, i));
                else
                    d.u.s[i] = bit(data2.u.t, i);
            }
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint) {
            d.data_type = Dtime;
            d.u.t = data1.u.t | data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data2.bits.size, 8*sizeof(vl_time_t)));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data2.bits.size)
                    d.u.s[i] = op_or(data2.u.s[i], bit(data1.u.t, i));
                else
                    d.u.s[i] = bit(data1.u.t, i);
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.t | data2.u.t;
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dreal)
        d.setx(1);
    return (d);
}


// Overload '^'
//
vl_var &
operator^(vl_var &data1, vl_var &data2)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dint) {
        if (data2.data_type == Dint) {
            d.data_type = Dint;
            d.u.i = data1.u.i ^ data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data2.bits.size, DefBits));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data2.bits.size)
                    d.u.s[i] = op_xor(data2.u.s[i], bit(data1.u.i, i));
                else
                    d.u.s[i] = op_xor(BitL, bit(data1.u.i, i));
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.i ^ data2.u.t;
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dbit) {
        if (data2.data_type == Dint) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, DefBits));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data1.bits.size)
                    d.u.s[i] = op_xor(data1.u.s[i], bit(data2.u.i, i));
                else
                    d.u.s[i] = op_xor(BitL, bit(data2.u.i, i));
            }
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, data2.bits.size));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                int i1 = (i < data1.bits.size ? data1.u.s[i] : (int)BitL);
                int i2 = (i < data2.bits.size ? data2.u.s[i] : (int)BitL);
                d.u.s[i] = op_xor(i1, i2);
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dbit;
            d.bits.set(max(data1.bits.size, 8*sizeof(vl_time_t)));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data1.bits.size)
                    d.u.s[i] = op_xor(data1.u.s[i], bit(data2.u.t, i));
                else
                    d.u.s[i] = op_xor(BitL, bit(data2.u.t, i));
            }
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dtime) {
        if (data2.data_type == Dint) {
            d.data_type = Dtime;
            d.u.t = data1.u.t ^ data2.u.i;
        }
        else if (data2.data_type == Dbit) {
            d.data_type = Dbit;
            d.bits.set(max(data2.bits.size, 8*sizeof(vl_time_t)));
            d.u.s = new char[d.bits.size];
            for (int i = 0; i < d.bits.size; i++) {
                if (i < data2.bits.size)
                    d.u.s[i] = op_xor(data2.u.s[i], bit(data1.u.t, i));
                else
                    d.u.s[i] = op_xor(BitL, bit(data1.u.t, i));
            }
        }
        else if (data2.data_type == Dtime) {
            d.data_type = Dtime;
            d.u.t = data1.u.t ^ data2.u.t;
        }
        else if (data2.data_type == Dreal)
            d.setx(1);
    }
    else if (data1.data_type == Dreal)
        d.setx(1);
    return (d);
}


// Overload '~'
//
vl_var &
operator~(vl_var &data1)
{
    vl_var &d = vl_new_var(CXalloc);
    if (data1.data_type == Dint) {
        d.data_type = Dint;
        int w = 0;
        int c = data1.u.i;
        while (c) {
            c >>= 1;
            w++;
        }
        if (!w)
            w = 1;
        int mask = 0;
        while (w) {
            mask <<= 1;
            mask |= 1;
            w--;
        }
        d.u.i = data1.u.i ^ mask;
    }
    else if (data1.data_type == Dbit) {
        d.data_type = Dbit;
        d.bits.set(data1.bits.size);
        d.u.s = new char[d.bits.size];
        for (int i = 0; i < d.bits.size; i++) {
            if (data1.u.s[i] == BitDC || data1.u.s[i] == BitZ)
                d.u.s[i] = BitDC;
            else if (data1.u.s[i] == BitH)
                d.u.s[i] = BitL;
            else
                d.u.s[i] = BitH;
        }
    }
    else if (data1.data_type == Dtime) {
        d.data_type = Dtime;
        int w = 0;
        vl_time_t c = data1.u.t;
        while (c) {
            c >>= 1;
            w++;
        }
        if (!w)
            w = 1;
        vl_time_t mask = 0;
        while (w) {
            mask <<= 1;
            mask |= 1;
            w--;
        }
        d.u.t = data1.u.t ^ mask;
    }
    else if (data1.data_type == Dreal)
        d.setx(1);
    return (d);
}


// Evaluate the tri-conditional data1 ? e1 : e2
//
vl_var &
tcond(vl_var &data1, vl_expr *e1, vl_expr *e2)
{
    int xx = BitL;
    if (data1.data_type == Dbit) {
        char *s = data1.array.size ? *(char**)data1.u.d : data1.u.s;
        for (int i = 0; i < data1.bits.size; i++) {
            if (s[i] == BitH)
                xx = BitH;
            else if (s[i] == BitDC || s[i] == BitZ) {
                xx = BitDC;
                break;
            }
        }
    }
    else if (data1.data_type == Dint)
        xx = data1.u.i ? BitH : BitL;
    else if (data1.data_type == Dtime)
        xx = data1.u.t ? BitH : BitL;
    else if (data1.data_type == Dreal)
        xx = data1.u.r != 0.0 ? BitH : BitL;

    if (xx == BitH)
        return (e1->eval());
    else if (xx == BitL)
        return (e2->eval());

    vl_var &d = vl_new_var(CXalloc);
    vl_var &d1 = e1->eval();
    int w1;
    if (d1.data_type == Dbit)
        w1 = d1.bits.size;
    else if (d1.data_type == Dint)
        w1 = sizeof(int)*8;
    else if (d1.data_type == Dtime)
        w1 = sizeof(vl_time_t)*8;
    else {
        d.set((int)0);
        return (d);
    }

    vl_var &d2 = e2->eval();
    int w2;
    if (d2.data_type == Dbit)
        w2 = d2.bits.size;
    else if (d2.data_type == Dint)
        w2 = sizeof(int)*8;
    else if (d2.data_type == Dtime)
        w2 = sizeof(vl_time_t)*8;
    else {
        d.set((int)0);
        return (d);
    }

    if (w2 > w1)
        w1 = w2;
    d.setx(w1);

    for (int i = 0; i < w1; i++) {
        int b1 = BitL;
        if (d1.data_type == Dbit)
            b1 = i < d1.bits.size ? d1.u.s[i] : (int)BitL;
        else if (d1.data_type == Dint)
            b1 = i < (int)sizeof(int)*8 ? bit(d1.u.i, i) : (int)BitL;
        else if (d1.data_type == Dtime)
            b1 = i < (int)sizeof(vl_time_t)*8 ?
                (((d1.u.t >> i) & 1) ? BitH : BitL) : (int)BitL;
        int b2 = BitL;
        if (d2.data_type == Dbit)
            b2 = i < d2.bits.size ? d2.u.s[i] : (int)BitL;
        else if (d2.data_type == Dint)
            b2 = i < (int)sizeof(int)*8 ? bit(d2.u.i, i) : (int)BitL;
        else if (d2.data_type == Dtime)
            b2 = i < (int)sizeof(vl_time_t)*8 ?
                (((d2.u.t >> i) & 1) ? BitH : BitL) : (int)BitL;

        if (b1 == b2)
            d.u.s[i] = (b1 != BitZ ? b1 : BitDC);
        else
            d.u.s[i] = BitDC;
    }
    return (d);
}


//---------------------------------------------------------------------------
//  Data variables and expressions
//---------------------------------------------------------------------------

void
vl_var::addb(vl_var &data1, int ival)
{
    setb(ival);
    int w2 = bits.size;
    char *s2 = u.s;
    bits.size = max(data1.bits.size, w2);
    bits.size++;
    u.s = new char[bits.size];
    data_type = Dbit;
    add_bits(u.s, data1.u.s, s2, bits.size, data1.bits.size, w2);
    delete [] s2;
    if (u.s[bits.size-1] != BitH)
        bits.size--;
    bits.hi_index = bits.size-1;
}


void
vl_var::addb(vl_var &data1, vl_time_t tval)
{
    sett(tval);
    int w2 = bits.size;
    char *s2 = u.s;
    bits.size = max(data1.bits.size, w2);
    bits.size++;
    u.s = new char[bits.size];
    data_type = Dbit;
    add_bits(u.s, data1.u.s, s2, bits.size, data1.bits.size, w2);
    delete [] s2;
    if (u.s[bits.size-1] != BitH)
        bits.size--;
    bits.hi_index = bits.size-1;
}


void
vl_var::addb(vl_var &data1, vl_var &data2)
{
    bits.size = max(data1.bits.size, data2.bits.size);
    bits.size++;
    u.s = new char[bits.size];
    data_type = Dbit;
    add_bits(u.s, data1.u.s, data2.u.s, bits.size, data1.bits.size,
        data2.bits.size);
    if (u.s[bits.size-1] != BitH)
        bits.size--;
    bits.hi_index = bits.size-1;
}


void
vl_var::subb(vl_var &data1, int ival)
{
    setb(ival);
    int w2 = bits.size;
    char *s2 = u.s;
    bits.set(max(data1.bits.size, w2));
    u.s = new char[bits.size];
    data_type = Dbit;

    for (int i = 0; i < w2; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(u.s, data1.u.s, s2, bits.size, data1.bits.size, w2, true);
    delete [] s2;
}


void
vl_var::subb(int ival, vl_var &data2)
{
    setb(ival);
    int w1 = bits.size;
    char *s1 = u.s;
    bits.set(max(data2.bits.size, w1));
    u.s = new char[bits.size];
    data_type = Dbit;

    char *s2 = new char[data2.bits.size];
    memcpy(s2, data2.u.s, data2.bits.size);
    for (int i = 0; i < data2.bits.size; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(u.s, s1, s2, bits.size, w1, data2.bits.size, true);
    delete [] s1;
    delete [] s2;
}


void
vl_var::subb(vl_var &data1, vl_time_t tval)
{
    sett(tval);
    int w2 = bits.size;
    char *s2 = u.s;
    bits.set(max(data1.bits.size, w2));
    u.s = new char[bits.size];
    data_type = Dbit;

    for (int i = 0; i < w2; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(u.s, data1.u.s, s2, bits.size, data1.bits.size, w2, true);
    delete [] s2;
}


void
vl_var::subb(vl_time_t tval, vl_var &data2)
{
    sett(tval);
    int w1 = bits.size;
    char *s1 = u.s;
    bits.set(max(data2.bits.size, w1));
    u.s = new char[bits.size];
    data_type = Dbit;

    char *s2 = new char[data2.bits.size];
    memcpy(s2, data2.u.s, data2.bits.size);
    for (int i = 0; i < data2.bits.size; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(u.s, s1, s2, bits.size, w1, data2.bits.size, true);
    delete [] s1;
    delete [] s2;
}


void
vl_var::subb(vl_var &data1, vl_var &data2)
{
    data_type = Dbit;
    bits.set(max(data1.bits.size, data2.bits.size));
    u.s = new char[bits.size];

    char *s2 = new char[data2.bits.size];
    memcpy(s2, data2.u.s, data2.bits.size);
    for (int i = 0; i < data2.bits.size; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(u.s, data1.u.s, s2, bits.size, data1.bits.size, data2.bits.size,
        true);
    delete [] s2;
}
// End of vl_var functions


// Expressions
//
// The vl_expr class is derived from the vl_var class, and can
// represent all of the various expression types, according to the
// etype field.  The vl_expr can serve as an input (i.e., can be
// assigned to) or as an output.  As an input, the following etypes
// are valid:
//
//  IDExpr                simple variable reference
//  BitSelExpr            bit-select reference
//  PartSelExpr           part-select reference
//  ConcatExpr            concatenation (NOT multiple)
//
// For each of the first three input types, the ux.ide.var field
// contains a pointer to the vl_var referenced, which is the variable
// that will be assigned to.  Calling the eval() function establishes
// the ux.ide.var pointer.  For ConcatExpr, the source is the
// ux.mcat.var field, which is created with the vl_expr, and contains
// the list of participating variables.
//
// All other etypes are read-only.  After calling eval(), the "this"
// vl_var contains the resulting value.


vl_expr::vl_expr()
{
    ux.exprs.e1 = 0;
    ux.exprs.e2 = 0;
    ux.exprs.e3 = 0;
}


vl_expr::vl_expr(vl_var *v)
{
    ux.exprs.e1 = 0;
    ux.exprs.e2 = 0;
    ux.exprs.e3 = 0;
    if (v->data_type == Dconcat) {
        etype = ConcatExpr;
        ux.mcat.var = v;
    }
    else {
        etype = IDExpr;
        ux.ide.name = vl_strdup(v->name);
        ux.ide.var = v;
    }
}


vl_expr::vl_expr(short t, int i, double r, void *p1, void *p2, void *p3)
{
    etype = t;
    ux.exprs.e1 = 0;
    ux.exprs.e2 = 0;
    ux.exprs.e3 = 0;
    switch (t) {
    case BitExpr:
        set((bitexp_parse*)p1);
        break;
    case IntExpr:
        set(i);
        break;
    case RealExpr:
        set(r);
        break;
    case StringExpr:
        set((char*)p1);
        break;
    case IDExpr:
        ux.ide.name = (char*)p1;
        break;
    case BitSelExpr:
    case PartSelExpr:
        ux.ide.name = (char*)p1;
        ux.ide.range = (vl_range*)p2;
        break;
    case ConcatExpr:
        ux.mcat.rep = (vl_expr*)p2;
        ux.mcat.var = new vl_var(0, 0, (lsList<vl_expr*>*)p1);
        break;
    case MinTypMaxExpr:
        ux.exprs.e1 = (vl_expr*)p1;
        ux.exprs.e2 = (vl_expr*)p2;
        ux.exprs.e3 = (vl_expr*)p3;
        break;
    case FuncExpr:
        ux.func_call.name = (char*)p1;
        ux.func_call.args = (lsList<vl_expr*>*)p2;
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        ux.exprs.e1 = (vl_expr*)p1;
        break;
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        ux.exprs.e1 = (vl_expr*)p1;
        ux.exprs.e2 = (vl_expr*)p2;
        break;
    case TcondExpr:
        ux.exprs.e1 = (vl_expr*)p1;
        ux.exprs.e2 = (vl_expr*)p2;
        ux.exprs.e3 = (vl_expr*)p3;
        break;
    case SysExpr:
        ux.systask = new vl_sys_task_stmt((char*)p2, (lsList<vl_expr*>*)p1);
        break;
    }
}


vl_expr::~vl_expr()
{
    switch (etype) {
    case IDExpr:
        delete [] ux.ide.name;
        break;
    case BitSelExpr:
    case PartSelExpr:
        delete [] ux.ide.name;
        delete ux.ide.range;
        break;
    case ConcatExpr:
        delete ux.mcat.rep;
        delete ux.mcat.var;
        break;                        
    case MinTypMaxExpr:
        delete ux.exprs.e1;
        delete ux.exprs.e2;
        delete ux.exprs.e3;
        break;                        
    case FuncExpr:
        delete [] ux.func_call.name;
        delete_list(ux.func_call.args);
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        delete ux.exprs.e1;
        break;
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        delete ux.exprs.e1;
        delete ux.exprs.e2;
        break;
    case TcondExpr:
        delete ux.exprs.e1;
        delete ux.exprs.e2;
        delete ux.exprs.e3;
        break;
    case SysExpr:
        delete ux.systask;
        break;
    }
}


vl_expr *
vl_expr::copy()
{
    vl_expr *retval = new vl_expr;
    retval->etype = etype;

    switch (etype) {
    case BitExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
        retval->assign(0, this, 0);
        break;
    case IDExpr:
        retval->ux.ide.name = vl_strdup(ux.ide.name);
        break;
    case BitSelExpr:
    case PartSelExpr:
        retval->ux.ide.name = vl_strdup(ux.ide.name);
        retval->ux.ide.range = ux.ide.range->copy();
        break;
    case ConcatExpr: {
        if (ux.mcat.rep)
            retval->ux.mcat.rep = ux.mcat.rep->copy();
        if (ux.mcat.var)
            retval->ux.mcat.var = ux.mcat.var->copy();
        break;                        
    }
    case MinTypMaxExpr:
        if (ux.exprs.e1)
            retval->ux.exprs.e1 = ux.exprs.e1->copy();
        if (ux.exprs.e2)
            retval->ux.exprs.e2 = ux.exprs.e2->copy();
        if (ux.exprs.e3)
            retval->ux.exprs.e3 = ux.exprs.e3->copy();
        break;                        
    case FuncExpr:
        retval->ux.func_call.name = vl_strdup(ux.func_call.name);
        retval->ux.func_call.args = copy_list(ux.func_call.args);
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        if (ux.exprs.e1)
            retval->ux.exprs.e1 = ux.exprs.e1->copy();
        break;
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        if (ux.exprs.e1)
            retval->ux.exprs.e1 = ux.exprs.e1->copy();
        if (ux.exprs.e2)
            retval->ux.exprs.e2 = ux.exprs.e2->copy();
        break;
    case TcondExpr:
        if (ux.exprs.e1)
            retval->ux.exprs.e1 = ux.exprs.e1->copy();
        if (ux.exprs.e2)
            retval->ux.exprs.e2 = ux.exprs.e2->copy();
        if (ux.exprs.e3)
            retval->ux.exprs.e3 = ux.exprs.e3->copy();
        break;
    case SysExpr:
        retval->ux.systask = ux.systask->copy();
        break;
    }
    return (retval);
}


// Create a new variable if it doesn't already exist.  This is how implicit
// declarations in port lists are dealt with
//
static vl_var *
check_var(vl_simulator *sim, const char *name)
{
    if (!sim->context) {
        vl_error("internal, no current context!");
        sim->abort();
        return (0);
    }
    vl_var *v = sim->context->lookup_var(name, false);
    if (!v) {
        vl_warn("implicit declaration of %s", name);
        vl_module *cmod = sim->context->currentModule();
        if (cmod) {
            v = new vl_var;
            v->name = vl_strdup(name);
            if (!cmod->sig_st)
                cmod->sig_st = new table<vl_var*>;
            cmod->sig_st->insert(v->name, v);
            v->flags |= VAR_IN_TABLE;
        }
        else {
            vl_error("internal, no current module!");
            sim->abort();
        }
    }
    return (v);
}


vl_var &
vl_expr::eval()
{
    vl_var &vo = *this;
    switch (etype) {
    case BitExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
        return (vo);
    }

    // The returned net type will always be REGnone, or REGevent for
    // events
    // 
    reset();
    switch (etype) {

    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
        if (!ux.ide.var) {
            ux.ide.var = check_var(simulator, ux.ide.name);
            if (!ux.ide.var)
                ux.ide.var = this;
        }
        assign(0, ux.ide.var, ux.ide.range);
        if (ux.ide.var->net_type == REGevent)
            vo.net_type = REGevent;
        return (vo);

    case ConcatExpr: {
        if (data_type != Dbit) {
            data_type = Dbit;
            bits.set(DefBits);
            u.s = new char[bits.size];
            memset(u.s, BitDC, bits.size);
        }
        char *endc = u.s;

        char alen = bits.size;
        bits.size = 0;
        int rep = 1;
        if (ux.mcat.rep)
            rep = (int)ux.mcat.rep->eval();
        for (int i = 0; i < rep; i++) {
            // order is msb first, loop in reverse order
            lsGen<vl_expr*> gen(ux.mcat.var->u.c, true);
            vl_expr *e;
            while (gen.prev(&e)) {
                vl_var &d = e->eval();
                int sz = d.array.size ? d.array.size : 1;
                for (int j = 0; j < sz; j++) {
                    int w;
                    char *sd = d.bit_elt(j, &w);
                    if (endc + w > u.s + alen) {
                        alen = endc + w - u.s;
                        char *str = new char[alen];
                        char *r = str;
                        char *s = u.s;
                        while (s < endc)
                            *r++ = *s++;
                        delete [] u.s;
                        u.s = str;
                        endc = r;
                    }
                    char *r = endc;
                    endc += w;
                    while (r < endc)
                        *r++ = *sd++; 
                    bits.size += w;
                }
            }
        }
        bits.hi_index = bits.size - 1;
        return (vo);
    }
    case MinTypMaxExpr: {
        // three numbers: min/typ/max
        // two numbers: min/typ/max=typ
        // one number: min=typ/typ/max=typ
        switch (simulator->dmode) {
        case DLYmin:
            return (ux.exprs.e1->eval());
        default:
        case DLYtyp:
            if (ux.exprs.e2)
                return (ux.exprs.e2->eval());
            return (ux.exprs.e1->eval());
        case DLYmax:
            if (ux.exprs.e3)
                return (ux.exprs.e3->eval());
            if (ux.exprs.e2)
                return (ux.exprs.e2->eval());
            else
                return (ux.exprs.e1->eval());
        }
        vl_warn("(internal) bad min/typ/max format");
        return (vo);
    }
    case FuncExpr:
        if (!ux.func_call.func) {
            ux.func_call.func =
                simulator->context->lookup_func(ux.func_call.name);
            if (!ux.func_call.func) {
                vl_error("unresolved function %s", ux.func_call.name);
                simulator->abort();
                return (vo);
            }
        }
        ux.func_call.func->eval_func(&vo, ux.func_call.args);
        return (vo);
    case UplusExpr:
        vo = ux.exprs.e1->eval();
        return (vo);
    case UminusExpr:
        vo.setx(DefBits);
        vo = ux.exprs.e1->eval();
        vo = -vo;
        return (vo);
    case UnotExpr:
        vo = !ux.exprs.e1->eval();
        return (vo);
    case UcomplExpr:
        vo = ~ux.exprs.e1->eval();
        return (vo);
    case UnandExpr:
    case UandExpr:
    case UnorExpr:
    case UorExpr:
    case UxnorExpr:
    case UxorExpr:
        vo = reduce(ux.exprs.e1->eval(), etype);
        return (vo);
    case BtimesExpr:
        vo = (ux.exprs.e1->eval() * ux.exprs.e2->eval());
        return (vo);
    case BdivExpr:  
        vo = (ux.exprs.e1->eval() / ux.exprs.e2->eval());
        return (vo);
    case BremExpr: 
        vo = (ux.exprs.e1->eval() % ux.exprs.e2->eval());
        return (vo);
    case BlshiftExpr: 
        vo = (ux.exprs.e1->eval() << ux.exprs.e2->eval());
        return (vo);
    case BrshiftExpr:
        vo = (ux.exprs.e1->eval() >> ux.exprs.e2->eval());
        return (vo);
    case Beq3Expr: 
        vo = case_eq(ux.exprs.e1->eval(), ux.exprs.e2->eval());
        return (vo);
    case Bneq3Expr: 
        vo = case_neq(ux.exprs.e1->eval(), ux.exprs.e2->eval());
        return (vo);
    case Beq2Expr: 
        vo = (ux.exprs.e1->eval() == ux.exprs.e2->eval());
        return (vo);
    case Bneq2Expr:
        vo = (ux.exprs.e1->eval() != ux.exprs.e2->eval());
        return (vo);
    case BlandExpr: 
        vo = (ux.exprs.e1->eval() && ux.exprs.e2->eval());
        return (vo);
    case BlorExpr:
        vo = (ux.exprs.e1->eval() || ux.exprs.e2->eval());
        return (vo);
    case BltExpr: 
        vo = (ux.exprs.e1->eval() < ux.exprs.e2->eval());
        return (vo);
    case BleExpr: 
        vo = (ux.exprs.e1->eval() <= ux.exprs.e2->eval());
        return (vo);
    case BgtExpr: 
        vo = (ux.exprs.e1->eval() > ux.exprs.e2->eval());
        return (vo);
    case BgeExpr:  
        vo = (ux.exprs.e1->eval() >= ux.exprs.e2->eval());
        return (vo);
    case BplusExpr: 
        vo = (ux.exprs.e1->eval() + ux.exprs.e2->eval());
        return (vo);
    case BminusExpr:
        vo = (ux.exprs.e1->eval() - ux.exprs.e2->eval());
        return (vo);
    case BandExpr: 
        vo = (ux.exprs.e1->eval() & ux.exprs.e2->eval());
        return (vo);
    case BorExpr:  
        vo = (ux.exprs.e1->eval() | ux.exprs.e2->eval());
        return (vo);
    case BxorExpr:
        vo = (ux.exprs.e1->eval() ^ ux.exprs.e2->eval());
        return (vo);
    case BxnorExpr:
        vo = ~(ux.exprs.e1->eval() ^ ux.exprs.e2->eval());
        return (vo);
    case TcondExpr:
        vo = tcond(ux.exprs.e1->eval(), ux.exprs.e2, ux.exprs.e3);
        return (vo);
    case SysExpr:
        vo = (simulator->*ux.systask->action)(ux.systask, ux.systask->args);
        return (vo);
    }
    vl_warn("(internal) bad expression type");
    return (vo);
} 


// Set up an asynchronous action to perform when data changes,
// for events if the stmt is a vl_action_item, or for continuous
// assign and formal/actual association otherwise
//   mode 0: chain
//   mode 1: unchain
//   mode 2: unchain by context
//
void
vl_expr::chcore(vl_stmt *stmt, int mode)
{
    vl_var vo = *this;
    switch (etype) {
    case BitExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
        // constants
        if (mode == 0) {
            vo.chain(stmt);
            vo.trigger();
        }
        else if (mode == 1)
            vo.unchain(stmt);
        else if (mode == 2)
            vo.unchain_disabled(stmt);
        return;
    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
        if (!ux.ide.var) {
            ux.ide.var = check_var(simulator, ux.ide.name);
            if (!ux.ide.var)
                ux.ide.var = this;
        }
        if (ux.ide.var != this) {
            if (mode == 0)
                ux.ide.var->chain(stmt);
            else if (mode == 1)
                ux.ide.var->unchain(stmt);
            else if (mode == 2)
                ux.ide.var->unchain_disabled(stmt);
            if (ux.ide.range) {
                if (mode == 0) {
                    ux.ide.range->left->chain(stmt);
                    if (ux.ide.range->right &&
                            ux.ide.range->right != ux.ide.range->left)
                        ux.ide.range->right->chain(stmt);
                }
                else if (mode == 1) {
                    ux.ide.range->left->unchain(stmt);
                    if (ux.ide.range->right &&
                            ux.ide.range->right != ux.ide.range->left)
                        ux.ide.range->right->unchain(stmt);
                }
                else if (mode == 2) {
                    ux.ide.range->left->unchain_disabled(stmt);
                    if (ux.ide.range->right &&
                            ux.ide.range->right != ux.ide.range->left)
                        ux.ide.range->right->unchain_disabled(stmt);
                }
            }
        }
        return;
    case ConcatExpr:
        if (mode == 0)
            ux.mcat.var->chain(stmt);
        else if (mode == 1)
            ux.mcat.var->unchain(stmt);
        else if (mode == 2)
            ux.mcat.var->unchain_disabled(stmt);
        return;
    case MinTypMaxExpr: 
        switch (simulator->dmode) {
        case DLYmin:
            if (ux.exprs.e1) {
                if (mode == 0)
                    ux.exprs.e1->chain(stmt);
                else if (mode == 1)
                    ux.exprs.e1->unchain(stmt);
                else if (mode == 2)
                    ux.exprs.e1->unchain_disabled(stmt);
            }
            break;
        default:
        case DLYtyp:
            if (ux.exprs.e2) {
                if (mode == 0)
                    ux.exprs.e2->chain(stmt);
                else if (mode == 1)
                    ux.exprs.e2->unchain(stmt);
                else if (mode == 2)
                    ux.exprs.e2->unchain_disabled(stmt);
            }
            else if (ux.exprs.e1) {
                if (mode == 0)
                    ux.exprs.e1->chain(stmt);
                else if (mode == 1)
                    ux.exprs.e1->unchain(stmt);
                else if (mode == 2)
                    ux.exprs.e1->unchain_disabled(stmt);
            }
            break;
        case DLYmax:
            if (ux.exprs.e3) {
                if (mode == 0)
                    ux.exprs.e3->chain(stmt);
                else if (mode == 1)
                    ux.exprs.e3->unchain(stmt);
                else if (mode == 2)
                    ux.exprs.e3->unchain_disabled(stmt);
            }
            else if (ux.exprs.e2) {
                if (mode == 0)
                    ux.exprs.e2->chain(stmt);
                else if (mode == 1)
                    ux.exprs.e2->unchain(stmt);
                else if (mode == 2)
                    ux.exprs.e2->unchain_disabled(stmt);
            }
            else if (ux.exprs.e1) {
                if (mode == 0)
                    ux.exprs.e1->chain(stmt);
                else if (mode == 1)
                    ux.exprs.e1->unchain(stmt);
                else if (mode == 2)
                    ux.exprs.e1->unchain_disabled(stmt);
            }
            break;
        }
        return;
    case FuncExpr: {
        if (!ux.func_call.func) {
            ux.func_call.func =
                simulator->context->lookup_func(ux.func_call.name);
            if (!ux.func_call.func) {
                vl_error("unresolved function %s", ux.func_call.name);
                simulator->abort();
                return;
            }
        }
        lsGen<vl_expr*> fgen(ux.func_call.args);
        vl_expr *e;
        while (fgen.next(&e)) {
            if (mode == 0)
                e->chain(stmt);
            else if (mode == 1)
                e->unchain(stmt);
            else if (mode == 2)
                e->unchain_disabled(stmt);
        }
        return;
    }

    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UnandExpr:
    case UandExpr:
    case UnorExpr:
    case UorExpr:
    case UxnorExpr:
    case UxorExpr:
        if (ux.exprs.e1) {
            if (mode == 0)
                ux.exprs.e1->chain(stmt);
            else if (mode == 1)
                ux.exprs.e1->unchain(stmt);
            else if (mode == 2)
                ux.exprs.e1->unchain_disabled(stmt);
        }
        return;
    case BtimesExpr:
    case BdivExpr:  
    case BremExpr: 
    case BlshiftExpr: 
    case BrshiftExpr:
    case Beq3Expr: 
    case Bneq3Expr: 
    case Beq2Expr: 
    case Bneq2Expr:
    case BlandExpr: 
    case BlorExpr:
    case BltExpr: 
    case BleExpr: 
    case BgtExpr: 
    case BgeExpr:  
    case BplusExpr: 
    case BminusExpr:
    case BandExpr: 
    case BorExpr:  
    case BxorExpr:
    case BxnorExpr:
        if (ux.exprs.e1) {
            if (mode == 0)
                ux.exprs.e1->chain(stmt);
            else if (mode == 1)
                ux.exprs.e1->unchain(stmt);
            else if (mode == 2)
                ux.exprs.e1->unchain_disabled(stmt);
        }
        if (ux.exprs.e2) {
            if (mode == 0)
                ux.exprs.e2->chain(stmt);
            else if (mode == 1)
                ux.exprs.e1->unchain(stmt);
            else if (mode == 2)
                ux.exprs.e1->unchain_disabled(stmt);
        }
        return;
    case TcondExpr:
        if (ux.exprs.e1) {
            if (mode == 0)
                ux.exprs.e1->chain(stmt);
            else if (mode == 1)
                ux.exprs.e1->unchain(stmt);
            else if (mode == 2)
                ux.exprs.e1->unchain_disabled(stmt);
        }
        if (ux.exprs.e2) {
            if (mode == 0)
                ux.exprs.e2->chain(stmt);
            else if (mode == 1)
                ux.exprs.e2->unchain(stmt);
            else if (mode == 2)
                ux.exprs.e2->unchain_disabled(stmt);
        }
        if (ux.exprs.e3) {
            if (mode == 0)
                ux.exprs.e3->chain(stmt);
            else if (mode == 1)
                ux.exprs.e3->unchain(stmt);
            else if (mode == 2)
                ux.exprs.e3->unchain_disabled(stmt);
        }
        return;
    case SysExpr:
        if (!strcmp(ux.systask->name, "$time")) {
            if (mode == 0)
                simulator->time_data.chain(stmt);
            else if (mode == 1)
                simulator->time_data.unchain(stmt);
            else if (mode == 2)
                simulator->time_data.unchain_disabled(stmt);
        }
        return;
    }
}


// Return the source vl_var for the expression, when the expression is a
// simple reference
//
vl_var *
vl_expr::source()
{
    switch (etype) {
    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
        if (!ux.ide.var) {
            ux.ide.var = check_var(simulator, ux.ide.name);
            if (!ux.ide.var)
                ux.ide.var = this;
        }
        return (ux.ide.var);
    case ConcatExpr:
        if (!ux.mcat.rep)
            return (ux.mcat.var);
    default:
        break;
    }
    return (0);
}
// End of vl_expr functions

