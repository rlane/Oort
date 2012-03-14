#ifndef OORT_RENDERER_BATCHES_DEBUG_LINES_H_
#define OORT_RENDERER_BATCHES_DEBUG_LINES_H_

#include "renderer/batch.h"
#include "gl/program.h"

namespace Oort {
namespace RendererBatches {

class DebugLinesPriv;

class DebugLinesBatch : public Batch {
public:
  DebugLinesBatch(Renderer &Renderer);
  virtual void render(float time_delta);
  virtual void snapshot(const Game &game);
private:
  std::shared_ptr<DebugLinesPriv> priv;
};

}
}

#endif
