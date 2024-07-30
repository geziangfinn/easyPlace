#include "plot.h"

using namespace PLOTTING;

int PLOTTING::getX(float regionLLx, float x, float unitX)
{
    return (x - regionLLx) * unitX;
}

// the Y-axis must be mirrored
int PLOTTING::getY(float regionHeight, float regionLLy, float y, float unitY)
{
    return (regionHeight - (y - regionLLy)) * unitY; //?
    // return (chipRegionHeight - y) * unitY;//?
}

void PLOTTING::plotCurrentPlacement(string imageName, PlaceDB *db)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

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
        int x1 = getX(db->chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(db->chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, db->chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, db->chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
    }

    for (Module *curNode : db->dbNodes)
    {
        assert(curNode);
        int x1 = getX(db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        if (curNode->isMacro)
        {
            img.draw_rectangle(x1, y1, x2, y2, Orange, opacity);
        }
        else
        {
            img.draw_rectangle(x1, y1, x2, y2, Red, opacity);
        }
    }

    img.draw_text(50, 50, imageName.c_str(), Black, NULL, 1, 30);
    img.save_bmp(string(plotPath + imageName + string(".bmp")).c_str());
    cout << "INFO: BMP HAS BEEN SAVED: " << imageName + string(".bmp") << endl;
}

void PLOTTING::plotEPlace_2D(string imageName, EPlacer_2D *eplacer)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

    float chipRegionWidth = eplacer->db->chipRegion.ur.x - eplacer->db->chipRegion.ll.x;
    float chipRegionHeight = eplacer->db->chipRegion.ur.y - eplacer->db->chipRegion.ll.y;

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

    for (Module *curTerminal : eplacer->db->dbTerminals)
    {
        assert(curTerminal);
        // ignore pin's location
        if (curTerminal->isNI)
        {
            continue;
        }
        int x1 = getX(eplacer->db->chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(eplacer->db->chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
    }

    for (Module *curNode : eplacer->db->dbNodes)
    {
        assert(curNode);
        int x1 = getX(eplacer->db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(eplacer->db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        if (curNode->isMacro)
        {
            img.draw_rectangle(x1, y1, x2, y2, Orange, opacity);
        }
        else
        {
            img.draw_rectangle(x1, y1, x2, y2, Red, opacity);
        }
    }

    if ((gArg.CheckExist("debug") || eplacer->placementStage == FILLERONLY))
    {
        for (Module *curNode : eplacer->ePlaceFillers)
        {
            assert(curNode);
            int x1 = getX(eplacer->db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
            int x2 = getX(eplacer->db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
            int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
            int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
            img.draw_rectangle(x1, y1, x2, y2, Green, opacity);
        }
    }

    img.draw_text(50, 50, imageName.c_str(), Black, NULL, 1, 30);
    img.save_bmp(string(plotPath + imageName + string(".bmp")).c_str());
    cout << "INFO: BMP HAS BEEN SAVED: " << imageName + string(".bmp") << endl;
}
