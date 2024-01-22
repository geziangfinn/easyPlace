#include <stdio.h>
#include <stdlib.h>
#include "fft.h"
#define fft2d_alloc_error_check(p)                        \
  {                                                       \
    if((p) == NULL) {                                     \
      fprintf(stderr, "fft2d memory allocation error\n"); \
      exit(1);                                            \
    }                                                     \
  }

#ifdef USE_FFT2D_PTHREADS
#define USE_FFT2D_THREADS
#ifndef FFT2D_MAX_THREADS
#define FFT2D_MAX_THREADS 4
#endif
#ifndef FFT2D_THREADS_BEGIN_N
#define FFT2D_THREADS_BEGIN_N 65536
#endif
#include <pthread.h>
#define fft2d_thread_t pthread_t
#define fft2d_thread_create(thp, func, argp)                   \
  {                                                            \
    if(pthread_create(thp, NULL, func, (void *)(argp)) != 0) { \
      fprintf(stderr, "fft2d thread error\n");                 \
      exit(1);                                                 \
    }                                                          \
  }
#define fft2d_thread_wait(th)                  \
  {                                            \
    if(pthread_join(th, NULL) != 0) {          \
      fprintf(stderr, "fft2d thread error\n"); \
      exit(1);                                 \
    }                                          \
  }
#endif /* USE_FFT2D_PTHREADS */

#ifdef USE_FFT2D_WINTHREADS
#define USE_FFT2D_THREADS
#ifndef FFT2D_MAX_THREADS
#define FFT2D_MAX_THREADS 4
#endif
#ifndef FFT2D_THREADS_BEGIN_N
#define FFT2D_THREADS_BEGIN_N 131072
#endif
#include <windows.h>
#define fft2d_thread_t HANDLE
#define fft2d_thread_create(thp, func, argp)                       \
  {                                                                \
    DWORD thid;                                                    \
    *(thp) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(func), \
                          (LPVOID)(argp), 0, &thid);               \
    if(*(thp) == 0) {                                              \
      fprintf(stderr, "fft2d thread error\n");                     \
      exit(1);                                                     \
    }                                                              \
  }
#define fft2d_thread_wait(th)          \
  {                                    \
    WaitForSingleObject(th, INFINITE); \
    CloseHandle(th);                   \
  }
#endif /* USE_FFT2D_WINTHREADS */

namespace replace {


void ddcst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w) {
#ifdef USE_FFT2D_THREADS
  void ddxt2d0_subth(int n1, int n2, int ics, int isgn, float **a, int *ip,
                     float *w);
  void ddxt2d_subth(int n1, int n2, int ics, int isgn, float **a, float *t,
                    int *ip, float *w);
#endif 
  int n, nw, nc, itnull, nthread, nt, i;

  n = n1;
  if(n < n2) {
    n = n2;
  }
  nw = ip[0];
  if(n > (nw << 2)) {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if(n > nc) {
    nc = n;
    makect(nc, ip, w + nw);
  }
  itnull = 0;
  if(t == NULL) {
    itnull = 1;
    nthread = 1;
#ifdef USE_FFT2D_THREADS
    nthread = FFT2D_MAX_THREADS;
#endif 
    nt = 4 * nthread * n1;
    if(n2 == 2 * nthread) {
      nt >>= 1;
    }
    else if(n2 < 2 * nthread) {
      nt >>= 2;
    }
    t = (float *)malloc(sizeof(float) * nt);
    fft2d_alloc_error_check(t);
  }
#ifdef USE_FFT2D_THREADS
  if((float)n1 * n2 >= (float)FFT2D_THREADS_BEGIN_N) {
    ddxt2d0_subth(n1, n2, 1, isgn, a, ip, w);
    ddxt2d_subth(n1, n2, 0, isgn, a, t, ip, w);
  }
  else
#endif 
  {
    for(i = 0; i < n1; i++) {
      ddst(n2, isgn, a[i], ip, w);
    }
    ddxt2d_sub(n1, n2, 0, isgn, a, t, ip, w);
  }
  if(itnull != 0) {
    free(t);
  }
}

//1
void ddsct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w) {
  #ifdef USE_FFT2D_THREADS
  void ddxt2d0_subth(int n1, int n2, int ics, int isgn, float **a, int *ip,
                     float *w);
  void ddxt2d_subth(int n1, int n2, int ics, int isgn, float **a, float *t,
                    int *ip, float *w);
#endif /* USE_FFT2D_THREADS */
  int n, nw, nc, itnull, nthread, nt, i;

  n = n1;
  if(n < n2) {
    n = n2;
  }
  nw = ip[0];
  if(n > (nw << 2)) {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if(n > nc) {
    nc = n;
    makect(nc, ip, w + nw);
  }
  itnull = 0;
  if(t == NULL) {
    itnull = 1;
    nthread = 1;
#ifdef USE_FFT2D_THREADS
    nthread = FFT2D_MAX_THREADS;
#endif /* USE_FFT2D_THREADS */
    nt = 4 * nthread * n1;
    if(n2 == 2 * nthread) {
      nt >>= 1;
    }
    else if(n2 < 2 * nthread) {
      nt >>= 2;
    }
    t = (float *)malloc(sizeof(float) * nt);
    fft2d_alloc_error_check(t);
  }
#ifdef USE_FFT2D_THREADS
  if((float)n1 * n2 >= (float)FFT2D_THREADS_BEGIN_N) {
    ddxt2d0_subth(n1, n2, 0, isgn, a, ip, w);
    ddxt2d_subth(n1, n2, 1, isgn, a, t, ip, w);
  }
  else
#endif /* USE_FFT2D_THREADS */
  {
    for(i = 0; i < n1; i++) {
      ddct(n2, isgn, a[i], ip, w);
    }
    ddxt2d_sub(n1, n2, 1, isgn, a, t, ip, w);
  }
  if(itnull != 0) {
    free(t);
  }
}


//1
void ddct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w) {
#ifdef USE_FFT2D_THREADS
  void ddxt2d0_subth(int n1, int n2, int ics, int isgn, float **a, int *ip,
                     float *w);
  void ddxt2d_subth(int n1, int n2, int ics, int isgn, float **a, float *t,
                    int *ip, float *w);
#endif /* USE_FFT2D_THREADS */
  int n, nw, nc, itnull, nthread, nt, i;

  n = n1;
  if(n < n2) {
    n = n2;
  }
  nw = ip[0];
  if(n > (nw << 2)) {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if(n > nc) {
    nc = n;
    makect(nc, ip, w + nw);
  }
  itnull = 0;
  if(t == NULL) {
    itnull = 1;
    nthread = 1;
#ifdef USE_FFT2D_THREADS
    nthread = FFT2D_MAX_THREADS;
#endif /* USE_FFT2D_THREADS */
    nt = 4 * nthread * n1;
    if(n2 == 2 * nthread) {
      nt >>= 1;
    }
    else if(n2 < 2 * nthread) {
      nt >>= 2;
    }
    t = (float *)malloc(sizeof(float) * nt);
    fft2d_alloc_error_check(t);
  }
#ifdef USE_FFT2D_THREADS
  if((float)n1 * n2 >= (float)FFT2D_THREADS_BEGIN_N) {
    ddxt2d0_subth(n1, n2, 0, isgn, a, ip, w);
    ddxt2d_subth(n1, n2, 0, isgn, a, t, ip, w);
  }
  else
#endif /* USE_FFT2D_THREADS */
  {
    for(i = 0; i < n1; i++) {
      ddct(n2, isgn, a[i], ip, w);
    }
    ddxt2d_sub(n1, n2, 0, isgn, a, t, ip, w);
  }
  if(itnull != 0) {
    free(t);
  }
}

//1
void ddxt2d_sub(int n1, int n2, int ics, int isgn, float **a, float *t, int *ip,
                float *w) {
  int i, j;

  if(n2 > 2) {
    for(j = 0; j < n2; j += 4) {
      for(i = 0; i < n1; i++) {
        t[i] = a[i][j];
        t[n1 + i] = a[i][j + 1];
        t[2 * n1 + i] = a[i][j + 2];
        t[3 * n1 + i] = a[i][j + 3];
      }
      if(ics == 0) {
        ddct(n1, isgn, t, ip, w);
        ddct(n1, isgn, &t[n1], ip, w);
        ddct(n1, isgn, &t[2 * n1], ip, w);
        ddct(n1, isgn, &t[3 * n1], ip, w);
      }
      else {
        ddst(n1, isgn, t, ip, w);
        ddst(n1, isgn, &t[n1], ip, w);
        ddst(n1, isgn, &t[2 * n1], ip, w);
        ddst(n1, isgn, &t[3 * n1], ip, w);
      }
      for(i = 0; i < n1; i++) {
        a[i][j] = t[i];
        a[i][j + 1] = t[n1 + i];
        a[i][j + 2] = t[2 * n1 + i];
        a[i][j + 3] = t[3 * n1 + i];
      }
    }
  }
  else if(n2 == 2) {
    for(i = 0; i < n1; i++) {
      t[i] = a[i][0];
      t[n1 + i] = a[i][1];
    }
    if(ics == 0) {
      ddct(n1, isgn, t, ip, w);
      ddct(n1, isgn, &t[n1], ip, w);
    }
    else {
      ddst(n1, isgn, t, ip, w);
      ddst(n1, isgn, &t[n1], ip, w);
    }
    for(i = 0; i < n1; i++) {
      a[i][0] = t[i];
      a[i][1] = t[n1 + i];
    }
  }
}
}
