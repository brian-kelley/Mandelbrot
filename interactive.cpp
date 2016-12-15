#ifdef INTERACTIVE
#include "interactive.h"
#include "SDL2/SDL.h"
#include "OpenGL/gl.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "mandelbrot.h"

//Window dimensions (set once)
static int w;
static int h;
static int tw;
static int th;
static SDL_Window* win;
static SDL_GLContext glcontext;
static GLuint textureID;
static int colorFuncSel;
//whether the image needs to be updated before next render
static bool imageStale;
static bool terminating;
static bool frameBufStale;

enum ColorFuncOptions
{
  HIST_SEL,
  LOG_SEL,
  EXPO_SEL,
  NUM_COLOR_FUNC_OPTIONS
};

//clear image (both frameBuf and the GL texture) to black
//called once in interactiveMain before main loop
static void textureClear()
{
  for(int i = 0; i < tw * th; i++)
  {
    frameBuf[i] = 0xFF000000;
  }
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, frameBuf);
  assert(!glGetError());
  imageStale = true;
}

//update frameBuf using iters
static void recomputeFramebuffer()
{
  colorMap();
  for(int i = 0; i < tw * th; i++)
  {
    Uint32 temp = frameBuf[i]; 
    frameBuf[i] = ((temp & 0xFF000000) >> 24) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF00) << 8) | ((temp & 0xFF) << 24);
  }
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, frameBuf);
  assert(!glGetError());
}

//internal interactive functions
static void* imageThreadRoutine(void* unused)
{
  imageStale = true;
  while(!terminating)
  {
    if(imageStale)
    {
      clearBitset(&computed);
      refinement = 0;
      bool interrupted = false;
      while(refinement != -1)
      {
        refinementStep();
        if(!runWorkers)
        {
          interrupted = true;
          break;
        }
        frameBufStale = true;
      }
      if(interrupted)
      {
        //drawBuf interrupted, restart, possibly with new parameters
        runWorkers = true;
        continue;
      }
      imageStale = false;
    }
    else
    {
      usleep(16667);
    }
  }
  return NULL;
}

static void resetView()
{
  zoomDepth = 0;
  maxiter = 1000;
  prec = 1;
  CHANGE_PREC(pstride, 1);
  CHANGE_PREC(targetX, 1);
  CHANGE_PREC(targetY, 1);
  loadValue(&pstride, 4.0 / tw);
  loadValue(&targetX, -0.5);
  loadValue(&targetY, 0);
}

void interactiveMain(int imageW, int imageH)
{
  w = imageW;
  h = imageH + 200;
  tw = imageW;
  th = imageH;
  iterScale = 1;
  colorFuncSel = 0;
  char target[64];
  strcpy(target, "target.bin");
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
  {
    puts("Failed to initialize SDL.");
    exit(1);
  }
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  win = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if(!win)
  {
    puts("Failed to create SDL window.");
    exit(1);
  }
  glcontext = SDL_GL_CreateContext(win);
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  ImGui_ImplSdl_Init(win);
  assert(!glGetError());
  int tsize = 1;
  {
    //get min power of two to hold the image in a square texture
    while(tsize < tw || tsize < th)
    {
      tsize <<= 1;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tsize, tsize,
        0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  //compute entire buffer immediately
  textureClear();
  //create asynchronous image generating threadw
  //always running along main thread
  //runs drawBuf when imageUpdate is true, otherwise sleeps for 16.7 ms
  //if main thread gets update event during image computation, drawBuf is
  //interrupted so that imageThread waits until ready to work again
  terminating = false;
  pthread_t imageThread;
  pthread_create(&imageThread, NULL, imageThreadRoutine, NULL);
  frameBufStale = true;
  while(true)
  {
    usleep(16667);
    SDL_Event event;
    bool quit = false;
    bool interruptWorkers = false;
    while(SDL_PollEvent(&event))
    {
      ImGui_ImplSdl_ProcessEvent(&event);
      if(event.type == SDL_QUIT)
        quit = true;
    }
    if(quit)
    {
      runWorkers = false;
      terminating = true; //signal imageThread to stop working
      pthread_join(imageThread, NULL);
      break;
    }
    ImGui_ImplSdl_NewFrame(win);
    //Have main ImGui frame fill SDL window
    SDL_GetWindowSize(win, &w, &h);
    ImVec2 initSize(w, h);
    ImVec2 initPos(0, 0);
    ImGui::SetNextWindowSize(initSize);
    ImGui::SetNextWindowPos(initPos);
    ImGui::Begin("", NULL,
                     ImGuiWindowFlags_NoTitleBar
                   | ImGuiWindowFlags_NoResize 
                   | ImGuiWindowFlags_NoMove 
                   | ImGuiWindowFlags_NoScrollbar
                   | ImGuiWindowFlags_NoScrollWithMouse);
    //*** Begin GUI ***
    //ImVec2 imgSize(tw, th);
    ImVec2 imgSize(1024, 128);
    //origin of texture, and one corner of image
    ImVec2 tex1(0, 0);
    //other corner of image
    ImVec2 tex2((float) tw / tsize, (float) th / tsize);
    imgSize = ImVec2(tw, th);
    if(frameBufStale)
    {
      recomputeFramebuffer();
      frameBufStale = false;
    }
    ImGui::Image((void*) (intptr_t) textureID, imgSize, tex1, tex2);
    ImGui::Columns(2);
    //get cursor pos within image
    ImVec2 cursor;
    {
      ImVec2 v1 = ImGui::GetMousePos();
      ImVec2 v2 = ImGui::GetItemRectMin();
      cursor.x = (v1.x - v2.x) - tw / 2;
      cursor.y = (v1.y - v2.y) - th / 2;
    }
    //check for click on image
    if(ImGui::IsItemClicked(0))
    {
      //left button, zoom in
      // formula: target += 0.5 * pstride * mousePos
      MAKE_STACK_FP(temp);
      //update targetX
      loadValue(&temp, (long double) cursor.x / 2);
      fpmul2(&temp, &pstride);
      fpadd2(&targetX, &temp);
      //update targetY
      loadValue(&temp, (long double) cursor.y / 2);
      fpmul2(&temp, &pstride);
      fpadd2(&targetY, &temp);
      fpshrOne(pstride);
      upgradePrec(true);
      upgradeIters();
      zoomDepth++;
      imageStale = true;
      interruptWorkers = true;
    }
    else if(ImGui::IsItemClicked(1))
    {
      //right button, zoom out
      if(zoomDepth > 0)
      {
        //formula: target -= 2 * pstride * mousePos
        //update targetX
        MAKE_STACK_FP(temp);
        loadValue(&temp, cursor.x);
        fpmul2(&temp, &pstride);
        fpsub2(&targetX, &temp);
        //update targetX
        loadValue(&temp, cursor.y);
        fpmul2(&temp, &pstride);
        fpsub2(&targetY, &temp);
        fpshlOne(pstride);
        downgradePrec(true);
        downgradeIters();
        zoomDepth--;
        imageStale = true;
        interruptWorkers = true;
      }
    }
    if(ImGui::Button("Reset View"))
    {
      resetView();
      imageStale = true;
      interruptWorkers = true;
    }
    ImGui::Text("Zoom level: %i", zoomDepth);
    ImGui::Text("Pixel distance: %.3Le", getValue(&pstride));
    ImGui::Text("Precision: %s", getPrecString());
    if(ImGui::Checkbox("Smooth coloring", &smooth))
    {
      imageStale = true;
      interruptWorkers = true;
    }
    if(ImGui::Checkbox("Supersampling", &supersample) && supersample)
    {
      imageStale = true;
      interruptWorkers = true;
    }
    ImGui::NextColumn();
    float inputIters = maxiter;
    if(ImGui::SliderFloat("Max Iters", &inputIters, 100, 1000000, "%.0f", 4))
    {
      if(inputIters > maxiter)
        imageStale = true;
      maxiter = inputIters;
      interruptWorkers = true;
    }
    //Color function selector
    {
      //possible (integer) values
      const char* options[] = {"Histogram", "Logarithmic", "Exponential"};
      if(ImGui::ListBox("Color Function", &colorFuncSel, options, NUM_COLOR_FUNC_OPTIONS))
      {
        if(colorFuncSel == HIST_SEL)
          colorMap = colorSunset;
        else if(colorFuncSel == LOG_SEL)
          colorMap = colorBasicLog;
        else if(colorFuncSel == EXPO_SEL)
          colorMap = colorBasicExpo;
        //don't recompute iters, just update colors
        frameBufStale = true;
      }
    }
    //Iter scaling
    float oldScale = iterScale;
    if(ImGui::SliderFloat("Color Scale", &iterScale, 0.001, 1000, "%.3f", 6))
    {
      //only need to update framebuffer if color map is affected by scaling
      if(colorFuncSel != HIST_SEL)
        frameBufStale = true;
    }
    //Target cache saving
    {
      ImGui::InputText("Target Cache", target, 64);
      if(ImGui::Button("Save Target"))
        saveTargetCache(target);
    }
    if(interruptWorkers && imageStale)
    {
      runWorkers = false;
    }
    //*** End GUI ***
    ImGui::End();
    glViewport(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    SDL_GL_SwapWindow(win);
  }
  ImGui_ImplSdl_Shutdown();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(win);
  SDL_Quit();
  exit(0);
}

#endif

