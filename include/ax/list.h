/*
 * Copyright (c) 2020 Li hsilin <lihsilyn@gmail.com>
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

#ifndef AX_LIST_H
#define AX_LIST_H
#include "seq.h"

#define AX_LIST_NAME AX_SEQ_NAME ".list"

#ifndef AX_LIST_DEFINED
#define AX_LIST_DEFINED
typedef struct ax_list_st ax_list;
#endif

#define AX_CLASS_BASE_list seq
#define AX_CLASS_ROLE_list(_l) _l AX_CLASS_PTR(list); AX_CLASS_ROLE_seq(_l)

AX_CLASS_STRUCT_ROLE(list);

extern const ax_seq_trait ax_list_tr;

ax_seq *__ax_list_construct(const ax_stuff_trait *elem_tr);

inline static AX_CLASS_CONSTRUCTOR(list, const ax_stuff_trait* trait)
{
	return __ax_list_construct(trait);
}

ax_list_r ax_list_create(ax_scope *scope, const ax_stuff_trait *elem_tr);

ax_list_r ax_list_init(ax_scope *scope, const char *fmt, ...);

#endif
