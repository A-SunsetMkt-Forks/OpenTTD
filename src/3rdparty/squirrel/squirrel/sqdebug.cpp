/*
 * see copyright notice in squirrel.h
 */

#include "../../../stdafx.h"
#include "../../fmt/format.h"

#include <squirrel.h>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"

#include "../../../string_func.h"

#include "../../../safeguards.h"

SQRESULT sq_getfunctioninfo(HSQUIRRELVM v,SQInteger level,SQFunctionInfo *fi)
{
	SQInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		SQVM::CallInfo &ci = v->_callsstack[cssize-level-1];
		if(sq_isclosure(ci._closure)) {
			SQClosure *c = _closure(ci._closure);
			SQFunctionProto *proto = _funcproto(c->_function);
			fi->funcid = proto;
			fi->name = type(proto->_name) == OT_STRING?_stringval(proto->_name):"unknown";
			fi->source = type(proto->_name) == OT_STRING?_stringval(proto->_sourcename):"unknown";
			return SQ_OK;
		}
	}
	return sq_throwerror(v,"the object is not a closure");
}

SQRESULT sq_stackinfos(HSQUIRRELVM v, SQInteger level, SQStackInfos *si)
{
	SQInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		*si = {};
		SQVM::CallInfo &ci = v->_callsstack[cssize-level-1];
		switch (type(ci._closure)) {
		case OT_CLOSURE:{
			SQFunctionProto *func = _funcproto(_closure(ci._closure)->_function);
			if (type(func->_name) == OT_STRING)
				si->funcname = _stringval(func->_name);
			if (type(func->_sourcename) == OT_STRING)
				si->source = _stringval(func->_sourcename);
			si->line = func->GetLine(ci._ip);
						}
			break;
		case OT_NATIVECLOSURE:
			si->source = "NATIVE";
			si->funcname = "unknown";
			if(type(_nativeclosure(ci._closure)->_name) == OT_STRING)
				si->funcname = _stringval(_nativeclosure(ci._closure)->_name);
			si->line = -1;
			break;
		default: break; //shutup compiler
		}
		return SQ_OK;
	}
	return SQ_ERROR;
}

void SQVM::Raise_Error(const std::string &msg)
{
	_lasterror = SQString::Create(_ss(this),msg);
}

void SQVM::Raise_Error(SQObjectPtr &desc)
{
	_lasterror = desc;
}

SQString *SQVM::PrintObjVal(const SQObject &o)
{
	switch(type(o)) {
	case OT_STRING: return _string(o);
	case OT_INTEGER:
		return SQString::Create(_ss(this), fmt::format("{}", _integer(o)));
	case OT_FLOAT:
		return SQString::Create(_ss(this), fmt::format("{:.14g}", _float(o)));
	default:
		return SQString::Create(_ss(this), GetTypeName(o));
	}
}

void SQVM::Raise_IdxError(const SQObject &o)
{
	SQObjectPtr oval = PrintObjVal(o);
	Raise_Error(fmt::format("the index '{:.50s}' does not exist", _stringval(oval)));
}

void SQVM::Raise_CompareError(const SQObject &o1, const SQObject &o2)
{
	SQObjectPtr oval1 = PrintObjVal(o1), oval2 = PrintObjVal(o2);
	Raise_Error(fmt::format("comparison between '{:.50s}' and '{:.50s}'", _stringval(oval1), _stringval(oval2)));
}


void SQVM::Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type)
{
	SQObjectPtr exptypes = SQString::Create(_ss(this), "");
	SQInteger found = 0;
	for(SQInteger i=0; i<16; i++)
	{
		SQInteger mask = 0x00000001LL << i;
		if(typemask & (mask)) {
			if(found>0) StringCat(exptypes,SQString::Create(_ss(this), "|"), exptypes);
			found ++;
			StringCat(exptypes,SQString::Create(_ss(this), IdType2Name((SQObjectType)mask)), exptypes);
		}
	}
	Raise_Error(fmt::format("parameter {} has an invalid type '{}' ; expected: '{}'", nparam, IdType2Name((SQObjectType)type), _stringval(exptypes)));
}
