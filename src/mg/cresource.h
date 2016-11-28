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
#include "core/core.h"
//#include "crtdbg.h"
#include <new>

#include <functional>

#pragma once

//This is necessary if we do not have it
//define in other places
#ifndef NULL
#define NULL 0
#endif

__ALX_BEGIN_NAMESPACE
////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////
//FORWARD DECLARATION FOR FRIEND
template<class Type>
class CResource ;
//////////////////////////////////////////////////////////////////////
template<class Type>
class CInternalResource 
{
	friend class CResource<Type>;
private:

	Type *m_pointer;
	long m_iRefCnt;

	std::function<void(Type*)> _cleanup;
	
	CInternalResource(Type *p):m_iRefCnt(0)
	{m_pointer = p;}
	~CInternalResource()
	{
		delete m_pointer;
	};
	
	static CInternalResource* create()
	{
		CInternalResource* tmp = new CInternalResource(new Type);
		tmp->addRef();
		return tmp;
	}

	static CInternalResource* create(Type *pointer)
	{
		CInternalResource* tmp = new CInternalResource(pointer);
		if(0 == tmp) throw std::bad_alloc();
		tmp->addRef();
			return tmp;
	}

	virtual void release()
	{
		m_iRefCnt--;
		if (0 == m_iRefCnt)
		{
			if (_cleanup)
				_cleanup(m_pointer);

			delete this;
		}
	}

	virtual void addRef()
	{
		m_iRefCnt++;
	}

	Type *get()
	{
		return m_pointer;
	}

};


template<class Type>
class CResource 
{
protected:
	//The internal pointer the 
	//the class that reference count the
	//pointer
	CInternalResource<Type> *m_pInternalResource;
public:
	
	//This constructor create a new resource 
	//and create a pointer to the managed resource
	CResource():m_pInternalResource(NULL)
	{
	}

	//assign the resource to another resource
	//Both resource will point to the same
	//underline pointer
	const CResource& Assign(const CResource& res)
	{
		if(m_pInternalResource) m_pInternalResource->release();
		m_pInternalResource = res.m_pInternalResource;
		if(m_pInternalResource) m_pInternalResource->addRef();	
		return res;
	}
   	//assign the resource to another resource
	//Both resource will point to the same
	//underline pointer
	const CResource& operator=(const CResource& res)
	{return Assign(res);}


	//Create a resource from an other resource
	CResource(const CResource& res)
	{
		m_pInternalResource = res.m_pInternalResource;
		if(m_pInternalResource) m_pInternalResource->addRef();	
	}
/*	
	//Assign an existing pointer to the resource
	//This operation make the resource owning the pointer
	//so it can no longer be deleted!!
	//It will be deleted when the reference count 
	//goes to 0
	const CResource& operator=(Type *pointer)
	{
		if(m_pInternalResource) m_pInternalResource->release();
		m_pInternalResource = CInternalResource<Type>::create(pointer);	
		return *this;
	}

	//in order to assign a pointer to an existing resource create a new resource and then assign it


*/
	//Create a resource from an existing pointer
	//This operation make the resource owning the pointer
	//so it can no longer be deleted!!
	//It will be deleted when the reference count 
	//goes to 0
	CResource(Type *pointer)
	{
		m_pInternalResource = CInternalResource<Type>::create(pointer);
	}

	virtual ~CResource()
	{
		if(m_pInternalResource) m_pInternalResource->release();
	}
	/*
	//User-Defined Conversion
	operator Type()
	{
		if(m_pInternalResource) 
			return m_pInternalResource->get();
		else 
			return NULL;
	}
	*/
	Type * operator->()
	{
		if(m_pInternalResource) 
			return m_pInternalResource->get();
		else 
			return NULL;
	}

	const Type * operator->() const
	{
		if(m_pInternalResource) 
			return m_pInternalResource->get();
		else 
			return NULL;
	}
/*
	Type* operator&() const	
	{
		if(m_pInternalResource) 
			return m_pInternalResource->get();
		else 
			return NULL;
	}
*/
	operator Type*()
	{
		if(m_pInternalResource) 
			return m_pInternalResource->get();
		else 
			return NULL;
	}

	Type& GetRef()
	{
		if (operator!())
		{
			MGBASECHECK(-1);
		}
		
		return *(m_pInternalResource->get());
		
	}

	bool operator !()
	{
		return(m_pInternalResource)?false:true;
		
	}

	operator bool()
	{
		return (m_pInternalResource) ? true : false;
	}

	void Create()
	{
		//static_assert(NULL == m_pInternalResource, "cannot recreate a Resource");

		if (!operator!())
		{
			MGBASECHECK(-1);
		}

		m_pInternalResource = CInternalResource<Type>::create();

	}

	void add_cleanup_function(std::function<void(Type*)> func)
	{

		if (operator!())
		{
			MGBASECHECK(-1);
		}

		_ASSERTE(NULL != m_pInternalResource);
		m_pInternalResource->_cleanup = func;
	}

	long count() const { return m_pInternalResource->m_iRefCnt; }


};
__ALX_END_NAMESPACE


