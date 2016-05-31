#include "mandelbrot.h"

FP pstride;      //size of a pixel
FP targetX;
FP targetY;
Uint32* pixbuf;
int* iterbuf;
Uint32* colortable;
int filecount;
int numThreads;
int maxiter;
int prec;
int workCol;
pthread_mutex_t workColMutex;
int winw;
int winh;

void initPositionVars()
{
    const long double startw = 4;
    pstride = FPCtorValue(1, startw / winw);  //pixels are always exactly square, no separate x and y
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

void zoomToTarget()
{
    //update pixel stride (divide by 2)
    fpshrOne(pstride);
}

void initColorTable()
{
    for(int i = 0; i < 360; i++)
    {
        int t = (i * 2 + 240) % 360;
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
    //make a steeper gradient for the first few iterations (better image 0)
    int steepCutoff = 50;
    if(num <= steepCutoff)
        return colortable[(num * 3) % 360];
    return colortable[((num - steepCutoff) + 3 * steepCutoff) % 360];
}

int getConvRate(FP* real, FP* imag)
{
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
            break;
    }
    return iter == maxiter ? -1 : iter;
}

int getConvRateLD(long double real, long double imag)
{
    //real, imag make up "c" in z = z^2 + c
    int iter = 0;
    long double zr = 0;
    long double zi = 0;
    MAKE_STACK_FP(asdf);
    MAKE_STACK_FP(four);
    loadValue(&four, 4);
    for(; iter < maxiter; iter++)
    {
        //printf("Doing iter %i\n", iter);
        //printf("zr = %LF\n", getFloatVal(&zr));
        //printf("zi = %LF\n", getFloatVal(&zi));
        long double zrsquare = zr * zr;
        long double zisquare = zi * zi;
        long double zri = zr * zi;
        long double mag = zrsquare + zisquare;
        zri *= 2;
        long double zrtemp = zrsquare - zisquare;
        zr = zrtemp + real;
        zi = zri + imag;
        //compare mag to 4.0
        //Use fact that 4.0 is the smallest Float value with exponent 2, regardless of precision
        if(mag >= 4)
            break;
    }
    return iter == maxiter ? -1 : iter;
}

void* workerFunc(void* unused)
{
    MAKE_STACK_FP(pixFloat);
    MAKE_STACK_FP(x);
    MAKE_STACK_FP(yiter);
    MAKE_STACK_FP(pstrideLocal);
    fpcopy(&pstrideLocal, &pstride);
    while(true)
    {
        //Fetch and increment the current work column
        int xpix;
        pthread_mutex_lock(&workColMutex);
        xpix = workCol++;
        pthread_mutex_unlock(&workColMutex);
        //if all work has been completed, quit and wait to be joined with main thread
        if(xpix >= winw)
            return NULL;
        //Otherwise, compute the pixels for column xpix
        int offset = xpix - winw / 2;
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
        //printf("x = %.10Lf\n", getValue(&x));
        //prepare y as the minimum y of the viewport
        fpcopy(&yiter, &targetY);
        for(int i = 0; i < winh / 2; i++)
            fpsub2(&yiter, &pstrideLocal);
        for(int ypix = 0; ypix < winh; ypix++)
        {
            int convRate = getConvRate(&x, &yiter);
            //int convRate = getConvRateLD(getValue(&x), getValue(&yiter));
            if(pixbuf)
                pixbuf[ypix * winw + xpix] = getColor(convRate);
            iterbuf[ypix * winw + xpix] = convRate;
            fpadd2(&yiter, &pstrideLocal);
        }
    }
    return NULL;
}

void drawBuf()
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
        if(pthread_create(&threads[i], NULL, workerFunc, NULL))
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

void recomputeMaxIter(int zoomExpo)
{
    const int normalIncrease = 40 * zoomExpo;
    const int boost = 50;
    int numPixels = winw * winh;
    int numColored = 0;
    int numMax = 0;
    for(int i = 0; i < numPixels; i++)
    {
        if(iterbuf[i] != -1)
            numColored++;
        if(iterbuf[i] == maxiter)
            numMax++;
    }
    if(numColored)
    {
        numMax /= numColored;
        int numBoosts = numMax / (numColored * 0.01);
        if(numBoosts)
        {
            printf("Boosting iteration count by %i\n", boost * numBoosts);
            maxiter += numBoosts * boost;
        }
    }
    maxiter += normalIncrease;
}

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache)
{
    FILE* cache = NULL;
    if(cacheFile && useCache)
    {
        cache = fopen(cacheFile, "r");
        targetX = fpRead(cache);
        targetY = fpRead(cache);
        fclose(cache);
        return;
    }
    targetX = FPCtorValue(1, 0);
    targetY = FPCtorValue(1, 0);
    //make a temporary iteration count buffer
    int gpx = 16;                       //size, in pixels, of GIL iteration buffer (must be PoT)
    winw = gpx;
    winh = gpx;
    iterbuf = (int*) malloc(gpx * gpx * sizeof(int));
    prec = 1;
    long double initViewport = 4.0;
    loadValue(&pstride, initViewport / gpx);
    int zoomExpo = 4;               //log_2 of zoom factor 
    FP fbestPX = FPCtor(1);
    FP fbestPY = FPCtor(1);
    maxiter = 300;
    while(getApproxExpo(&pstride) >= minExpo)
    {
        printf("Pixel stride = %Le, iter cap is %i\n", getValue(&pstride), maxiter);
        //drawBuf() only depends on pixel stride, which is already set 
        drawBuf();
        int bestPX;
        int bestPY;
        int bestIters = 0;
        int numBest = 0;
        u64 itersum = 0;
        //Find the maximum non-converging iteration count,
        //and then randomly pick a point with that iter count to be target
        for(int i = 0; i < gpx; i++)
        {
            for(int j = 0; j < gpx; j++)
            {
                itersum += iterbuf[i + j * gpx];
                if(iterbuf[i + j * gpx] > bestIters)
                {
                    bestIters = iterbuf[i + j * gpx];
                    numBest = 1;
                }
                else if(iterbuf[i + j * gpx] == bestIters)
                    numBest++;
            }
        }
        int targetChoice = rand() % numBest;
        for(int i = 0; i < gpx; i++)
        {
            for(int j = 0; j < gpx; j++)
            {
                if(iterbuf[i + j * gpx] == bestIters)
                {
                    if(--targetChoice == 0)
                    {
                        bestPX = i;
                        bestPY = j;
                    }
                }
            }
        }
        itersum /= (gpx * gpx); //compute average iter count
        recomputeMaxIter(zoomExpo);
        if(bestIters == 0)
        {
            puts("getInterestingLocation() got stuck on window of converging points!");
            break;
        }
        bool allSame = true;
        int val = iterbuf[0];
        for(int i = 0; i < winw * winh; i++)
        {
            if(iterbuf[i] != val)
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
            for(int i = 0; i < -xmove; i++)
                fpsub2(&targetX, &pstride);
        }
        else if(xmove > 0)
        {
            for(int i = 0; i < xmove; i++)
                fpadd2(&targetX, &pstride);
        }
        if(ymove < 0)
        {
            for(int i = 0; i < -ymove; i++)
                fpsub2(&targetY, &pstride);
        }
        else if(xmove > 0)
        {
            for(int i = 0; i < ymove; i++)
                fpadd2(&targetY, &pstride);
        }
        //zoom in according to the PoT zoom factor defined above
        fpshr(pstride, zoomExpo);
        //set screen position to the best pixel
        if(prec < getPrec(getApproxExpo(&pstride)))
        {
            INCR_PREC(pstride);
            INCR_PREC(targetX);
            INCR_PREC(targetY);
            prec++;
            printf("*** Increasing precision to level %i ***\n", prec);
        }
    }
    free(iterbuf);
    iterbuf = NULL;
    //do not lose any precision when storing targetX, targetY
    puts("Saving target position.");
    if(cacheFile)
    {
        cache = fopen(cacheFile, "w");
        //write targetX, targetY to the cache file
        fpWrite(&targetX, cache);
        fpWrite(&targetY, cache);
        fclose(cache);
    }
    FPDtor(&fbestPX);
    FPDtor(&fbestPY);
}

int getPrec(int expo)
{
    return ceil(-expo / 45) + 1;
}

int main(int argc, const char** argv)
{
    //Process cli arguments first
    //Set all the arguments to default first
    const char* targetCache = NULL;
    bool useTargetCache = false;
    numThreads = 4;
    const int defaultWidth = 640;
    const int defaultHeight = 480;
    int imageWidth = defaultWidth;
    int imageHeight = defaultHeight;
    const char* resumeFile = NULL;
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
    }
    printf("Running on %i thread(s).\n", numThreads);
    if(targetCache && useTargetCache)
        printf("Will read target location from \"%s\"\n", targetCache);
    else if(targetCache)
        printf("Will write target location to \"%s\"\n", targetCache);
    printf("Will output %ix%i images.\n", imageWidth, imageHeight);
    pstride = FPCtor(1);
    getInterestingLocation(deepestExpo, targetCache, useTargetCache);
    prec = 1;
    CHANGE_PREC(pstride, prec);
    winw = imageWidth;
    winh = imageHeight;
    initPositionVars();
    printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
    maxiter = 100;
    colortable = (Uint32*) malloc(sizeof(Uint32) * 360);
    initColorTable();
    pixbuf = (Uint32*) malloc(sizeof(Uint32) * winw * winh);
    iterbuf = (int*) malloc(sizeof(int) * winw * winh);
    filecount = 0;
    //resume file: filecount, last maxiter, prec
    while(getApproxExpo(&pstride) >= deepestExpo)
    {
        time_t start = time(NULL);
        drawBuf();
        writeImage();
        zoomToTarget();
        recomputeMaxIter(1);
        int timeDiff = time(NULL) - start;
        if(getPrec(getApproxExpo(&pstride)) > prec)
        {
            INCR_PREC(pstride);
            prec++;
            printf("*** Increasing precision to level %i (%i bits) ***\n", prec, 63 * prec);
        }
        printf("Image #%i took %i seconds. ", filecount - 1, timeDiff);
        printf("iter cap: %i, ", maxiter);
        printf("precision: %i, ", prec);
        printf("px/sec/thread: %i\n", winw * winh / max(1, timeDiff) / numThreads);
    }
    free(iterbuf);
    free(pixbuf);
    free(colortable);
}

