#ifdef INTERACTIVE
#include "interactive.h"
#include "SDL2/SDL.h"
#include "OpenGL/gl.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"

//Window dimensions (set once)
static int w;
static int h;
static int tw;
static int th;
static SDL_Window* win;
static SDL_GLContext glcontext;
static GLuint textureID;

extern "C" void interactiveMain(int windowW, int windowH, int imageW, int imageH)
{
  w = windowW;
  h = windowH;
  tw = imageW;
  th = imageH;
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
  {
    puts("Failed to initialize SDL.");
    exit(1);
  }
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);
  win = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if(!win)
  {
    puts("Failed to create SDL window.");
    exit(1);
  }
  glcontext = SDL_GL_CreateContext(win);
  ImGui_ImplSdl_Init(win);
  glGenTextures(1, &textureID);
  //allocate the GPU memory for texture, never re-allocate
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th,
      0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  bool running = true;
  while(running)
  {
    ImGui_ImplSdl_NewFrame(win);
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      ImGui_ImplSdl_ProcessEvent(&event);
      if(event.type == SDL_QUIT)
        running = false;
    }
    //Have main ImGui frame fill SDL window
    {
      ImVec2 initSize(w, h);
      ImVec2 initPos(0, 0);
      ImGui::SetNextWindowSize(initSize);
      ImGui::SetNextWindowPos(initPos);
    }
    ImGui::Begin("", NULL,
                     ImGuiWindowFlags_NoTitleBar
                   | ImGuiWindowFlags_NoResize 
                   | ImGuiWindowFlags_NoMove 
                   | ImGuiWindowFlags_NoScrollbar
                   | ImGuiWindowFlags_NoScrollWithMouse);
    //Begin GUI
    ImGui::Text("Hello world");
    //End GUI
    ImGui::End();
    glViewport(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    SDL_GL_SwapWindow(win);
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, buf);
  }
  ImGui_ImplSdl_Shutdown();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(win);
  SDL_Quit();
  exit(0);
}

#endif

