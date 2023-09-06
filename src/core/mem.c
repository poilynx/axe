/*
 * Copyright (c) 2021-2023 Li Xilin <lixilin@gmx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ax/def.h"
#include "ax/mem.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include "check.h"
void ax_memswp1(void *ptr1, void *ptr2, size_t size)
{
	CHECK_PARAM_NULL(ptr1);
	CHECK_PARAM_NULL(ptr2);

	if (ptr1 == ptr2)
		return;

	register size_t fast_size = size / sizeof(ax_fast_uint);
	register size_t slow_size = size % sizeof(ax_fast_uint);

	ax_fast_uint *pf1 = ptr1, *pf2 = ptr2;
	for (size_t i = 0; i!= fast_size; i++) {
		pf1[i] = pf1[i] ^ pf2[i];
		pf2[i] = pf1[i] ^ pf2[i];
		pf1[i] = pf1[i] ^ pf2[i];
	}

	uint8_t *p1 = (ax_byte *)ptr1 + size - slow_size, *p2 = (ax_byte*)ptr2 + size - slow_size;
	for (size_t i = 0; i!= slow_size; i++) {
		p1[i] = p1[i] ^ p2[i];
		p2[i] = p1[i] ^ p2[i];
		p1[i] = p1[i] ^ p2[i];
	}
}

char *ax_strdup(const char *s)
{
	CHECK_PARAM_NULL(s);

	size_t size = (strlen(s) + 1) * sizeof(char);
	char *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, s, size);
	return copy;
}

char *ax_strdup2(const char *s, size_t *lenp)
{
	CHECK_PARAM_NULL(s);

	size_t len = strlen(s);
	size_t size = (len + 1) * sizeof(char);
	char *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, s, size);
	*lenp = len;
	return copy;
}


wchar_t *ax_wcsdup(const wchar_t* s)
{
	CHECK_PARAM_NULL(s);

	size_t size = (wcslen(s) + 1) * sizeof(wchar_t);
	wchar_t *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, s, size);
	return copy;
}

void *ax_memdup(const void *p, size_t size)
{
	CHECK_PARAM_NULL(p);

	ax_byte *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, p, size);
	return copy;
}

size_t ax_strhash(const char *s)
{
	CHECK_PARAM_NULL(s);

	register size_t h = 5381;
	int c;
	while ((c = *s++)) {
		h = (h ^ (h << 5)) ^ c; /* hash * 33 + c */
	}
	return h;
}

size_t ax_wcshash(const wchar_t *s)
{
	CHECK_PARAM_NULL(s);

	register size_t h = 5381;
	int c;
	while ((c = *s++)) {
		h = (h ^ (h << 5)) ^ c; /* hash * 33 + c */
	}
	return h;
}

size_t ax_memhash(const void *p, size_t size)
{
	CHECK_PARAM_NULL(p);

	register size_t h = 5381;
	for (size_t i = 0; i < size; i++) {
		h = (h ^ (h << 5)) ^ ((ax_byte *)p)[i];
	}
	return h;
}

char *ax_strsplit(char **s, char ch)
{
	CHECK_PARAM_NULL(s);

	char *ret;
	if (*s) {
		for (char *p = *s; *p; p++) {
			if (*p != ch)
				continue;
			*p = '\0';
			ret = *s;
			*s = p + 1;
			return ret ;
		}
		ret = *s;
	} else {
		ret = NULL;
	}
	*s = NULL;
	return ret;
}

char *ax_strrepl(const char *orig, const char *rep, const char *with)
{
	CHECK_PARAM_NULL(orig);
	CHECK_PARAM_NULL(rep);
	CHECK_PARAM_NULL(with);
	ax_assert(rep[0], "length of parameter rep is 0");

	const char *ins;  /* the next insert point */
	char *result;     /* the return string */
	char *tmp;        /* varies */

	size_t len_rep,   /* length of rep (the string to remove) */
	       len_with,  /* length of with (the string to replace rep with) */
	       len_front, /* distance between rep and end of last rep */
	       count;     /* number of replacements */

	len_rep = strlen(rep);
	if (len_rep == 0)
		return NULL; /* empty rep causes infinite loop during count */
	len_with = strlen(with);

	/* count the number of replacements needed */
	ins = orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count) {
		ins = tmp + len_rep;
	}

	tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
		return NULL;

	/* first time through the loop, all the variable are set correctly
	 * from here on,
	 *    tmp points to the end of the result string
	 *    ins points to the next occurrence of rep in orig
	 *    orig points to the remainder of orig after "end of rep" */
	while (count--) {
		ins = strstr(orig, rep);
		len_front = ins - orig;
		tmp = strncpy(tmp, orig, len_front) + len_front;
		tmp = strcpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	strcpy(tmp, orig);
	return result;
}

uint64_t ax_hash64_thomas(uint64_t key)
{
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

uint64_t ax_hash64_inv_thomas(uint64_t key)
{
	uint64_t tmp;

	// Invert key = key + (key << 31)
	tmp = key-(key<<31);
	key = key-(tmp<<31);

	// Invert key = key ^ (key >> 28)
	tmp = key^key>>28;
	key = key^tmp>>28;

	// Invert key *= 21
	key *= 14933078535860113213u;

	// Invert key = key ^ (key >> 14)
	tmp = key^key>>14;
	tmp = key^tmp>>14;
	tmp = key^tmp>>14;
	key = key^tmp>>14;

	// Invert key *= 265
	key *= 15244667743933553977u;

	// Invert key = key ^ (key >> 24)
	tmp = key^key>>24;
	key = key^tmp>>24;

	// Invert key = (~key) + (key << 21)
	tmp = ~key;
	tmp = ~(key-(tmp<<21));
	tmp = ~(key-(tmp<<21));
	key = ~(key-(tmp<<21));

	return key;
}

inline static void char2hex(char dst[2], ax_byte src)
{
	ax_byte major = (src >> 4) , minor = src & 0x0F;
	dst[0] = (major < 0xA) ? '0' + major : 'A' + (major - 0xA);
	dst[1] = (minor < 0xA) ? '0' + minor : 'A' + (minor - 0xA);
}

char *ax_memtohex(const void *p, size_t size, char *out)
{
	for (int i = 0; i < size; i++)
		char2hex(out + i * 2, ((ax_byte *)p)[i]);
	out[size * 2] = '\0';
	return out;
}

void ax_membyhex(const char *text, void *out)
{
	char *buf = out;
	int i = 0;
	for (i = 0; text[i] != '\0'; i++)
		ax_assert(isdigit((int)text[i]) || (text[i] >= 'A' && text[i] <= 'Z'), "invalid charactor '%c'", text[i]);
	ax_assert(i % 2 == 0, "length of text is odd number");

	for (i = 0; text[i] != '\0'; i += 2)
		buf[i / 2] = (text[i] - (isdigit((int)text[i]) ? '0' : ('A' - 0xA))) * 0x10
			+ (text[i + 1] - (isdigit((int)text[i + 1]) ? '0' : ('A' - 0xA)));
}

size_t ax_strtoargv(char *s, char *argv[], size_t argc_max)
{
	CHECK_PARAM_NULL(s);
	ax_assert(argc_max > 0, "argc_max must be positive integer");
	char *next_ptr = s, *item;
	size_t count = 0;
	while (count < argc_max - 1 && (item = ax_strsplit(&next_ptr, ' '))) {
		if (*item == '\0')
			continue;
		argv[count] = item;
		count++;
	}
	argv[count] = NULL;
	return count;
}

char **ax_strargv(const char *cmdline, int* count)
{
	ax_assert(count, "count ! =  NULL");
	ax_assert(cmdline, "cmdline ! =  NULL");

	char **argv, *d;
	int argc, qcount, bcount;

	/* --- First count the arguments */
	argc = 1;
	const char *s = cmdline;
	/* The first argument, the executable path, follows special rules */
	if (*s == '"') {
		/* The executable path ends at the next quote, no matter what */
		s++;
		while (*s)
			if (*s++ == '"')
				break;
	}
	else {
		/* The executable path ends at the next space, no matter what */
		while (*s && *s != ' ' && *s != '\t')
			s++;
	}
	/* skip to the first argument, if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (*s)
		argc++;

	/* Analyze the remaining arguments */
	qcount = bcount = 0;
	while (*s) {
		if ((*s == ' ' || *s == '\t') && qcount == 0) {
			/* skip to the next argument and count it if any */
			while (*s == ' ' || *s == '\t')
				s++;
			if (*s)
				argc++;
			bcount = 0;
		}
		else if (*s == '\\') {
			/* '\', count them */
			bcount++;
			s++;
		}
		else if (*s == '"') {
			/* '"' */
			if ((bcount & 1) == 0)
				qcount++; /* unescaped '"' */
			s++;
			bcount = 0;
			/* consecutive quotes, see comment in copying code below */
			while (*s == '"') {
				qcount++;
				s++;
			}
			qcount = qcount % 3;
			if (qcount == 2)
				qcount = 0;
		}
		else {
			/* a regular character */
			bcount = 0;
			s++;
		}
	}

	/* Allocate in a single lump, the string array, and the strings that go
	 * with it. This way the caller can make a single LocalFree() call to free
	 * both, as per MSDN.
	 */
	argv = malloc((argc+1) * sizeof(char *) + (strlen(cmdline) + 1) * sizeof(char));
	if (!argv)
		return NULL;
	char *cmdline1 = (char *)(argv + argc + 1);
	strcpy(cmdline1, cmdline);

	/* --- Then split and copy the arguments */
	argv[0] = d = cmdline1;
	argc = 1;
	/* The first argument, the executable path, follows special rules */
	if (*d == '"') {
		/* The executable path ends at the next quote, no matter what */
		s = d+1;
		while (*s) {
			if (*s == '"') {
				s++;
				break;
			}
			*d++ = *s++;
		}
	}
	else {
		/* The executable path ends at the next space, no matter what */
		while (*d && *d != ' ' && *d != '\t')
			d++;
		s = d;
		if (*s)
			s++;
	}
	/* close the executable path */
	*d++ = 0;
	/* skip to the first argument and initialize it if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (!*s) {
		/* There are no parameters so we are all done */
		argv[argc] = NULL;
		*count = argc;
		return argv;
	}

	/* Split and copy the remaining arguments */
	argv[argc++] = d;
	qcount = bcount = 0;
	while (*s) {
		if ((*s == ' ' || *s == '\t') && qcount == 0) {
			/* close the argument */
			*d++ = 0;
			bcount = 0;

			/* skip to the next one and initialize it if any */
			do {
				s++;
			} while (*s == ' ' || *s == '\t');
			if (*s)
				argv[argc++] = d;
		}
		else if (*s == '\\') {
			*d++ = *s++;
			bcount++;
		}
		else if (*s == '"') {
			if ((bcount & 1) == 0) {
				/* Preceded by an even number of '\', this is half that
				 * number of '\', plus a quote which we erase.
				 */
				d -= bcount/2;
				qcount++;
			}
			else {
				/* Preceded by an odd number of '\', this is half that
				 * number of '\' followed by a '"'
				 */
				d = d-bcount/2-1;
				*d++ = '"';
			}
			s++;
			bcount = 0;
			/* Now count the number of consecutive quotes. Note that qcount
			 * already takes into account the opening quote if any, as well as
			 * the quote that lead us here.
			 */
			while (*s == '"') {
				if (++qcount == 3)
				{
					*d++ = '"';
					qcount = 0;
				}
				s++;
			}
			if (qcount == 2)
				qcount = 0;
		}
		else {
			/* a regular character */
			*d++ = *s++;
			bcount = 0;
		}
	}
	*d = '\0';
	argv[argc] = NULL;
	*count = argc;

	return argv;
}
