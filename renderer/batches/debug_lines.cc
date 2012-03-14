#include "renderer/batches/debug_lines.h"

#include <memory>
#include <boost/foreach.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "sim/game.h"
#include "sim/ship.h"
#include "sim/ship_class.h"
#include "sim/team.h"
#include "gl/check.h"
#include "common/resources.h"
#include "renderer/bunch.h"

using glm::vec2;
using glm::vec4;
using std::make_shared;
using std::shared_ptr;

namespace Oort {
namespace RendererBatches {

struct DebugLinesVertex {
  glm::vec2 p;
  glm::vec4 color;
};

struct DebugLinesSegment {
  DebugLinesVertex a, b;
};

template<> std::vector<GLuint> Bunch<DebugLine>::buffer_freelist = std::vector<GLuint>();

struct DebugLinesPriv {
  GL::Program debug_line_prog;
  std::list<Bunch<DebugLine>> bunches;

  DebugLinesPriv()
    : debug_line_prog(GL::Program::from_resources("debug_line"))
  {
  }

};

DebugLinesBatch::DebugLinesBatch(Renderer &renderer)
  : Batch(renderer),
    priv(make_shared<DebugLinesPriv>())
{
}

void DebugLinesBatch::render(float time_delta) {
  if (!renderer.render_all_debug_lines) {
    return;
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(1.2f);

  auto &prog = priv->debug_line_prog;
  prog.use();
  prog.enable_attrib_array("vertex");
  prog.uniform("p_matrix", renderer.p_matrix);
  prog.uniform("color", glm::vec4(0.29f, 0.83f, 0.8f, 0.66f));

  BOOST_FOREACH(auto &bunch, priv->bunches) {
    if (bunch.size == 0) {
      continue;
    }
    bunch.bind();
    prog.attrib_ptr("vertex", (glm::vec2*)0, sizeof(glm::vec2));
    glDrawArrays(GL_LINES, 0, bunch.size*2);
  }
  Bunch<DebugLine>::unbind();

  prog.disable_attrib_array("vertex");
  GL::Program::clear();
}

void DebugLinesBatch::snapshot(const Game &game) {
  std::vector<DebugLine> tmp_lines;

  priv->bunches.clear();

  BOOST_FOREACH(auto ship, game.ships) {
    tmp_lines.insert(tmp_lines.end(), ship->debug_lines.begin(), ship->debug_lines.end());
  }

  priv->bunches.push_front(Bunch<DebugLine>(game.time, std::move(tmp_lines)));
}

}
}
