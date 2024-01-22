#ifndef __REPLACE_FFT__
#define __REPLACE_FFT__

#include <vector>
#include <algorithm>

namespace replace {

class FFT_2D {
  public:
    FFT_2D();
    FFT_2D(int binCntX, int binCntY, float binSizeX, float binSizeY);
    ~FFT_2D();

    // input func
    void updateDensity(int x, int y, float density);

    // do FFT
    void doFFT();

    // returning func
    std::pair<float, float> getElectroForce(int x, int y) const;
    float getElectroPhi(int x, int y) const;

  private:
    // 2D array; width: binCntX_, height: binCntY_;
    // No hope to use Vector at this moment...
    float** binDensity_;
    float** electroPhi_;
    float** electroForceX_;
    float** electroForceY_;

    // cos/sin table (prev: w_2d)
    // length:  max(binCntX, binCntY) * 3 / 2
    std::vector<float> csTable_;

    // wx. length:  binCntX_
    std::vector<float> wx_;
    std::vector<float> wxSquare_;

    // wy. length:  binCntY_
    std::vector<float> wy_;
    std::vector<float> wySquare_;

    // work area for bit reversal (prev: ip)
    // length: round(sqrt( max(binCntX_, binCntY_) )) + 2
    std::vector<int> workArea_;

    int binCntX_;
    int binCntY_;
    float binSizeX_;
    float binSizeY_;

    void init();
};

void makewt(int nw, int *ip, float *w);
void cftfsub(int n, float *a, int *ip, int nw, float *w);
void cftbsub(int n, float *a, int *ip, int nw, float *w);
void makect(int nc, int *ip, float *c);
void rftfsub(int n, float *a, int nc, float *c);
void rftbsub(int n, float *a, int nc, float *c);
void dctsub(int n, float *a, int nc, float *c);
void dstsub(int n, float *a, int nc, float *c);
void makeipt(int nw, int *ip);
void bitrv2(int n, int *ip, float *a);
void bitrv216(float * a);
void bitrv208(float * a);
void cftf1st(int n, float *a, float *w);
void cftrec4(int n, float *a, int nw, float *w);
void cftleaf(int n, int isplt, float *a, int nw, float *w);
void cftfx41(int n, float *a, int nw, float *w);
void cftf161(float * a, float * w);
void cftf081(float * a, float * w);
void cftf040(float * a);
void cftx020(float * a);
void bitrv2conj(int n, int *ip, float *a);
void bitrv216neg(float * a);
void bitrv208neg(float * a);
void cftb1st(int n, float *a, float *w);
void cftb040(float * a);
int cfttree(int n, int j, int k, float *a, int nw, float *w);
void cftmdl1(int n, float *a, float *w);
void cftmdl2(int n, float *a, float *w);
void cftf162(float * a, float * w);
void cftf082(float * a, float * w);
void cdft(int n, int isgn, float *a, int *ip, float *w);
void cdft2d_sub(int n1, int n2, int isgn, float **a, float *t, int *ip,
	float *w);
void rdft(int n, int isgn, float *a, int *ip, float *w);
void rdft2d_sub(int n1, int isgn, float **a);
void ddxt2d_sub(int n1, int n2, int ics, int isgn, float **a, float *t, int *ip,
	float *w);

/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, float *a, int *ip, float *w);
void ddct(int n, int isgn, float *a, int *ip, float *w);
void ddst(int n, int isgn, float *a, int *ip, float *w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, float **, float *, int *, float *);
void rdft2d(int, int, int, float **, float *, int *, float *);
void ddct2d(int, int, int, float **, float *, int *, float *);
void ddst2d(int, int, int, float **, float *, int *, float *);
void ddsct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
void ddcst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);

/// 3D FFT ////////////////////////////////////////////////////////////////
void cdft3d(int, int, int, int, float ***, float *, int *, float *);
void rdft3d(int, int, int, int, float ***, float *, int *, float *);
void ddct3d(int, int, int, int, float ***, float *, int *, float *);
void ddst3d(int, int, int, int, float ***, float *, int *, float *);
void ddscct3d(int, int, int, int isgn, float ***, float *, int *, float *);
void ddcsct3d(int, int, int, int isgn, float ***, float *, int *, float *);
void ddccst3d(int, int, int, int isgn, float ***, float *, int *, float *);

}

#endif
