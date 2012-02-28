#include "ui/nacl/oort_instance.h"
#include "ppapi/gles2/gl2ext_ppapi.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/completion_callback.h"
#include "gl/gl.h"
#include "ui/gui.h"
#include "renderer/renderer.h"
#include "common/log.h"

using namespace Oort;

enum {
	initial_screen_width = 800,
	initial_screen_height = 600,
};

void OortInstance::GraphicsInit() {
	int32_t attribs[] = {
		PP_GRAPHICS3DATTRIB_WIDTH, initial_screen_width,
		PP_GRAPHICS3DATTRIB_HEIGHT, initial_screen_height,
		PP_GRAPHICS3DATTRIB_NONE
	};

	gl_context = pp::Graphics3D(this, pp::Graphics3D(), attribs);
	if (gl_context.is_null()) {
		glSetCurrentContextPPAPI(0);
		throw std::runtime_error("Failed to create pp::Graphics3D");
	}

	this->BindGraphics(gl_context);
	glSetCurrentContextPPAPI(gl_context.pp_resource());
	log("graphics bound");

	log("OpenGL extensions: %s", glGetString(GL_EXTENSIONS));
	glClearColor(0.0f, 0.0f, 0.03f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	log("cleared");

	schedule_swap();
	log("finished init");
}

void OortInstance::schedule_swap() {
	//log("scheduling swap: fn=%p data=%p", static_swap_callback, this);
	pp::CompletionCallback cb(static_swap_callback, this);
	gl_context.SwapBuffers(cb);
}

void OortInstance::swap_callback() {
	if (gui) {
		gui->render();
	}
	schedule_swap();
}

void OortInstance::static_swap_callback(void* user_data, int32_t result)
{
	OortInstance *instance = (OortInstance*)user_data;
	instance->swap_callback();
}

void OortInstance::DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
	log("DidChangeView");
	size = position.size();
	gl_context.ResizeBuffers(size.width(), size.height());
	if (gui) {
		gui->handle_resize(size.width(), size.height());
	}
}
