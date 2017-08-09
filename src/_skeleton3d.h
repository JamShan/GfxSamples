#pragma once
#ifndef _skeleton3d_h
#define _skeleton3d_h

#include <frm/AppSample3d.h>

class _skeleton3d: public frm::AppSample3d
{
	typedef AppSample3d AppBase;
public:
	_skeleton3d(): AppBase("_skeleton3d");
	virtual ~_skeleton3d();

	virtual bool init(const apt::ArgList& _args) override;
	virtual void shutdown() override;
	virtual bool update() override;
	virtual void draw() override;

protected:

};


#endif // AppSkeleton_h
