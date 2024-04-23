#ifndef LEGALIZER_H
#define LEGALIZER_H
#include "global.h"
#include "objects.h"
#include "placedb.h"
#define ABACUS_TRIAL true
#define ABACUS_FINAL false
class Obstacle : public CRect
{
};

struct Segtree
{
    int cover;
    int length;
    int max_length;
};

class SAMacroLegalizationBin_2D
{
public:
    SAMacroLegalizationBin_2D()
    {
        init();
    }
    POS_2D center;
    POS_2D ll;
    POS_2D ur;
    float width;
    float height;
    float area;

    float cellArea;     // overlapped area between this bin and std cells
    float macroArea;    // overlapped area between this bin and macros
    float terminalArea; // overlapped area between this bin and terminals, should always be 0!!!!
    float baseArea;
    // todo: add virtual area?
    float nonMacroDensity; // = (cell area + terminal area + base area) / bin area

    void init()
    {
        center.SetZero();
        ll.SetZero();
        ur.SetZero();
        cellArea = 0;
        macroArea = 0;
        terminalArea = 0;
        baseArea = 0;
        area = 0;
        width = 0;
        height = 0;
    }
    float getWidth() { return width; }
    float getHeight() { return height; }
    float getArea()
    {
        area = width * height;
        return width * height;
    }
};

class AbacusCellCluster
{
public:
    // cluster of std cells, used in abacus
    AbacusCellCluster()
    {
        Init();
    }
    void Init()
    {
        cells.clear();
        index = -1;
        x = 0.0;
        e = 0.0;
        w = 0.0;
        q = 0.0;
    }
    vector<Module *> cells; // cells should be ordered by x coordinate(non decreasing)
    int index;              // its index in the vector clusters of class AbacusRow
    // x,e,w,q of a cluster, see the abacus paper
    float x;
    float e;
    float w;
    float q;
};

class AbacusRow : public SiteRow
{
public:
    AbacusRow()
    {
        clusters.clear();
        width = 0;
        // lastClusterIndex=-1;
    }
    vector<AbacusCellCluster> clusters;
    double width;
    void addCell(int, Module *);
    void addCluster(int, int);
    void collapse(int);
};

class AbacusLegalizer
{
    // strictly follow the abacus paper, read the paper for more details
public:
    AbacusLegalizer()
    {
        Init();
    }
    AbacusLegalizer(PlaceDB *_db)
    {
        Init();
        placeDB = _db;
    }
    void Init()
    {
        placeDB = NULL;
    }
    PlaceDB *placeDB;
    vector<AbacusRow> subrows;  // generated from dbSiteRows and Terminals, this is actually used during legalization
    vector<Obstacle> obstacles; // include macro and terminals, macros should be legalized first!
    vector<Module *> dbCells;   // all std cells, sorted by x coordinate(non decreasing order)
    void legalization();

private:
    // set these functions private because they are called in other member functions only
    // follow the abacus paper
    void initialization();
    void initializeCells();
    void initializeObstacles(); // obstacles include macros and terminals, macros should be legalized first!
    void initializeSubrows();
    double placeRow(Module *, int, bool);
};

class SAMacroLegalizer
{
    // todo: implement a sa-based macro legalizer according to ePlace-MS
    // ! a crucial assumption: no macro in terminals!!!! terminals in the MMS benchmark should all have 0 area
public:
    SAMacroLegalizer()
    {
        Init();
    }
    SAMacroLegalizer(PlaceDB *_db)
    {
        Init();
        placeDB = _db;
    }
    void Init()
    {
        placeDB = NULL;
        dbMacros.clear();
        totalMacroArea = 0;
        bins.clear();
        binDimension.SetZero();
        binStep.SetZero();
        targetDensity = 1.0;

        totalHPWL = 0;
        totalCellAreaCovered = 0;
        totalMacroOverlap = 0;
        overlapFree = false;

        miuD = 0;
        miuO = 0;
        jLimit = 0;
        kLimit = 0;
        SAtemperature = 0;
        SAtemperatureCoef = 0;
        r.SetZero();
        u.SetZero();
        sa_r_stp.SetZero();
        beta = 0;
    }
    PlaceDB *placeDB;
    vector<Module *> dbMacros; // all std cells, sorted by x coordinate(non decreasing order)

    int totalMacroArea;
    int totalCellArea;

    float targetDensity; // equals to eplacer target density if mGP was ran

    vector<vector<SAMacroLegalizationBin_2D *>> bins;
    VECTOR_2D_INT binDimension; // How many bins in X/Y direction
    VECTOR_2D binStep;          // length of a bin in X/Y direction

    // three things in the SA cost function!
    float totalHPWL;
    float totalCellAreaCovered;
    int totalMacroOverlap;

    bool overlapFree; // key stop condition of macro legalization
    //! SA params
    //! 0. cost weights
    float miuD; // see ePlace-MS paper equation(30)
    float miuO; // see ePlace-MS paper equation(30)
    //! 1. iteration count upper limits
    int jLimit;
    int kLimit;
    //! 2. params related to SA computation
    float SAtemperature;
    float SAtemperatureCoef;
    VECTOR_2D r;
    VECTOR_2D u;
    VECTOR_2D sa_r_stp; // name is same as in RePlAce
    float beta;         // see ePlace-MS paper

    void legalization();

    void setTargetDensity(float _targetdensity)
    {
        targetDensity = _targetdensity;
    }

private:
    // set these functions private because they are called in other member functions only
    // follow the abacus paper
    void initialization();

    void initializeMacros();
    void initializeBins();
    void initializeCost();
    void initializeSAparams();

    int getAreaCoveredByMacros();
    int getAreaCoveredByMacrosDebug();
    float getCellAreaCoveredByAllMacros();

    float getCellAreaCoveredByMacro(Module *);
    int getMacroOverlapArea(Module *);
    float getMacroCost(Module *, float &, float &, float &);

    void SAMacroLegalization();
    void SAperturb();
    bool acceptPerturb(float, float);
};

class RectangleAreaSolution
{
public:
    int rectangleArea(vector<vector<int>> &rectangles)
    {
        int n = rectangles.size();
        for (const auto &rect : rectangles)
        {
            // 下边界
            hbound.push_back(rect[1]);
            // 上边界
            hbound.push_back(rect[3]);
        }
        sort(hbound.begin(), hbound.end());
        hbound.erase(unique(hbound.begin(), hbound.end()), hbound.end());
        int m = hbound.size();
        // 线段树有 m-1 个叶子节点，对应着 m-1 个会被完整覆盖的线段，需要开辟 ~4m 大小的空间
        tree.resize(m * 4 + 1);
        init(1, 1, m - 1);

        vector<tuple<int, int, int>> sweep;
        for (int i = 0; i < n; ++i)
        {
            // 左边界
            sweep.emplace_back(rectangles[i][0], i, 1);
            // 右边界
            sweep.emplace_back(rectangles[i][2], i, -1);
        }
        sort(sweep.begin(), sweep.end());

        long long ans = 0;
        for (int i = 0; i < sweep.size(); ++i)
        {
            int j = i;
            while (j + 1 < sweep.size() && get<0>(sweep[i]) == get<0>(sweep[j + 1]))
            {
                ++j;
            }
            if (j + 1 == sweep.size())
            {
                break;
            }
            // 一次性地处理掉一批横坐标相同的左右边界
            for (int k = i; k <= j; ++k)
            {
                auto &&[_, idx, diff] = sweep[k];
                // 使用二分查找得到完整覆盖的线段的编号范围
                int left = lower_bound(hbound.begin(), hbound.end(), rectangles[idx][1]) - hbound.begin() + 1;
                int right = lower_bound(hbound.begin(), hbound.end(), rectangles[idx][3]) - hbound.begin();
                update(1, 1, m - 1, left, right, diff);
            }
            ans += static_cast<long long>(tree[1].length) * (get<0>(sweep[j + 1]) - get<0>(sweep[j]));
            i = j;
        }
        return ans;
    }

    void init(int idx, int l, int r)
    {
        tree[idx].cover = tree[idx].length = 0;
        if (l == r)
        {
            tree[idx].max_length = hbound[l] - hbound[l - 1];
            return;
        }
        int mid = (l + r) / 2;
        init(idx * 2, l, mid);
        init(idx * 2 + 1, mid + 1, r);
        tree[idx].max_length = tree[idx * 2].max_length + tree[idx * 2 + 1].max_length;
    }

    void update(int idx, int l, int r, int ul, int ur, int diff)
    {
        if (l > ur || r < ul)
        {
            return;
        }
        if (ul <= l && r <= ur)
        {
            tree[idx].cover += diff;
            pushup(idx, l, r);
            return;
        }
        int mid = (l + r) / 2;
        update(idx * 2, l, mid, ul, ur, diff);
        update(idx * 2 + 1, mid + 1, r, ul, ur, diff);
        pushup(idx, l, r);
    }

    void pushup(int idx, int l, int r)
    {
        if (tree[idx].cover > 0)
        {
            tree[idx].length = tree[idx].max_length;
        }
        else if (l == r)
        {
            tree[idx].length = 0;
        }
        else
        {
            tree[idx].length = tree[idx * 2].length + tree[idx * 2 + 1].length;
        }
    }

private:
    vector<Segtree> tree;
    vector<int> hbound;
    // 作者：力扣官方题解
    // 链接：https://leetcode.cn/problems/rectangle-area-ii/solutions/1825859/ju-xing-mian-ji-ii-by-leetcode-solution-ulqz/
    // 来源：力扣（LeetCode）
    // 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
};

#endif