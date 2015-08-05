#include "webcam.cpp"
#include "drawwindow.cpp"
#include "shapes.cpp"
#include "regiontopaths.cpp"

// use webcam to determine where best to paint given a paint color

int g_brush_thresh = 30;
int g_brush_max_thresh = 100;
int g_error_thresh = 30;
int g_error_max_thresh = 100;

double colorCloseness(cv::Vec3b c1, cv::Vec3b c2) {
  return (double)(abs(c1[0]-c2[0]) + abs(c1[1]-c2[1]) + abs(c1[2]-c2[2]))/(3. * 256.);
}

/** @function thresh_callback */
void thresh_callback(int, void* ) { }

int main(void)
{
  // mostly to just have something to cycle waitKey (w/o a window, waitKey doesn't work?)
  DrawWindow INTRO = DrawWindow(400,80,"Webcam Tool Instructions"); 
  cv::createTrackbar( "Paint Brush Threshold:", "Webcam Tool Instructions", &g_brush_thresh, g_brush_max_thresh, thresh_callback );
  cv::createTrackbar( "Painting Error Threshold:", "Webcam Tool Instructions", &g_error_thresh, g_error_max_thresh, NULL );

  INTRO.drawText(10,10,"s=snap a webcam frame for desired painting");
  INTRO.drawText(10,20,"m=start judging");
  INTRO.drawText(10,30,"c=calibrate webcam(1-4,arrows,s=save,esc=done)");
  INTRO.show(); 

  int width=460;
  int height=300;
  int border=20;
  cv::Mat frame;
  cv::Mat bwframe;

  Webcam W;
  //W.setFlip(0);
  W.setMapSize(width,height);

  Brush brush = Brush(5,5,"ellipse");
  //brush.setDrawMode("paint");
  brush.setColor(0,0,0);

  DrawWindow DW = DrawWindow(width,height,"Desired Painting"); 
  DW.moveWindow(20,150);
  W.getMappedFrame(&frame);
  DW.setMat(frame.clone()); 
  DW.show();

  DrawWindow EW = DrawWindow(width,height,"Error Window");
  EW.moveWindow(2*(width+5)+20,150);

  DrawWindow BW = DrawWindow(width,height,"Brush matches desired painting",0);
  BW.moveWindow(0*(width+5)+20,150 + (height+15));

  DrawWindow PW = DrawWindow(width+2*border,height+2*border,"Where to Paint",0);
  PW.moveWindow(1*(width+5)+20,150 + (height+15));

  DrawWindow BSW = DrawWindow(width+2*border,height+2*border,"Brush Strokes",0);
  BSW.moveWindow(2*(width+5)+20,150 + (height+15));

  DrawWindow MW = DrawWindow(width,height,"Mapped Webcam",0);
  MW.moveWindow(1*(width+5)+20,150 + 0*(height+15));

  while (1) {   
    int k = cv::waitKey(33);
    //if (k>0) { printf("key = %d\n",k); }
    if (k==27) { return(0); }  // Esc key to stop
    if (k == int('c')) { W.calibrateWebcam(); }
    if (k == int('e')) { W.calibrateWebcam(1); }
    
    if (k == int('s')) {  // make the desired painting be a snap of the webcam
      W.getMappedFrame(&frame);
      DW.setMat(frame.clone()); 
      DW.show();
      BSW.setCanvasColor(230,230,230);
      BSW.clearWindow();
      BSW.show();
    }

    if (k == int('p')) { // paint
      // get webcam
      W.getMappedFrame(&frame);
      MW.setMat(frame);
      MW.show();

      // find which pixels (region) could be painted that are most off - insert in region
      RegionToPaths RTP = RegionToPaths(width,height,border);
      RTP.clear();

      EW.setCanvasColor(230,230,230);
      EW.clearWindow();
      EW.setPenColor(200,0,0);
      BW.setCanvasColor(230,230,230);
      BW.clearWindow();
      BW.setPenColor(0,0,200);

      cv::Scalar bc = brush.getColor();
      cv::Vec3b brush_color(bc[0],bc[1],bc[2]);

      double brush_threshold = g_brush_thresh/100.;
      double error_threshold = g_error_thresh/100.;
      printf("%f brush, %f error\n",brush_threshold,error_threshold);

      for (int i=0; i<width; i++) { //loop rows
	for (int j=0; j<height; j++) { //loop columns
	  cv::Vec3b desired_color = DW.getColor(i,j); //desired" pixel color
	  cv::Vec3b webcam_color = frame.at<cv::Vec3b>(cv::Point(i,j)); 

	  double webcam_desired_diff = colorCloseness(desired_color,webcam_color);
	  double brush_desired_diff = colorCloseness(brush_color,desired_color);

	  if (brush_desired_diff<brush_threshold) {
	    BW.drawPixel(i,j); 
	    RTP.addOverpaintablePixel(i,j);
	  }
	  if (webcam_desired_diff>error_threshold) { EW.drawPixel(i,j); }

	  // if it's a pixel in desired image that's the brush color & the webcam image isn't close enought, add to region
	  if (brush_desired_diff<brush_threshold && webcam_desired_diff>error_threshold) { 
	    RTP.addDesiredPixel(i,j);
	  }
	}
      }

      RTP.putGridOnWindow(&PW,border,border);
      PW.show();
      BW.show();
      EW.show();

      // find brush strokes to cover that region
      RTP.defineBrush(&brush);
      RTP.definePaths();

      // paint
      RTP.defineBrushStrokes();
      RTP.drawBrushStrokes(&BSW);
     
      BSW.show();
    }
  }
}