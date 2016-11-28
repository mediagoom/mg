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
#include "file.h"

__MGCORE_BEGIN_NAMESPACE

#define FILE_DEBUG 0
#define NEXT_FRONT_DEBUG 0

void cfile::uv_file_cb(uv_fs_t* req)
{
	//std::cout << "file call back" << req->type << " " << req->result << std::endl;

	_ASSERTE(0 < req->data);

	struct_file_cb * fcb = reinterpret_cast<struct_file_cb *>(req->data);


#ifdef _DEBUG

	if (req->fs_type == UV_FS_READ )//|| req->fs_type == UV_FS_WRITE)
	{
/*		_ASSERTE(fcb->_sequence == (*fcb->_p_arrive)++ );
		_ASSERTE(0 >= req->result || fcb->_expt == req->result);
*/	

		FDBGC1(FILE_DEBUG, "<<<READ\t%u", fcb->_sequence);
		FDBGC1(FILE_DEBUG, "<<<FORW\t%d", (fcb->_p_forward->empty())?11:10);
	}

#endif
	
	if (fcb->_sequence > (*fcb->_p_served)
		&& req->fs_type == UV_FS_READ
		//&& 0 <= req->result
		)
	{
		FDBGC2(NEXT_FRONT_DEBUG, "UNORDERED-READ-PUSH\t%u\t%u"
			, fcb->_sequence
			, (*fcb->_p_served)
		);
		//unordered read operation
		fcb->_p_forward->push_front(fcb);

		std::forward_list<struct_file_cb *>::iterator it = fcb->_p_forward->begin();

		while (it != fcb->_p_forward->end()
			&& (*it.operator->())->_sequence > (*fcb->_p_served))
		{
			it++;

#ifdef _DEBUG
			if (it != fcb->_p_forward->end())
			{
				FDBGC2(NEXT_FRONT_DEBUG, "UNORDERED-READ-LOOK\t%u\t%u"
					, (*it.operator->())->_sequence
					, (*fcb->_p_served)
				);
			}
#endif
		}

		if (it == fcb->_p_forward->end())
		{
			//not found
			FDBGC0(FILE_DEBUG, "UNORDERED-READ-NOT-FOUND");
			return;
		}

		//switch
		struct_file_cb * temp = (*it.operator->());

		fcb->_p_forward->remove(temp);

		fcb = temp;
	
	}

	struct_file_cb * temp = NULL;

	//loop for next item
	//if (fcb->_p_forward->begin() != fcb->_p_forward->end())
	
	bool fcb_empty = fcb->_p_forward->empty();
	
	FDBGC3(FILE_DEBUG, "-**-FORW\t%d\t%d\t%d", (fcb_empty)?1:0, (fcb->_p_forward->empty())?11:10, 1);

	if(fcb_empty)
	{
		FDBGC0(FILE_DEBUG, "***EMPTY");
	}
	
	if(!fcb_empty)
	{
		//fcb->_p_forward->

		_ASSERTE(!fcb->_p_forward->empty());

		temp = fcb->_p_forward->front();  //(*fcb->_p_forward->begin().operator->());

		FDBGC1(FILE_DEBUG, "---FORW\t%u", fcb->_p_forward->empty());

		FDBGC3(NEXT_FRONT_DEBUG, "--RE-READ\t%u\t%u\t%u"
			, (*fcb->_p_served)
			, temp->_sequence
			, fcb->_sequence
		);

		_ASSERTE(temp->_sequence < 10000);

		fcb->_p_forward->pop_front();
	}

	
	(*fcb->_p_served)++;
	
	
	if(0 > req->result)
	{

		std::cout << "!!!file async error " << uv_strerror(req->result)
			<< _T("\t")
			<< std::endl;

		fcb->call_er(std::make_exception_ptr(MGCORE::mgexception(req->result, uv_strerror(req->result), _T(__FILE__), __LINE__)));
	}
	else
	{
		fcb->call_cb(fcb->_uv_fs_t);
	}
	
	delete fcb;

	if (temp)
	{
		_ASSERTE(temp->_sequence < 10001);

		FDBGC1(NEXT_FRONT_DEBUG, "---RE-READ\t%u", temp->_sequence);
		uv_file_cb(temp->_uv_fs_t);
	}

	
}


cfile::cfile(::mg::uv::loopthread & loop)
    : _loop(loop)
	, _file(0)
	, _position(0)
	, _outstanding(0)
	, _served(0)
{
	::memset(&_file_stat, 0, sizeof(uv_stat_t));

#ifdef _DEBUG
	//_leave_r_sequence   = 0;
	_arrive_r_sequence  = 0;
	_leave_wr_sequence  = 0;
	_arrive_wr_sequence = 0;
#endif

}

cfile::~cfile()
{
	if (_file)
		close(false);
}

CResource<CPromise<uv_fs_t> > cfile::open(const TCHAR * path, int flags, bool async)
{
	int mode = S_IFMT | S_IFDIR | S_IFCHR | S_IFREG | S_IREAD | S_IWRITE;
		
		//S_IRWXU | S_IRWXG | S_IRWXO;
//|		S_IEXEC

	_ASSERTE(0 == _file);

	uv_fs_t req;
	
		
	int r = uv_fs_stat(_loop->get_loop(), &req, path, NULL); //sinc
	if (!(UV_ENOENT == req.result && (flags && O_CREAT)))
	{
		UVCHECK(r);
	}

	if (0 > req.result)
	{
		if (!(UV_ENOENT == req.result && (flags && O_CREAT)))
		{
			CResource<CPromise<uv_fs_t> > prom;
			                              prom.Create();

										  prom->call_er(std::make_exception_ptr(::mg::core::mgexception(req.result, uv_strerror(req.result), _T(__FILE__), __LINE__)));

										  return prom;

		}
		
	}
	else
	{
		_file_stat = req.statbuf;
	}	

	if (O_TRUNC & flags)
		_file_stat.st_size = 0;
	
	if (async)
	{
		struct_file_cb * cb = new struct_file_cb; // { std::bind(&cfile::handle_size, this, std::placeholders::_1) };

		cb->_uv_fs_t.Create();
		cb->_uv_fs_t->data = cb;
		cb->_uv_fs_t.add_cleanup_function([](uv_fs_t * ptr) {uv_fs_req_cleanup(ptr); });

		//cb->set_cb(std::bind(&cfile::handle_size, this, std::placeholders::_1));

		cfile * myself = this;

		cb->set_cb([myself](CResource<uv_fs_t> & req) {
			myself->_file = req->result;
		}
		);

		cb->set_er();

		CResource<CPromise<uv_fs_t>> f = cb->forward();

		do_outstanding(cb);

		do {

			auto_lock l(_loop->loop_lock());
			int r = uv_fs_open(_loop->get_loop(), cb->_uv_fs_t, path, flags, mode, uv_file_cb);
			UVCHECK(r);

		} while (0);

		

		return f;
	}
	else
	{
		r = uv_fs_open(_loop->get_loop(), &req, path, flags, mode, NULL);
		UVCHECK(r);

		UVCHECK(req.result);

		_file = req.result;

		return CResource<CPromise<uv_fs_t> >();
	}
}

CResource<CPromise<uv_fs_t> > cfile::close(bool async)
{
	_ended.reset();

	//std::cout << _T("cfile::close\t") << _served << _T("\t") << _outstanding << std::endl;

	while (this->_served != this->_outstanding)
	{
		_ended.wait(10);
	}


	_ASSERTE(this->_served == this->_outstanding);

	
	//std::cout << _T("cfile::close 2\t") << _served << _T("\t") << _outstanding << std::endl;

	if (0 == _file)
		return CResource<CPromise<uv_fs_t>>();
		
	if (async)
	{
		struct_file_cb * cb = new struct_file_cb;

		cb->_uv_fs_t.Create();
		cb->_uv_fs_t->data = cb;
		cb->_uv_fs_t.add_cleanup_function([](uv_fs_t * ptr) {uv_fs_req_cleanup(ptr); });

		cb->set_er();

		uv_file * pfile = &_file;

		cb->set_cb([ pfile ](CResource<uv_fs_t> & req) 
		{
			(*pfile) = 0; 
		}
		);

		CResource<CPromise<uv_fs_t>> f = cb->forward();

		do_outstanding(cb);


		do {

			auto_lock l(_loop->loop_lock());
			int r = uv_fs_close(_loop->get_loop(), cb->_uv_fs_t, _file, uv_file_cb);
			UVCHECK(r);

		} while (0);


		

		return f;
	}
	else
	{
		uv_fs_t req;

		int r = uv_fs_close(_loop->get_loop(), &req, _file, NULL);
		UVCHECK(r);

		UVCHECK(req.result);
		


	//std::cout << _T("cfile::close 3\t") << _served << _T("\t") << _outstanding << std::endl;

		_file = 0;

		return CResource<CPromise<uv_fs_t>>();
	}
}

CResource<CPromise<uv_fs_t>> cfile::write(const unsigned char * p_buffer, uint32_t size)
{
	_ASSERTE(0 < size);

	CResource<CBuffer<unsigned char> > pBuffer(new CBuffer<unsigned char>(size));
	                                   pBuffer->add(p_buffer, size);

	uv_buf_t buf;
	buf.len  = size;
	buf.base = reinterpret_cast<char*>(pBuffer->get());

	struct_file_cb * cb = new struct_file_cb;

	cb->_uv_fs_t.Create();
	cb->_uv_fs_t->data = cb;
	cb->_uv_fs_t.add_cleanup_function([](uv_fs_t * ptr) {uv_fs_req_cleanup(ptr); });

	cb->set_er();

	uint64_t * ppos = &_position;
	uint64_t    pos =  _position;

#ifdef _DEBUG
	cb->_sequence = _leave_wr_sequence++;
	cb->_p_arrive = &_arrive_wr_sequence;
	cb->_expt = size;
#endif
	
	cb->set_cb([pBuffer/*, ppos*/](CResource<uv_fs_t> & req)  
		{
			/*(*ppos) += pBuffer->size();*/
			//keep an active reference to the buffer. //we can work in fire and forget.
			_ASSERTE(req->result = pBuffer->size());
	    }
	);

	CResource<CPromise<uv_fs_t>> f = cb->forward();

	do_outstanding(cb);

	do {

		auto_lock l(_loop->loop_lock());
		int r = uv_fs_write(_loop->get_loop(), cb->_uv_fs_t, _file, &buf, 1, pos, uv_file_cb);
		UVCHECK(r);

	} while (0);

	_position += size;
	
	return f;
}

CResource<CPromise<CBuffer<unsigned char>>> cfile::read(uint32_t size)
{
	_ASSERTE(size);

	CResource<CBuffer<unsigned char> > pBuffer( new CBuffer<unsigned char>(size) );
	                                    
	CResource<CPromise<CBuffer<unsigned char>> >  promise;
												  promise.Create();
												  promise->set_er();

		uv_buf_t buf;
		buf.len = size;
		buf.base = reinterpret_cast<char*>(pBuffer->get());

		struct_file_cb * cb = new struct_file_cb; 

		cb->_uv_fs_t.Create();
		cb->_uv_fs_t->data = cb;
		cb->_uv_fs_t.add_cleanup_function([](uv_fs_t * ptr) {uv_fs_req_cleanup(ptr); });
		
		cb->set_er(
			[promise](std::exception_ptr  eptr)  mutable -> bool 
			{ promise->call_er(eptr); return true; }
		);
			

		uint64_t * ppos = &_position;
		uint64_t    pos = _position;

		if ((pos + size) > this->size())
			size = static_cast<uint32_t>(this->size() - pos);

#ifdef _DEBUG
		//cb->_sequence = _leave_r_sequence++;
		cb->_p_arrive = &_arrive_r_sequence;
		cb->_expt = size;
#endif

		cb->set_cb([pBuffer, promise/*, ppos*/](CResource<uv_fs_t> & req) mutable {
			//const_cast< CResource<CPromise<CBuffer<unsigned char*>> > & >(promise)->call_cb(pBuffer);
			    pBuffer->updatePosition(req->result);

				//(*ppos) += pBuffer->size();

				promise->call_cb(pBuffer);
			}
		);

		do_outstanding(cb);

		do {

			auto_lock l(_loop->loop_lock());
			int r = uv_fs_read(_loop->get_loop(), cb->_uv_fs_t, _file, &buf, 1, pos, uv_file_cb);
			UVCHECK(r);

		} while (0);

		

		_position += size;

		return promise;

}


__MGCORE_END_NAMESPACE

