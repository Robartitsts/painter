#pragma once
#include "Sketchpad.h"

using namespace cv;

/**
 * @brief constructor
 * @param width
 * @param height
 * @param ss
 * @param W
 * @param parent
 */
Sketchpad::Sketchpad(int width, int height, Shapes *ss, CytonRunner *Ava, Webcam *W, QWidget *parent) :
QMainWindow(parent),
ui(new Ui::Sketchpad)
{
	//settting class variables
	this->width = width;
	this->height = height;
	this->shapes = ss;
	this->Ava = Ava;
	this->Web = W;
	this->paintingNamePath = "unpathed";
	this->title = "unnamed";

	//setting up Qt's misc. toolbars & windows.
	ui->setupUi(this);
	this->setWindowTitle(("RHobart - " + title).c_str());
	setupQt();

	//Linking opencv to Qt.
	this->translator = new CVImageWidget(ui->widget);
	connect(translator, SIGNAL(emitRefresh(int, int)), this, SLOT(refresh(int, int)));
	this->cvWindow = new DrawWindow(width, height, "garbage_name", 1);

	//Drawing set-up logic
	ui->actionDraw_Line->setChecked(true); //defaults to PolyLine
	getColor(); //sets class's color
	cvWindow->grid.setTo(cv::Scalar(255, 255, 255)); //clear the grid
	shapes->drawAll(cvWindow); //redraw the window
	translator->showImage(cvWindow->grid); //actually redraw the window
	this->startNewCommand(); //prep for initial command

	//robot logic
	//ui->menuRobot->setDisabled(true);
	connected = false;
	ui->actionShutdown->setDisabled(true);
	ui->actionABB->setCheckable(true);
	ui->actionCyton->setCheckable(true);
	ui->actionConnect->setDisabled(true);
	ui->menuWorkspace->setDisabled(true);
	connect(ui->actionABB, SIGNAL(triggered()), this, SLOT(setABB()));
	connect(ui->actionCyton, SIGNAL(triggered()), this, SLOT(setCyton()));
	connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectRobot()));
	connect(Ava, SIGNAL(finishedSettingWorkspace()), this, SLOT(completeConnection()));
}

/**
* @brief sets up the Qt ui with all buttons, actions, etc.
*/
void Sketchpad::setupQt() {
	//Moving sketchpad to top right corner
	this->setFixedHeight(height + ui->toolBar_2->height() + ui->menubar->height() + 15);
	this->setFixedWidth(width + 20);
	QRect r = QApplication::desktop()->availableGeometry();
	this->move(r.right() - (width + 35), r.top());

	//building kMeans dialog
	kMeansFirstAccept = true;
	kMeansForm = new QDialog();
	kMeansUi.setupUi(kMeansForm);
	kMeansUi.ColorInput->setValidator(new QIntValidator(1, 64));
	kMeansUi.SizeInput->setValidator(new QIntValidator(1, 500));
	kMeansUi.ImageInput->setDisabled(true);
	connect(kMeansUi.browse, SIGNAL(clicked()), this, SLOT(browseClicked()));
	connect(kMeansUi.buttonBox, SIGNAL(accepted()), this, SLOT(kMeansAccepted()));

	//building canny dialog
	cannyFirstAccept = true;
	cannyForm = new QDialog();
	cannyUi.setupUi(cannyForm);
	cannyUi.ThresholdInput->setValidator(new QIntValidator(1, 100));
	cannyUi.LengthInput->setValidator(new QIntValidator(1, 100));
	cannyUi.ImageInput->setDisabled(true);
	connect(cannyUi.browse, SIGNAL(clicked()), this, SLOT(browseClicked()));
	connect(cannyUi.buttonBox, SIGNAL(accepted()), this, SLOT(cannyAccepted()));

	//building kMeans toolbar
	this->addToolBarBreak();
	kMeansToolbar = new QToolBar();
	this->addToolBar(kMeansToolbar);
	kMeansToolbar->hide();

	colorCountBox = new QSpinBox();
	colorCountBox->setKeyboardTracking(false);
	colorCountBox->setFixedWidth(60);
	colorCountBox->setMinimum(0);
	colorCountBox->setMaximum(64);
	colorCountBox->setSingleStep(1);
	kMeansToolbar->addWidget(new QLabel("# colors: "));
	kMeansToolbar->addWidget(colorCountBox);
	kMeansToolbar->addSeparator();
	connect(colorCountBox, SIGNAL(valueChanged(int)), this, SLOT(kMeansAdjusted()));

	minSizeBox = new QSpinBox();
	minSizeBox->setKeyboardTracking(false);
	minSizeBox->setFixedWidth(60);
	minSizeBox->setMinimum(0);
	minSizeBox->setMaximum(500);
	minSizeBox->setSingleStep(5);
	kMeansToolbar->addWidget(new QLabel("minimum region size: "));
	kMeansToolbar->addWidget(minSizeBox);
	connect(minSizeBox, SIGNAL(valueChanged(int)), this, SLOT(kMeansAdjusted()));

	//building canny toolbar
	cannyToolbar = new QToolBar();
	this->addToolBar(cannyToolbar);
	cannyToolbar->hide();

	lengthBox = new QSpinBox();
	lengthBox->setKeyboardTracking(false);
	lengthBox->setFixedWidth(60);
	lengthBox->setMinimum(0);
	lengthBox->setMaximum(100);
	lengthBox->setSingleStep(1);
	cannyToolbar->addWidget(new QLabel("Minimum line length: "));
	cannyToolbar->addWidget(lengthBox);
	cannyToolbar->addSeparator();
	connect(lengthBox, SIGNAL(valueChanged(int)), this, SLOT(cannyAdjusted()));

	thresholdBox = new QSpinBox();
	thresholdBox->setKeyboardTracking(false);
	thresholdBox->setFixedWidth(60);
	thresholdBox->setMinimum(0);
	thresholdBox->setMaximum(100);
	thresholdBox->setSingleStep(1);
	cannyToolbar->addWidget(new QLabel("Threshold (%): "));
	cannyToolbar->addWidget(thresholdBox);
	connect(thresholdBox, SIGNAL(valueChanged(int)), this, SLOT(cannyAdjusted()));

	//shape connections
	QActionGroup *actionGroup = new QActionGroup(this);
	actionGroup->addAction(ui->actionDraw_Square);
	actionGroup->addAction(ui->actionDraw_Circle);
	actionGroup->addAction(ui->actionDraw_Line);
	actionGroup->addAction(ui->actionDraw_Filled_Circle);
	actionGroup->addAction(ui->actionDraw_Filled_Rectangle);
	actionGroup->addAction(ui->actionDraw_Filled_Polygon);
	actionGroup->addAction(ui->actionActionFill);
	ui->actionDraw_Square->setCheckable(true);
	ui->actionDraw_Circle->setCheckable(true);
	ui->actionDraw_Line->setCheckable(true);
	ui->actionDraw_Filled_Rectangle->setCheckable(true);
	ui->actionDraw_Filled_Circle->setCheckable(true);
	ui->actionDraw_Filled_Polygon->setCheckable(true);
	ui->actionActionFill->setCheckable(true);
	connect(actionGroup, SIGNAL(triggered(QAction *)), this, SLOT(startNewCommand()));

	//shape modifier creation && connections
	color = new QComboBox();
	thickness = new QSpinBox();
	QStringList colors;
	colors << "black" << "dark grey" << "medium grey" << "light grey" << "white" << "yellow" << "orange" << "red" << "purple" << "blue" << "green";
	color->addItems(colors);
	thickness->setFixedWidth(60);
	thickness->setMinimum(1);
	thickness->setMaximum(25);
	thickness->setSingleStep(1);
	thickness->setValue(4);
	ui->toolBar_2->addWidget(color);
	ui->toolBar_2->addWidget(thickness);
	connect(color, SIGNAL(currentIndexChanged(int)), this, SLOT(redraw()));
	connect(thickness, SIGNAL(valueChanged(int)), this, SLOT(redraw()));

	//load/save connections
	connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newClicked()));
	connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openClicked()));
	connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveClicked()));
	connect(ui->actionSave_As, SIGNAL(triggered()), this, SLOT(saveAsClicked()));

	//image connections
	webcamSnapActive = false;
	connect(ui->actionLoad_Photo_Canny, SIGNAL(triggered()), this, SLOT(loadPhotoCannyClicked()));
	connect(ui->actionLoad_Photo_Kmeans, SIGNAL(triggered()), this, SLOT(loadPhotoKmeansClicked()));
	connect(ui->actionCalibrate, SIGNAL(triggered()), this, SLOT(calibrateWebcam()));
	connect(ui->actionView, SIGNAL(triggered()), this, SLOT(viewWebcam()));
	connect(ui->actionSwitch, SIGNAL(triggered()), this, SLOT(switchWebcam()));
	connect(ui->actionJudge, SIGNAL(triggered()), this, SLOT(judgeWebcam()));
	connect(ui->actionSnap_Webcam_Picture, SIGNAL(triggered()), this, SLOT(loadWebcamPicture()));

	//robot connections
	connect(ui->actionLoad, SIGNAL(triggered()), this, SLOT(loadWorkspaceClicked()));
	connect(ui->actionCreate, SIGNAL(triggered()), this, SLOT(createWorkspaceClicked()));
	connect(ui->actionShutdown, SIGNAL(triggered()), this, SLOT(shutDownClicked()));

	//misc
	connect(ui->actionClear, SIGNAL(triggered()), this, SLOT(reset()));
	connect(ui->actionSet_sketch_window_size, SIGNAL(triggered()), this, SLOT(changeSize()));
}

/**
 * @brief deconstructor
 */
Sketchpad::~Sketchpad()
{
	delete ui;
}

void Sketchpad::closeEvent(QCloseEvent *event) {
	bool unsaved = this->windowTitle().at(this->windowTitle().length() - 1) == "*";
	if (!unsaved) {
		this->close();
	}
	else {
		QMessageBox::StandardButton dialog;
		dialog = QMessageBox::warning(this, "close warning", "You have unsaved work.  Do you still want to close?",
			QMessageBox::Yes | QMessageBox::No);
		if (dialog == QMessageBox::Yes) { this->close(); }
		else { event->ignore(); }
	}
}

//redraws everything on the grid.
void Sketchpad::redraw() {
	getColor();
	startNewCommand();

	cvWindow->grid.setTo(cv::Scalar(255, 255, 255)); //clear the grid
	shapes->drawAll(cvWindow); //redraw the window
	translator->showImage(cvWindow->grid); //actually redraw the window
}

//begin a new command on the sketchpad.
void Sketchpad::startNewCommand() {
	prevX = -10;
	prevY = -10;

	if (ui->actionDraw_Line->isChecked() || ui->actionDraw_Filled_Polygon->isChecked()) {
		curPolyLine = new PolyLine();
		curPolyLine->setThickness(thickness->text().toInt());
		curPolyLine->setPenColor(rgbColor.at(0), rgbColor.at(1), rgbColor.at(2));
		this->currentShape = curPolyLine;
	}

	else if (ui->actionDraw_Circle->isChecked() || ui->actionDraw_Filled_Circle->isChecked()) {
		curCircle = new EllipseShape();
		if (ui->actionDraw_Filled_Circle->isChecked()) curCircle->setFill(1);
		curCircle->setPenColor(rgbColor.at(0), rgbColor.at(1), rgbColor.at(2));
		curCircle->setThickness(thickness->text().toInt());
		this->currentShape = curCircle;
	}
	else if (ui->actionDraw_Square->isChecked() || ui->actionDraw_Filled_Rectangle->isChecked()) {
		curRectangle = new RectangleShape();
		if (ui->actionDraw_Filled_Rectangle->isChecked()) curRectangle->setFill(1);
		curRectangle->setPenColor(rgbColor.at(0), rgbColor.at(1), rgbColor.at(2));
		curRectangle->setThickness(thickness->text().toInt());
		this->currentShape = curRectangle;
	}
	else if (ui->actionActionFill->isChecked()) {
		curPixelRegion = new PixelRegion();
		curPixelRegion->setPenColor(rgbColor.at(0), rgbColor.at(1), rgbColor.at(2));
		this->currentShape = curPixelRegion;
	}
}

/**
 * @brief Updates the sketchpad based on input.
 * @param x
 * @param y
 */
void Sketchpad::refresh(int x, int y) {
	//DRAW CLICK CIRCLE//
	cvWindow->setPenColor(200, 200, 200);
	cvWindow->setLineThickness(2);
	cvWindow->drawCircle(cvPoint(x, y), 6);
	bool reset = false;

	//DRAW POLYLINE//
	if (ui->actionDraw_Line->isChecked() || ui->actionDraw_Filled_Polygon->isChecked()) {
		if (abs(x - prevX) < 6 && abs(y - prevY) < 6) {	//finish old line
			reset = true;
			//actually, we were drawing a polyshape
			if (ui->actionDraw_Filled_Polygon->isChecked()) {
				PolyPoints *pp = new PolyPoints();
				pp->setPenColor(rgbColor.at(0), rgbColor.at(1), rgbColor.at(2));
				pp->setThickness(4);
				for (size_t i = 0; i < curPolyLine->points.size(); i++) {
					pp->addPoint(curPolyLine->points.at(i).x, curPolyLine->points.at(i).y);
				}
				pp->addPoint(x, y);
				this->currentShape = pp;
			}
		}
		else { //continue prev. lines
			curPolyLine->addPoint(x, y);
		}
	}

	//DRAW CIRCLE//
	else if (ui->actionDraw_Circle->isChecked() || ui->actionDraw_Filled_Circle->isChecked()) {
		if (prevX == -10) { //first point clicked
			prevX = x;
			prevY = y;
			translator->showImage(cvWindow->grid);
			return;
		}
		else { //second point clicked
			int radius = sqrt((x - prevX)*(x - prevX) + (y - prevY)*(y - prevY));
			curCircle->setData(prevX, prevY, 2 * radius);
			reset = true;
		}
	}

	//DRAW RECTANGLE
	else if (ui->actionDraw_Square->isChecked() || ui->actionDraw_Filled_Rectangle->isChecked()) {
		if (prevX == -10) { //first point clicked
			prevX = x;
			prevY = y;
			translator->showImage(cvWindow->grid);
			return;
		}
		else { //second point clicked
			curRectangle->setCorners(x, y, prevX, prevY);
			reset = true;
		}
	}

	//DRAW PIXELREGION (fill)
	else if (ui->actionActionFill->isChecked()) {
		reset = true;
		cvWindow->grid.setTo(cv::Scalar(255, 255, 255)); //clear the grid

		shapes->drawAll(cvWindow);
		flood(Point(x, y));
	}

	//DELETE CLICK CIRCLES
	if (reset) {
		shapes->addShape(currentShape);
		startNewCommand();
		cvWindow->grid.setTo(cv::Scalar(255, 255, 255)); //clear the grid
		shapes->drawAll(cvWindow); //redraw window
		this->setWindowTitle(("RHobart - " + title + "*").c_str());
		emit prodOtherWindows();
	}
	else {
		currentShape->draw(cvWindow);
		prevX = x; //iterate
		prevY = y; //iterate
	}

	translator->showImage(cvWindow->grid); //actually redraw the window

}

//fills in a region (essentially the bucket from MS Paint)
void Sketchpad::flood(Point p) {
	Point p2 = p;
	Mat processed;
	processed = Mat(cvWindow->grid.size().width, cvWindow->grid.size().height, CV_64F, cvScalar(0.));
	Vec3b floodColor = cvWindow->grid.at<Vec3b>(p);

	std::vector<Point> pointVec;
	pointVec.push_back(p);
	while (pointVec.size() > 0)
	{
		p = pointVec.back();
		pointVec.pop_back();
		Vec3b curPix = cvWindow->grid.at<Vec3b>(p);
		bool skip = false;

		if (floodColor[0] == curPix[0] && floodColor[1] == curPix[1] && floodColor[2] == curPix[2]) {
			curPixelRegion->addPoint(p.x, p.y);
		}
		else skip = true;

		if (!skip && p.y - 1 >= 0 && processed.at<double>(p.x, p.y - 1) != 1) { //recurse down
			pointVec.push_back(Point(p.x, p.y - 1));
			processed.at<double>(p.x, p.y - 1) = 1;
		}
		if (!skip && p.x - 1 >= 0 && processed.at<double>(p.x - 1, p.y) != 1) {	//recurse left
			pointVec.push_back(Point(p.x - 1, p.y));
			processed.at<double>(p.x - 1, p.y) = 1;
		}
		if (!skip && p.y + 1 < cvWindow->grid.size().height && processed.at<double>(p.x, p.y + 1) != 1) { //recurse up
			pointVec.push_back(Point(p.x, p.y + 1));
			processed.at<double>(p.x, p.y + 1) = 1;

		}
		if (!skip && p.x + 1 < cvWindow->grid.size().width && processed.at<double>(p.x + 1, p.y) != 1) { //recurse right
			pointVec.push_back(Point(p.x + 1, p.y));
			processed.at<double>(p.x + 1, p.y) = 1;
		}
	}
	curPixelRegion->addPoint(p2.x + 1, p2.y + 1);

}

/**
 * @brief transforms a text color into useable rgb code.
 */
void Sketchpad::getColor() {
	QString col = this->color->currentText();
	std::vector<int> toReplace;
	if (col == "black") {
		toReplace.push_back(0);
		toReplace.push_back(0);
		toReplace.push_back(0);
	}
	else if (col == "orange") {
		toReplace.push_back(30);
		toReplace.push_back(144);
		toReplace.push_back(255);
	}
	else if (col == "yellow") {
		toReplace.push_back(0);
		toReplace.push_back(255);
		toReplace.push_back(255);
	}
	else if (col == "green") {
		toReplace.push_back(34);
		toReplace.push_back(139);
		toReplace.push_back(34);
	}
	else if (col == "red") {
		toReplace.push_back(34);
		toReplace.push_back(34);
		toReplace.push_back(178);
	}
	else if (col == "blue") {
		toReplace.push_back(255);
		toReplace.push_back(144);
		toReplace.push_back(30);
	}
	else if (col == "purple") {
		toReplace.push_back(240);
		toReplace.push_back(32);
		toReplace.push_back(160);
	}
	else if (col == "dark grey"){
		toReplace.push_back(75);
		toReplace.push_back(75);
		toReplace.push_back(75);
	}
	else if (col == "medium grey"){
		toReplace.push_back(150);
		toReplace.push_back(150);
		toReplace.push_back(150);
	}
	else if (col == "light grey"){
		toReplace.push_back(225);
		toReplace.push_back(225);
		toReplace.push_back(225);
	}
	else if (col == "white"){
		toReplace.push_back(255);
		toReplace.push_back(255);
		toReplace.push_back(255);
	}
	this->rgbColor = toReplace;
}

void Sketchpad::loadPhotoKmeansClicked(){ kMeansForm->show(); kMeansFirstAccept = true; }

void Sketchpad::loadPhotoCannyClicked(){ cannyForm->show(); cannyFirstAccept = true; }

void Sketchpad::browseClicked() {
	QFileDialog directory;
	QStringList filters;
	filters << "Images (*.png *.xpm *.jpg)";
	directory.setNameFilters(filters);
	if (directory.exec()) {
		kMeansUi.ImageInput->setText(directory.selectedFiles().at(0));
		cannyUi.ImageInput->setText(directory.selectedFiles().at(0));
	}
}

void Sketchpad::kMeansAccepted() {
	this->setWindowTitle(("RHobart - " + title + "*").c_str());

	std::string location = kMeansUi.ImageInput->text().toStdString();
	if (location == "" && !webcamSnapActive) return; //make sure they didn't click "ok" with no image.
	int colorCount = kMeansUi.ColorInput->text().toInt();
	if (colorCount == 0) colorCount = 1; //don't let them have 0 colors.
	int minPixel = kMeansUi.SizeInput->text().toInt();
	if (minPixel == 0) minPixel = 1; //don't let them have empty regions.
	reset();

	if (!webcamSnapActive) { savedPicture = cv::imread(location); }
	emit loadPhotoKmeans(savedPicture, colorCount, minPixel);

	if (kMeansFirstAccept) {
		colorCountBox->setValue(kMeansUi.ColorInput->text().toInt());
		minSizeBox->setValue(kMeansUi.SizeInput->text().toInt());
		cannyToolbar->hide();
		kMeansToolbar->show();
		kMeansFirstAccept = false;
	}
}

void Sketchpad::cannyAccepted() {
	this->setWindowTitle(("RHobart - " + title + "*").c_str());

	std::string location = cannyUi.ImageInput->text().toStdString();
	if (location == "") return; //make sure they didn't click "ok" with no image.
	int minLength = cannyUi.LengthInput->text().toInt();
	if (minLength == 0) minLength = 1; //don't let them have dimensionless lines.
	int threshold = cannyUi.ThresholdInput->text().toInt();
	if (threshold == 0) threshold = 1; //don't let them have 0% threshold.
	reset();

	savedPicture = cv::imread(location);
	emit loadPhotoCanny(savedPicture, threshold, minLength);

	if (cannyFirstAccept) {
		lengthBox->setValue(cannyUi.LengthInput->text().toInt());
		thresholdBox->setValue(cannyUi.ThresholdInput->text().toInt());
		kMeansToolbar->hide();
		cannyToolbar->show();
		cannyFirstAccept = false;
	}
}

void Sketchpad::kMeansAdjusted() {
	if (kMeansFirstAccept) return; //don't start endless loop
	kMeansUi.ColorInput->setText(colorCountBox->text());
	kMeansUi.SizeInput->setText(minSizeBox->text());
	kMeansAccepted();
}

void Sketchpad::cannyAdjusted() {
	if (cannyFirstAccept) return; //don't start endless loop
	cannyUi.LengthInput->setText(lengthBox->text());
	cannyUi.ThresholdInput->setText(thresholdBox->text());
	cannyAccepted();
}

//clicking yields resizing.
void Sketchpad::calibrateWebcam() { printf("switch focus to the \"calibrate webcam\" window.\n"); Web->calibrateWebcam(); }

//show webcam.
void Sketchpad::viewWebcam() { Web->showWebcam(); }

//change webcam.
void Sketchpad::switchWebcam() { Web->switchWebcam(); }

//judge via webcam.
void Sketchpad::judgeWebcam() { Web->judge(this->cvWindow->grid); }

//load picture from webcam.
void Sketchpad::loadWebcamPicture() {
	savedPicture = Web->getWebcamSnap(cvWindow->grid);
	if (savedPicture.size().height == 1 && savedPicture.size().width == 1) { return; }
	webcamSnapActive = true;
	kMeansAccepted();
}

/**
 * @brief saveAs functionality.
 */
void Sketchpad::saveAsClicked() {
	QFileDialog saveDirectory;
	QStringList filters;
	saveDirectory.setAcceptMode(QFileDialog::AcceptSave);
	filters << "Text files (*.xml)";
	saveDirectory.setNameFilters(filters);
	if (saveDirectory.exec()) {
		paintingNamePath = saveDirectory.selectedFiles().at(0).toStdString();
		emit save(paintingNamePath);
		QString temp = saveDirectory.selectedFiles().at(0);
		temp.chop(4);
		title = temp.split("/").back().toStdString();
		this->setWindowTitle(("RHobart - " + title).c_str());
	}
}
/**
 * @brief save functionality.
 */
void Sketchpad::saveClicked() {
	if (paintingNamePath == "unpathed") saveAsClicked();
	else {
		emit save(paintingNamePath);
		this->setWindowTitle(("RHobart - " + title).c_str());
	}
}

/**
 * @brief open functionality.
 */
bool Sketchpad::openClicked() {
	bool unsaved = this->windowTitle().at(this->windowTitle().length() - 1) == "*";
	QMessageBox *m = new QMessageBox();
	m->setText("Are you sure?");
	m->setInformativeText("All unsaved changes will be lost");
	m->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	if (m->exec() != QMessageBox::Yes){ return false; }
	QFileDialog directory;
	QStringList filters;
	filters << "Text files (*.xml)";
	directory.setNameFilters(filters);
	if (directory.exec()) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file((directory.selectedFiles().at(0).toStdString()).c_str());
		if (!doc.child("robot").empty()){
			newClicked();
			emit load(directory.selectedFiles().at(0).toStdString());
			emit prodOtherWindows();
			this->paintingNamePath = directory.selectedFiles().at(0).toStdString();
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}
/**
 * @brief New project functionality.
 */
void Sketchpad::newClicked(){
	bool unsaved = this->windowTitle().at(this->windowTitle().length() - 1) == "*";
	QMessageBox *m = new QMessageBox();
	m->setText("Are you sure?");
	m->setInformativeText("All unsaved changes will be lost");
	m->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	if (m->exec() != QMessageBox::Yes){ return; }

	this->shapes->clear();
	this->paintingNamePath = "unpathed";
	this->setWindowTitle("RHobart - untitled");
	this->redraw();
}

//turn into clear button
void Sketchpad::reset() {
	QMessageBox *m = new QMessageBox();
	m->setText("Are you sure?");
	m->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	if (m->exec() == QMessageBox::Yes){
		this->shapes->clear();
		this->paintingNamePath = "unpathed";
		this->redraw();
		emit prodOtherWindows();
	}
}

//connects to the cyton and brings up a dialog to load a workspace.
void Sketchpad::loadWorkspaceClicked(){

	QFileDialog directory;
	QStringList filters;
	filters << "Text files (*.xml)";
	directory.setNameFilters(filters);

	if (directory.exec() == 0) return;
	emit loadRobot(directory.selectedFiles().at(0).toStdString());
}

//connects to the cyton and tells it to create a workspace.
void Sketchpad::createWorkspaceClicked(){
	Ava->createWorkspace();
}

//helper method to assist in creating workspaces
void Sketchpad::completeConnection() {
	Ava->startup();
	connected = true;
}

//tells the cyton to shut down.
void Sketchpad::shutDownClicked(){
	if (Ava->shutdown()){
		ui->actionShutdown->setDisabled(true);
		connected = false;
	}
}

//used to highlight a selected shape.
void Sketchpad::highlightShape(int index) {
	if (index == -1) { return; }
	cvWindow->grid.setTo(cv::Scalar(255, 255, 255)); //clear the grid
	shapes->drawAll(cvWindow); //redraw window

	DrawWindow Woverlay = DrawWindow(width, height, "overlay", 1);
	Woverlay.clearWindow(0, 0, 0);
	shapes->drawOne(&Woverlay, index + 1, 250, 0, 200); // r,g,b should not be 0,0,0
	cvWindow->overlay(&Woverlay, 1);

	translator->showImage(cvWindow->grid); //actually redraw the window
}

void Sketchpad::setABB(){
	ui->actionCyton->setChecked(false);
	if (ui->actionABB->isChecked()){
		ui->actionConnect->setEnabled(true);
		robotSelected = 1;
	}
	else{
		ui->actionConnect->setDisabled(true);
		robotSelected = -1;
	}

}

void Sketchpad::setCyton(){
	ui->actionABB->setChecked(false);
	if (ui->actionCyton->isChecked()){
		ui->actionConnect->setEnabled(true);
		robotSelected = 0;
	}
	else{
		ui->actionConnect->setDisabled(true);
		robotSelected = -1;
	}
}

void Sketchpad::connectRobot(){
	if (robotSelected == 1){
		printf("connect to ABB\n");
	}
	else if (robotSelected == 0){
		if (!Ava->connected){
			QMessageBox *m = new QMessageBox();
			m->setInformativeText("Please ensure that CytonViewer.exe is running before continuing.");
			m->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			if (m->exec() != QMessageBox::Ok) return;

			if (Ava->connect()){
				ui->menuWorkspace->setEnabled(true);
				ui->actionConnect->setText("Disconnect");
				ui->menuSelect_Robot->setDisabled(true);
			}
		}
		else{
			if (Ava->shutdown()){
				ui->menuWorkspace->setDisabled(true);
				ui->actionConnect->setText("Connect");
				ui->menuSelect_Robot->setEnabled(true);
			}
		}
	}
}

void Sketchpad::changeSize(){
	bool xOk, yOk;
	int x = QInputDialog::getInt(this, tr("QInputDialog::getInteger()"), tr("Width (pixels):"), 600, 10, 800, 10, &xOk);
	int y = QInputDialog::getInt(this, tr("QInputDialog::getInteger()"), tr("Height (pixels):"), 600, 10, 800, 10, &yOk);
	if (!xOk || !yOk){return;}
	this->width = x;
	this->height = y;

	this->setFixedHeight(height + ui->toolBar_2->height() + ui->menubar->height() + 15);
	this->setFixedWidth(width + 20);

	this->cvWindow = new DrawWindow(width, height, title, 1);


	
}