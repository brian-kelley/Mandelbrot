#include "mandelbrot.h"

Float screenX;
Float screenY;
Float width;
Float height;
Float pstride;      //size of a pixel
Float targetX;
Float targetY;
Float scratch1;     //to be used like registers by the convergence and zoom computationsa
Float scratch2;
Float scratch3;
Float scratch4;
Uint32* pixbuf;
int* iterbuf;
Uint32* colortable;
int filecount;
int numThreads;
int maxiter;
int prec;

void initPositionVars()
{
    const long double startw = 4;
    const long double starth = 4 * winh / winw;
    screenX = floatLoad(1, -startw / 2);
    screenY = floatLoad(1, -starth / 2);
    width = floatLoad(1, startw);
    height = floatLoad(1, starth);
    pstride = floatLoad(1, startw / winw);  //pixels are always exactly square, no separate x and y
}

void initFloatScratch()
{
    scratch1 = FloatCtor(1);
    scratch2 = FloatCtor(1);
    scratch3 = FloatCtor(1);
    scratch4 = FloatCtor(1);
}

#define INCR_PREC(f) \
    f.mantissa.size++; \
    f.mantissa.val = (u64*) realloc(&f.mantissa.val, (f.mantissa.size) * sizeof(u64));\
    f.mantissa.val[f.mantissa.size - 1] = 0;

void increasePrecision()
{
    //reallocate space for the persistent Float variables
    //INCR_PREC macro copies significant words into reallocated space and updates mantissa size
    INCR_PREC(screenX);
    INCR_PREC(screenY);
    INCR_PREC(width);
    INCR_PREC(height);
    INCR_PREC(pstride);
    INCR_PREC(scratch1);
    INCR_PREC(scratch2);
    INCR_PREC(scratch3);
    INCR_PREC(scratch4);
    prec++;
}

void writeImage()
{
    char name[32];
    sprintf(name, "output/mandel%i.png", filecount++);
    for(int i = 0; i < winw * winh; i++)
    {
        Uint32 temp = pixbuf[i]; 
        pixbuf[i] = 0xFF000000 | (temp & 0xFF << 8) | (temp & 0xFF00 >> 8) | (temp & 0xFF0000 >> 0);
    }
    lodepng_encode32_file(name, (unsigned char*) pixbuf, winw, winh);
}

void zoom()
{
    /*
    //get the highest (non-convergent) iteration count
    //in the middle 1/9 of window area
    int bestX = 0;
    int bestY = 0;
    int bestIter = 0;
    int xStart = winw * (1 - zoomRange) / 2;
    int yStart = winh * (1 - zoomRange) / 2;
    int xRange = winw * zoomRange;
    int yRange = winh * zoomRange;
    for(int i = xStart; i < xStart + xRange; i++)
    {
        for(int j = yStart; j < yStart + yRange; j++)
        {
            if(iterbuf[i + j * winw] > bestIter)
            {
                bestX = i;
                bestY = j;
                bestIter = iterbuf[i + j * winw];
            }
        }
    }
    printf("Deepest pixel has %i iters.\n", bestIter);
    //now zoom to (bestX, bestY)
    //zoom in by factor of 1.5
    //get the decrease in width and height
    real dw = width * (1 - 1 / zoomFactor);
    real dh = height * (1 - 1 / zoomFactor);
    screenX += dw * ((real) bestX / winw);
    screenY += dh * ((real) bestY / winh);
    width -= dw;
    height -= dh;
    */
}

void zoomToTarget()
{
    //should zoom towards exact center of the screen
    //choose the deepest pixel in a 4x4 area in the center of the view
    Float sizeFactor = floatLoad(prec, 1 / zoomFactor);
    fcopy(&scratch1, &width);
    fmul(&width, &scratch1, &sizeFactor);
    fcopy(&scratch1, &height);
    fmul(&height, &scratch1, &sizeFactor);
    fcopy(&scratch1, &pstride);
    fmul(&pstride, &scratch1, &sizeFactor);
    //width / 2 --> scratch1
    fcopy(&scratch1, &width);
    scratch1.expo--;
    fsub(&screenX, &targetX, &scratch1);
    fcopy(&scratch1, &height);
    scratch1.expo--;
    fsub(&screenY, &targetY, &scratch1);
}

void initColorTable()
{
    for(int i = 0; i < numColors; i++)
    {
        int t = (i * 5) % 360;
        int r = 0;
        int g = 0;
        int b = 0;
        float slope = 255.0 / 120;
        if(t <= 120)
            r = 255 - slope * t;
        if(t >= 240)
            r = 255 - slope * (360 - t);
        if(t <= 240)
            g = 255 - slope * abs((t - 120) % 360);
        if(t >= 120)
            b = 255 - slope * abs((t - 240) % 360);
        colortable[i] = 0xFF000000 | r << 0 | g << 8 | b << 16;
    }
}

Uint32 getColor(int num)
{
    if(num == -1)
        return 0xFF000000;                  //in the mandelbrot set = opaque black
    return colortable[num % numColors];
}

int getConvRate(Float* real, Float* imag)
{
    int iter = 0;
    while(magSquared(&z) < stop)
    {
        complex temp;
        //z = z^2 + c
        temp.r = z.r * z.r - z.i * z.i + c->r;
        temp.i = 2 * z.r * z.i + c->i;
        z = temp;
        iter++;
        if(iter == maxiter)
        {
            //not diverging
            break;
        }
    }
    return iter == maxiter ? -1 : iter;
}

void* workerFunc(void* indexAsInt)
{
    int index = *((int*) indexAsInt);
    int colsPerThread = winw / numThreads;
    for(int i = 0; i < colsPerThread; i++)
    {
        for(int j = 0; j < winh; j++)
        {
            Float realPos = screenX + width * (real) i / winw;
            real imagPos = screenY + height * (real) j / winh;
            int convRate = getConvRate(&c);
            pixbuf[j * winw + i] = getColor(convRate);
            iterbuf[j * winw + i] = convRate;
        }
    }
    return NULL;
}

void drawBuf()
{
    if(numThreads == 1)
    {
        //directly run serial code; don't spawn a single worker thread
        
        for(int i = 0; i < winw; i++)
        {
            for(int j = 0; j < winh; j++)
            {
                real realPos = screenX + width * (real) i / winw;
                real imagPos = screenY + height * (real) j / winh;
                complex c = {realPos, imagPos};
                int convRate = getConvRate(&c);
                pixbuf[j * winw + i] = getColor(convRate);
                iterbuf[j * winw + i] = convRate;
            }
        }
        return;
    }
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
    int* indices = (int*) malloc(sizeof(int) * numThreads);
    for(int i = 0; i < numThreads; i++)
    {
        indices[i] = i;
        if(pthread_create(&threads[i], NULL, workerFunc, indices + i))
        {
            puts("Failed to create thread.");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numThreads; i++)
        pthread_join(threads[i], NULL);
    free(indices);
    free(threads);
}

void recomputeMaxIter()
{
    int numPixels = winw * winh;
    unsigned long long total = 0;
    int numColored = 0;
    for(int i = 0; i < numPixels; i++)
    {
        if(iterbuf[i] != -1)
        total += iterbuf[i];
        numColored++;
    }
    double avg = (double) total / numColored;
    maxiter = avg + 2000;
}

void getInterestingLocation(int depth, real minWidth)
{
    //make a temporary iteration count buffer
    int pw = 30;    //pixel width
    int ph = 30;    //pixel height
    real x = 0;
    real y = 0;
    real w = 2;
    real h = 2;
    int* escapeTimes = (int*) malloc(sizeof(int) * pw * ph);
    real quickzoom = 20;
    maxiter = 50;
    while(depth > 0 && w >= minWidth)
    {
        real xstep = w / pw;
        real ystep = h / ph;
        real ystart = y - h / 2;
        complex iter = {x - w / 2, ystart};
        for(int px = 0; px < pw; px++)
        {
            iter.i = ystart;
            for(int py = 0; py < ph; py++)
            {
                escapeTimes[px + py * pw] = getConvRate(&iter);
                iter.i += ystep;
            }
            iter.r += xstep;
        }
        int bestPX;
        int bestPY;
        int bestIters = 0;
        for(int i = 0; i < pw; i++)
        {
            for(int j = 0; j < ph; j++)
            {
                if(escapeTimes[i + j * pw] > bestIters)
                {
                    bestPX = i;
                    bestPY = j;
                    bestIters = escapeTimes[i + j * pw];
                }
            }
        }
        if(bestIters == 0)
        {
            puts("getInterestingLocation() stuck on window of converging points!");
            printf("Window width is %Le\n", w);
            puts("Decrease zoom factor or increase resolution.");
            break;
        }
        maxiter = bestIters + 100;
        bestPX -= pw / 2;   //center the pixel position
        bestPY -= ph / 2;
        x += xstep * bestPX;
        y += ystep * bestPY;
        w /= quickzoom;
        h /= quickzoom;
        depth--;
    }
    free(escapeTimes);
    printf("Will zoom towards %.10Lf, %.10Lf\n", x, y);
    targetX = x;
    targetY = y;
}

int main(int argc, const char** argv)
{
    staticPrecInit(100);
    // START TEST CODE HERE
    getInterestingLocation(100, 1e-13);
    maxiter = 500;
    colortable = (Uint32*) malloc(sizeof(Uint32) * numColors);
    initColorTable();
    width = 3.2;
    height = 2;
    screenX = targetX - width / 2;
    screenY = targetY - height / 2;
    if(argc == 2)
        sscanf(argv[1], "%i", &numThreads);
    else
        numThreads = 4;
    printf("Running on %i threads.\n", numThreads);
    pixbuf = (Uint32*) malloc(sizeof(Uint32) * winw * winh);
    iterbuf = (int*) malloc(sizeof(int) * winw * winh);
    filecount = 0;
    for(int i = 0; i < 150; i++)
    {
        time_t start = time(NULL);
        drawBuf();
        writeImage();
        //zoom();
        zoomToTarget();
        recomputeMaxIter();
        if(maxiter > totalIter)
            maxiter = totalIter;
        int timeDiff = time(NULL) - start;
        printf("Generated image #%4i in %6i seconds.\n", filecount - 1, timeDiff);
    }
    free(iterbuf);
    free(pixbuf);
    free(colortable);
}
