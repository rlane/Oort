#include "renderer/batches/bullet.h"

#include <memory>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "sim/game.h"
#include "sim/bullet.h"
#include "gl/check.h"
#include "gl/buffer.h"
#include "common/resources.h"

using glm::vec2;
using glm::vec4;
using std::make_shared;
using std::shared_ptr;

namespace Oort {
namespace RendererBatches {

struct BulletVertex {
  vec2 p;
  vec4 color;
};

struct BulletState {
  BulletVertex a, b;
};

struct BulletPriv {
  GL::Program prog;
  GL::Buffer buf;
  bool buf_valid;
  std::vector<BulletState> bullets;

  BulletPriv()
    : prog(GL::Program::from_resources("bullet")),
      buf_valid(false)
  {
  }
};

BulletBatch::BulletBatch(Renderer &renderer)
  : Batch(renderer),
    priv(make_shared<BulletPriv>())
{
}

void BulletBatch::snapshot(const Game &game) {
  vec4 color1(0, 0, 0, 0);
  vec4 color2(0.96f, 0.73f, 0.25f, 1.0f/Renderer::jitters.size());

  priv->bullets.clear();
  BOOST_FOREACH(auto bullet, game.bullets) {
    if (bullet->dead || bullet->def.type == GunType::PLASMA) {
      continue;
    }

    auto p = bullet->get_position(game.time);
    auto v = bullet->velocity;
    auto dp = v * (1.0f/40);

    priv->bullets.emplace_back(BulletState{
      BulletVertex{ p - dp, color1 },
      BulletVertex{ p, color2 }
    });
  }

  priv->buf_valid = false;
}

void BulletBatch::render(float time_delta) {
  auto &prog = priv->prog;
  glBlendFunc(GL_ONE, GL_ONE);
  glLineWidth(1.2f);

  prog.use();

  prog.enable_attrib_array("vertex");
  prog.enable_attrib_array("color");
  prog.uniform("p_matrix", renderer.p_matrix);
  prog.uniform("mv_matrix", glm::mat4{1.0f});

  if (!priv->buf_valid) {
    priv->buf.data(priv->bullets);
    priv->buf_valid = true;
  }

  priv->buf.bind();

  BulletVertex *v = NULL;
  auto stride = sizeof(*v);
  prog.attrib_ptr("vertex", &v->p, stride);
  prog.attrib_ptr("color", &v->color, stride);

  BOOST_FOREACH(auto jitter, Renderer::jitters) {
    prog.attrib("jitter", jitter*(4.0f/1600));
    glDrawArrays(GL_LINES, 0, priv->bullets.size()*2);
  }

  GL::Buffer::unbind();

  prog.disable_attrib_array("vertex");
  prog.disable_attrib_array("color");
  GL::Program::clear();
}

}
}
