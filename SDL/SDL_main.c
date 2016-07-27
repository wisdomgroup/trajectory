//
//  SDL_main.c
//  trajectory
//
//  Created by Thomas Malthouse on 7/13/16.
//  Copyright © 2016 Thomas Malthouse. All rights reserved.
//

#include "SDL_main.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "SDL_coords.h"
#include "../body.h"
#include "../types.h"
#include "../debug.h"
#include "../color.h"

const uint32_t max_fps = 30;

const uint32_t ms_per_frame = 1000/(max_fps);

Vector2dPair min_max_xy_coords(Body *sys, uint64_t count)
{
    Vector2d min = {0,0};
    Vector2d max = {0,0};
    
    for (uint64_t i=0; i<count; i++) {
        Vector3d pos = sys[i].pos;
        
        if (pos.x < min.x) {
            min.x = pos.x;
        }
        if (pos.y < min.y) {
            min.y = pos.y;
        }
        
        if (pos.x > max.x) {
            max.x = pos.x;
        }
        if (pos.y > max.y) {
            max.y = pos.y;
        }
    }
    
    return (Vector2dPair){min, max};
}

void render_system(Body *sys, uint64_t body_count, SDL_Renderer *renderer, Vector2d screen_size)
{
    // First, we need to clear the screen and paint it black.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);
    
    // Then, we set the color to planet color (in this case, white for now)
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    for (uint64_t i=0; i<body_count; i++) {
        Vector2d spos = calculate_screencoord(sys[i].pos);
        logger("Screenpos of body %llu is %f, %f\n", i, spos.x, spos.y);
        //SDL_RenderDrawPoint(renderer, spos.x, spos.y);
        Color c = sys[i].color;
        filledCircleRGBA(renderer, spos.x, spos.y, 4, c.r, c.g, c.b, c.a);
    }
    
    //Finally, we push our changes to the screen
    SDL_RenderPresent(renderer);
}

int event_handler(SDL_Event *e)
{
    while(SDL_PollEvent(e)){
        switch (e->type) {
            case SDL_QUIT:
                return 1;
                break;
            case SDL_MOUSEWHEEL:
            {
                SDL_MouseWheelEvent mousewheel = e->wheel;
                if (mousewheel.y>0) {
                    scale_display(1.1);
                } else {
                    scale_display(0.9);
                }
            }
            default:;
                //dblogger("Event detected");
        }
    }
    return 0;
}

int update(Body *sys, uint64_t body_count, SDL_Renderer *renderer,  Vector2d screen_size, Time *t, SDL_Event *e)
{
    if (event_handler(e)) {
        return 1;
    }
    
    Time dt = 10000;
    for (int i=0; i<100; i++) {
        system_update(sys, body_count, dt, t);
        *t+=dt;
    }
    
    render_system(sys, body_count, renderer, screen_size);
    return 0;
}


void rungame()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    
    int scrx, scry;
    
    
    SDL_Window *win = SDL_CreateWindow("Trajectory!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    SDL_GetWindowSize(win, &scrx, &scry);
    Vector2d screensize = {scrx, scry};
    
    SDL_Renderer *render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(render, 0x00, 0x00, 0x00, 0xFF);
    
    bool quit = false;
    Time t=0;
    uint32_t last_update_time = SDL_GetTicks();
    SDL_Event e;
    
    Body sys[10];
    /*
    sys[0] = (Body){.mass =  1e8, .pos =  (Vector3d){0, 0, 0}, .vel =  (Vector3d){0,0,0}};
    sys[1] = (Body){.mass =  1, .pos =  (Vector3d){1e3, 1e3, 0}, .vel =  (Vector3d){0,.01,0}};
    sys[2] = (Body){.mass =  1, .pos =  (Vector3d){-1e3, -1e3, 0}, .vel =  (Vector3d){.002,0,0}};
    sys[3] = (Body){.mass =  1,  .pos =  (Vector3d){5e2, 5e2, 0}, .vel = (Vector3d){-.001, 0, 0}};
    */
    sys[0] = (Body){.name="Sun", .mass=1.988e30, .vel = {20.0,0.0,0.0}, .color = hex_to_color(COLOR_YELLOW)};
    sys[1] = (Body){.name="Mercury", .mass=3.3e23, .vel={47362,0,0}, .pos={0,-5.7e10,0}, .color = hex_to_color(COLOR_GRAY)};
    sys[2] = (Body){.name="Venus", .mass=4.87e24, .vel={0,35002,0}, .pos={1.08e11, 0,0}, .color = hex_to_color(COLOR_ORANGE)};
    sys[3] = (Body){.name="Earth", .mass=5.97e24, .vel={-29780, 0, 0}, .pos={0, 1.49e11,0}, .color = hex_to_color(COLOR_BLUE)};
    sys[4] = (Body){.name="Mars", .mass=6.42e23, .vel={0, -24000,0}, .pos={-2.27e11, 0,0}, .color = hex_to_color(COLOR_RED)};
    sys[5] = (Body){.name="Jupiter", .mass=1.89e27, .vel={-13070,0,0}, .pos={0, 7.41e11,0}, .color = hex_to_color(COLOR_ORANGE)};
    sys[6] = (Body){.name="Saturn", .mass=5.68e26, .vel={9690,0,0}, .pos={0,-1.509e12,0}, .color = hex_to_color(COLOR_YELLOW)};
    sys[7] = (Body){.name="Uranus", .mass=8.68e25, .vel={6800,0,0}, .pos={0,-2.875e12,0}, .color = hex_to_color(COLOR_TEAL)};
    sys[8] = (Body){.name="Neptune", .mass=1.02e26, .vel={5430,0,0}, .pos={0,-4.504e12,0}, .color = hex_to_color(COLOR_BLUE)};
    sys[9] = (Body){.name="Pluto", .mass=1.3e23, .vel={6100, 0,0}, .pos={0,-4.436e12,0}, .color = hex_to_color(COLOR_GRAY)};

    
    set_screensize(screensize);
    Vector2dPair minmax = min_max_xy_coords(sys, 10);
    set_minmax_coords(minmax.a, minmax.b);
    
    while (!quit) {
        if (update(sys, 10, render, screensize, &t, &e)) {
            quit = true;
        }
        
        //If we haven't elapsed the minimum per frame yet
        while (last_update_time + ms_per_frame > SDL_GetTicks()) {
            //Sleep 1 ms before checking again
            SDL_Delay(5);
        }
        last_update_time = SDL_GetTicks();
    }
    
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(win);
    end_logger();
}
