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
#include "../../exception.h"

#include <stdlib.h>
#include <sys/stat.h>

__MGCORE_BEGIN_NAMESPACE


Cstring get_env_variable(const TCHAR* pszname)
{
	Cstring tmp;
	size_t len;


        char * penv = getenv(pszname);

	if(NULL != penv)
		tmp = penv;
	  


	return tmp;

       
}


void delete_file(const TCHAR* pszname)
{
	int r = remove(pszname);
	MGCHECK(r);
}

void create_directory(const TCHAR* pszname)
{
    int r = mkdir(pszname , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(EEXIST != r)
        MGCHECK(r);
}

__MGCORE_END_NAMESPACE

