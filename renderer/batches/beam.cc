#include "renderer/batches/beam.h"

#include <memory>
#include <boost/foreach.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "sim/game.h"
#include "sim/beam.h"
#include "gl/buffer.h"
#include "gl/check.h"
#include "common/resources.h"

using glm::vec2;
using glm::vec4;
using std::make_shared;
using std::shared_ptr;

namespace Oort {
namespace RendererBatches {

struct BeamState {
  vec2 p;
  float h;
  float width;
  float length;
};

struct BeamPriv {
  GL::Program prog;
  GL::Buffer texcoords_buf;
  GL::Buffer vertices_buf;
  std::vector<BeamState> beams;

  BeamPriv()
    : prog(GL::Program::from_resources("beam"))
  {
    std::vector<vec2> texcoords{
      vec2(0, 1),
      vec2(0, 0),
      vec2(1, 1),
      vec2(1, 0)
    };

    texcoords_buf.data(texcoords);

    std::vector<vec2> vertices{
      vec2(0, 0.5f),
      vec2(0, -0.5f),
      vec2(1.0f, 0.5f),
      vec2(1.0f, -0.5f)
    };

    vertices_buf.data(vertices);
  }
};

BeamBatch::BeamBatch(Renderer &renderer)
  : Batch(renderer),
    priv(make_shared<BeamPriv>()) {}

void BeamBatch::snapshot(const Game &game) {
  priv->beams.clear();
  BOOST_FOREACH(auto &beam, game.beams) {
    priv->beams.emplace_back(BeamState{
      beam->get_position(),
      beam->get_heading(),
      beam->get_def().width,
      beam->get_def().length
    });
  }
}

void BeamBatch::render(float time_delta) {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  auto &prog = priv->prog;
  prog.use();
  prog.uniform("p_matrix", renderer.p_matrix);
  prog.enable_attrib_array("vertex");
  prog.enable_attrib_array("texcoord");

  priv->texcoords_buf.bind();
  prog.attrib_ptr("texcoord", (vec2*)NULL);
  priv->vertices_buf.bind();
  prog.attrib_ptr("vertex", (vec2*)NULL);
  GL::Buffer::unbind();

  BOOST_FOREACH(auto &beam, priv->beams) {
    glm::vec4 color = glm::vec4(0.5f, 0.5f, 1.0f, 1.0f);

    glm::mat4 mv_matrix{1.0f};
    mv_matrix = glm::translate(mv_matrix, glm::vec3(beam.p, 0));
    mv_matrix = glm::rotate(mv_matrix, glm::degrees(beam.h), glm::vec3(0, 0, 1));

    prog.uniform("mv_matrix", mv_matrix);
    prog.uniform("color", color);
    prog.uniform("dimensions", vec2(beam.length, beam.width));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  prog.disable_attrib_array("vertex");
  prog.disable_attrib_array("texcoord");
  GL::Program::clear();
}

}
}
