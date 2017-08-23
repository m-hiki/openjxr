#include <stdint.h>
#include "common.h"
#include "enc.h"


static inline void t_h2x2(int *a, int *b, int *c, int *d, int r)
{
    int t1, t2;
    *a += *d;
    *b -= *c;
    t1 = ((*a - *b + r) >> 1);
    t2 = *c;
    *c = t1 - *d;
    *d = t1 - t2;
    //CHECK5(long_word_flag, *a, *b, t1, *c, *d);
    *a -= *d;
    *b += *c;
    //CHECK2(long_word_flag, *a, *b);
}


static inline void t_odd(int *a, int *b, int *c, int *d)
{
    *b -= *c;
    *a += *d;
    *c += (*b + 1) >> 1;
    *d = ((*a + 1) >> 1) - *d;
    // CHECK4(long_word_flag, *b, *a, *c, *d);

    *b -= ((*a << 1) + *a + 4) >> 3;
    *a += ((*b << 1) + *b + 4) >> 3;
    *d -= ((*c << 1) + *c + 4) >> 3;
    *c += ((*d << 1) + *d + 4) >> 3;
    // CHECK4(long_word_flag, *b, *a, *d, *c);

    *d += *b >> 1;
    *c -= (*a + 1) >> 1;
    *b -= *d;
    *a += *c;
    // CHECK4(long_word_flag, *d, *c, *b, *a);
}


static inline void it_odd(int *a, int *b, int *c, int *d)
{
    *b += *d;
    *a -= *c;
    *d -= *b >> 1;
    *c += (*a + 1) >> 1;
    //CHECK4(long_word_flag, *a, *b, *c, *d);
    *a -= ((*b << 1) + *b + 4) >> 3;
    *b += ((*a << 1) + *a + 4) >> 3;
    *c -= ((*d << 1) + *d + 4) >> 3;
    *d += ((*c << 1) + *c + 4) >> 3;
    //CHECK4(long_word_flag, *a, *b, *c, *d);
    *c -= (*b + 1) >> 1;
    *d = ((*a + 1) >> 1) - *d;
    *b += *c;
    *a -= *d;
    //CHECK4(long_word_flag, *a, *b, *c, *d);
}


static inline void t_odd_odd(int *a, int *b, int *c, int *d)
{
    int t1, t2;
    *b = -*b;
    *c = -*c;
    //CHECK2(long_word_flag, *b, *c);

    *d += *a;
    *c -= *b;
    t1 = *d >> 1;
    t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    //CHECK4(long_word_flag, *d, *c, *a, *b);

    *a += ((*b << 1) + *b + 4) >> 3;
    *b -= ((*a << 1) + *a + 3) >> 2;
    //CHECK2(long_word_flag, *a, *b);
    *a += ((*b << 1) + *b + 3) >> 3;

    *b -= t2;
    //CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    //CHECK3(long_word_flag, *a, *c, *d);
}


static inline void it_odd_odd(int *a, int *b, int *c, int *d)
{
    int t1, t2;
    *d += *a;
    *c -= *b;
    t1 = *d >> 1;
    t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    //CHECK4(long_word_flag, *a, *b, *c, *d);
    *a -= ((*b << 1) + *b + 3) >> 3;
    *b += ((*a << 1) + *a + 3) >> 2;
    //CHECK2(long_word_flag, *a, *b);
    *a -= ((*b << 1) + *b + 4) >> 3;
    *b -= t2;
    //CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    *b = -*b;
    *c = -*c;
    //CHECK4(long_word_flag, *a, *b, *c, *d);
}


static inline void t_h2x2_pre(int *a, int *b, int *c, int *d)
{
    int t1, t2;
    *a += *d;
    *b -= *c;
    //CHECK2(long_word_flag, *a, *b);
    t1 = *d;
    t2 = *c;
    *c = ((*a - *b) >> 1) - t1;
    *d = t2 + (*b >> 1);
    *b += *c;
    *a -= ((*d << 1) + *d + 4) >> 3;
    //CHECK4(long_word_flag, *c, *d, *b, *a);
}


static inline void t_h2x2_post(int *a, int *b, int *c, int *d)
{
    int t1;
    *b -= *c;
    *a += ((*d << 1) + *d + 4) >> 3;
    *d -= *b >> 1;
    t1 = ((*a - *b) >> 1) - *c;
    //CHECK4(long_word_flag, *b, *a, *d, t1);
    *c = *d;
    *d = t1;
    *a -= *d;
    *b += *c;
    //CHECK2(long_word_flag, *a, *b);
}


static inline void t_odd_odd_pre(int *a, int *b, int *c, int *d)
{
    int t1, t2;
    *d += *a;
    *c -= *b;
    t1 = *d >> 1;
    t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    //CHECK4(long_word_flag, *d, *c, *a, *b);
    *a += ((*b << 1) + *b + 4) >> 3;
    *b -= ((*a << 1) + *a + 2) >> 2;
    //CHECK2(long_word_flag, *a, *b);
    *a += ((*b << 1) + *b + 6) >> 3;
    *b -= t2;
    //CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    //CHECK3(long_word_flag, *a, *c, *d);
}


static inline void t_odd_odd_post(int *a, int *b, 
                                  int *c, int *d)
{
    int t1, t2;
    *d += *a;
    *c -= *b;
    t1 = *d >> 1;
    t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    //CHECK4(long_word_flag, *d, *c, *a, *b);
    *a -= ((*b << 1) + *b + 6) >> 3;
    *b += ((*a << 1) + *a + 2) >> 2;
    //CHECK2(long_word_flag, *a, *b);
    *a -= ((*b << 1) + *b + 4) >> 3;
    *b -= t2;
    //CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    //CHECK3(long_word_flag, *a, *c, *d);
}


static inline void inv_permute2pt(int *a, int *b)
{
    int t1 = *a;
    *a = *b;
    *b = t1;
}


static inline void t_2pt(int *a, int *b)
{
    *a -= (*b + 1) >> 1;
    *b += *a;
    //CHECK2(long_word_flag, *a, *b);
}


static inline void fwd2pt(int *a, int *b)
{
    *b -= *a;
    *a += (*b + 1) >> 1;
    //CHECK2(long_word_flag, *b, *a);
}


static inline void rotate(int *a, int *b)
{
    *b -= (*a + 1) >> 1;
    *a += (*b + 1) >> 1;
    //CHECK2(long_word_flag, *b, *a);
}


static inline void irotate(int *a, int *b)
{
    *a -= (*b + 1) >> 1;
    *b += (*a + 1) >> 1;
    //CHECK2(long_word_flag, *a, *b);
}


static inline void scale(int *a, int *b)
{
    *b -= ((*a << 1) + *a) >> 4;
    //CHECK1(long_word_flag, *b);
    *b -= *a >> 7;
    //CHECK1(long_word_flag, *b);
    *b += *a >> 10;
    *a -= ((*b << 1) + *b) >> 3;
    //CHECK2(long_word_flag, *b, *a);
    *b = (*a >> 1) - *b;
    *a -= *b;
    //CHECK2(long_word_flag, *b, *a);
}


static inline void iscale(int *a, int *b)
{
    *a += *b;
    *b = (*a >> 1) - *b;
    //CHECK2(long_word_flag, *a, *b);
    *a += ((*b << 1) + *b) >> 3;
    *b -= *a >> 10;
    //CHECK2(long_word_flag, *a, *b);
    *b += *a >> 7;
    *b += ((*a << 1) + *a) >> 4;
    //CHECK1(long_word_flag, *b);
}


static void pot4pt(int *a, int *b, int *c, int *d)
{
    *a += *d;
    *b += *c;
    *d -= (*a + 1) >> 1;
    *c -= (*b + 1) >> 1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    rotate(c, d);
    *d *= -1;
    *c *= -1;
    *a -= *d;
    *b -= *c;
    //CHECK4(long_word_flag, *d, *c, *a, *b);
    *d += *a >> 1;
    *c += *b >> 1;
    *a -= ((*d << 1) + *d + 4) >> 3;
    *b -= ((*c << 1) + *c + 4) >> 3;
    //CHECK4(long_word_flag, *d, *c, *a, *b);
    scale(a, d);
    scale(b, c);
    *d += ((*a + 1) >> 1);
    *c += ((*b + 1) >> 1);
    *a -= *d;
    *b -= *c;
    //CHECK4(long_word_flag, *d, *c, *a, *b);
}


static void ipot4pt(int *a, int *b, int *c, int *d)
{
    *a += *d;
    *b += *c;
    *d -= (*a + 1) >> 1;
    *c -= (*b + 1) >> 1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    iscale(a, d);
    iscale(b, c);
    *a += ((*d << 1) + *d + 4) >> 3;
    *b += ((*c << 1) + *c + 4) >> 3;
    *d -= *a >> 1;
    *c -= *b >> 1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    *a += *d;
    *b += *c;
    *d *= -1;
    *c *= -1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    irotate(c, d);
    *d += (*a + 1) >> 1;
    *c += (*b + 1) >> 1;
    *a -= *d;
    *b -= *c;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
}


#define BUF(idx) (buf + (((idx) >> 2) & 0x3) * step + ((idx) & 0x3))


static void fpot4x4(int *buf, int step)
{
    t_h2x2_pre(BUF(0), BUF(3), BUF(12), BUF(15));//a, d, m, p
    t_h2x2_pre(BUF(1), BUF(2), BUF(13), BUF(14));//b, c, n, o
    t_h2x2_pre(BUF(4), BUF(7), BUF(8), BUF(11));//e, h, i, l
    t_h2x2_pre(BUF(5), BUF(6), BUF(9), BUF(10));//f, g, j, k

    scale(BUF(0), BUF(15));//a, p
    scale(BUF(1), BUF(14));//b, o
    scale(BUF(4), BUF(11));//e, l
    scale(BUF(5), BUF(10));//f, k

    rotate(BUF(13), BUF(12));//n, m
    rotate(BUF(9), BUF(8));//j, i
    rotate(BUF(7), BUF(3));//h, d
    rotate(BUF(6), BUF(2));//g, c
    t_odd_odd_pre(BUF(10), BUF(11), BUF(14), BUF(15));//k, l, o, p

    t_h2x2(BUF(0), BUF(12), BUF(3), BUF(15) ,0);//a, m, d, p
    t_h2x2(BUF(1), BUF(2), BUF(13), BUF(14) ,0);//b, c, n, o
    t_h2x2(BUF(4), BUF(7), BUF(8), BUF(11), 0);//e, h, i, l
    t_h2x2(BUF(5), BUF(6), BUF(9), BUF(10), 0);//f, g, j, k
}


static void fpot4h2(int *buf, int step)
{
    pot4pt(BUF(0), BUF(1), BUF(2), BUF(3));
    pot4pt(BUF(4), BUF(5), BUF(6), BUF(7));
}


static void fpot4v2(int *buf, int step)
{
    pot4pt(BUF(0), BUF(4), BUF(8), BUF(12));
    pot4pt(BUF(1), BUF(5), BUF(9), BUF(13));
}


static void fpot2x2_corner(int *buf, int step)
{
    pot4pt(BUF(0), BUF(1), BUF(4), BUF(5));
}


static void fpot2x2(int *buf, int step)
{
    int *a, *b, *c, *d;

    a = buf;
    b = buf + 1;
    c = buf + step;
    d = buf + step + 1;

    *a += *d;
    *b += *c;
    *d -= (*a + 1) >> 1;
    *c -= (*b + 1) >> 1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    *b -= (*a + 2) >> 2;
    *a -= *b >> 5;
    //CHECK2(long_word_flag, *b, *a);
    *a -= *b >> 9;
    // CHECK1(long_word_flag, *a);
    *a -= *b >> 13;
    //CHECK1(long_word_flag, *a);
    *a -= (*b + 1) >> 1;
    *b -= (*a + 2) >> 2;
    *d += (*a + 1) >> 1;
    *c += (*b + 1) >> 1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    *a -= *d;
    *b -= *c;
    //CHECK2(long_word_flag, *a, *b);
}


static void fpot2pt(int *a, int *b)
{
    *b -= (*a + 2) >> 2;
    *a -= *b >> 13;
    //CHECK2(long_word_flag, *b, *a);
    *a -= *b >> 9;
    //CHECK1(long_word_flag, *a);
    *a -= *b >> 5;
    //CHECK1(long_word_flag, *a);
    *a -= (*b + 1) >> 1;
    *b -= (*a + 2) >> 2;
    //CHECK2(long_word_flag, *a, *b);
}


static const uint8_t FWD_PERMUTE[16] = {
    0, 8, 4, 6, 2, 10, 14, 12, 1, 11, 15, 13, 9, 3, 7, 5
};


static void fpct4x4(int *buf, int step)
{
    int t[16];
    int i;
    
    t_h2x2(BUF(0), BUF(3), BUF(12), BUF(15), 0);
    t_h2x2(BUF(5), BUF(6), BUF(9), BUF(10), 0);
    t_h2x2(BUF(1), BUF(2), BUF(13), BUF(14), 0);
    t_h2x2(BUF(4), BUF(7), BUF(8), BUF(11), 0);

    t_h2x2(BUF(0), BUF(1), BUF(4), BUF(5), 1);
    t_odd(BUF(2), BUF(3), BUF(6), BUF(7));
    t_odd(BUF(8), BUF(12), BUF(9), BUF(13));
    t_odd_odd(BUF(10), BUF(11), BUF(14), BUF(15));

    for (i = 0 ; i < 16 ; i++) t[FWD_PERMUTE[i]] = *BUF(i);
    for (i = 0 ; i < 16 ; i++) *BUF(i) = t[i];
}


static void fpct2x4(int *buf, int step)
{
    inv_permute2pt(buf + 1, buf + 2);
    inv_permute2pt(buf + step * 2 + 1, buf + step * 3);
    t_h2x2(buf, buf + 1, buf + step, buf + step + 1, 0);
    t_h2x2(buf + step * 2, buf + step * 2 + 1,
           buf + step * 3, buf + step * 3 + 1, 0);
    fwd2pt(buf, buf + step * 2);
}


static void fpct2x2(int *buf, int step)
{
    inv_permute2pt(buf + 1, buf + step);
    t_h2x2(buf, buf + 1, buf + step, buf + step + 1, 0);
}


static void ipct2x2(int *buf, int step)
{
    t_h2x2(buf, buf + 1, buf + step, buf + step + 1, 0);
    inv_permute2pt(buf + 1, buf + step);
}


static void ipc2x4(int *buf, int step)
{
    t_2pt(buf, buf + step * 2);
    t_h2x2(buf, buf + 1, buf + step, buf + step + 1, 0);
    t_h2x2(buf + step * 2, buf + step * 2 + 1,
           buf + step * 3, buf + step * 3 + 1, 0);
    inv_permute2pt(buf + 1, buf + step);
    inv_permute2pt(buf + step * 2 + 1, buf + step * 3);
}


static const uint8_t INV_PERMUTE[16] = {
    0, 8, 4, 13, 2, 15, 3, 14, 1, 12, 5, 9, 7, 11, 6, 10
};


static void ipct4x4(int *buf, int step)
{
    int t[16];
    int i;

    for (i = 0 ; i < 16 ; i++) t[INV_PERMUTE[i]] = *BUF(i);
    for (i = 0 ; i < 16 ; i++) *BUF(i) = t[i];

    t_h2x2(BUF(0), BUF(1), BUF(4), BUF(5), 1);
    it_odd(BUF(2), BUF(3), BUF(6), BUF(7));
    it_odd(BUF(8), BUF(12), BUF(9), BUF(13));
    it_odd_odd(BUF(10), BUF(11), BUF(14), BUF(15));

    t_h2x2(BUF(0), BUF(3), BUF(12), BUF(15), 0);
    t_h2x2(BUF(5), BUF(6), BUF(9), BUF(10), 0);
    t_h2x2(BUF(1), BUF(2), BUF(13), BUF(14), 0);
    t_h2x2(BUF(4), BUF(7), BUF(8), BUF(11), 0);
}


static void ipot4x4(int *buf, int step)
{
    t_h2x2(BUF(0), BUF(3), BUF(12), BUF(15), 0);
    t_h2x2(BUF(1), BUF(2), BUF(13), BUF(14), 0);
    t_h2x2(BUF(4), BUF(7), BUF(8), BUF(11), 0);
    t_h2x2(BUF(5), BUF(6), BUF(9), BUF(10), 0);

    irotate(BUF(13), BUF(12));
    irotate(BUF(9), BUF(8));
    irotate(BUF(7), BUF(3));
    irotate(BUF(6), BUF(2));
    t_odd_odd_post(BUF(10), BUF(11), BUF(14), BUF(15));

    iscale(BUF(0), BUF(15));
    iscale(BUF(1), BUF(14));
    iscale(BUF(4), BUF(11));
    iscale(BUF(5), BUF(10));

    t_h2x2_post(BUF(0), BUF(3), BUF(12), BUF(15));
    t_h2x2_post(BUF(1), BUF(2), BUF(13), BUF(14));
    t_h2x2_post(BUF(4), BUF(7), BUF(8), BUF(11));
    t_h2x2_post(BUF(5), BUF(6), BUF(9), BUF(10));
}


static void ipot4h2(int *buf, int step)
{
    ipot4pt(BUF(0), BUF(1), BUF(2), BUF(3));
    ipot4pt(BUF(4), BUF(5), BUF(6), BUF(7));
}


static void ipot4v2(int *buf, int step)
{
    ipot4pt(BUF(0), BUF(4), BUF(8), BUF(12));
    ipot4pt(BUF(1), BUF(5), BUF(9), BUF(13));
}


static void ipot2x2(int *buf, int step)
{
    int *a, *b, *c, *d;

    a = buf;
    b = buf + 1;
    c = buf + step;
    d = buf + step + 1;

    *a += *d;
    *b += *c;
    *d -= (*a + 1) >> 1;
    *c -= (*b + 1) >> 1;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    *b += (*a + 2) >> 2;
    *a += (*b + 1) >> 1;

    *a += (*b >> 5);
    *a += (*b >> 9);
    *a += (*b >> 13);
    //CHECK2(long_word_flag, *a, *b);

    *b += (*a + 2) >> 2;

    *d += (*a + 1) >> 1;
    *c += (*b + 1) >> 1;
    *a -= *d;
    //CHECK4(long_word_flag, *a, *b, *d, *c);
    *b -= *c;
    //CHECK1(long_word_flag, *b);
}


static void ipot2pt(int *a, int *b)
{
    *b += (*a + 2) >> 2;
    *a += (*b + 1) >> 1;
    *a += *b >> 5;
    *a += *b >> 9;
    //CHECK2(long_word_flag, *a, *b);
    *a += *b >> 13;
    *b += (*a + 2) >> 2;
    //CHECK2(long_word_flag, *a, *b);
}


void fwd_trx_stage1(int *mb, int *dclp, rect_t *rect, uint32_t overlap_mode)
{
    int *p;
    int left = rect->left;  // Start of block index in x
    int right = rect->right - 4;// End of block index in x
    int top = rect->top;  // Start of block index in y
    int bottom = rect->bottom - 4;// End of block index in y
    int width = rect->right - rect->left >= 18 ? 4 : 2;
    int height = rect->bottom - rect->top >= 18 ? 4 : 2;
    int i, j;
    
    if (overlap_mode > 0) {
        if (left == 0) {//Bottom edge
            left += 2;// move left -2 -> 0
            
            if (top == 0) {//Top edge
                top += 2;// move top -2 -> 0
                fpot2x2_corner(mb, BW); // top and left edge
                
                for (i = left; i <= right; i += 4) {
                    fpot4h2(mb + i, BW); // top edge at 0, 4, 8, 12 for x
                }
            } else if (bottom == EOBLK || bottom == EOBLK_CH) {//Bottom edge
                bottom -= 2;
                fpot2x2_corner(mb + (bottom + 4) * BW, BW);
                
                for (i = left; i <= right; i += 4) {
                    fpot4h2(mb + (bottom + 4) * BW + i, BW);
                }
            }
            
            for (i = top; i <= bottom; i += 4) {
                fpot4v2(mb + i * BW, BW);
            }
        } else if (right == EOBLK || right == EOBLK_CH) {//Right edge
            right -= 2;

            if (top == 0) {
                top += 2;
                fpot2x2_corner(mb + (right + 4), BW);
                
                for (i = left; i <= right; i += 4) {
                    fpot4h2(mb + i, BW);
                }
            } else if (bottom == EOBLK || bottom == EOBLK_CH) {
                bottom -= 2;
                fpot2x2_corner(mb + (right + 4) + (bottom + 4) * BW, BW);
                
                for (i = left; i <= right; i += 4) {
                    fpot4h2(mb + (bottom + 4) * BW + i, BW);
                }
            }
            
            for (i = top; i <= bottom; i += 4) {
                fpot4v2(mb + (right + 4) + i * BW, BW);
            }
        } else if (top == 0) {
            top += 2;// move top -2 -> 0
                
            for (i = left; i <= right; i += 4) {
                fpot4h2(mb + i, BW); // top edge at 0, 4, 8, 12 for x
            }
        } else if (bottom == EOBLK || bottom == EOBLK_CH) {
            bottom -= 2;
            
            for (i = left; i <= right; i += 4) {
                fpot4h2(mb + (bottom + 4) * BW + i, BW);
            }
        }

        for (i = top; i <= bottom; i += 4) {
            for (j = left; j <= right; j += 4) {
                fpot4x4(mb + i * BW + j, BW);
            }
        }
    }
    
    // stage 1 PCT
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            p = mb + i * BW * 4 + j * 4;
            fpct4x4(p, BW);
            dclp[i * DCLPW + j] = p[0];
        }
    }
}


void fwd_trx_stage2(int *dclp, rect_t *rect, uint32_t overlap_mode)
{
    int width = rect->right - rect->left >= 18 ? 4 : 2;
    int height = rect->bottom - rect->top >= 18 ? 4 : 2;

    // stage 2 POT
    if (overlap_mode == 2) {
        // TODO
    }

    // stage 2 PCT
    if (width == 2 && height == 2) {
        fpct2x2(dclp, DCLPW);
    } else if (width == 2 && height == 4) {
        fpct2x4(dclp, DCLPW);
    } else {
        fpct4x4(dclp, DCLPW);
    }
}


void inv_trx_stage1(int *dclp, rect_t *rect)
{
    int w, h;

    h = rect->bottom - 4 - rect->top >= 12 ? 4 : 2;
    w = rect->right - 4 - rect->left >= 12 ? 4 : 2;
    
    if (h == 4 && w == 4) {
        ipct4x4(dclp, 4);
    } else if (h == 4 && w == 2) {
        
    } else { // h == 2 && w == 2
        ipct2x2(dclp, 4);
    }
}


void inv_trx_stage2(int *mb, int *dclp, rect_t *rect)
{
    int *p;
    int left = rect->left;  // Start of block index in x
    int right = rect->right - 4;// End of block index in x
    int top = rect->top;  // Start of block index in y
    int bottom = rect->bottom - 4;// End of block index in y
    int w, h;
    int i, j;

    h = rect->bottom - 4 - rect->top >= 12 ? 4 : 2;
    w = rect->right - 4 - rect->left >= 12 ? 4 : 2;

	for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            p = mb + i * BW * 4 + j * 4;
            p[0] = dclp[i * DCLPW + j];
            ipct4x4(p, BW);
        }
    }
    
    if (left == 0) {
        left += 2;
        
        if (top == 0) {
            top += 2;
            ipot2x2(mb, BW);
            
            for (i = left; i <= right; i += 4) {
                ipot4h2(mb + i, BW);
            }
        } else if (bottom == 12 || bottom == 6) {
            bottom -= 2;
            ipot2x2(mb + (bottom + 4) * BW, BW);

            for (i = left; i <= right; i += 4) {
                ipot4h2(mb + (bottom + 4) * BW + i, BW);
            }
        }
        
        for (i = top; i <= bottom; i += 4) {
            ipot4v2(mb + i * BW, BW);
        }
    } else if (right == EOBLK || right == EOBLK_CH) {
        right -= 2;
        
        if (top == 0) {
            top += 2;
            ipot2x2(mb + (right + 4), BW);

            for (i = left; i <= right; i += 4) {
                ipot4h2(mb + i, BW);
            }
        } else if (bottom == EOBLK || right == EOBLK_CH) {
            bottom -= 2;
            ipot2x2(mb + (right + 4) + (bottom + 4) * BW, BW);
            
            for (i = left; i <= right; i += 4) {
                ipot4h2(mb + (bottom + 4) * BW + i, BW);
            }
        }
        
        for (i = top; i <= bottom; i += 4) {
            ipot4v2(mb + (right + 4) + i * BW, BW);
        }
    }

    for (i = top; i <= bottom; i += 4) {
        for (j = left; j <= right; j += 4) {
            ipot4x4(mb + i * BW + j, BW);
        }
    }
}
