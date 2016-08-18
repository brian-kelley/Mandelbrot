#include "mandelbrot.h"

unsigned* colortable;
int winw;
int winh;
int* iters;
unsigned* colors;
FP targetX;
FP targetY;
FP pstride;
int filecount;
int numThreads;
int maxiter;
int prec;
int deepestExpo;
bool verbose;

int savings;

#define NUM_COLORS 360
#define NOT_COMPUTED -2

void writeImage()
{
    char name[32];
    sprintf(name, "output/mandel%i.png", filecount++);
    for(int i = 0; i < winw * winh; i++)
    {
        Uint32 temp = colors[i]; 
        //convert rgba to big endian
        colors[i] = ((temp & 0xFF000000) >> 24) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF00) << 8) | ((temp & 0xFF) << 24);
    }
    lodepng_encode32_file(name, (unsigned char*) colors, winw, winh);
}

void initColorTable()
{
    colortable = (unsigned*) malloc(NUM_COLORS * sizeof(unsigned));
    for(int i = 0; i < NUM_COLORS; i++)
    {
        int t = (i * 2 + 240) % NUM_COLORS;
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
    if(num == NOT_COMPUTED)
        return getColor(100);
    if(num == -1)
        return 0x000000FF;                  //in the mandelbrot set = opaque black
    //make a steeper gradient for the first few iterations (better image 0)
    int steepCutoff = 0;
    if(num <= steepCutoff)
        return colortable[(num * 3) % NUM_COLORS];
    return colortable[((num - steepCutoff) + 3 * steepCutoff) % NUM_COLORS];
}

void getPixelConvRate(int x, int y)
{
    if(iters[x + y * winw] != NOT_COMPUTED)
        return;
    //printf("Getting conv rate @ %i, %i\n", x, y);
    if(prec == 1)
    {
        long double r = getValue(&pstride) * (x - winw / 2) + getValue(&targetX);
        long double i = getValue(&pstride) * (y - winh / 2) + getValue(&targetY);
        iters[x + y * winw] = getConvRateLD(r, i);
    }
    else
    {
        //convert x, y to offsets rel. to viewport center
        MAKE_STACK_FP(r);
        MAKE_STACK_FP(i);
        MAKE_STACK_FP(temp);
        fpcopy(&r, &pstride);
        loadValue(&temp, x - winw / 2);
        fpmul2(&r, &temp);
        fpcopy(&temp, &targetX);
        fpadd2(&r, &temp);
        fpcopy(&i, &pstride);
        loadValue(&temp, y - winh / 2);
        fpmul2(&i, &temp);
        fpcopy(&temp, &targetY);
        fpadd2(&i, &temp);
        iters[x + y * winw] = getConvRateFP(&r, &i);
    }
}

int getConvRateFP(FP* real, FP* imag)
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
            break;
    }
    return iter == maxiter ? -1 : iter;
}

int getConvRateLD(long double real, long double imag)
{
    long double zr = 0;
    long double zi = 0;
    long double zrsquare, zisquare, zri, mag;
    int iter = 0;
    for(; iter < maxiter; iter++)
    {
        zrsquare = zr * zr;
        zisquare = zi * zi;
        zri = 2.0 * zr * zi;
        //want 2 * zr * zi
        zr = zrsquare - zisquare;
        zr += real;
        zi = zri + imag;
        mag = zrsquare + zisquare;
        if(mag >= 4.0 || mag <= -4.0)
            break;
    }
    return iter == maxiter ? -1 : iter;
}

void fillRect(Rect r)
{
    //printf("Filling rect: (%i, %i), %ix%i\n", r.x, r.y, r.w, r.h);
    //get the exact # of pixels to be computed
    int numPx = 0;
    //get indices of corners
    int upperLeft = r.x + r.y * winw;
    int upperRight = upperLeft + r.w - 1;
    int lowerLeft = upperLeft + (r.h - 1) * winw;
    Point* queue = malloc(2 * (r.w + r.h) * sizeof(Point));
    int count = 0;
    for(int i = 0; i < r.w; i++)
    {
        if(iters[upperLeft + i] == NOT_COMPUTED)
        {
            queue[count].x = r.x + i;
            queue[count].y = r.y;
            count++;
        }
        if(iters[lowerLeft + i] == NOT_COMPUTED)
        {
            queue[count].x = r.x + i;
            queue[count].y = r.y + r.h - 1;
            count++;
        }
    }
    for(int i = 0; i < r.h; i++)
    {
        if(iters[upperLeft + i * winw] == NOT_COMPUTED)
        {
            queue[count].x = r.x;
            queue[count].y = r.y + i;
            count++;
        }
        if(iters[upperRight + i * winw] == NOT_COMPUTED)
        {
            queue[count].x = r.x + r.w - 1;
            queue[count].y = r.y + i;
            count++;
        }
    }
    iteratePointQueue(queue, count);
    free(queue);
    //done if rectangle can't be subdivided & is fully computed
    //done if all boundary points have same iter count
    int iterCount = iters[upperLeft];
    bool allSame = true;
    for(int i = 0; i < r.w; i++)
    {
        if(iters[i + upperLeft] != iterCount ||
           iters[i + lowerLeft] != iterCount)
        {
            allSame = false;
            break;
        }
    }
    for(int i = 1; i < r.h - 1; i++)
    {
        if(iters[i * winw + upperLeft] != iterCount ||
           iters[i * winw + upperRight] != iterCount)
        {
            allSame = false;
            break;
        }
    }
    if(allSame)
    {
        //flood fill the rectangle with iterCount
        for(int i = 1; i < r.w - 1; i++)
        {
            for(int j = 1; j < r.h - 1; j++)
            {
                iters[(i + r.x) + (j + r.y) * winw] = iterCount;
                savings++;
            }
        }
        return;
    }
    //partition the rect into two smaller ones
    //make sure they share all possible edges, but otherwise don't overlap
    //cut along shorter dimension
    //cut at the first different pixel along boundary
    if(r.w > r.h)
    {
        Rect first = {r.x, r.y, r.w / 2, r.h};
        fillRect(first);
        Rect second = {r.x + r.w / 2, r.y, r.w - r.w / 2, r.h};
        fillRect(second);
        return;
    }
    else
    {
        Rect first = {r.x, r.y, r.w, r.h / 2};
        fillRect(first);
        Rect second = {r.x, r.y + r.h / 2, r.w, r.h - r.h / 2};
        fillRect(second);
        return;
    }
}

void iteratePointQueue(Point* queue, int num)
{
    if(num == 0)
        return;
    //have count points to recompute
    //partition evenly across threads
    //don't make more threads than there are points
    int numWorkers = min(numThreads, num);
    pthread_t* threads = alloca(numWorkers * sizeof(pthread_t));
    WorkInfo* wi = alloca(numWorkers * sizeof(WorkInfo));
    for(int i = 0; i < numWorkers; i++)
    {
        //get work range in points for this thread
        int start = i * (double) num / numWorkers;
        int end = (i + 1) * (double) num / numWorkers;
        //make sure all work gets finished
        if(i == numWorkers - 1)
            end = num;
        wi[i].points = &queue[start];
        wi[i].numPoints = end - start;
        pthread_create(&threads[i], NULL, workerFunc, &wi[i]);
    }
    for(int i = 0; i < numWorkers; i++)
        pthread_join(threads[i], NULL);
}

void* workerFunc(void* wiRaw)
{
    WorkInfo* wi = (WorkInfo*) wiRaw;
    MAKE_STACK_FP(pixFloat);
    MAKE_STACK_FP(x);
    MAKE_STACK_FP(yiter);
    MAKE_STACK_FP(pstrideLocal);
    fpcopy(&pstrideLocal, &pstride);
    for(int i = 0; i < wi->numPoints; i++)
        getPixelConvRate(wi->points[i].x, wi->points[i].y);
    return NULL;
}

void drawBuf()
{
    if(verbose)
        printf("Drawing buf with pstride = %Le\n", getValue(&pstride));
    savings = 0;
    for(int i = 0; i < winw * winh; i++)
        iters[i] = NOT_COMPUTED;
    Rect all = {0, 0, winw, winh};
    fillRect(all);
    if(colors)
    {
        for(int i = 0; i < winw * winh; i++)
            colors[i] = getColor(iters[i]);
    }
    if(verbose)
        printf("Saved %i of %i pixels (%f%%)\n", savings, winw * winh, 100.0 * savings / (winw * winh));
}

void recomputeMaxIter(int zoomExpo)
{
    const int normalIncrease = 400 * zoomExpo;
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
    int gpx = 8;                       //size, in pixels, of GIL iteration buffer (must be PoT)
    prec = 1;
    targetX = FPCtorValue(prec, 0);
    targetY = FPCtorValue(prec, 0);
    iters = (int*) malloc(gpx * gpx * sizeof(int));
    colors = NULL;
    pstride = FPCtorValue(prec, initViewport / gpx);
    winw = gpx;
    winh = gpx;
    int zoomExpo = 2;               //log_2 of zoom factor 
    FP fbestPX = FPCtor(1);
    FP fbestPY = FPCtor(1);
    maxiter = 300;
    int count = 0;
    Point* queue = malloc(gpx * gpx * sizeof(Point));
    for(int i = 0; i < gpx; i++)
    {
        for(int j = 0; j < gpx; j++)
        {
            queue[i + j * gpx].x = i;
            queue[i + j * gpx].y = j;
        }
    }
    while(true)
    {
        if(getApproxExpo(&pstride) <= minExpo)
        {
            if(verbose)
                printf("Stopping because pstride <= 2^%i\n", getApproxExpo(&pstride));
            break;
        }
        printf("Pixel stride = %Le, iter cap is %i\n", getValue(&pstride), maxiter);
        printf("Pstride as int: ");
        biPrint(&pstride.value);
        for(int i = 0; i < gpx * gpx; i++)
            iters[i] = NOT_COMPUTED;
        iteratePointQueue(queue, gpx * gpx);
        if(verbose)
        {
            puts("**********The buffer:**********");
            for(int i = 0; i < gpx * gpx; i++)
            {
                printf("%5i ", iters[i]);
                if(i % gpx == gpx - 1)
                    puts("");
            }
            puts("*******************************");
        }
        int bestPX = 0;
        int bestPY = 0;
        int bestIters = 0;
        u64 itersum = 0;
        //get point with maximum iteration count (that didn't converge)
        for(int i = 0; i < gpx; i++)
        {
            for(int j = 0; j < gpx; j++)
            {
                itersum += iters[i + j * gpx];
                if(iters[i + j * gpx] > bestIters)
                {
                    bestIters = iters[i + j * gpx];
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
        int val = iters[0];
        for(int i = 0; i < gpx * gpx; i++)
        {
            if(iters[i] != val)
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
                fpsub2(&targetX, &pstride);
        }
        else if(xmove > 0)
        {
            for(int i = 0; i < xmove; i++)
                fpadd2(&targetX, &pstride);
        }
        if(ymove < 0)
        {
            for(int i = ymove; i < 0; i++)
                fpsub2(&targetY, &pstride);
        }
        else if(ymove > 0)
        {
            for(int i = 0; i < ymove; i++)
                fpadd2(&targetY, &pstride);
        }
        //set screen position to the best pixel
        if(upgradePrec(&pstride))
        {
            INCR_PREC(pstride);
            INCR_PREC(targetX);
            INCR_PREC(targetY);
            prec++;
            if(verbose)
                printf("*** Increased precision to level %i ***\n", prec);
        }
        //zoom in according to the PoT zoom factor defined above
        printf("target x: ");
        biPrint(&targetX.value);
        printf("target y: ");
        biPrint(&targetY.value);
        fpshr(pstride, zoomExpo);
    }
    free(iters); 
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
    free(queue);
    FPDtor(&pstride);
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
    prec = 1;
    iters = (int*) malloc(imageWidth * imageHeight * sizeof(int));
    colors = (unsigned*) malloc(imageWidth * imageHeight * sizeof(unsigned));
    winw = imageWidth;
    winh = imageHeight;
    pstride = FPCtorValue(prec, 4.0 / imageWidth);
    printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
    maxiter = 5000;
    colortable = (Uint32*) malloc(sizeof(Uint32) * 360);
    initColorTable();
    filecount = 0;
    //resume file: filecount, last maxiter, prec
    while(getApproxExpo(&pstride) >= deepestExpo)
    {
        time_t start = time(NULL);
        //create coarse image with full iteration count
        drawBuf();
        writeImage();
        fpshrOne(pstride);
        recomputeMaxIter(1);
        int timeDiff = time(NULL) - start;
        if(upgradePrec(&pstride))
        {
            INCR_PREC(pstride);
            prec++;
            if(verbose)
                printf("*** Increasing precision to level %i (%i bits) ***\n", prec, 63 * prec);
        }
        printf("Image #%i took %i seconds. ", filecount - 1, timeDiff);
        if(verbose)
        {
            printf("iter cap: %i, ", maxiter);
            printf("precision: %i, ", prec);
            printf("px/sec/thread: %i\n", winw * winh / max(1, timeDiff) / numThreads);
        }
        else
            puts(".");
    }
    free(iters);
    free(colors);
    free(colortable);
}

