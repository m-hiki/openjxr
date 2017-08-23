#ifndef BITBUF_H_
#define BITBUF_H_


typedef struct {
    uint32_t *buf;
    uint32_t *p;
    int left;
    //int length;
    uint32_t *mark;
    int l;
} bitbuf_t;


typedef struct {
    uint16_t code;
    uint16_t length;
} vlc_t;


static inline uint32_t conv_endian(uint32_t x)
{
    return (x << 24) + ((x & 0xff00) << 8) + ((x & 0xff0000) >> 8) + (x >> 24);
    //__asm__ ("bswap %0":"+r"(x));return x;
}


static inline void write_u1(bitbuf_t *b, int bit)
{
    b->left--;
    *b->p |= bit << b->left;
    
    if (b->left == 0) {
#ifndef __BIG_ENDIAN__
        *b->p = conv_endian(*b->p);
#endif
        b->p++;
        b->left = 32;
    }
}


static inline void write_u(bitbuf_t *b, uint32_t bits, int n)
{
    int l = n - b->left;
    bits &= (1 << n) - 1;
    
    if (l < 0) {// n < left
        b->left = -l;
        *b->p |= bits << b->left;
    } else {// n >= left
        *b->p |= bits >> l;

#ifndef __BIG_ENDIAN__
        *b->p = conv_endian(*b->p);
#endif
        b->p++;
        b->left = 32 - l;
        if (b->left < 32) {// 32 - l < 32
            *b->p = bits << b->left;
        }
    }
}


static inline void write_e(bitbuf_t *b, const vlc_t vlc)
{
    write_u(b, vlc.code, vlc.length);
}


static inline void write_align(bitbuf_t *b)
{
    while (b->left & 0x7) write_u1(b, 0);
    if (b->left != 32) {
#ifndef __BIG_ENDIAN__
        *b->p = conv_endian(*b->p);
#endif
        b->p += (32 - b->left) >> 3;
    }
}


static inline int read_u1(bitbuf_t *b)
{
    int v;
    
    b->left--;
    v = (*b->p >> b->left) & 1;
    
    if (b->left == 0) {
#ifndef __BIG_ENDIAN__
        *b->p = conv_endian(*b->p);
#endif
        b->p++;
        b->left = 32;
    }
    
    return v;
}


static inline uint32_t read_u(bitbuf_t *b, int n)
{
    int l = n - b->left;
    uint32_t v = 0;

    if (l < 0) {
        b->left = -l;
        v = (*b->p >> b->left) & ((1 << n) - 1);
    } else {
        v = (*b->p & ((1 << b->left) - 1)) << l;
        b->p++;

#ifndef __BIG_ENDIAN__
        *b->p = conv_endian(*b->p);
#endif
        b->left = 32 - l;
        if (b->left < 32) {
            v |= *b->p >> b->left;
        }
    }

    return v;
}


static inline int read_e(bitbuf_t *b, const vlc_t *vlc, int n)
{
    uint32_t *p = b->p;
    int left = b->left;
    int v;
    int i;
    
    for (i = 0; i < n; i++) {
        b->p = p;
        b->left = left;
        v = read_u(b, vlc[i].length);

        if (v == vlc[i].code) break;
    }
    
    return i;
}


static inline void mark(bitbuf_t *b)
{
    b->mark = b->p;
    b->l = b->left;
}


static inline int get_nbits(bitbuf_t *b)
{
    int byte = (int)(b->p - b->mark) * 32;
    int bit =  b->l - b->left;
    return  byte + bit;
}


static inline int get_total_bits(bitbuf_t *b)
{
    return (int)(b->p - b->buf) * 32 + (32 - b->left);
}


static inline void concat(bitbuf_t *b1, bitbuf_t *b2)
{
    int len_b2 = (int)(b2->p - b2->buf) + 1;
    int i;
    
    for (i = 0; i < len_b2; i++) {
        *b1->p++ = *(b2->buf + i);
    }
}


#endif /* BIT_BUF_H_ */
