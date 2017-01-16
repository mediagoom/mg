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

#include "exception.h"
#include <map>
#include <vector>
//Always check that argv is different from NULL	if(argv) 

__ALX_BEGIN_NAMESPACE

//<summary>
//TO READ AN INTEGER or int64_t
//	virtual void I(int argc, TCHAR *argv[]) throw(mgexception)
//	{
//		Cstring inter = *argv;
//		interval = inter;
//	}
//A Cstring
//	virtual void G(int argc, TCHAR *argv[]) throw(mgexception)
//	{
//		graph = *argv;
//	}
//</summary>
class Cmdline
{
		TCHAR ** m_argvs;
		int m_cnt;
		TCHAR m_cmd;
		bool m_bhelp;
public:
	Cmdline(): m_bhelp(false)
	{}

	Cmdline(int argc, TCHAR *argv[]) //throw(mgexception)
		: m_bhelp(false) 
	{
		process(argc, argv);
	}

	void process(int argc, TCHAR *argv[]) //throw(mgexception)
	{
		 m_argvs = 0;
		 m_cnt = 0;
		 m_cmd = 0;

		if(2 > argc)
	 	{
	 		Usage();
			return;
	 	}

					for(int i = 1; i < argc; ++i)
					{
								//Read the first letter of the argument
								TCHAR cmd = *argv[i];

								if(_T('-') == cmd || _T('/') == cmd)
								{
										Dispatch();
										TCHAR  *psz = argv[i] + 1;
										m_cmd = *psz;
										m_cnt = 0;
										continue;
								}

								if(m_cnt++)
									continue;
								else
									m_argvs = &argv[i];

					}

			//Dispatch the last command
			Dispatch();
	}

	virtual bool help() //throw(mgexception)
	{
		return m_bhelp;
	}

protected:
	void Dispatch()
	{
			if(!m_cmd)
				return;              //Nothing to dispatch
		
			switch(m_cmd)
			{
					case _T('a'):
					case _T('A'):
								A(m_cnt, m_argvs);
						break;
					case _T('b'):
					case _T('B'):
								B(m_cnt, m_argvs);
						break;
					case _T('c'):
					case _T('C'):
								C(m_cnt, m_argvs);
						break;
					case _T('d'):
					case _T('D'):
								D(m_cnt, m_argvs);
						break;

					case _T('e'):
					case _T('E'):
								E(m_cnt, m_argvs);
						break;

					case _T('f'):
					case _T('F'):
								F(m_cnt, m_argvs);
								break;

					case _T('g'):
					case _T('G'):
								G(m_cnt, m_argvs);
								break;

					case _T('h'):
					case _T('H'):
								H(m_cnt, m_argvs);
						break;

					case _T('i'):
					case _T('I'):
								I(m_cnt, m_argvs);
						break;

					case _T('j'):
					case _T('J'):
								J(m_cnt, m_argvs);
						break;

					case _T('k'):
					case _T('K'):
								K(m_cnt, m_argvs);
						break;
					
					case _T('l'):
					case _T('L'):
								L(m_cnt, m_argvs);
						break;

					case _T('m'):
					case _T('M'):
								M(m_cnt, m_argvs);
						break;

					case _T('n'):
					case _T('N'):
								N(m_cnt, m_argvs);
						break;

					case _T('o'):
					case _T('O'):
								O(m_cnt, m_argvs);
						break;

					case _T('p'):
					case _T('P'):
								P(m_cnt, m_argvs);
						break;

					case _T('q'):
					case _T('Q'):
								Q(m_cnt, m_argvs);
						break;

					case _T('r'):
					case _T('R'):
								R(m_cnt, m_argvs);
						break;

					case _T('s'):
					case _T('S'):
								S(m_cnt, m_argvs);
						break;

					case _T('t'):
					case _T('T'):
								T(m_cnt, m_argvs);
						break;

					case _T('u'):
					case _T('U'):
								U(m_cnt, m_argvs);
						break;

					case _T('v'):
					case _T('V'):
								V(m_cnt, m_argvs);
						break;

					case _T('x'):
					case _T('X'):
								X(m_cnt, m_argvs);
						break;

					case _T('y'):
					case _T('Y'):
								Y(m_cnt, m_argvs);
						break;

					case _T('w'):
					case _T('W'):
								W(m_cnt, m_argvs);
						break;
						
					case _T('z'):
					case _T('Z'):
								Z(m_cnt, m_argvs);
						break;

					case _T('?'):
							preUsage();
						break;

					default:
								Defaul_T(m_cnt, m_argvs, m_cmd);
			}

	}

	virtual void A(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void B(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void C(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void D(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void E(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void F(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void G(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}
	
	virtual void H(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void I(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void J(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void K(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void L(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void M(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void N(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void O(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void P(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void Q(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void R(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void S(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void X(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void Y(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void W(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}
	
	virtual void T(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void U(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void V(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}
	
	virtual void Z(int argc, TCHAR *argv[]) //throw(mgexception)
	{
	}

	virtual void preUsage() //throw(mgexception)
	{
		m_bhelp = true;
		Usage();
	}

	virtual void Usage() //throw(mgexception)
	{
	}

	virtual void Defaul_T(int argc, TCHAR *argv[], TCHAR cmd) //throw(mgexception)
	{
	    _ASSERTE(false);
	}

};
/** 
* \brief This class parse a command line.
* 
  usage:
  console_command c;

			c.add(L"command1", console_command::type_string);
			c.add(L"command2", console_command::type_string, true, L"desc 2");
			c.add(L"command3", console_command::type_int   , true, L"desc 3", 'r');
			c.add(L"bool"    , console_command::type_bool  , true);

			bool r = c.process(argc, argv);
*/
class console_command
{
public:
	enum command_type{type_string, type_int, type_bool};
private:

	Cstring _last_error;

	struct console_item
	{
		command_type t;
		bool required;
		Cstring psz_description;
		TCHAR short_cut;
		std::vector<Cstring> psz_value;
	};

	std::map<Cstring, console_item> _commands;

public:
	void add(const TCHAR * psz_name
		   , command_type t = type_string
		   , bool required = false
		   , const TCHAR * psz_description = _T("")
		   , TCHAR short_cut = 0x0)
	{
		Cstring n(psz_name);
		
		if(_commands.end() != _commands.find(n))
		{
			ALXTHROW_T(_T("command already exist"));
		}

		console_item item = {t, required, psz_description, short_cut};
		_commands[n] = item;
	}

	bool process(int argc, TCHAR *argv[])
	{
		Cstring command;
		Cstring name;
		Cstring value;

		for(int i = 1; i < argc; ++i)
		{
			TCHAR* psz = argv[i];
			command = psz;
			name    = _T("");
			value   = _T("");

			if(Equals(command.subString(0, 1), _T("-")))
			{
				int x = command.find(_T(":"));
				if(x < 0)
					x = command.find(_T("="));

				//check for --command
				int ss = 1;
				if(Equals(command.subString(1, 1), _T("-")))
				{
					ss = 2;
				}

				
				//no args = true
				if(x < 0)
				{
					name  = command.subString(ss, command.len() - ss);

					if( _commands.find(name) != _commands.end()
					    && _commands[name].t == type_bool)
				    {
						
						value = _T("T");
				    }
				}
				else
				{
					name  = command.subString(ss, x - ss);
					value = command.subString(x + 1, command.len() - (x + 1));
				}

				if(_commands.find(name) == _commands.end())
				{
					bool add = true;

					if(1 == name.len())//look for shortcut
					{
						TCHAR s = *(name.operator TCHAR *());
						std::map<Cstring, console_item>::const_iterator it = _commands.begin();
						while(it != _commands.end())
						{
							if(s == it->second.short_cut)
							{
								name = it->first;
								add = false;
								break;
							}

							it++;
						}
					}

					if(add)
					{
						console_item item = {type_string, false, _T("unclussified parameter"), '0'};
						_commands[name] = item;
					}
				}				
				//TODO: VALIDATE TYPE	
				_commands[name].psz_value.push_back(value);
			}
		}

		std::map<Cstring, console_item>::const_iterator it = _commands.begin();
		while(it != _commands.end())
		{
			if(it->second.required && 0 == it->second.psz_value.size())
			{
				_last_error = _T("missing parameter: ");
				_last_error += it->first;
				
				return false;

				
			}

			it++;
		}

		return true;
	}

	Cstring & get_error_message()
	{
		return _last_error;
	}

	int64_t get_integer64_value(const TCHAR * pszname, uint32_t idx)
	{
		return get_value(pszname, idx);
	}

	int64_t get_integer64_value(const TCHAR * pszname)
	{
		return get_value(pszname, 0);
	}

	int get_integer_value(const TCHAR * pszname)
	{
		return static_cast<int>(get_integer64_value(pszname));
	}

	Cstring get_value(const TCHAR * pszname, uint32_t idx)
	{
		_ASSERTE(_commands.find(pszname) != _commands.end());
		_ASSERTE(idx < _commands[pszname].psz_value.size());

		return _commands[pszname].psz_value[idx];
	}

    void set_value(const TCHAR * pszname, const TCHAR * value, uint32_t idx)
    {
		_ASSERTE(_commands.find(pszname) != _commands.end());
		_ASSERTE(idx < _commands[pszname].psz_value.size());


		_commands[pszname].psz_value[idx] = value;
    }

	Cstring get_value(const TCHAR * pszname)
	{
		return get_value(pszname, 0);
	}

	uint32_t get_command_count(const TCHAR * pszname)
	{
		//_ASSERTE(_commands.find(pszname) != _commands.end());
		return ST_U32(_commands[pszname].psz_value.size());
	}

	Cstring get_help()
	{
		Cstring t;
		std::map<Cstring, console_item>::const_iterator it = _commands.begin();
		while(it != _commands.end())
		{
			t += it->first;
			t += _T(":[value]\t");
			t += it->second.psz_description;
			if(it->second.required)
				t += _T(" - required");
			if(it->second.short_cut)
			{	
				t += _T(" - shortcut: ");
				TCHAR v[] = {it->second.short_cut, 0x0};
				t += v;
			}
			t += _T("\r\n");

			it++;
		}

		return t;
	}
	bool command_specified(const TCHAR * pszname, uint32_t idx = 0)
	{
		Cstring n(pszname);
		if(_commands.end() != _commands.find(n))
		{
			return (_commands[n].psz_value.size() > idx);
		}

		return false;
	}
};

__ALX_END_NAMESPACE

