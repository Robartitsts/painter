#include "RunLogic.h"
#include <windows.h>

/**
 * @brief constructor
 * @param width
 * @param height
 * @param shapes
 */
RunLogic::RunLogic(int width, int height, Shapes *shapes, CytonRunner *Ava) {
	COMMAND_DELAY = 10; //ms
	this->width = width;
	this->height = height;
	this->shapes = shapes;
	this->Ava = Ava;
	this->simWin = new DrawWindow(width, height, "Simulation Window");
	clearClicked();
	simWin->hideWindow();
	mode = "Simulate Delayed Brush";
}

/**
 * @brief updates the stop index.
 */
void RunLogic::shapesChanged() { stopIndex = shapes->length(); }

/**
 * @brief stops and clears simulation.
 */
void RunLogic::clearClicked() {
	stepTaken = 2;
	running = false;
	simWin->clearWindow(255, 255, 255); //white
	simWin->show();
	currentShapeIndex = 0;
	stopIndex = shapes->length();
	for (int i = 0; i < shapes->length(); i++) {
		emit updateCommandList(i, "clear");
	}
}
/**
 * @brief pauses simulation.
 */
void RunLogic::pauseClicked() { running = false; }

/**
 * @brief steps forward.
 */
void RunLogic::forwardClicked() {

	this->simWin->showWindow();
	if (stepTaken == 0){
		currentShapeIndex++;
	}
	if (currentShapeIndex >= shapes->length()){
		stepTaken = 2;
		return;
	}

	running = false;
	runOnly(currentShapeIndex);
	stepTaken = 1;
}
/**
 * @brief steps backwards.
 */
void RunLogic::backwardClicked() {


	if (currentShapeIndex == 0 && stopIndex == 0){
		stepTaken = 2;
		return;
	}

	this->simWin->showWindow();
	running = false;
	emit updateCommandList(currentShapeIndex, "clear");
	simWin->clearWindow(255, 255, 255); //white
	if (currentShapeIndex <= 0) {
		stepTaken = 2;
		simWin->show();
		return;
	}
	int temp = currentShapeIndex;
	for (int i = 0; i < temp; i++) {
		runOnly(i);
		currentShapeIndex--;
	}
	stepTaken = 0;
}
/**
 * @brief runs simulation.
 */
void RunLogic::runClicked() {
	this->simWin->showWindow();
	if (running) return; //don't start multiple window threads (that's bad...)
	running = true;
	stepTaken = 2;
	if (mode == "Simulate Delayed Brush") {
		auto d1 = std::async(&RunLogic::simulateDelayedBrushThread, this, simWin);
	}
	else if (mode == "Simulate Real Brush") {
		printf("mode not yet implemented\n");
	}
	else if (mode == "Non-touch robot motion") {
		printf("mode not yet developed\n");
	}
	else if (mode == "Paint w/o feedback") {
		if (!Ava->connected) { printf("Please connect the robot before continuing\n"); return; }
		auto d1 = std::async(&RunLogic::paintWOFeedbackThread, this, simWin);
	}
	else if (mode == "Paint w/ feedback") {
		printf("mode not yet implemented\n");
	}
}

/**
 * @brief runs simulation from specified index.
 * @param index
 */
void RunLogic::runFrom(int index) {
	this->simWin->showWindow();
	currentShapeIndex = index;
	stopIndex = shapes->length();
	runClicked();
	stepTaken = 2;
}
/**
 * @brief runs only the specified command.
 * @param index
 */
void RunLogic::runOnly(int index) {
	this->simWin->showWindow();
	shapes->at(index)->draw(simWin);
	currentShapeIndex = index + 1;
	if (currentShapeIndex == stopIndex) currentShapeIndex--;

	emit(updateCommandList(index, "finished"));
	simWin->show();
}

//clear away everything and reset variables.
void RunLogic::reset(){
	stepTaken = 2;
	shapes->clear();
	clearClicked();
	simWin = new DrawWindow(width, height, "Simulation Window");
	//simWin->hideWindow();
}

void RunLogic::updateMode(QString mode, int delay) {
	this->mode = mode;
	this->COMMAND_DELAY = delay;
}

//thread to handle actual drawing.
void RunLogic::simulateDelayedBrushThread(DrawWindow *W){
	printf("starting delayed brush thread\n");
	while (running && currentShapeIndex < stopIndex) {
		//Updating index
		Shape *s = this->shapes->at(currentShapeIndex);
		//handling breakpoint
		if (s->isBreakPoint) {
			running = false;
			return;
		}
		if (s->fill) { //painting filled region
			emit updateCommandList(currentShapeIndex, "running");
			RegionToPaths RTP = RegionToPaths(width, height, 30);
			PixelRegion *p = s->toPixelRegion();
			std::vector<cv::Point> pts = p->getPoints();
			for (int j = 0; j < W->grid.size().height; j++) {
				for (int k = 0; k < W->grid.size().width; k++) {
					RTP.addOverpaintablePixel(k, j);
				}
			}

			for (size_t i = 0; i < pts.size(); i++){ RTP.addDesiredPixel(pts.at(i).x, pts.at(i).y); }

			Brush *curBrush = Ava->curBrush;
			curBrush->setColor(s->getPenColor());
			RTP.defineBrush(curBrush);
			RTP.definePaths();
			std::vector<std::vector<cv::Point>> pathVec = RTP.getBrushStrokes();
			for (size_t i = 0; i < pathVec.size(); i++){ //running through vector of strokes
				int prevX = pathVec.at(i).at(0).x;
				int prevY = pathVec.at(i).at(0).y;
				for (size_t j = 1; j < pathVec.at(i).size(); j++) { //running through points in one stroke
					if (!running) { return; }
					curBrush->drawLine(W, prevX, prevY, pathVec.at(i).at(j).x, pathVec.at(i).at(j).y);
					prevX = pathVec.at(i).at(j).x;
					prevY = pathVec.at(i).at(j).y;

					W->show();
					Sleep(COMMAND_DELAY);
				}
			}
		}
		else { //painting polyline object
			emit updateCommandList(currentShapeIndex, "running");
			std::vector<cv::Point> pts = s->toPolyline()->points;

			int prevX = pts.at(0).x;
			int prevY = pts.at(0).y;
			for (size_t i = 1; i < pts.size(); i++) { //running through points in one stroke
				if (!running) { return; }

				Brush *curBrush = Ava->curBrush;
				curBrush->setColor(s->getPenColor());
				curBrush->drawLine(W, prevX, prevY, pts.at(i).x, pts.at(i).y);

				prevX = pts.at(i).x;
				prevY = pts.at(i).y;

				W->show();
				Sleep(COMMAND_DELAY);
			}
		}
		emit updateCommandList(currentShapeIndex, "finished");

		currentShapeIndex++;
	}
	if (currentShapeIndex == stopIndex) currentShapeIndex--;
	running = false;
}

//thread to handle actual drawing.
void RunLogic::paintWOFeedbackThread(DrawWindow *W){
	printf("starting painting w/o feedback 'thread'\n");
	while (running && currentShapeIndex < stopIndex) {
		//Updating index
		Shape *s = this->shapes->at(currentShapeIndex);
		//handling breakpoint
		if (s->isBreakPoint) {
			running = false;
			return;
		}

		if (s->fill) { //painting filled region
			emit updateCommandList(currentShapeIndex, "running");
			Brush *curBrush;
			RegionToPaths RTP = RegionToPaths(width, height, 30);
			PixelRegion *p = s->toPixelRegion();
			std::vector<cv::Point> pts = p->getPoints();

			if (Ava->paint.size() >= 2){
				cv::Scalar temp = s->getPenColor();
				int r = temp[0];
				int g = temp[1];
				int b = temp[2];
				Ava->decidePaint(r, g, b);
			}

			for (int j = 0; j < W->grid.size().height; j++) {
				for (int k = 0; k < W->grid.size().width; k++) {
					RTP.addOverpaintablePixel(k, j);
				}
			}
			for (size_t i = 0; i < pts.size(); i++){ RTP.addDesiredPixel(pts.at(i).x, pts.at(i).y); }

			curBrush = Ava->curBrush;
			RTP.defineBrush(Ava->curBrush);

			RTP.definePaths();
			std::vector<std::vector<cv::Point>> pathVec = RTP.getBrushStrokes();
			for (size_t i = 0; i < pathVec.size(); i++){ //running through vector of strokes
				int prevX = pathVec.at(i).at(0).x;
				int prevY = pathVec.at(i).at(0).y;
				for (size_t j = 1; j < pathVec.at(i).size(); j++) { //running through points in one stroke
					if (!running) { return; }
					printf("--------------------------------------------------------------------------------------------------------------------\n");
					Ava->stroke(cv::Point(prevX, prevY), pathVec.at(i).at(j));
					curBrush->drawLine(W, prevX, prevY, pathVec.at(i).at(j).x, pathVec.at(i).at(j).y);
					prevX = pathVec.at(i).at(j).x;
					prevY = pathVec.at(i).at(j).y;

					W->show();
				}
				Ava->strokeInProgress = false;
			}
		}
		else { //painting polyline object
			emit updateCommandList(currentShapeIndex, "running");
			std::vector<cv::Point> pts = s->toPolyline()->points;

			if (Ava->connected && Ava->paint.size() >= 2){
				cv::Scalar temp = s->getPenColor();
				int r = temp[0];
				int g = temp[1];
				int b = temp[2];
				Ava->decidePaint(r, g, b);
			}

			int prevX = pts.at(0).x;
			int prevY = pts.at(0).y;
			for (size_t i = 1; i < pts.size(); i++) { //running through points in one stroke
				if (!running) { return; }

				if (Ava->connected) { //connected, polyline
					Ava->stroke(cv::Point(prevX, prevY), pts.at(i));
					Ava->curBrush->drawLine(W, prevX, prevY, pts.at(i).x, pts.at(i).y);

				}
				else { //not connected, polyline
					Brush curBrush = Brush(15, 10, "ellipse");
					curBrush.setColor(s->getPenColor());
					curBrush.drawLine(W, prevX, prevY, pts.at(i).x, pts.at(i).y);
				}
				prevX = pts.at(i).x;
				prevY = pts.at(i).y;

				W->show();
			}
			Ava->strokeInProgress = false;
		}
		emit updateCommandList(currentShapeIndex, "finished");

		currentShapeIndex++;
	}
	if (currentShapeIndex == stopIndex) currentShapeIndex--;
	running = false;
	Ava->changePaint(0);
	Ava->goToJointHome(1);
}