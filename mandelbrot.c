#include "mandelbrot.h"

unsigned* colortable;
FP targetX;
FP targetY;
int filecount;
int numThreads;
int maxiter;
int prec;
int workCol;
pthread_mutex_t workColMutex;
int deepestExpo;
bool verbose;

void writeImage(Buffer* buf)
{
    char name[32];
    sprintf(name, "output/mandel%i.png", filecount++);
    for(int i = 0; i < buf->w * buf->h; i++)
    {
        Uint32 temp = buf->colors[i]; 
        //convert rgba to big endian
        buf->colors[i] = ((temp & 0xFF000000) >> 24) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF00) << 8) | ((temp & 0xFF) << 24);
    }
    lodepng_encode32_file(name, (unsigned char*) buf->colors, buf->w, buf->h);
}

void initColorTable()
{
    colortable = (unsigned*) malloc(360 * sizeof(unsigned));
    for(int i = 0; i < 360; i++)
    {
        int t = (i * 2 + 240) % 360;
        unsigned r = 0;
        unsigned g = 0;
        unsigned b = 0;
        float slope = 255.0 / 120;
        if(t <= 120)
            r = 255 - slope * t;
        if(t >= 240)
            r = 255 - slope * (360 - t);
        if(t <= 240)
            g = 255 - slope * abs((t - 120) % 360);
        if(t >= 120)
            b = 255 - slope * abs((t - 240) % 360);
        colortable[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }
}

Uint32 getColor(int num)
{
    if(num == -1)
        return 0xFF;                  //in the mandelbrot set = opaque black
    //make a steeper gradient for the first few iterations (better image 0)
    int steepCutoff = 0;
    if(num <= steepCutoff)
        return colortable[(num * 3) % 360];
    return colortable[((num - steepCutoff) + 3 * steepCutoff) % 360];
}

int getConvRate(FP* real, FP* imag)
{
    //printf("Iterating (%Lf, %Lf)\n", getValue(real), getValue(imag));
    //real, imag make up "c" in z = z^2 + c
    assert(real->value.size == prec && imag->value.size == prec);
    MAKE_STACK_FP(four);
    loadValue(&four, 4);
    MAKE_STACK_FP(zr);
    loadValue(&zr, 0);
    MAKE_STACK_FP(zi);
    loadValue(&zi, 0);
    MAKE_STACK_FP(zrsquare);
    MAKE_STACK_FP(zisquare);
    MAKE_STACK_FP(zri);
    MAKE_STACK_FP(mag);
    int iter = 0;
    for(; iter < maxiter; iter++)
    {
        fpmul3(&zrsquare, &zr, &zr);
        fpmul3(&zisquare, &zi, &zi);
        fpmul3(&zri, &zr, &zi);
        //want 2 * zr * zi
        fpshlOne(zri);
        fpsub3(&zr, &zrsquare, &zisquare);
        fpadd2(&zr, real);
        fpadd3(&zi, &zri, imag);
        fpadd3(&mag, &zrsquare, &zisquare);
        if(mag.value.val[0] >= four.value.val[0])
        {
            break;
        }
    }
    return iter == maxiter ? -1 : iter;
}

int getConvRateCapped(FP* real, FP* imag, int localMaxIter)
{
    assert(real->value.size == prec && imag->value.size == prec);
    //real, imag make up "c" in z = z^2 + c
    MAKE_STACK_FP(four);
    loadValue(&four, 4);
    MAKE_STACK_FP(zr);
    loadValue(&zr, 0);
    MAKE_STACK_FP(zi);
    loadValue(&zi, 0);
    MAKE_STACK_FP(zrsquare);
    MAKE_STACK_FP(zisquare);
    MAKE_STACK_FP(zri);
    MAKE_STACK_FP(mag);
    int iter = 0;
    for(; iter < localMaxIter; iter++)
    {
        fpmul3(&zrsquare, &zr, &zr);
        fpmul3(&zisquare, &zi, &zi);
        fpmul3(&zri, &zr, &zi);
        //want 2 * zr * zi
        fpshlOne(zri);
        fpsub3(&zr, &zrsquare, &zisquare);
        fpadd2(&zr, real);
        fpadd3(&zi, &zri, imag);
        fpadd3(&mag, &zrsquare, &zisquare);
        if(mag.value.val[0] >= four.value.val[0])
            break;
    }
    return iter == localMaxIter ? -1 : iter;
}

void* workerFunc(void* buffer)
{
    Buffer* buf = (Buffer*) buffer;
    MAKE_STACK_FP(pixFloat);
    MAKE_STACK_FP(x);
    MAKE_STACK_FP(yiter);
    MAKE_STACK_FP(pstrideLocal);
    fpcopy(&pstrideLocal, &buf->pstride);
    assert(buf->pstride.value.size == prec);
    assert(x.value.size == prec);
    while(true)
    {
        //Fetch and increment the current work column
        int xpix;
        pthread_mutex_lock(&workColMutex);
        xpix = workCol++;
        pthread_mutex_unlock(&workColMutex);
        //if all work has been completed, quit and wait to be joined with main thread
        if(xpix >= buf->w)
            return NULL;
        //Otherwise, compute the pixels for column xpix
        int offset = xpix - buf->w / 2;
        fpcopy(&x, &targetX);
        if(offset < 0)
        {
            for(int i = offset; i < 0; i++)
                fpsub2(&x, &pstrideLocal);
        }
        else if(offset > 0)
        {
            for(int i = 0; i < offset; i++)
                fpadd2(&x, &pstrideLocal);
        }
        //prepare y as the minimum y of the viewport
        fpcopy(&yiter, &targetY);
        for(int i = 0; i < buf->h / 2; i++)
            fpsub2(&yiter, &pstrideLocal);
        for(int ypix = 0; ypix < buf->h; ypix++)
        {
            if(filecount % 8 == 0 || buf->iters[xpix + ypix * buf->w] == 0)
            {
                /*
                puts("Getting conv rate @:");
                printf("x: ");
                biPrint(&x.value);
                printf("y: ");
                biPrint(&yiter.value);
                */
                int convRate = getConvRate(&x, &yiter);
                buf->iters[xpix + ypix * buf->w] = convRate;
            }
            fpadd2(&yiter, &pstrideLocal);
        }
    }
    return NULL;
}

void* workerFuncCapped(void* buffers)
{
    Buffer** bufs = (Buffer**) buffers;
    Buffer* buf = bufs[0];
    Buffer* coarse = bufs[1];
    MAKE_STACK_FP(pixFloat);
    MAKE_STACK_FP(x);
    MAKE_STACK_FP(yiter);
    MAKE_STACK_FP(pstrideLocal);
    fpcopy(&pstrideLocal, &buf->pstride);
    while(true)
    {
        //Fetch and increment the current work column
        int xpix;
        pthread_mutex_lock(&workColMutex);
        xpix = workCol++;
        pthread_mutex_unlock(&workColMutex);
        //if all work has been completed, quit and wait to be joined with main thread
        if(xpix >= buf->w)
            return NULL;
        //Otherwise, compute the pixels for column xpix
        int offset = xpix - buf->w / 2;
        fpcopy(&x, &targetX);
        if(offset < 0)
        {
            for(int i = offset; i < 0; i++)
                fpsub2(&x, &pstrideLocal);
        }
        else if(offset > 0)
        {
            for(int i = 0; i < offset; i++)
                fpadd2(&x, &pstrideLocal);
        }
        int coarseX = xpix * ((float) coarse->w / buf->w);
        int coarseY;
        //prepare y as the minimum y of the viewport
        fpcopy(&yiter, &targetY);
        for(int i = 0; i < buf->h / 2; i++)
            fpsub2(&yiter, &pstrideLocal);
        for(int ypix = 0; ypix < buf->h; ypix++)
        {
            if(filecount % 8 == 0 || buf->iters[xpix + ypix * buf->w] == 0)
            {
                coarseY = ypix * ((float) coarse->h / buf->h);
                int convRate = getConvRateCapped(&x, &yiter, 
                        coarse->iters[coarseX + coarseY * coarse->w] + 100);
                buf->iters[xpix + ypix * buf->w] = convRate;
            }
            fpadd2(&yiter, &pstrideLocal);
        }
    }
    return NULL;
}

void drawBuf(Buffer* buf, bool doRecycle)
{
    printf("Drawing buf with pstride = %Le\n", getValue(&buf->pstride));
    if(doRecycle)
        recycle(buf);
    else
        memset(buf->iters, 0, buf->w * buf->h * sizeof(int));
    launchWorkers(buf);
}

void fastDrawBuf(Buffer* buf, Buffer* coarse, bool doRecycle)
{
    if(doRecycle)
        recycle(buf);
    else
        memset(buf->iters, 0, buf->w * buf->h * sizeof(int));
    drawBuf(coarse, false); //never try to recycle coarse, because artifacts are amplified
    float xscl = (float) buf->w / coarse->w;
    float yscl = (float) buf->h / coarse->h;
    reduceIters(coarse->iters, 5 * xscl, coarse->w, coarse->h);
    //copy pixels from solid regions of coarse
#define GET_COARSE(x, y) (coarse->iters[x + coarse->w * y])
    int numSaved = 0;
    for(int i = 0; i < coarse->w - 1; i++)
    {
        for(int j = 0; j < coarse->h - 1; j++)
        {
            //if the 2x2 region of pixels in coarse is all the same,
            //fill all pixels in corresponding area of fine image 
            int fillValue = GET_COARSE(i, j);
            if(fillValue == GET_COARSE(i + 1, j) &&
               fillValue == GET_COARSE(i + 1, j + 1) &&
               fillValue == GET_COARSE(i, j + 1))
            {
                int xlo = ceil(buf->w / 2 + xscl * (i - coarse->w / 2));
                int xhi = floor(buf->w / 2 + xscl * (i + 1 - coarse->w / 2));
                int ylo = ceil(buf->h / 2 + yscl * (j - coarse->h / 2));
                int yhi = floor(buf->h / 2 + yscl * (j + 1 - coarse->h / 2));
                xlo = max(xlo, 0);
                ylo = max(ylo, 0);
                xhi = min(xhi, buf->w - 1);
                yhi = min(yhi, buf->h - 1);
                for(int ii = xlo; ii <= xhi; ii++)
                {
                    for(int jj = ylo; jj <= yhi; jj++)
                    {
                        if(buf->iters[ii + jj * buf->w] == 0)
                        {
                            buf->iters[ii + jj * buf->w] = fillValue;
                            numSaved++;
                        }
                    }
                }
            }
        }
    }
    if(verbose)
        printf("Saved %i (%.1f%%) pixels.\n", numSaved, 100.0 * numSaved / (buf->w * buf->h));
    launchWorkers(buf);
    reduceIters(buf->iters, 5, buf->w, buf->h);
    for(int i = 0; i < buf->w * buf->h; i++)
        buf->colors[i] = getColor(buf->iters[i]);
    blockFilter(0.1, buf->colors, buf->w, buf->h);
}

void launchWorkers(Buffer* buf)
{
    workCol = 0;
    if(pthread_mutex_init(&workColMutex, NULL))
    {
        puts("Failed to create mutex.");
        exit(EXIT_FAILURE);
    }
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
    for(int i = 0; i < numThreads; i++)
    {
        if(pthread_create(&threads[i], NULL, workerFunc, buf))
        {
            puts("Fatal error: Failed to create thread.");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numThreads; i++)
        pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&workColMutex);
    free(threads);
}

void launchWorkersCapped(Buffer* buf, Buffer* coarse)
{
    workCol = 0;
    if(pthread_mutex_init(&workColMutex, NULL))
    {
        puts("Failed to create mutex.");
        exit(EXIT_FAILURE);
    }
    Buffer* buffers[2] = {buf, coarse};
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
    for(int i = 0; i < numThreads; i++)
    {
        if(pthread_create(&threads[i], NULL, workerFuncCapped, buffers))
        {
            puts("Fatal error: Failed to create thread.");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numThreads; i++)
        pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&workColMutex);
    free(threads);
}

void recycle(Buffer* buf)
{
    //iterate in rows from top to middle then from bottom to middle
    //if an itercount+color can be borrowed from the last frame, copy it
    //otherwise, set to 0
    #define recyclePix \
    { \
        int oldx = buf->w / 4 + i / 2; \
        int oldy = buf->h / 4 + j / 2; \
        if(i % 2 == 0 && j % 2 == 0 && buf->iters[oldx + oldy * buf->w] != -1) \
            buf->iters[i + j * buf->w] = buf->iters[oldx + oldy * buf->w]; \
        else \
            buf->iters[i + j * buf->w] = 0; \
    }
    for(int i = 0; i < buf->w / 2; i++)
    {
        for(int j = 0; j < buf->h / 2; j++)
            recyclePix;
        for(int j = buf->h - 1; j >= buf->h / 2; j--)
            recyclePix;
    }
    for(int i = buf->w - 1; i >= buf->w / 2; i--)
    {
        for(int j = 0; j < buf->h / 2; j++)
            recyclePix;
        for(int j = buf->h - 1; j >= buf->h / 2; j--)
            recyclePix;
    }
}

void recomputeMaxIter(int zoomExpo)
{
    const int normalIncrease = 70 * zoomExpo;
    maxiter += normalIncrease;
}

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache)
{
    FILE* cache = NULL;
    if(cacheFile && useCache)
    {
        cache = fopen(cacheFile, "rb");
        targetX = fpRead(cache);
        targetY = fpRead(cache);
        fclose(cache);
        return;
    }
    long double initViewport = 4;
    //make a temporary iteration count buffer
    Buffer buf; 
    int gpx = 8;                       //size, in pixels, of GIL iteration buffer (must be PoT)
    prec = 1;
    targetX = FPCtorValue(prec, 0);
    targetY = FPCtorValue(prec, 0);
    buf.iters = (int*) malloc(gpx * gpx * sizeof(int));
    buf.colors = NULL;
    buf.pstride = FPCtorValue(prec, initViewport / gpx);
    buf.w = gpx;
    buf.h = gpx;
    int zoomExpo = 2;               //log_2 of zoom factor 
    FP fbestPX = FPCtor(1);
    FP fbestPY = FPCtor(1);
    maxiter = 300;
    int count = 0;
    while(true)
    {
        if(getApproxExpo(&buf.pstride) <= minExpo)
        {
            if(verbose)
            {
                printf("Stopping because pstride <= 2^%i\n", getApproxExpo(&buf.pstride));
            }
            break;
        }
        printf("Pixel stride = %Le, iter cap is %i\n", getValue(&buf.pstride), maxiter);
        printf("Pstride as int: ");
        biPrint(&buf.pstride.value);
        drawBuf(&buf, count % 7 != 0);
        reduceIters(buf.iters, 2, gpx, gpx);
        if(verbose)
        {
            puts("**********The buffer:**********");
            for(int i = 0; i < gpx * gpx; i++)
            {
                printf("%5i ", buf.iters[i]);
                if(i % gpx == gpx - 1)
                    puts("");
            }
            puts("*******************************");
        }
        int bestPX;
        int bestPY;
        int bestIters = 0;
        u64 itersum = 0;
        //get point with maximum iteration count (that didn't converge)
        for(int i = 0; i < gpx; i++)
        {
            for(int j = 0; j < gpx; j++)
            {
                itersum += buf.iters[i + j * gpx];
                if(buf.iters[i + j * gpx] > bestIters)
                {
                    bestIters = buf.iters[i + j * gpx];
                    bestPX = i;
                    bestPY = j;
                }
            }
        }
        printf("Zooming toward pixel (%i, %i)\n", bestPX, bestPY);
        itersum /= (gpx * gpx); //compute average iter count
        recomputeMaxIter(zoomExpo);
        if(bestIters == 0)
        {
            puts("getInterestingLocation() frame contains all converging points!");
            break;
        }
        bool allSame = true;
        int val = buf.iters[0];
        for(int i = 0; i < gpx * gpx; i++)
        {
            if(buf.iters[i] != val)
            {
                allSame = false;
                break;
            }
        }
        if(allSame)
        {
            puts("getInterestingLocation() frame contains all the same iteration count!");
            break;
        }
        int xmove = bestPX - gpx / 2;
        int ymove = bestPY - gpx / 2;
        if(xmove < 0)
        {
            for(int i = xmove; i < 0; i++)
                fpsub2(&targetX, &buf.pstride);
        }
        else if(xmove > 0)
        {
            for(int i = 0; i < xmove; i++)
                fpadd2(&targetX, &buf.pstride);
        }
        if(ymove < 0)
        {
            for(int i = ymove; i < 0; i++)
                fpsub2(&targetY, &buf.pstride);
        }
        else if(ymove > 0)
        {
            for(int i = 0; i < ymove; i++)
                fpadd2(&targetY, &buf.pstride);
        }
        //set screen position to the best pixel
        if(upgradePrec(&buf.pstride))
        {
            INCR_PREC(buf.pstride);
            INCR_PREC(targetX);
            INCR_PREC(targetY);
            prec++;
            if(verbose)
            {
                printf("*** Increased precision to level %i ***\n", prec);
            }
        }
        //zoom in according to the PoT zoom factor defined above
        printf("target x: ");
        biPrint(&targetX.value);
        printf("target y: ");
        biPrint(&targetY.value);
        fpshr(buf.pstride, zoomExpo);
    }
    free(buf.iters); 
    //do not lose any precision when storing targetX, targetY
    puts("Saving target position.");
    if(cacheFile)
    {
        cache = fopen(cacheFile, "wb");
        //write targetX, targetY to the cache file
        fpWrite(&targetX, cache);
        fpWrite(&targetY, cache);
        fclose(cache);
    }
    FPDtor(&buf.pstride);
    FPDtor(&fbestPX);
    FPDtor(&fbestPY);
    count++;
}

bool upgradePrec(FP* pstride)
{
    //is a bit in the lowest half of pstride high?
    int numBits = pstride->value.size * 64;
    //when the most significant 1 bit is this far from end, return true
    const int minSigBits = 32;
    //Scan for the first nonzero bit
    for(int i = 0; i < numBits; i++)
    {
        if(biNthBit(&pstride->value, i))
        {
            if(numBits - i <= minSigBits)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    assert(!"pstride is zero!");
    return false;
}

int main(int argc, const char** argv)
{
    //Process cli arguments
    //set all the arguments to default first
    const char* targetCache = NULL;
    bool useTargetCache = false;
    numThreads = 4;
    const int defaultWidth = 640;
    const int defaultHeight = 480;
    int imageWidth = defaultWidth;
    int imageHeight = defaultHeight;
    const char* resumeFile = NULL;
    verbose = false;
    deepestExpo = -300;
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-n") == 0)
        {
            //# of threads is next
            sscanf(argv[++i], "%i", &numThreads);
            //sanity check it
            if(numThreads < 1)
            {
                puts("Can't have < 1 threads.");
                numThreads = 1;
            }
        }
        else if(strcmp(argv[i], "--targetcache") == 0)
            targetCache = argv[++i];
        else if(strcmp(argv[i], "--usetargetcache") == 0)
            useTargetCache = true;
        else if(strcmp(argv[i], "--size") == 0)
        {
            if(2 != sscanf(argv[++i], "%ix%i", &imageWidth, &imageHeight))
            {
                puts("Size must be specified in the format <width>x<height>");
                imageHeight = defaultWidth;
                imageHeight = defaultHeight;
            }
        }
        else if(strcmp(argv[i], "--resume") == 0)
            resumeFile = argv[++i];
        else if(strcmp(argv[i], "--verbose") == 0)
            verbose = true;
        else if(strcmp(argv[i], "--depth") == 0)
            sscanf(argv[++i], "%i", &deepestExpo);
    }
    printf("Running on %i thread(s).\n", numThreads);
    if(targetCache && useTargetCache)
        printf("Will read target location from \"%s\"\n", targetCache);
    else if(targetCache)
        printf("Will write target location to \"%s\"\n", targetCache);
    printf("Will output %ix%i images.\n", imageWidth, imageHeight);
    getInterestingLocation(deepestExpo, targetCache, useTargetCache);
    return 0;
    prec = 1;
    Buffer image;
    image.iters = (int*) malloc(imageWidth * imageHeight * sizeof(int));
    image.colors = (unsigned*) malloc(imageWidth * imageHeight * sizeof(unsigned));
    image.w = imageWidth;
    image.h = imageHeight;
    image.pstride = FPCtorValue(prec, 4.0 / imageWidth);
    Buffer coarse;
    coarse.w = 200;
    coarse.h = coarse.w * imageHeight / imageWidth;
    coarse.iters = (int*) malloc(coarse.w * coarse.h * sizeof(int));
    coarse.colors = NULL;
    coarse.pstride = FPCtorValue(prec, 4.0 / coarse.w);
    printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
    maxiter = 500;
    colortable = (Uint32*) malloc(sizeof(Uint32) * 360);
    initColorTable();
    filecount = 0;
    //resume file: filecount, last maxiter, prec
    while(getApproxExpo(&image.pstride) >= deepestExpo)
    {
        time_t start = time(NULL);
        //create coarse image with full iteration count
        //fastDrawBuf(&image, &coarse, false);
/* TESTING */
        drawBuf(&image, false);
        for(int i = 0; i < image.w * image.h; i++)
            image.colors[i] = getColor(image.iters[i]);

        writeImage(&image);
        fpshrOne(image.pstride);
        fpshrOne(coarse.pstride);
        recomputeMaxIter(1);
        int timeDiff = time(NULL) - start;
        if(upgradePrec(&image.pstride))
        {
            INCR_PREC(image.pstride);
            INCR_PREC(coarse.pstride);
            prec++;
            if(verbose)
                printf("*** Increasing precision to level %i (%i bits) ***\n", prec, 63 * prec);
        }
        printf("Image #%i took %i seconds. ", filecount - 1, timeDiff);
        if(verbose)
        {
            printf("iter cap: %i, ", maxiter);
            printf("precision: %i, ", prec);
            printf("px/sec/thread: %i\n", image.w * image.h / max(1, timeDiff) / numThreads);
        }
        else
            puts(".");
    }
    free(image.iters);
    free(image.colors);
    free(coarse.iters);
    free(colortable);
}

