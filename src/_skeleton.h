#pragma once
#ifndef _skeleton_h
#define _skeleton_h

#include <frm/AppSample.h>

class _skeleton: public frm::AppSample
{
	typedef AppSample AppBase;
public:
	_skeleton();
	virtual ~_skeleton();

	virtual bool init(const apt::ArgList& _args) override;
	virtual void shutdown() override;
	virtual bool update() override;
	virtual void draw() override;

protected:

};


#endif // _skeleton_h
