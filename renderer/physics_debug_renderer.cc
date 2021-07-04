/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* iPhone port by Simon Oliver - http://www.simonoliver.com - http://www.handcircus.com
*
* Oort integration by Rich Lane
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "renderer/physics_debug_renderer.h"

#include <cstdio>
#include <cstdarg>
#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"

#include "sim/math_util.h"
#include "common/resources.h"

namespace Oort {

PhysicsDebugRenderer::PhysicsDebugRenderer()
  : prog(new GL::Program(
      std::make_shared<GL::VertexShader>(load_resource("shaders/ship.v.glsl")),
      std::make_shared<GL::FragmentShader>(load_resource("shaders/ship.f.glsl")))) {
}

void PhysicsDebugRenderer::reshape(int screen_width, int screen_height) {
  this->screen_width = screen_width;
  this->screen_height = screen_height;
  this->aspect_ratio = float(screen_width)/screen_height;
}

void PhysicsDebugRenderer::begin_render(float view_radius,
                                        glm::vec2 view_center) {
  prog->use();
  glEnableVertexAttribArray(prog->attrib_location("vertex"));

#ifndef __native_client__
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
#endif

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glLineWidth(1.2f);

  auto scaled_view_center = view_center*(1.0f/SCALE);
  float width = view_radius*2/SCALE;
  float height = width/aspect_ratio;
  glm::mat4 p_matrix = glm::ortho(scaled_view_center.x - width/2,
                                  scaled_view_center.x + width/2,
                                  scaled_view_center.y - height/2,
                                  scaled_view_center.y + height/2);
  prog->uniform("p_matrix", p_matrix);

  glm::mat4 mv_matrix{1.0f};
  prog->uniform("mv_matrix", mv_matrix);

  prog->uniform("color", glm::vec4(1.0, 1.0, 1.0, 1.0));
}

void PhysicsDebugRenderer::end_render() {
  glDisableVertexAttribArray(prog->attrib_location("vertex"));
  GL::Program::clear();
}

void PhysicsDebugRenderer::DrawPolygon(const b2Vec2* old_vertices,
                                       int32 vertexCount,
                                       const b2Color& color) {
  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, old_vertices);
  glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
}

void PhysicsDebugRenderer::DrawSolidPolygon(const b2Vec2* old_vertices,
                                            int32 vertexCount,
                                            const b2Color& color) {
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, old_vertices);

  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0)*0.5f);
  glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);

  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));
  glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
}

void PhysicsDebugRenderer::DrawCircle(const b2Vec2& center, float32 radius,
                                      const b2Color& color) {
  const float32 k_segments = 16.0f;
  int vertexCount = 16;
  const float32 k_increment = 2.0f * b2_pi / k_segments;
  float32 theta = 0.0f;

  GLfloat        glVertices[vertexCount*2];
  for (int32 i = 0; i < k_segments; ++i) {
    b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
    glVertices[i*2]=v.x;
    glVertices[i*2+1]=v.y;
    theta += k_increment;
  }

  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, glVertices);

  glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
}

void PhysicsDebugRenderer::DrawSolidCircle(const b2Vec2& center,
                                           float32 radius,
                                           const b2Vec2& axis,
                                           const b2Color& color) {

  const float32 k_segments = 16.0f;
  int vertexCount = 16;
  const float32 k_increment = 2.0f * b2_pi / k_segments;
  float32 theta = 0.0f;

  GLfloat        glVertices[vertexCount*2];
  for (int32 i = 0; i < k_segments; ++i) {
    b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
    glVertices[i*2] = v.x;
    glVertices[i*2+1] = v.y;
    theta += k_increment;
  }

  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0) * 0.5f);
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, glVertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));
  glDrawArrays(GL_LINE_LOOP, 0, vertexCount);

  // Draw the axis line
  DrawSegment(center, center+radius*axis, color);
}

void PhysicsDebugRenderer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2,
                                       const b2Color& color) {
  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));
  GLfloat        glVertices[] = {
    p1.x, p1.y,
    p2.x, p2.y
  };
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, glVertices);
  glDrawArrays(GL_LINES, 0, 2);
}

void PhysicsDebugRenderer::DrawTransform(const b2Transform& xf) {
  b2Vec2 p1 = xf.p, p2;
  const float32 k_axisScale = 0.4f;

  p2 = p1 + k_axisScale * b2Vec2(xf.q.c, xf.q.s);
  DrawSegment(p1, p2, b2Color(1, 0, 0));

  p2 = p1 + k_axisScale * b2Vec2(-xf.q.s, xf.q.c);
  DrawSegment(p1, p2, b2Color(0, 1, 0));
}

void PhysicsDebugRenderer::DrawPoint(const b2Vec2& p, float32 size,
                                     const b2Color& color) {
  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));
#ifndef __native_client__
  glPointSize(size);
#endif
  GLfloat        glVertices[] = {
    p.x, p.y
  };
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, glVertices);
  glDrawArrays(GL_POINTS, 0, 1);
#ifndef __native_client__
  glPointSize(1.0f);
#endif
}

void PhysicsDebugRenderer::DrawString(int x, int y, const char *string, ...) {
}

void PhysicsDebugRenderer::DrawAABB(b2AABB* aabb, const b2Color& color) {
  prog->uniform("color", glm::vec4(color.r, color.g, color.b, 1.0));

  GLfloat        glVertices[] = {
    aabb->lowerBound.x, aabb->lowerBound.y,
    aabb->upperBound.x, aabb->lowerBound.y,
    aabb->upperBound.x, aabb->upperBound.y,
    aabb->lowerBound.x, aabb->upperBound.y
  };
  glVertexAttribPointer(prog->attrib_location("vertex"),
                        2, GL_FLOAT, GL_FALSE, 0, glVertices);
  glDrawArrays(GL_LINE_LOOP, 0, 8);

}

}
