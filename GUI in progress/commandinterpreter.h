#ifndef COMMANDINTERPRETER_H
#define COMMANDINTERPRETER_H

#include "painter.h"
#include "CytonController.h"
#include <QListWidget>
#include <QTimer>
#include <vector>

class CommandInterpreter : public QObject
{
    Q_OBJECT

public:
    CommandInterpreter(QString projectName);
    void beginPaintingCommands(int index);
    void stopPaintingCommands();
    void stepForwardCommands();
    void stepBackwardCommands();
    void setProjectName(QString name);
    void pausePaintingCommands();
    void clear();
	void setList(QListWidget* list);

private slots:
    void SendNext();

public slots:
    void beginConnecting(QString robot);
	void getInstructed(int current);

private:
	//General Variables
    Painter *picasso;
    QString projectName;
	CytonController *bender;
	QTimer updateTimer;
	QListWidget *list;
	//General Variables


	//CommandList Variables
	int lineIndex, solidIndex, pixelIndex, commandIndex;
	QList<QString *> listOfCommandTypes;
	QString CurrentCommandType;

	std::vector<int> x, y, lineWidth; //line
	std::vector<QString> lineColor, lineStyle; //line
	int lineAttributeIndex; //special
	//??			//Solid vars.
	//??			//Pixel vars.
	//CommandList Variables


	bool finished, paused, stopped, connected, prevContinuous, continuePainting;
	int startCommandIndex;
	void BuildCommands();
	void ResetIndicies();
	void drawUntilCommand(int stopIndex);
	void MakeLine(QString commandName);
	void MakeSolid(QString commandName);
	void MakePixel(QString commandName);
	void sendLine();
	void sendSolid();
	void sendPixel();


signals:
	void tell_go_home(int pos);
	void send_Pos(double x1, double y1, double x2, double y2, bool continuous, bool prevContinuous, int currentEditor);
};

#endif // COMMANDINTERPRETER_H
