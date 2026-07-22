/**
 * Software Rasterizer Playground.
 *
 * Splash screen.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <filesystem>
#include <random>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "containers/vector.h"

/**
 * Get a random PNG image from a folder.
 *
 * @param folder The folder containing PNGs.
 * @returns Path to a PNG.
 * @throws Throws a `std::runtime_error` if the folder does not contain PNG images.
 */
inline std::filesystem::path get_random_png(
  const std::filesystem::path& folder)
{
    swr::vector<std::filesystem::path> splashes;

    for(const auto& entry: std::filesystem::directory_iterator(folder))
    {
        if(entry.is_regular_file()
           && entry.path().extension() == ".png")
        {
            splashes.push_back(entry.path());
        }
    }

    if(splashes.empty())
    {
        throw std::runtime_error{
          "No splash images found."};
    }

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, splashes.size() - 1);

    return splashes[dist(rng)];
}

/** A splash screen. */
class SplashScreen final
{
    static constexpr float font_size = 16.f;
    static constexpr float status_height = 28.f;

    /** Status displayed in the splash screen. */
    std::string status;

    TTF_Font* font{nullptr};
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};
    SDL_Texture* image{nullptr};

    /** Window width and height. */
    int width{0}, height{0};

    /** Redraw the splash screen. */
    void redraw()
    {
        SDL_Surface* surface = TTF_RenderText_Blended(
          font,
          status.c_str(),
          status.length(),
          SDL_Color{255, 255, 255, 255});
        if(surface == nullptr)
        {
            throw std::runtime_error{
              SDL_GetError()};
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if(texture == nullptr)
        {
            SDL_DestroySurface(surface);
            throw std::runtime_error{
              SDL_GetError()};
        }

        float img_w, img_h;
        SDL_GetTextureSize(image, &img_w, &img_h);

        float scale = std::max(
          width / img_w,
          height / img_h);

        SDL_FRect image_dst{
          (width - img_w * scale) * 0.5f,
          (height - img_h * scale) * 0.5f,
          img_w * scale,
          img_h * scale};

        float font_width, font_height;
        SDL_GetTextureSize(texture, &font_width, &font_height);

        SDL_FRect font_dst{
          (width - font_width) / 2.0f,
          static_cast<float>(height - status_height) + (status_height - font_height) / 2.f,
          font_width,
          font_height};

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, image, nullptr, &image_dst);

        SDL_FRect panel{
          0.f,
          static_cast<float>(height - status_height),
          static_cast<float>(width),
          status_height};

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_RenderFillRect(renderer, &panel);

        SDL_RenderTexture(renderer, texture, nullptr, &font_dst);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

public:
    /**
     * Create a splash screen. Displays a status text on a
     * randomly selected PNG image.
     *
     * @param width Width of the splash screen.
     * @param height Height of the splash screen.
     * @param font_path The TTF font to use.
     * @param splash_path Folder containing splash screen PNGs.
     * @throws Throws `std::runtime_error` if creation fails.
     */
    SplashScreen(
      int width,
      int height,
      std::string_view font_path = "assets/fonts/inter/Inter-Regular.ttf",
      std::string_view splash_path = "assets/textures/splash")
    : width{width}
    , height{height}
    {
        font = TTF_OpenFont(
          std::string{font_path}.c_str(),
          font_size);
        if(font == nullptr)
        {
            throw std::runtime_error{
              SDL_GetError()};
        }

        window = SDL_CreateWindow(
          "",    // no title, since it's not displayed anyway
          width, height,
          SDL_WINDOW_BORDERLESS);
        if(window == nullptr)
        {
            throw std::runtime_error{
              SDL_GetError()};
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if(renderer == nullptr)
        {
            throw std::runtime_error{
              SDL_GetError()};
        }

        SDL_Surface* surface = SDL_LoadPNG(
          get_random_png(splash_path).string().c_str());
        if(surface == nullptr)
        {
            throw std::runtime_error{
              SDL_GetError()};
        }

        image = SDL_CreateTextureFromSurface(renderer, surface);
        if(image == nullptr)
        {
            SDL_DestroySurface(surface);
            throw std::runtime_error{
              SDL_GetError()};
        }

        SDL_DestroySurface(surface);

        set_status("Initializing...");
    }

    /** Destructor. */
    ~SplashScreen()
    {
        if(image != nullptr)
        {
            SDL_DestroyTexture(image);
            image = nullptr;
        }

        if(renderer != nullptr)
        {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if(window != nullptr)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        if(font != nullptr)
        {
            TTF_CloseFont(font);
            font = nullptr;
        }
    }

    /**
     * Set the status text and refresh the splash screen.
     *
     * @param text The new status text.
     */
    void set_status(
      std::string_view text)
    {
        status = text;
        redraw();
    }
};
