#include "CytonRunner.h"
#include <qmessagebox>
#include <qpushbutton.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qplaintextedit.h>

#define FRAME_EE_SET 1
#define JOINT_CONTROL_EE_SET 0xFFFFFFFF
#define POINT_EE_SET 0
#define M_PI 3.141592653

using namespace Ec;
using namespace cv;


CytonRunner::CytonRunner()
{
	currentX = 0;
	currentY = 0;
	raiseHeight = 0.1;
}


CytonRunner::~CytonRunner()
{
}

bool CytonRunner::connect(){
	if (init()){
		printf("established connection to Cyton\n");
		return true;
	}
	else{
		printf("failed to connect to Cyton\n");
	}
	return false;
}


void CytonRunner::loadWorkspace(std::string fileLocation){
	//clear all variables.
	startJointPosition.clear();
	canvasCorners.clear();
	paint.clear();
	dx = dy = dz = 0;
	brushType = "";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file((fileLocation).c_str());

	pugi::xml_node start = doc.child("workspace").child("starting");
	pugi::xml_node canvas = doc.child("workspace").child("canvas");
	pugi::xml_node paintPickup = doc.child("workspace").child("paintPickup");
	pugi::xml_node brush = doc.child("workspace").child("brush");

	for (pugi::xml_node temp = start.first_child(); temp; temp = temp.next_sibling()){
		double rot = temp.attribute("s").as_double();
		startJointPosition.push_back(rot);
	}

	for (pugi::xml_node temp = canvas.first_child(); temp; temp = temp.next_sibling()){
		double x = temp.attribute("x").as_double();
		double y = temp.attribute("y").as_double();
		double z = temp.attribute("z").as_double();
		std::vector<double> corner;
		corner.push_back(x);
		corner.push_back(y);
		corner.push_back(z);
		canvasCorners.push_back(corner);
	}
	for (pugi::xml_node temp = paintPickup.first_child(); temp; temp = temp.next_sibling()){
		paint;
		double x = temp.attribute("x").as_double();
		double y = temp.attribute("y").as_double();
		double id = temp.attribute("id").as_int();

		std::pair<double, double> pos(x, y);
		std::pair<double, std::pair<double, double>> paintPickup(id, pos);
		paint.push_back(paintPickup);
	}

	dx = brush.attribute("dx").as_double();
	dy = brush.attribute("dy").as_double();
	dz = brush.attribute("dz").as_double();
	printf("dz is: %d", dz);
	brushType = brush.next_sibling().attribute("type").as_string();

	//figure out roll, pitch, and yaw.  Not used as of yet.
	this->phi = 0;
	this->theta = 0;
	this->psi = 0;


}
void CytonRunner::createWorkspace(){
	

}
void CytonRunner::startup(){
	goToJointHome(1);
}
bool CytonRunner::shutdown(){
	QMessageBox message;
	message.setInformativeText("Please clear away any and all paints before continuing");
	QPushButton *okButton = message.addButton(QMessageBox::Ok);
	message.addButton(QMessageBox::Cancel);
	message.exec();
	if (message.clickedButton() == okButton){
		if (goToJointHome(0)){
			startJointPosition.clear();
			canvasCorners.clear();
			paint.clear();
			dx = dy = dz = 0;
			currentX = currentY = 0;
			brushType = "";
			Ec::shutdown();
			printf("shut down\n");
			return true;
		}
	}
	return false;
}
void CytonRunner::goToPos(double x, double y, double z){
	EcCoordinateSystemTransformation pose;

	std::vector<double> vec = convert(x, y, z);
	pose.setTranslationX(vec.at(0));
	pose.setTranslationY(vec.at(1));
	pose.setTranslationZ(vec.at(2));
	EcOrientation orientation;

	//roll about x-axis, pitch about y-axis,Yaw about z-axis
	orientation.setFrom123Euler(0, 0, 0);

	pose.setOrientation(orientation);

	setEndEffectorSet(FRAME_EE_SET); // point end effector set index
	EcEndEffectorPlacement desiredPlacement(pose);
	EcManipulatorEndEffectorPlacement actualEEPlacement;
	EcCoordinateSystemTransformation offset, zero, actualCoord;

	//until further notice, canvas is on ground.
	zero.setTranslation(EcVector(0, 0, 0));

	//set the desired position
	setDesiredPlacement(desiredPlacement, 0, 0);

	// if it hasnt been achieved after 5 sec, return false
	EcU32 timeout = 5000;
	EcU32 interval = 10;
	EcU32 count = 0;
	EcBoolean achieved = EcFalse;
	while (!achieved && !(count >= timeout / interval))
	{
		EcSLEEPMS(interval);
		count++;

		getActualPlacement(actualEEPlacement);
		actualCoord = actualEEPlacement.offsetTransformations()[0].coordSysXForm();

		//get the transformation between the actual and desired 
		offset = (actualCoord.inverse()) * pose;

		if (offset.approxEq(zero, .00001))
		{
			achieved = EcTrue;
		}
	}
	currentX = x;
	currentY = y;
}
void CytonRunner::raiseBrush(){
	if (!isUp){
		printf("going up!\n");
		goToPos(currentX, currentY, raiseHeight);
	}
	isUp = true;
}
void CytonRunner::lowerBrush(){
	if (isUp){
		printf("going down!\n");
		goToPos(currentX + 0.01, currentY + 0.01, 0.05);
		//almost there, let's not push the tip straight down.
		goToPos(currentX - 0.01, currentY - 0.01, 0);
	}
	isUp = false;
}
void CytonRunner::getPaint(int paint_can_id){
	raiseBrush();
	double x;
	double y;
	for (size_t i = 0; i < this->paint.size(); i++){
		if (this->paint.at(i).first == paint_can_id){
			x = this->paint.at(i).second.first;
			y = this->paint.at(i).second.second;
			break;
		}
	}
	goToPos(x, y, raiseHeight);
}
void CytonRunner::drawPoint(std::pair<double, double> pt){
	raiseBrush();
	goToPos(pt.first, pt.second, raiseHeight);
	lowerBrush();
	raiseBrush();
}
void CytonRunner::stroke(std::pair<double, double> pt1, std::pair<double, double> pt2){
	raiseBrush();
	goToPos(pt1.first, pt1.second, raiseHeight);
	lowerBrush();
	goToPos(pt2.first, pt2.second, 0);

}

void CytonRunner::stroke(std::vector<std::pair<double, double>> pts){
	raiseBrush();
	goToPos(pts.at(0).first, pts.at(0).second, raiseHeight);
	lowerBrush();
	for (int i = 0; i < pts.size(); i++){
		goToPos(pts.at(i).first, pts.at(i).second, 0);
	}
	raiseBrush();
	
}

bool CytonRunner::goToJointHome(int type){
	//type 0 sets all joints to 0;
	EcRealVector jointPosition(7);
	if (type == 1){
		jointPosition = startJointPosition;
		isUp = true;
	}
	const EcReal angletolerance = .000001;

	EcBoolean retVal = EcTrue;
	setEndEffectorSet(JOINT_CONTROL_EE_SET);
	EcSLEEPMS(500);

	//vector of EcReals that holds the set of joint angles
	EcRealVector currentJoints;
	retVal &= getJointValues(currentJoints);

	size_t size = currentJoints.size();
	if (size < jointPosition.size())
	{
		size = currentJoints.size();
	}
	else if (size >= jointPosition.size())
	{
		size = jointPosition.size();
	}

	for (size_t ii = 0; ii<size; ++ii)
	{
		currentJoints[ii] = jointPosition[ii];
	}

	retVal &= setJointValues(currentJoints);

	//Check if achieved
	EcBooleanVector jointAchieved;
	jointAchieved.resize(size);
	EcBoolean positionAchieved = EcFalse;

	// if it hasnt been achieved after 5 sec, return false
	EcU32 timeout = 5000;
	EcU32 interval = 10;
	EcU32 count = 0;

	while (!positionAchieved && !(count >= timeout / interval))
	{
		EcSLEEPMS(interval);
		count++;
		getJointValues(currentJoints);
		for (size_t ii = 0; ii<size; ++ii)
		{

			if (std::abs(jointPosition[ii] - currentJoints[ii])<angletolerance)
			{
				jointAchieved[ii] = EcTrue;
			}
		}

		for (size_t ii = 0; ii<size; ++ii)
		{
			if (!jointAchieved[ii])
			{
				positionAchieved = EcFalse;
				break;
			}
			else
			{
				positionAchieved = EcTrue;
			}
		}
	}
	return positionAchieved;
}



//returns vector containing x, y, and z appropriate for the canvas's location.
//after recieving x, y, and z relative to top left corner of canvas.
//CHECK LATER
std::vector<double> CytonRunner::convert(double x, double y, double z){
	double x1 = canvasCorners.at(0).at(0);
	double y1 = canvasCorners.at(0).at(1);
	double z1 = canvasCorners.at(0).at(2);


	//need to figure out plane transformations
	//double xNew = x*cos(theta) + z*sin(theta) + y*sin(psi) + x*sin(psi) + x1 + dx;
	//double yNew = y*cos(phi) + z*sin(phi) + y*sin(psi) - x*sin(psi) + y1 + dy;
	//double zNew = -y*sin(phi) + (z / 2.0)*(cos(phi) + cos(theta)) - x*sin(theta) + z1 + dz;

	double xNew = x + x1 + dx;
	double yNew = y + y1 + dy;
	double zNew = z + z1 + dz;

	std::vector<double> temp;
	temp.push_back(xNew);
	temp.push_back(yNew);
	temp.push_back(zNew);

	printf("%d, %f, %i", dz, dz, dz);

	printf("x: %f, y: %f, z: %f\n", xNew, &yNew, &zNew);
	return temp;
}