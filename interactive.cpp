#ifdef INTERACTIVE

//Compile imgui sources
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_impl_sdl.cpp"

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
//mutex on framebuffer - don't read while being written
static pthread_mutex_t texLock;
//extra buffer for iter values that are actually on screen
//needed to prevent artifacts when zooming quickly
//must always match frameBuf
static float* auxIters;

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
  pthread_mutex_lock(&texLock);
  for(int i = 0; i < tw * th; i++)
  {
    frameBuf[i] = 0xFF000000;
  }
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, frameBuf);
  assert(!glGetError());
  imageStale = true;
  pthread_mutex_unlock(&texLock);
}

//update frameBuf using iters
static void recomputeFramebuffer()
{
  pthread_mutex_lock(&texLock);
  colorMap();
  for(int i = 0; i < tw * th; i++)
  {
    Uint32 temp = frameBuf[i]; 
    frameBuf[i] = ((temp & 0xFF000000) >> 24) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF00) << 8) | ((temp & 0xFF) << 24);
  }
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, frameBuf);
  pthread_mutex_unlock(&texLock);
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
      bool interrupted = false;
      refinement = 0;
      while(refinement != -1)
      {
        refinementStep();
        if(!runWorkers)
        {
          interrupted = true;
          break;
        }
      }
      if(interrupted)
      {
        //drawBuf interrupted, restart, possibly with completely new parameters/zoom/location
        clearBitset(&computed);
        runWorkers = true;
        continue;
      }
      imageStale = false;
      memcpy(auxIters, iters, winw * winh * sizeof(float));
      frameBufStale = true;
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

static void zoomIn(int mouseX, int mouseY)
{
  MAKE_STACK_FP(temp);
  //update targetX
  loadValue(&temp, (long double) mouseX / 2);
  fpmul2(&temp, &pstride);
  fpadd2(&targetX, &temp);
  //update targetY
  loadValue(&temp, (long double) mouseY / 2);
  fpmul2(&temp, &pstride);
  fpadd2(&targetY, &temp);
  fpshrOne(pstride);
  upgradePrec(true);
  upgradeIters();
  zoomDepth++;
  //expand all pixels to twice their distance from mouse location
  //must do only one quadrant at a time
  //get frame of pixels in current FB that will fill entire frame after zoom
  //
  //correct pixels that are translated to exact grid points in zoomed frame
  //are also correct and can be marked as ccomputed immediately
  mouseX += winw / 2;
  mouseY += winh / 2;
  int lox = mouseX / 2;
  int hix = mouseX + (winw - mouseX) / 2;
  int loy = mouseY / 2;
  int hiy = mouseY + (winh - mouseY) / 2;
//Macro to copy val to (mapx, mapy), set appropriate bit in computed, and do bounds checking
#define EXPAND_VAL { \
  if(mapx < 0 || mapx >= winw || mapy < 0 || mapy >= winh) \
    continue; \
  int srcIndex = x + y * winw; \
  bool pixelComputed = getBit(&computed, srcIndex); \
  float val = iters[srcIndex]; \
  float auxVal = auxIters[srcIndex]; \
  if(pixelComputed) \
    setBit(&computed, mapx + mapy * winw, 1); \
  bool xbound = mapx + 1 < winw; \
  bool ybound = mapy + 1 < winh; \
  iters[mapx + mapy * winw] = val; \
  auxIters[mapx + mapy * winw] = auxVal; \
  if(xbound) \
  { \
    iters[(mapx + 1) + mapy * winw] = val; \
    auxIters[(mapx + 1) + mapy * winw] = auxVal; \
    setBit(&computed, (mapx + 1) + mapy * winw, 0); \
  } \
  if(ybound) \
  { \
    iters[mapx + (mapy + 1) * winw] = val; \
    auxIters[mapx + (mapy + 1) * winw] = auxVal; \
    setBit(&computed, mapx + (mapy + 1) * winw, 0); \
  } \
  if(xbound && ybound) \
  { \
    iters[(mapx + 1) + (mapy + 1) * winw] = val; \
    auxIters[(mapx + 1) + (mapy + 1) * winw] = auxVal; \
    setBit(&computed, (mapx + 1) + (mapy + 1) * winw, 0); \
  } \
}
  for(int y = loy; y < mouseY; y++)
  {
    int mapy = mouseY - (mouseY - y) * 2;
    if(mapy < 0)
      continue;
    for(int x = lox; x < mouseX; x++)
    {
      int mapx = mouseX - (mouseX - x) * 2;
      EXPAND_VAL;
    }
    for(int x = hix; x >= mouseX; x--)
    {
      int mapx = mouseX + (x - mouseX) * 2;
      EXPAND_VAL;
    }
  }
  for(int y = hiy; y >= mouseY; y--)
  {
    int mapy = mouseY + (y - mouseY) * 2;
    if(mapy >= winh)
      continue;
    for(int x = lox; x < mouseX; x++)
    {
      int mapx = mouseX - (mouseX - x) * 2;
      EXPAND_VAL;
    }
    for(int x = hix; x >= mouseX; x--)
    {
      int mapx = mouseX + (x - mouseX) * 2;
      EXPAND_VAL;
    }
  }
  recomputeFramebuffer();
}

static void zoomOut(int mouseX, int mouseY)
{
  MAKE_STACK_FP(temp);
  loadValue(&temp, mouseX);
  fpmul2(&temp, &pstride);
  fpsub2(&targetX, &temp);
  //update targetX
  loadValue(&temp, mouseY);
  fpmul2(&temp, &pstride);
  fpsub2(&targetY, &temp);
  fpshlOne(pstride);
  downgradePrec(true);
  downgradeIters();
  zoomDepth--;
}

void interactiveMain(int imageW, int imageH)
{
  pthread_mutex_init(&texLock, NULL);
  w = imageW;
  //GUI controls take up about 200 pixels of vertical space below image
  h = imageH + 200;
  tw = imageW;
  th = imageH;
  iterScale = 1;
  colorFuncSel = 0;
  auxIters = (float*) malloc(winw * winh * sizeof(float));
  for(int i = 0; i < winw * winh; i++)
  {
    auxIters[i] = -1;
  }
  setImageIters(auxIters);
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
  clearBitset(&computed);
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
      zoomIn(cursor.x, cursor.y);
      imageStale = true;
      interruptWorkers = true;
    }
    else if(ImGui::IsItemClicked(1))
    {
      //right button, zoom out
      if(zoomDepth > 0)
      {
        //formula: target -= 2 * pstride * mousePos
        zoomOut(cursor.x, cursor.y);
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
  free(auxIters);
  pthread_mutex_destroy(&texLock);
  exit(0);
}

#endif

