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




#define E_INVALIDARG (E_BASE_CUSTOM -1)
#define ERROR_HANDLE_EOF (E_BASE_CUSTOM -2)



__MGCORE_BEGIN_NAMESPACE


#define ALXTHROW_LASTERR       throw ::mg::core::mgexception(E_BASE_CUSTOM, (_T(__FILE__)), __LINE__);
#define ALXTHROW_LASTERR1(err) throw ::mg::core::mgexception(err, (_T(__FILE__)), __LINE__);

__MGCORE_END_NAMESPACE



