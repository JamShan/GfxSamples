#include <frm/def.h>
#include <frm/gl.h>
#include <frm/AppSample.h>
#include <frm/GlContext.h>
#include <frm/Window.h>

#include <apt/ArgList.h>

using namespace frm;
using namespace apt;

int main(int _argc, char** _argv)
{
	AppSample* app = AppSample::GetCurrent();
	if (!app->init(ArgList(_argc, _argv))) {
		APT_ASSERT(false);
		return 1;
	}
	Window* win = app->getWindow();
	GlContext* ctx = app->getGlContext();
	while (app->update()) {
		APT_VERIFY(GlContext::MakeCurrent(ctx));
		ctx->setFramebufferAndViewport(0);
		glAssert(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		glAssert(glClear(GL_COLOR_BUFFER_BIT));
		app->draw();
	}
	app->shutdown();
	return 0;
}
