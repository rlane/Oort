#include "renderer/renderer.h"
#include <boost/foreach.hpp>
#include <boost/units/detail/utility.hpp>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "gl/check.h"
#include "renderer/batches/ship.h"
#include "renderer/batches/tail.h"
#include "renderer/batches/bullet.h"
#include "renderer/batches/beam.h"
#include "renderer/batches/text.h"
#include "renderer/batches/particle.h"
#include "renderer/batches/clear.h"
#include "renderer/batches/debug_lines.h"

using glm::vec2;
using glm::vec4;
using std::make_shared;
using std::shared_ptr;
using boost::units::detail::demangle;
using namespace Oort::RendererBatches;

namespace Oort {

std::vector<vec2> Renderer::jitters{
#ifdef __native_client__
#define SAMPLES 4
#else
#define SAMPLES 1
#endif

#if SAMPLES == 4
#warning "4 samples"
	vec2(0.375, 0.25),
	vec2(0.125, 0.75),
	vec2(0.875, 0.25),
	vec2(0.625, 0.75),
#elif SAMPLES == 16
#warning "16 samples"
	vec2(0.375, 0.4375), vec2(0.625, 0.0625), vec2(0.875, 0.1875), vec2(0.125, 0.0625),
	vec2(0.375, 0.6875), vec2(0.875, 0.4375), vec2(0.625, 0.5625), vec2(0.375, 0.9375),
	vec2(0.625, 0.3125), vec2(0.125, 0.5625), vec2(0.125, 0.8125), vec2(0.375, 0.1875),
	vec2(0.875, 0.9375), vec2(0.875, 0.6875), vec2(0.125, 0.3125), vec2(0.625, 0.8125),
#elif SAMPLES == 1
	vec2(0, 0),
#else
#error "unsupported number of samples"
#endif
};

Renderer::Renderer() {
	benchmark = false;
	render_all_debug_lines = false;

	GL::check();
	glEnable(GL_BLEND);
#ifndef __native_client__
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_PROGRAM_POINT_SIZE);
#endif
	GL::check();

	add_batch<ClearBatch>();
	add_batch<TailBatch>();
	add_batch<BulletBatch>();
	add_batch<BeamBatch>();
	add_batch<ParticleBatch>();
	add_batch<ShipBatch>();
	add_batch<DebugLinesBatch>();
	add_batch<TextBatch>();
	GL::check();
}

template <typename T>
void Renderer::add_batch() {
	batches.push_back(std::make_shared<T>(*this));
}

void Renderer::reshape(int screen_width, int screen_height) {
	this->screen_width = screen_width;
	this->screen_height = screen_height;
	this->aspect_ratio = float(screen_width)/screen_height;
}

void Renderer::render(float view_radius,
                      glm::vec2 view_center,
                      float time_delta) {
	Timer timer;
	GL::check();

	p_matrix = glm::ortho(view_center.x - view_radius,
	                      view_center.x + view_radius,
	                      view_center.y - view_radius/aspect_ratio,
	                      view_center.y + view_radius/aspect_ratio);

	view_scale = screen_width/view_radius;

	BOOST_FOREACH(auto batch, batches) {
		//log("rendering batch %s", typeid(*batch).name());
		Timer timer;
		batch->render(time_delta);
		//GL::check();
		if (benchmark) {
			glFinish();
			batch->render_perf.update(timer);
		}
	}

	glFlush();
	GL::check();

	texts.clear();

	if (benchmark) {
		render_perf.update(timer);
	}
}

void Renderer::snapshot(const Game &game) {
	Timer timer;
	BOOST_FOREACH(auto batch, batches) {
		//log("renderer snapshotting batch %s", typeid(*batch).name());
		Timer timer;
		batch->snapshot(game);
		if (benchmark) {
			batch->snapshot_perf.update(timer);
		}
	}
	if (benchmark) {
		snapshot_perf.update(timer);
	}
}

vec2 Renderer::pixel2screen(vec2 p) {
	return vec2((float) (2*p.x/screen_width-1),
	            (float) (-2*p.y/screen_height+1));
}

void Renderer::text(int x, int y, const std::string &str) {
	texts.emplace_back(Text{x, y, str});
}

void Renderer::dump_perf() {
	log("render   overall: %s", render_perf.summary().c_str());
	log("snapshot overall: %s", snapshot_perf.summary().c_str());
	BOOST_FOREACH(auto batch, batches) {
		auto name_str = demangle(typeid(*batch).name());
		auto name = strrchr(name_str.c_str(), ':') + 1;
		log("render   %13s %s", name, batch->render_perf.summary().c_str());
		log("snapshot %13s %s", name, batch->snapshot_perf.summary().c_str());
	}
}

}
