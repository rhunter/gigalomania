//---------------------------------------------------------------------------
#include "stdafx.h"

#include "resources.h"
#include "utils.h"

#include <cstring>

//---------------------------------------------------------------------------
#ifndef NULL
#define NULL 0
#endif

using namespace Gigalomania;

//Vector TrackedObject::tags;
vector<TrackedObject *> TrackedObject::tags;
//VisionException *Vision::error = NULL;

TrackedObject::TrackedObject() {
	this->tag = TrackedObject::addTag(this);
	//LOG("New Tracked Object, tag = %d\n", this->tag);
	this->deleteLevel = 0;
}

TrackedObject::~TrackedObject() {
	//LOG("Deleting Tracked Object, tag = %d\n", this->tag);
	removeTag(this->tag);
}

void TrackedObject::flushAll() {
	LOG("TrackedObject::flushAll()\n");
	for(size_t i=0;i<tags.size();i++) {
		TrackedObject *vo = tags.at(i);
		if(vo != NULL) {
			delete vo;
			tags[i] = NULL;
		}
	}
}

void TrackedObject::flush(int deleteLevel) {
	LOG("TrackedObject::flush(%d)\n", deleteLevel);
	for(size_t i=0;i<tags.size();i++) {
		TrackedObject *vo = tags.at(i);
		if(vo != NULL && vo->deleteLevel >= deleteLevel) {
			delete vo;
			tags[i] = NULL;
		}
	}
}

void TrackedObject::cleanup() {
	LOG("TrackedObject::cleanup()\n");
	flushAll();

	/*if(error != NULL)
	delete error;*/

	/*#ifdef _DEBUG
	dumpAllocs();
	#endif*/
}

size_t TrackedObject::addTag(TrackedObject *ptr) {
	size_t tag = tags.size() + 1;
	tags.push_back(ptr);
	return tag;
}

TrackedObject *TrackedObject::ptrFromTag(size_t tag) {
	TrackedObject *ptr = NULL;
	if( tag == 0 ||tag > tags.size() ) {
		// error
		ptr = NULL;
	}
	else {
		ptr = tags.at(tag-1);
	}
	return ptr;
}

void TrackedObject::removeTag(size_t tag) {
	//tags.getData()[tag - 1] = NULL;
	tags[tag-1] = NULL;
}

size_t TrackedObject::getNumTags() {
	return tags.size();
}

TrackedObject *TrackedObject::getTag(size_t index) {
	//return (TrackedObject *)tags.elementAt(index);
	return tags.at(index);
}

/*VisionException *Vision::getError() {
return error;
}

void Vision::setError(VisionException *ve) {
if(error != NULL)
delete error;
throw ve;
error = ve;
}*/

bool TrackedObject::isClass(const char *classname) const {
	return ( strcmp( this->getClass(), classname ) == 0 );
}

