/**
 * Software Rasterizer Playground.
 *
 * Viewport Framebuffer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cmath>

#include "framebuffer.h"

void Framebuffer::resize(int width, int height)
{
    width = std::max(1, width);
    height = std::max(1, height);

    if(width != this->width || height != this->height)
    {
        data = nullptr;

        if(context != nullptr)
        {
            release();

            swr::DestroyContext(context);
            context = nullptr;
        }
    }

    if(context == nullptr)
    {
        context = swr::CreateOffscreenContext(width, height);
        if(!swr::MakeContextCurrent(context))
        {
            throw std::runtime_error("MakeCurrentContext failed");
        }

        swr::GetContextInfo(
          context,
          reinterpret_cast<void**>(&data),
          &this->width,
          &this->height,
          nullptr);

        std::println("Viewport dimensions: {}x{}", get_width(), get_height());

        initialize();
    }
}

void Framebuffer::update(float delta_time)
{
    if(data == nullptr || width <= 0 || height <= 0)
    {
        return;
    }

    /*
     * update animation.
     */
    if(update_animation)
    {
        gear_rotation += delta_time;
    }
    if(gear_rotation >= 2 * static_cast<float>(M_PI))
    {
        gear_rotation -= 2 * static_cast<float>(M_PI);
    }

    begin_render();
    draw_gears();
    end_render();
}