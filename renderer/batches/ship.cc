#include "renderer/batches/ship.h"

#include <memory>
#include <boost/foreach.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "sim/game.h"
#include "sim/ship.h"
#include "sim/ship_class.h"
#include "sim/model.h"
#include "sim/team.h"
#include "gl/buffer.h"
#include "gl/check.h"
#include "common/resources.h"

using glm::vec2;
using glm::vec4;
using std::make_shared;
using std::shared_ptr;

namespace Oort {
namespace RendererBatches {

struct ShipState {
  vec2 p;
  vec2 v;
  float h;
  float w;
  const ShipClass &klass;
  const Team &team;
};

struct ShipPriv {
  GL::Program prog;
  GL::Buffer vertex_buf;
  std::list<ShipState> ships;

  ShipPriv()
    : prog(GL::Program::from_resources("ship"))
  {
    std::vector<vec2> vertices;
    BOOST_FOREACH(auto &klass_pair, ShipClass::klasses) {
      auto &klass = klass_pair.second;
      BOOST_FOREACH(Shape &shape, klass.model->shapes) {
        shape.offset = vertices.size();
        vertices.insert(vertices.end(),
                        shape.vertices.begin(), shape.vertices.end());
      }
    }

    vertex_buf.data(vertices);
  }
};

ShipBatch::ShipBatch(Renderer &renderer)
  : Batch(renderer),
    priv(make_shared<ShipPriv>()) {}

void ShipBatch::render(float time_delta) {
  auto &prog = priv->prog;
  glBlendFunc(GL_ONE, GL_ONE);
  glLineWidth(1.2f);
  prog.use();
  prog.enable_attrib_array("vertex");
  prog.uniform("p_matrix", renderer.p_matrix);
  priv->vertex_buf.bind();
  prog.attrib_ptr("vertex", (vec2*)NULL);
  priv->vertex_buf.unbind();

  BOOST_FOREACH(auto &ship, priv->ships) {
    glm::mat4 mv_matrix{1.0f};
    auto p = ship.p + ship.v * time_delta;
    auto h = ship.h + ship.w * time_delta;
    mv_matrix = glm::translate(mv_matrix, glm::vec3(p, 0));
    mv_matrix = glm::rotate(mv_matrix, glm::degrees(h), glm::vec3(0, 0, 1));
    mv_matrix = glm::scale(mv_matrix, glm::vec3(1, 1, 1) * ship.klass.scale);
    glm::vec4 color(ship.team.color, ship.klass.model->alpha/Renderer::jitters.size());

    prog.uniform("mv_matrix", mv_matrix);
    prog.uniform("color", color);

    BOOST_FOREACH(Shape &shape, ship.klass.model->shapes) {
      BOOST_FOREACH(auto jitter, Renderer::jitters) {
        prog.uniform("jitter", jitter*(4.0f/1600));
        glDrawArrays(GL_LINE_LOOP, shape.offset, shape.vertices.size());
      }
    }
  }

  prog.disable_attrib_array("vertex");
  GL::Program::clear();
}

void ShipBatch::snapshot(const Game &game) {
  priv->ships.clear();
  BOOST_FOREACH(auto ship, game.ships) {
    if (ship->dead) {
      continue;
    }

    priv->ships.emplace_back(ShipState{
      ship->get_position(),
      ship->get_velocity(),
      ship->get_heading(),
      ship->get_angular_velocity(),
      ship->klass,
      *ship->team
    });
  }
}

}
}
