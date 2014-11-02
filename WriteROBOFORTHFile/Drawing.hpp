#ifndef DRAWING_H_INCLUDED
#define DRAWING_H_INCLUDED

#include <iostream>
#include <fstream>
#include <vector>
#include <opencv/cv.h>
#include "Brush.hpp"

using namespace std;
using namespace cv;

void writeROBOFORTHFromContours(ofstream &myfile, vector<vector<Point> > &contours, double z, double scale = 1);
void stopBrush(ofstream &myfile, Brush &currentBrush);
void startBrush(ofstream &myfile, Brush &nextBrush);
#endif
