#include "plot.h"
using namespace cimg_library;
void Plotter::plotCurrentPlacement(string imageName, string imagePath)
{
    float chipRegionWidth = db->chipRegion.ur.x - db->chipRegion.ll.x;
    float chipRegionHeight = db->chipRegion.ur.y - db->chipRegion.ll.y;

    int minImgaeLength = 1000;

    int imageHeight;
    int imageWidth;

    float opacity = 0.7;
    int xMargin = 30, yMargin = 30;

    if (chipRegionWidth < chipRegionHeight)
    {
        imageHeight = 1.0 * chipRegionHeight / (chipRegionWidth / minImgaeLength);
        imageWidth = minImgaeLength;
    }
    else
    {
        imageWidth = 1.0 * chipRegionWidth / (chipRegionHeight / minImgaeLength);
        imageHeight = minImgaeLength;
    }

    CImg<unsigned char> img(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);

    float unitX = imageWidth / chipRegionWidth,
          unitY = imageHeight / chipRegionHeight;

    for (Module *curTerminal : db->dbTerminals)
    {
        assert(curTerminal);
        // ignore pin's location
        if (curTerminal->isNI)
        {
            continue;
        }
        int x1 = getX(curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(curTerminal->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
    }

    for (Module *curNode : db->dbNodes)
    {
        assert(curNode);
        int x1 = getX(curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(curNode->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Red, opacity);
    }

    img.draw_text(50, 50, imageName.c_str(), Black, NULL, 1, 30);
    img.save_bmp(string(imagePath + imageName + string(".bmp")).c_str());
    cout << "INFO: BMP HAS BEEN SAVED: " << imageName + string(".bmp") << endl;
}

int Plotter::getX(float x, float unitX)
{
    return (x - db->chipRegion.ll.x) * unitX;
}

// the Y-axis must be mirrored
int Plotter::getY(float y, float unitY)
{
    float chipRegionHeight = db->chipRegion.ur.y - db->chipRegion.ll.y;
    return (chipRegionHeight - (y - db->chipRegion.ll.y)) * unitY;//? 
    //return (chipRegionHeight - y) * unitY;//? 
}