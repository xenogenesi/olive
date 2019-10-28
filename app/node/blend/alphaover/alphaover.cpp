/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "alphaover.h"

#include "render/gl/functions.h"
#include "render/gl/shadergenerators.h"
#include "render/rendertexture.h"
#include "render/video/videorenderer.h"

AlphaOverBlend::AlphaOverBlend()
{

}

QString AlphaOverBlend::Name()
{
  return tr("Alpha Over");
}

QString AlphaOverBlend::id()
{
  return "org.olivevideoeditor.Olive.alphaoverblend";
}

QString AlphaOverBlend::Description()
{
  return tr("A blending node that composites one texture over another using its alpha channel.");
}

NodeCode AlphaOverBlend::Code(NodeOutput *output)
{
  if (output == texture_output()) {
    return NodeCode("AlphaOver",
                    "void AlphaOver(const pixel *base_in, const pixel *blend_in, pixel *tex_out) {"
                    "  int i = get_global_id(0);"
                    "  tex_out[i].r = base_in.r - blend_in.a + blend_in.r;"
                    "  tex_out[i].g = base_in.g - blend_in.a + blend_in.g;"
                    "  tex_out[i].b = base_in.b - blend_in.a + blend_in.b;"
                    "  tex_out[i].a = base_in.a - blend_in.a + blend_in.a;"
                    "}");
  }

  return Node::Code(output);
}

void AlphaOverBlend::Release()
{
}

QVariant AlphaOverBlend::Value(NodeOutput *param, const rational &in, const rational &out)
{
  // Find the current Renderer instance
  RenderInstance* renderer = VideoRendererProcessor::CurrentInstance();

  // If nothing is available, don't return a texture
  if (renderer == nullptr) {
    return 0;
  }

  // The only parameter should be texture output, but for future proofing we put this here
  if (param == texture_output()) {
    RenderTexturePtr base = base_input()->get_value(in, out).value<RenderTexturePtr>();
    RenderTexturePtr blend = blend_input()->get_value(in, out).value<RenderTexturePtr>();

    if (base == nullptr && blend == nullptr) {
      return 0;
    } else if (base == nullptr) {
      return QVariant::fromValue(blend);
    } else if (blend == nullptr) {
      return QVariant::fromValue(base);
    }

    // Attach framebuffer to the backbuffer of base
    renderer->buffer()->Attach(base);
    renderer->buffer()->Bind();

    // Bind blend
    blend->Bind();

    // Set compositing strategy to alpha over
    renderer->context()->functions()->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw blend on base
    olive::gl::Blit(renderer->default_pipeline());

    // Release all
    blend->Release();
    renderer->buffer()->Release();
    renderer->buffer()->Detach();

    // Return base texture which now has blend composited on top
    // NOTE: Blend texture will be implicitly deleted here (if it's not used anywhere else)
    return QVariant::fromValue(base);
  }

  return 0;
}
