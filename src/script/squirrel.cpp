/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file squirrel.cpp the implementation of the Squirrel class. It handles all Squirrel-stuff and gives a nice API back to work with. */

#include <stdarg.h>
#include "../stdafx.h"
#include "../debug.h"
#include "../fileio_func.h"
#include "../string.h"
#include "squirrel.hpp"
#include <sqstdaux.h>
#include <../squirrel/sqpcheader.h>
#include <../squirrel/sqvm.h>

void Squirrel::CompileError(HSQUIRRELVM vm, const char *desc, const char *source, SQInteger line, SQInteger column)
{
	char buf[1024];
	bstrfmt (buf, "Error %s:" OTTD_PRINTF64 "/" OTTD_PRINTF64 ": %s", source, line, column, desc);

	/* Check if we have a custom print function */
	Squirrel *engine = Squirrel::Get(vm);
	engine->crashed = true;
	SQPrintFunc *func = engine->print_func;
	if (func == NULL) {
		DEBUG(misc, 0, "[Squirrel] Compile error: %s", buf);
	} else {
		(*func)(true, buf);
	}
}

void Squirrel::ErrorPrintFunc(HSQUIRRELVM vm, const char *s, ...)
{
	va_list arglist;
	char buf[1024];

	va_start(arglist, s);
	bstrvfmt (buf, s, arglist);
	va_end(arglist);

	/* Check if we have a custom print function */
	SQPrintFunc *func = Squirrel::Get(vm)->print_func;
	if (func == NULL) {
		fprintf(stderr, "%s", buf);
	} else {
		(*func)(true, buf);
	}
}

void Squirrel::RunError(HSQUIRRELVM vm, const char *error)
{
	/* Set the print function to something that prints to stderr */
	SQPRINTFUNCTION pf = sq_getprintfunc(vm);
	sq_setprintfunc(vm, &Squirrel::ErrorPrintFunc);

	/* Check if we have a custom print function */
	char buf[1024];
	bstrfmt (buf, "Your script made an error: %s\n", error);
	Squirrel *engine = Squirrel::Get(vm);
	SQPrintFunc *func = engine->print_func;
	if (func == NULL) {
		fprintf(stderr, "%s", buf);
	} else {
		(*func)(true, buf);
	}

	/* Print below the error the stack, so the users knows what is happening */
	sqstd_printcallstack(vm);
	/* Reset the old print function */
	sq_setprintfunc(vm, pf);
}

SQInteger Squirrel::_RunError(HSQUIRRELVM vm)
{
	const char *sErr = 0;

	if (sq_gettop(vm) >= 1) {
		if (SQ_SUCCEEDED(sq_getstring(vm, -1, &sErr))) {
			Squirrel::RunError(vm, sErr);
			return 0;
		}
	}

	Squirrel::RunError(vm, "unknown error");
	return 0;
}

void Squirrel::PrintFunc(HSQUIRRELVM vm, const char *s, ...)
{
	va_list arglist;
	va_start(arglist, s);

	/* Check if we have a custom print function */
	SQPrintFunc *func = Squirrel::Get(vm)->print_func;
	if (func == NULL) {
		vprintf(s, arglist);
		putchar('\n');
	} else {
		sstring<1024> buf;
		buf.vfmt (s, arglist);
		if (buf.full()) buf.truncate(buf.length() - 1);
		buf.append('\n');
		(*func)(false, buf.c_str());
	}

	va_end(arglist);
}

void Squirrel::AddMethod(const char *method_name, SQFUNCTION proc, uint nparam, const char *params, void *userdata, int size)
{
	sq_pushstring(this->vm, method_name, -1);

	if (size != 0) {
		void *ptr = sq_newuserdata(vm, size);
		memcpy(ptr, userdata, size);
	}

	sq_newclosure(this->vm, proc, size != 0 ? 1 : 0);
	if (nparam != 0) sq_setparamscheck(this->vm, nparam, params);
	sq_setnativeclosurename(this->vm, -1, method_name);
	sq_newslot(this->vm, -3, SQFalse);
}

void Squirrel::AddConst(const char *var_name, int value)
{
	sq_pushstring(this->vm, var_name, -1);
	sq_pushinteger(this->vm, value);
	sq_newslot(this->vm, -3, SQTrue);
}

void Squirrel::AddConst(const char *var_name, bool value)
{
	sq_pushstring(this->vm, var_name, -1);
	sq_pushbool(this->vm, value);
	sq_newslot(this->vm, -3, SQTrue);
}

void Squirrel::AddClassBegin(const char *class_name)
{
	sq_pushroottable(this->vm);
	sq_pushstring(this->vm, class_name, -1);
	sq_newclass(this->vm, SQFalse);
}

void Squirrel::AddClassBegin(const char *class_name, const char *parent_class)
{
	sq_pushroottable(this->vm);
	sq_pushstring(this->vm, class_name, -1);
	sq_pushstring(this->vm, parent_class, -1);
	if (SQ_FAILED(sq_get(this->vm, -3))) {
		DEBUG(misc, 0, "[squirrel] Failed to initialize class '%s' based on parent class '%s'", class_name, parent_class);
		DEBUG(misc, 0, "[squirrel] Make sure that '%s' exists before trying to define '%s'", parent_class, class_name);
		return;
	}
	sq_newclass(this->vm, SQTrue);
}

void Squirrel::AddClassEnd()
{
	sq_newslot(vm, -3, SQFalse);
	sq_pop(vm, 1);
}

bool Squirrel::MethodExists(HSQOBJECT instance, const char *method_name)
{
	assert(!this->crashed);
	int top = sq_gettop(this->vm);
	/* Go to the instance-root */
	sq_pushobject(this->vm, instance);
	/* Find the function-name inside the script */
	sq_pushstring(this->vm, method_name, -1);
	if (SQ_FAILED(sq_get(this->vm, -2))) {
		sq_settop(this->vm, top);
		return false;
	}
	sq_settop(this->vm, top);
	return true;
}

bool Squirrel::Resume(int suspend)
{
	assert(!this->crashed);
	/* Did we use more operations than we should have in the
	 * previous tick? If so, subtract that from the current run. */
	if (this->overdrawn_ops > 0 && suspend > 0) {
		this->overdrawn_ops -= suspend;
		/* Do we need to wait even more? */
		if (this->overdrawn_ops >= 0) return true;

		/* We can now only run whatever is "left". */
		suspend = -this->overdrawn_ops;
	}

	this->crashed = !sq_resumecatch(this->vm, suspend);
	this->overdrawn_ops = -this->vm->_ops_till_suspend;
	return this->vm->_suspended != 0;
}

void Squirrel::ResumeError()
{
	assert(!this->crashed);
	sq_resumeerror(this->vm);
}

void Squirrel::CollectGarbage()
{
	sq_collectgarbage(this->vm);
}

bool Squirrel::CallMethod (HSQOBJECT instance, const char *method_name, int suspend, HSQOBJECT *ret)
{
	assert(!this->crashed);
	/* Store the stack-location for the return value. We need to
	 * restore this after saving or the stack will be corrupted
	 * if we're in the middle of a DoCommand. */
	SQInteger last_target = this->vm->_suspended_target;
	/* Store the current top */
	int top = sq_gettop(this->vm);
	/* Go to the instance-root */
	sq_pushobject(this->vm, instance);
	/* Find the function-name inside the script */
	sq_pushstring(this->vm, method_name, -1);
	if (SQ_FAILED(sq_get(this->vm, -2))) {
		DEBUG(misc, 0, "[squirrel] Could not find '%s' in the class", method_name);
		sq_settop(this->vm, top);
		return false;
	}
	/* Call the method */
	sq_pushobject(this->vm, instance);
	if (SQ_FAILED(sq_call(this->vm, 1, ret == NULL ? SQFalse : SQTrue, SQTrue, suspend))) return false;
	if (ret != NULL) sq_getstackobj(vm, -1, ret);
	/* Reset the top, but don't do so for the script main function, as we need
	 *  a correct stack when resuming. */
	if (suspend == -1 || !this->IsSuspended()) sq_settop(this->vm, top);
	/* Restore the return-value location. */
	this->vm->_suspended_target = last_target;

	return true;
}

/* static */ bool Squirrel::CreateClassInstanceVM(HSQUIRRELVM vm, const char *class_name, void *real_instance, HSQOBJECT *instance, SQRELEASEHOOK release_hook, bool prepend_API_name)
{
	Squirrel *engine = Squirrel::Get(vm);

	int oldtop = sq_gettop(vm);

	/* First, find the class */
	sq_pushroottable(vm);

	if (prepend_API_name) {
		char *class_name2 = (char *)alloca(strlen(class_name) + strlen(engine->GetAPIName()) + 1);
		sprintf(class_name2, "%s%s", engine->GetAPIName(), class_name);

		sq_pushstring(vm, class_name2, -1);
	} else {
		sq_pushstring(vm, class_name, -1);
	}

	if (SQ_FAILED(sq_get(vm, -2))) {
		DEBUG(misc, 0, "[squirrel] Failed to find class by the name '%s%s'", prepend_API_name ? engine->GetAPIName() : "", class_name);
		sq_settop(vm, oldtop);
		return false;
	}

	/* Create the instance */
	if (SQ_FAILED(sq_createinstance(vm, -1))) {
		DEBUG(misc, 0, "[squirrel] Failed to create instance for class '%s%s'", prepend_API_name ? engine->GetAPIName() : "", class_name);
		sq_settop(vm, oldtop);
		return false;
	}

	if (instance != NULL) {
		/* Find our instance */
		sq_getstackobj(vm, -1, instance);
		/* Add a reference to it, so it survives for ever */
		sq_addref(vm, instance);
	}
	sq_remove(vm, -2); // Class-name
	sq_remove(vm, -2); // Root-table

	/* Store it in the class */
	sq_setinstanceup(vm, -1, real_instance);
	if (release_hook != NULL) sq_setreleasehook(vm, -1, release_hook);

	if (instance != NULL) sq_settop(vm, oldtop);

	return true;
}

bool Squirrel::CreateClassInstance(const char *class_name, void *real_instance, HSQOBJECT *instance)
{
	return Squirrel::CreateClassInstanceVM(this->vm, class_name, real_instance, instance, NULL);
}

static SQInteger squirrel_require (HSQUIRRELVM vm)
{
	SQInteger top = sq_gettop(vm);
	const char *filename;

	sq_getstring(vm, 2, &filename);

	/* Get the script-name of the current file, so we can work relative from it */
	SQStackInfos si;
	sq_stackinfos(vm, 1, &si);
	if (si.source == NULL) {
		DEBUG(misc, 0, "[squirrel] Couldn't detect the script-name of the 'require'-caller; this should never happen!");
		return SQ_ERROR;
	}

	/* Keep the dir, remove the rest */
	const char *pathsep = strrchr (si.source, PATHSEPCHAR);
	char *path;
	if (pathsep == NULL) {
		path = str_fmt ("%s%s", si.source, filename);
	} else {
		/* Keep the PATHSEPCHAR there, remove the rest */
		path = str_fmt ("%.*s%s",
			(int)(pathsep - si.source + 1), si.source, filename);
	}

	/* Tars dislike opening files with '/' on Windows.. so convert it to '\\' ;) */
#if (PATHSEPCHAR != '/')
	for (char *n = path; *n != '\0'; n++) if (*n == '/') *n = PATHSEPCHAR;
#endif

	bool ret = Squirrel::Get(vm)->LoadScript (path);

	/* Reset the top, so the stack stays correct */
	sq_settop(vm, top);
	free(path);

	return ret ? 0 : SQ_ERROR;
}

static SQInteger squirrel_notifyallexceptions (HSQUIRRELVM vm)
{
	SQBool b;

	if (sq_gettop(vm) >= 1) {
		if (SQ_SUCCEEDED(sq_getbool(vm, -1, &b))) {
			sq_notifyallexceptions(vm, b);
			return 0;
		}
	}

	return SQ_ERROR;
}

void Squirrel::Initialize()
{
	this->print_func = NULL;
	this->crashed = false;
	this->overdrawn_ops = 0;
	this->vm = sq_open(1024);

	/* Handle compile-errors ourself, so we can display it nicely */
	sq_setcompilererrorhandler(this->vm, &Squirrel::CompileError);
	sq_notifyallexceptions(this->vm, SQTrue);
	/* Set a good print-function */
	sq_setprintfunc(this->vm, &Squirrel::PrintFunc);
	/* Handle runtime-errors ourself, so we can display it nicely */
	sq_newclosure(this->vm, &Squirrel::_RunError, 0);
	sq_seterrorhandler(this->vm);

	/* Set the foreign pointer, so we can always find this instance from within the VM */
	sq_setforeignptr(this->vm, this);

	sq_pushroottable(this->vm);

	/* We don't use squirrel_helper here, as we want to register
	 * to the global scope and not to a class. */
	this->AddMethod ("require",             &squirrel_require,             2, ".s");
	this->AddMethod ("notifyallexceptions", &squirrel_notifyallexceptions, 2, ".b");
}

class SQFile {
private:
	FILE *file;
	size_t size;
	size_t pos;

public:
	SQFile(FILE *file, size_t size) : file(file), size(size), pos(0) {}

	size_t Read(void *buf, size_t elemsize, size_t count)
	{
		assert(elemsize != 0);
		if (this->pos + (elemsize * count) > this->size) {
			count = (this->size - this->pos) / elemsize;
		}
		if (count == 0) return 0;
		size_t ret = fread(buf, elemsize, count, this->file);
		this->pos += ret * elemsize;
		return ret;
	}
};

static WChar _io_file_lexfeed_ASCII(SQUserPointer file)
{
	unsigned char c;
	if (((SQFile *)file)->Read(&c, sizeof(c), 1) > 0) return c;
	return 0;
}

static WChar _io_file_lexfeed_UTF8(SQUserPointer file)
{
	char buffer[5];

	/* Read the first character, and get the length based on UTF-8 specs. If invalid, bail out. */
	if (((SQFile *)file)->Read(buffer, sizeof(buffer[0]), 1) != 1) return 0;
	uint len = Utf8EncodedCharLen(buffer[0]);
	if (len == 0) return -1;

	/* Read the remaining bits. */
	if (len > 1 && ((SQFile *)file)->Read(buffer + 1, sizeof(buffer[0]), len - 1) != len - 1) return 0;

	/* Convert the character, and when definitely invalid, bail out as well. */
	WChar c;
	if (Utf8Decode(&c, buffer) != len) return -1;

	return c;
}

static WChar _io_file_lexfeed_UCS2_no_swap(SQUserPointer file)
{
	unsigned short c;
	if (((SQFile *)file)->Read(&c, sizeof(c), 1) > 0) return (WChar)c;
	return 0;
}

static WChar _io_file_lexfeed_UCS2_swap(SQUserPointer file)
{
	unsigned short c;
	if (((SQFile *)file)->Read(&c, sizeof(c), 1) > 0) {
		c = ((c >> 8) & 0x00FF)| ((c << 8) & 0xFF00);
		return (WChar)c;
	}
	return 0;
}

static SQInteger _io_file_read(SQUserPointer file, SQUserPointer buf, SQInteger size)
{
	SQInteger ret = ((SQFile *)file)->Read(buf, 1, size);
	if (ret == 0) return -1;
	return ret;
}

SQRESULT Squirrel::LoadFile(HSQUIRRELVM vm, const char *filename, SQBool printerror)
{
	size_t size;
	FILE *file;
	SQInteger ret;
	unsigned short us;
	unsigned char uc;
	SQLEXREADFUNC func;

	if (strncmp(this->GetAPIName(), "AI", 2) == 0) {
		file = FioFOpenFile(filename, "rb", AI_DIR, &size);
		if (file == NULL) file = FioFOpenFile(filename, "rb", AI_LIBRARY_DIR, &size);
	} else if (strncmp(this->GetAPIName(), "GS", 2) == 0) {
		file = FioFOpenFile(filename, "rb", GAME_DIR, &size);
		if (file == NULL) file = FioFOpenFile(filename, "rb", GAME_LIBRARY_DIR, &size);
	} else {
		NOT_REACHED();
	}

	if (file != NULL) {
		SQFile f(file, size);
		ret = fread(&us, 1, sizeof(us), file);
		/* Most likely an empty file */
		if (ret != 2) us = 0;

		switch (us) {
			case SQ_BYTECODE_STREAM_TAG: { // BYTECODE
				if (fseek(file, -2, SEEK_CUR) < 0) {
					FioFCloseFile(file);
					return sq_throwerror(vm, "cannot seek the file");
				}
				if (SQ_SUCCEEDED(sq_readclosure(vm, _io_file_read, &f))) {
					FioFCloseFile(file);
					return SQ_OK;
				}
				FioFCloseFile(file);
				return sq_throwerror(vm, "Couldn't read bytecode");
			}
			case 0xFFFE:
				/* Either this file is encoded as big-endian and we're on a little-endian
				 * machine, or this file is encoded as little-endian and we're on a big-endian
				 * machine. Either way, swap the bytes of every word we read. */
				func = _io_file_lexfeed_UCS2_swap;
				break;
			case 0xFEFF: func = _io_file_lexfeed_UCS2_no_swap; break;
			case 0xBBEF: // UTF-8
			case 0xEFBB: // UTF-8 on big-endian machine
				if (fread(&uc, 1, sizeof(uc), file) == 0) {
					FioFCloseFile(file);
					return sq_throwerror(vm, "I/O error");
				}
				if (uc != 0xBF) {
					FioFCloseFile(file);
					return sq_throwerror(vm, "Unrecognized encoding");
				}
				func = _io_file_lexfeed_UTF8;
				break;
			default: // ASCII
				func = _io_file_lexfeed_ASCII;
				if (fseek(file, -2, SEEK_CUR) < 0) {
					FioFCloseFile(file);
					return sq_throwerror(vm, "cannot seek the file");
				}
				break;
		}

		if (SQ_SUCCEEDED(sq_compile(vm, func, &f, filename, printerror))) {
			FioFCloseFile(file);
			return SQ_OK;
		}
		FioFCloseFile(file);
		return SQ_ERROR;
	}
	return sq_throwerror(vm, "cannot open the file");
}

bool Squirrel::LoadScript(HSQUIRRELVM vm, const char *script, bool in_root)
{
	/* Make sure we are always in the root-table */
	if (in_root) sq_pushroottable(vm);

	SQInteger ops_left = vm->_ops_till_suspend;
	/* Load and run the script */
	if (SQ_SUCCEEDED(LoadFile(vm, script, SQTrue))) {
		sq_push(vm, -2);
		if (SQ_SUCCEEDED(sq_call(vm, 1, SQFalse, SQTrue, 100000))) {
			sq_pop(vm, 1);
			/* After compiling the file we want to reset the amount of opcodes. */
			vm->_ops_till_suspend = ops_left;
			return true;
		}
	}

	vm->_ops_till_suspend = ops_left;
	DEBUG(misc, 0, "[squirrel] Failed to compile '%s'", script);
	return false;
}

bool Squirrel::LoadScript(const char *script)
{
	return LoadScript(this->vm, script);
}

void Squirrel::Uninitialize()
{
	/* Clean up the stuff */
	sq_pop(this->vm, 1);
	sq_close(this->vm);
}

void Squirrel::InsertResult(bool result)
{
	sq_pushbool(this->vm, result);
	if (this->IsSuspended()) { // Called before resuming a suspended script?
		vm->GetAt(vm->_stackbase + vm->_suspended_target) = vm->GetUp(-1);
		vm->Pop();
	}
}

void Squirrel::InsertResult(int result)
{
	sq_pushinteger(this->vm, result);
	if (this->IsSuspended()) { // Called before resuming a suspended script?
		vm->GetAt(vm->_stackbase + vm->_suspended_target) = vm->GetUp(-1);
		vm->Pop();
	}
}

/* static */ void Squirrel::DecreaseOps(HSQUIRRELVM vm, int ops)
{
	vm->DecreaseOps(ops);
}

bool Squirrel::IsSuspended()
{
	return this->vm->_suspended != 0;
}

bool Squirrel::HasScriptCrashed()
{
	return this->crashed;
}

void Squirrel::CrashOccurred()
{
	this->crashed = true;
}

bool Squirrel::CanSuspend()
{
	return sq_can_suspend(this->vm);
}

SQInteger Squirrel::GetOpsTillSuspend()
{
	return this->vm->_ops_till_suspend;
}
