/*****************************************************************************
 * Copyright (C) 2016 mg project
 *
 * Authors: MediaGoom <admin@mediagoom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE or NONINFRINGEMENT. 
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * For more information, contact us at info@mediagoom.com.
 *****************************************************************************/
#pragma once

int test_uvloop();
int test_file(const Cstring & src);
int test_file_exception();
int test_file_delayed(const Cstring & src);
int test_file_read(const Cstring & src);
int test_file_bitset();
int test_fixed_file();
int test_bitstream_read_write(const Cstring & src);
int test_file_err();
int test_bitstream_read_write_sync(const Cstring & src);
CResource<CBuffer<unsigned char> > file_fill_buffer(const Cstring & f, CResource<::mg::uv::uvloopthread> & t, uint64_t size = 0);

int test_randow_async_file(const Cstring & src);

