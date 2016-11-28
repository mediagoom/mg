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
#include <mgmedia.h>

using namespace MGCORE;

#include "test_base.h"


class res_count
{ 
    int _count;
    res_count(res_count & rhs);
    res_count& operator=(res_count & rhs);
    int end(){ 
	  CHECK(1, _count, _T("invalid res count"));
	  TEST_OK;
	  }
public:
	res_count():_count(1)
	{
	//	 std::cout << _T("res: ") << _count << std::endl;

	}

	void add(){_count++;}
	int get() const {return _count;}
	void sub(){_count--;}
	
	virtual ~res_count()
	{
		end();
	}


	

};

//coverage declarations
template class CResource<res_count>; 

int test_resource_create()
{
	int exc(0);
	try{

		CResource<res_count> r;
		r.Create();
		r.Create();
	
	}catch(std::exception & ex)
	{
	   std::cout << ex.what() << std::endl;
	   exc++;
	}

	try {
		CResource<res_count> r;
		r.GetRef();
	}
	catch (std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		exc++;
	}
	
	try{	CResource<res_count> r;
		r.add_cleanup_function([](res_count*){std::cout << "cleanup";});
	}
	catch (std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		exc++;
	}
	
	CHECK(exc, 3, _T("RESOUCE CHECKING IS INVALID"));

	TEST_OK;
}

int test_resource()
{

	CResource<res_count> k(new res_count);
	CHECK(1, k->get(), _T("INVALID INITIAL VALUE"));
	  
	CResource<res_count> a;

	CHECK(static_cast<bool>(a), false, _T("INVALID BOOL OP"));

	a.Create();
	
	CHECK(1, a->get(), _T("INVALID INITIAL VALUE"));

	a->add(); CHECK(2, a->get(), _T("INVALID ADD VALUE"));
	{
		CResource<res_count> b(a);  CHECK(2, b->get(), _T("INVALID B INITIAL VALUE"));
		b->add();b->add();a->sub(); CHECK(3, b->get(), _T("INVALID MIDDLE VALUE"));
		CResource<res_count> c = b;   CHECK(3, c->get(), _T("INVALID MIDDLE VALUE"));
	}
	

	CHECK(3, a->get(), _T("INVALID MIDDLE VALUE"));
	a->sub();a->sub();



	CResource<res_count> c; //empty

	const CResource<res_count> & d = c;



        CHECK(true, (!c), _T("INVALID BOOL OPERATOR"));
	CHECK(0, c.operator->(), _T("->"));
	CHECK(0, d.operator->(), _T("const ->"));

	const res_count * pc = c;

	c = a;

	CHECK(1, c->get(), _T("CHECK C"));
	c->add();

	CHECK(false, (!c), _T("CHECK BOOL OPERATOR FAILED"));

	pc = c;

    const res_count & cc = (*c);

	CHECK(2, pc->get(), _T("CHECK CONST POINTER"));

	CHECK(2, d->get(), _T("CHECK CONST POINTER"));
	
	CHECK(2, cc.get(), _T("CHECK REF"));

	res_count & kk = c.GetRef();

	CHECK(2, kk.get(), _T("CHECK REF"));

	a->sub();

	a.add_cleanup_function(
		[](res_count * p) 
	    {std::cout << "cleanup funciton " << p->get() << std::endl; }
	);

	TEST_OK;
}

