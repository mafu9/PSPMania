/*
 * SHA1 hash algorithm. Used in SSH2 as a MAC, and the transform is
 * also used as a `stirring' function for the PuTTY random number
 * pool. Implemented directly from the specification by Simon
 * Tatham.
 */

#include "global.h"
#include "CryptSHA.h"

/* ----------------------------------------------------------------------
 * Core SHA algorithm: processes 16-word blocks into a message digest.
 */

#define rol(x,y) ( ((x) << (y)) | (((uint32_t)x) >> (32-y)) )

void SHATransform(unsigned int *digest, unsigned int *block)
{
	unsigned int w[80];
	unsigned int a, b, c, d, e;
	int t;

	for (t = 0; t < 16; t++)
		w[t] = block[t];

	for (t = 16; t < 80; t++) {
		unsigned int tmp = w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16];
		w[t] = rol(tmp, 1);
	}

	a = digest[0];
	b = digest[1];
	c = digest[2];
	d = digest[3];
	e = digest[4];

	for (t = 0; t < 20; t++) {
		unsigned int tmp =
			rol(a, 5) + ((b & c) | (d & ~b)) + e + w[t] + 0x5a827999;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}
	for (t = 20; t < 40; t++) {
		unsigned int tmp = rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0x6ed9eba1;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}
	for (t = 40; t < 60; t++) {
		unsigned int tmp = rol(a,
				5) + ((b & c) | (b & d) | (c & d)) + e + w[t] +
					0x8f1bbcdc;
				e = d;
				d = c;
				c = rol(b, 30);
				b = a;
				a = tmp;
	}
	for (t = 60; t < 80; t++) {
		unsigned int tmp = rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0xca62c1d6;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}

	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
}

/*
 * PuTTY is copyright 1997-2001 Simon Tatham.
 * 
 * Portions copyright Robert de Bath, Joris van Rantwijk, Delian
 * Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,
 * Justin Bradford, and CORE SDI S.A.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
