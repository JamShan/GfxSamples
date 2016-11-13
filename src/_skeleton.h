#pragma once
#ifndef App_skeleton_h
#define App_skeleton_h

#include <frm/AppSample.h>

class App_skeleton: public frm::AppSample
{
	typedef AppSample AppBase;
public:
	App_skeleton(): AppBase("_skeleton") {}
	virtual ~App_skeleton() {}

	virtual bool init(const apt::ArgList& _args) override;
	virtual void shutdown() override;
	virtual bool update() override;
	virtual void draw() override;

protected:

};


#endif // App_skeleton_h
