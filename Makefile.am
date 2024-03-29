NULL = 

AUTOMAKE_OPTIONS = foreign

noinst_HEADERS = \
  gl/gl.h gl/buffer.h gl/check.h gl/program.h gl/shader.h gl/texture.h \
  common/log.h common/resources.h common/constexpr.h \
  sim/game.h sim/ship.h sim/ship_class.h sim/team.h sim/ai.h \
	sim/scenario.h sim/entity.h sim/bullet.h sim/test.h sim/math_util.h \
	sim/ai_lib.h sim/model.h sim/beam.h sim/weapon.h sim/explosion.h \
	renderer/renderer.h renderer/physics_debug_renderer.h renderer/font.h \
	renderer/batch.h renderer/batches/ship.h renderer/batches/tail.h \
	renderer/batches/bullet.h renderer/batches/beam.h renderer/batches/text.h \
	renderer/batches/particle.h common/perf.h renderer/batches/clear.h \
	renderer/batches/debug_lines.h test/testcase.h ui/nacl/oort_instance.h

AM_CXXFLAGS = \
	-include $(CONFIG_HEADER) \
	-I $(top_srcdir) \
	$(GLM_CFLAGS) \
	$(LUA_CFLAGS) \
	$(BOX2D_CFLAGS) \
	$(GLEW_CFLAGS) \
	$(NACL_CFLAGS) \
	$(SDL_CFLAGS) \
	$(JSON_CFLAGS) \
	$(NULL)

SIM_SRCS = \
  sim/ai.cc \
  sim/ai_lib.cc \
  sim/bullet.cc \
  sim/builtin_ai.cc \
  sim/beam.cc \
  sim/entity.cc \
  sim/explosion.cc \
  sim/game.cc \
  sim/model.cc \
  sim/scenario.cc \
  sim/ship.cc \
  sim/ship_class.cc \
  sim/test.cc

RENDERER_SRCS = \
	renderer/physics_debug_renderer.cc \
	renderer/renderer.cc \
	renderer/font.c \
	renderer/batches/ship.cc \
	renderer/batches/tail.cc \
	renderer/batches/bullet.cc \
	renderer/batches/beam.cc \
	renderer/batches/text.cc \
	renderer/batches/particle.cc \
	renderer/batches/clear.cc \
	renderer/batches/debug_lines.cc \
	$(NULL)

TESTS := test/basic.so test/beam.so test/benchmark_bullet.so test/chase.so test/furball.so test/gun.so test/missile.so test/move.so

TEST_SCENARIO_SRCS = $(wildcard $(top_srcdir)/test/*.scn)
TEST_SCENARIOS = $(patsubst $(top_srcdir)/test/%.scn,$(top_builddir)/test/%.json,$(TEST_SCENARIO_SRCS))

SCENARIO_SRCS = $(wildcard $(top_srcdir)/scenarios/*.scn)
SCENARIOS = $(patsubst $(top_srcdir)/scenarios/%.scn,$(top_builddir)/scenarios/%.json,$(SCENARIO_SRCS))

GLM_DIST := $(shell find $(top_srcdir)/third_party/glm -path $(top_srcdir)/third_party/glm/.git -prune -o -type f -a -print)
JSON_DIST := $(shell find $(top_srcdir)/third_party/json-spirit -path $(top_srcdir)/third_party/json-spirit/.git -prune -o -type f -a -print)
INSRC_DIST := $(shell find $(top_srcdir)/third_party/insrc -path $(top_srcdir)/third_party/insrc/.git -prune -o -type f -a -print)
SHADERS_DIST := $(wildcard $(top_srcdir)/shaders/*.glsl)
MODELS_DIST := $(wildcard $(top_srcdir)/models/*.json)

EXTRA_DIST = $(GLM_DIST) $(JSON_DIST) $(INSRC_DIST) $(TEST_SRCS) $(SHADERS_DIST) $(MODELS_DIST)
CLEANFILES = $(TESTS)

bin_PROGRAMS := $(FRONTENDS)
EXTRA_PROGRAMS := oort_headless oort_sdl oort_nacl

all: $(bin_PROGRAMS) $(TESTS) $(TEST_SCENARIOS) $(SCENARIOS)

oort_headless_SOURCES := ui/headless/main.cc $(SIM_SRCS)
oort_headless_LDADD=$(LUA_LIBS) $(BOX2D_LIBS) $(BOOST_LIBS) -lrt

oort_sdl_SOURCES := ui/gui.cc ui/sdl/main.cc $(SIM_SRCS) $(RENDERER_SRCS)
oort_sdl_LDADD=$(SDL_LIBS) $(GLEW_LIBS) $(LUA_LIBS) $(BOX2D_LIBS) $(BOOST_LIBS) -lrt -lGL

oort_nacl_SOURCES := ui/gui.cc ui/nacl/main.cc ui/nacl/embedded_resources.c ui/nacl/messaging.cc ui/nacl/input.cc ui/nacl/graphics.cc $(SIM_SRCS) $(RENDERER_SRCS)
oort_nacl_LDADD=$(SDL_LIBS) $(NACL_LIBS) $(LUA_LIBS) $(BOX2D_LIBS)

TESTS_ENVIRONMENT = $(top_builddir)/oort_headless -t

test/%.so: test/%.cc
	@mkdir -p test
	$(AM_V_CXX) $(CXX) -shared -fPIC $(CXXFLAGS) $(AM_CXXFLAGS) $< -o $@

lint:
	find sim renderer ui gl -iname '*.cc' -o -iname '*.h' | xargs python cpplint.py --filter=-whitespace/tab,-build/header_guard,-\whitespace/blank_line,-whitespace/labels

# TODO depend on resource files
ui/nacl/embedded_resources.c: $(top_srcdir)/resources-manifest $(EXTRA_DIST) $(TEST_SCENARIOS) $(SCENARIOS)
	$(RUBY) $(top_srcdir)/third_party/insrc/insrc.rb -d $(top_builddir) -d $(top_srcdir) $(top_srcdir)/resources-manifest > $(top_builddir)/ui/nacl/embedded_resources.c

%.json: %.scn devel/scenario_builder.rb
	@mkdir -p $(dir $@)
	$(AM_V_GEN)$(RUBY) $(top_srcdir)/devel/scenario_builder.rb $< > $@
