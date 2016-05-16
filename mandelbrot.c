#include "mandelbrot.h"

Float screenX;
Float screenY;
Float pstride;      //size of a pixel
Float targetX;
Float targetY;
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
    screenX = FloatCtor(1);
    screenY = FloatCtor(1);
    pstride = floatLoad(1, startw / winw);  //pixels are always exactly square, no separate x and y
    computeScreenPos();
}

void increasePrecision()
{
    //reallocate space for the persistent Float variables
    //INCR_PREC macro copies significant words into reallocated space and updates mantissa size
    INCR_PREC(screenX);
    INCR_PREC(screenY);
    INCR_PREC(pstride);
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

void zoomToTarget()
{
    //update pixel stride
    MAKE_STACK_FLOAT(sizeFactor);
    MAKE_STACK_FLOAT(temp);
    pstride.expo--;     //hard-code the 2x zoom factor
    computeScreenPos();
}

void computeScreenPos()
{
    MAKE_STACK_FLOAT(pixWidth);
    MAKE_STACK_FLOAT(pixHeight);
    MAKE_STACK_FLOAT(temp);
    //need copies of targetX, targetY with matching precision
    MAKE_STACK_FLOAT(targetXlocal);
    MAKE_STACK_FLOAT(targetYlocal);
    fcopy(&targetXlocal, &targetX);
    fcopy(&targetYlocal, &targetY);
    storeFloatVal(&pixWidth, winw);
    storeFloatVal(&pixHeight, winh);
    pixWidth.expo--;
    pixHeight.expo--;
    fmul(&temp, &pstride, &pixWidth);
    fsub(&screenX, &targetXlocal, &temp);
    fmul(&temp, &pstride, &pixHeight);
    fsub(&screenY, &targetYlocal, &temp);
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

int getConvRate(Float* real, Float* imag)
{
    //real, imag make up "c" in z = z^2 + c
    int iter = 0;
    u64* localbuf = (u64*) alloca(8 * prec * sizeof(u64));
    MAKE_FLOAT(zr, &localbuf[prec * 0]);
    storeFloatVal(&zr, 0);
    MAKE_FLOAT(zi, &localbuf[prec * 1]);
    storeFloatVal(&zi, 0);
    MAKE_FLOAT(zrsquare, &localbuf[prec * 2]);
    MAKE_FLOAT(zisquare, &localbuf[prec * 3]);
    MAKE_FLOAT(zri, &localbuf[prec * 4]);
    MAKE_FLOAT(zrtemp, &localbuf[prec * 5]);
    MAKE_FLOAT(zitemp, &localbuf[prec * 6]);
    MAKE_FLOAT(mag, &localbuf[prec * 7]);
    for(; iter < maxiter; iter++)
    {
        //printf("Doing iter %i\n", iter);
        //printf("zr = %LF\n", getFloatVal(&zr));
        //printf("zi = %LF\n", getFloatVal(&zi));
        fmul(&zrsquare, &zr, &zr);
        fmul(&zisquare, &zi, &zi);
        //printf("zrsquare = %Lf\n", getFloatVal(&zrsquare));
        //printf("zisquare = %Lf\n", getFloatVal(&zisquare));
        fadd(&mag, &zrsquare, &zisquare);
        //printf("mag = %Lf\n", getFloatVal(&mag));
        fmul(&zri, &zr, &zi);
        //printf("r*i = %Lf\n", getFloatVal(&zri));
        if(zri.expo)            //multiply by 2
            zri.expo++;
        fsub(&zrtemp, &zrsquare, &zisquare);
        //printf("zrtemp = %Lf\n", getFloatVal(&zrtemp));
        fadd(&zr, &zrtemp, real);
        fadd(&zi, &zri, imag);
        //compare mag to 4.0
        //Use fact that 4.0 is the smallest Float value with exponent 2, regardless of precision
        if((long long) mag.expo - expoBias > 2)
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
        int expo;
        frexpl(mag, &expo);
        if((long long) expo >= 3)
            break;
    }
    return iter == maxiter ? -1 : iter;
}

void* workerFunc(void* unused)
{
    MAKE_STACK_FLOAT(xpixFloat);
    MAKE_STACK_FLOAT(x);
    MAKE_STACK_FLOAT(yiter);
    MAKE_STACK_FLOAT(addtemp);  //need a temporary destination for iter += pstride
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
        storeFloatVal(&xpixFloat, xpix);
        fmul(&addtemp, &pstride, &xpixFloat);
        fadd(&x, &screenX, &addtemp);
        fcopy(&yiter, &screenY);
        for(int ypix = 0; ypix < winh; ypix++)
        {
            int convRate = getConvRate(&x, &yiter);
            if(pixbuf)
                pixbuf[ypix * winw + xpix] = getColor(convRate);
            iterbuf[ypix * winw + xpix] = convRate;
            fcopy(&addtemp, &yiter);
            fadd(&yiter, &addtemp, &pstride);
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

void recomputeMaxIter()
{
    const int normalIncrease = 20;
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
    numMax /= numColored;
    int numBoosts = numMax / (numColored * 0.01);
    if(numBoosts)
    {
        printf("Boosting iteration count by %i\n", boost * numBoosts);
        maxiter += numBoosts * boost;
    }
    maxiter += normalIncrease;
}

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache)
{
    FILE* cache = NULL;
    if(cacheFile && useCache)
    {
        cache = fopen(cacheFile, "r");
        targetX = floatRead(cache);
        targetY = floatRead(cache);
        fclose(cache);
        return;
    }
    targetX = FloatCtor(1);
    targetY = FloatCtor(1);
    //make a temporary iteration count buffer
    int gpx = 32;                       //size, in pixels, of GIL iteration buffer (must be PoT)
    winw = gpx;
    winh = gpx;
    iterbuf = (int*) malloc(gpx * gpx * sizeof(int));
    prec = 1;
    long double initViewport = 4;
    storeFloatVal(&pstride, initViewport / gpx);
    int zoomExpo = 3;               //log_2 of zoom factor 
    Float fbestPX = FloatCtor(1);
    Float fbestPY = FloatCtor(1);
    Float temp = FloatCtor(1);
    Float temp2 = FloatCtor(1);
    Float halfSize = floatLoad(1, gpx / 2);
    storeFloatVal(&screenX, 0);
    storeFloatVal(&screenY, 0);
    maxiter = 100;
    while((long long) pstride.expo - expoBias >= minExpo)
    {
        printf("Pixel stride = %.10Le\n", getFloatVal(&pstride));
        //compute screenX, screenY
        fmul(&temp, &pstride, &halfSize);
        fsub(&temp2, &screenX, &temp);
        fcopy(&screenX, &temp2);
        fsub(&temp2, &screenY, &temp);
        fcopy(&screenY, &temp2);
        drawBuf();
        int bestPX;
        int bestPY;
        int bestIters = 0;
        u64 itersum = 0;
        for(int i = 0; i < gpx; i++)
        {
            for(int j = 0; j < gpx; j++)
            {
                itersum += iterbuf[i + j * gpx];
                if(iterbuf[i + j * gpx] > bestIters)
                {
                    bestPX = i;
                    bestPY = j;
                    bestIters = iterbuf[i + j * gpx];
                }
            }
        }
        itersum /= (gpx * gpx); //compute average iter count
        maxiter = (itersum * 2 + bestIters) / 3 + 200;
        if(bestIters == 0)
        {
            puts("getInterestingLocation() got stuck on window of converging points!");
            puts("Decrease zoom factor or increase resolution.");
            break;
        }
        recomputeMaxIter();
        storeFloatVal(&fbestPX, bestPX);
        storeFloatVal(&fbestPY, bestPY);
        //set screenX/screenY to location of best pixel
        fmul(&temp, &fbestPX, &pstride);
        fcopy(&temp2, &screenX);
        fadd(&screenX, &temp, &temp2);
        fmul(&temp, &fbestPY, &pstride);
        fcopy(&temp2, &screenY);
        fadd(&screenY, &temp, &temp2);
        pstride.expo -= zoomExpo;
        fmul(&temp, &fbestPX, &pstride);
        //set screen position to the best pixel
        if(prec < getPrec((long long) pstride.expo - expoBias))
        {
            increasePrecision();
            INCR_PREC(temp);
            INCR_PREC(temp2);
            INCR_PREC(halfSize);
        }
    }
    free(iterbuf);
    iterbuf = NULL;
    //do not lose any precision when storing targetX, targetY
    puts("Saving target position.");
    CHANGE_PREC(targetX, prec);
    CHANGE_PREC(targetY, prec);
    fcopy(&targetX, &screenX);
    fcopy(&targetY, &screenY);
    if(cacheFile)
    {
        cache = fopen(cacheFile, "w");
        //write targetX, targetY to the cache file
        floatWrite(&targetX, cache);
        floatWrite(&targetY, cache);
        fclose(cache);
    }
    FloatDtor(&temp);
    FloatDtor(&temp2);
    FloatDtor(&halfSize);
}

int getPrec(int expo)
{
    return -expo / 50;
}

void saveResumeState(const char* fname)
{
}

void loadResumeState(const char* fname)
{

}

int main(int argc, const char** argv)
{
    //Process cli arguments first
    //Set all the arguments to default first
    const char* targetCache = NULL;
    bool useTargetCache = false;
    numThreads = 1;
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
    screenX = FloatCtor(1);
    screenY = FloatCtor(1);
    pstride = FloatCtor(1);
    getInterestingLocation(deepestExpo, targetCache, useTargetCache);
    prec = 1;
    CHANGE_PREC(screenX, 1);
    CHANGE_PREC(screenY, 1);
    CHANGE_PREC(pstride, 1);
    winw = imageWidth;
    winh = imageHeight;
    initPositionVars();
    printf("Will zoom towards %.30Lf, %.30Lf\n", getFloatVal(&targetX), getFloatVal(&targetY));
    maxiter = 100;
    colortable = (Uint32*) malloc(sizeof(Uint32) * 360);
    initColorTable();
    computeScreenPos();     //get initial screenX, screenY
    pixbuf = (Uint32*) malloc(sizeof(Uint32) * winw * winh);
    iterbuf = (int*) malloc(sizeof(int) * winw * winh);
    filecount = 0;
    //resume file: filecount, last maxiter, prec
    while((long long) pstride.expo - expoBias >= deepestExpo)
    {
        time_t start = time(NULL);
        drawBuf();
        writeImage();
        zoomToTarget();
        recomputeMaxIter();
        int timeDiff = time(NULL) - start;
        if(getPrec(pstride.expo) > prec)
        {
            increasePrecision();
            printf("*** Increasing precision to %i bits. ***\n", 63 * prec);
        }
        printf("Generated image #%i in %i seconds. (", filecount - 1, timeDiff); 
        if(timeDiff == 0)
            putchar('>');
        printf("%i pixels per second)\n", winw * winh / max(timeDiff, 1));
    }
    free(iterbuf);
    free(pixbuf);
    free(colortable);
}
