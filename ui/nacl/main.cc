#include "ui/nacl/oort_instance.h"
#include "ui/gui.h"
#include <iostream>
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/gles2/gl2ext_ppapi.h"
#include "common/log.h"

using namespace Oort;

OortInstance *instance;

OortInstance::OortInstance(PP_Instance instance)
	: pp::Instance(instance) {}

bool OortInstance::Init(uint32_t argc, const char* argn[], const char* argv[]) {
	std::cout << "instance init" << std::endl;

	log("initializing ship classes");
	ShipClass::initialize();

	InputInit();
	GraphicsInit();

	return true;
}

namespace Oort {
	static void log_handler_cb(void* user_data, int32_t result) {
		char *msg = (char*)user_data;
		pp::Var var(msg);
		instance->PostMessage(var);
	}

	void log_handler(char *msg) {
		if (instance) {
			pp::Core* core = pp::Module::Get()->core();
			pp::CompletionCallback cb(log_handler_cb, (void*)msg);
			core->CallOnMainThread(0, cb);
		}
	}
}

class OortModule : public pp::Module {
public:
	OortModule() : pp::Module() {}

	virtual ~OortModule() {
		glTerminatePPAPI();
	}

	virtual bool Init() {
		log("OortModule::Init");
		return glInitializePPAPI(get_browser_interface()) == GL_TRUE;
	}

	virtual pp::Instance* CreateInstance(PP_Instance instance) {
		log("OortModule::CreateInstance");
		return (::instance = new OortInstance(instance));
	}
};

namespace pp {
	Module* CreateModule() {
		log("pp::CreateModule");
		return new OortModule();
	}
}
