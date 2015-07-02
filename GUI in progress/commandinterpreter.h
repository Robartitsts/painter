#ifndef COMMANDINTERPRETER_H
#define COMMANDINTERPRETER_H

#include "painter.h"
#include "CytonController.h"
#include <QListWidget>
#include <QThread>
#include <QTimer>
#include <vector>



class CommandInterpreter : public QObject
{
    Q_OBJECT
    QThread workerThread;

public:
    CommandInterpreter(QString projectName);
    void beginPaintingCommands(QListWidget* widget, int index);
    void stopPaintingCommands();
    void stepForwardCommands();
    void stepBackwardCommands();
    void setProjectName(QString name);
    void pausePaintingCommands();
    void clear();

private slots:
    void sendCommand();

public slots:
    void beginConnecting(QString robot);

private:
    Painter *picasso;
    QString projectName;
    bool continuePainting;
    bool stopped, connected, prevContinuous;
    QTimer updateTimer;
    std::vector<int> x1,x2,y1,y2;
    int currentPos, startPos;
    std::vector<QString> colorList, styleList;
	CytonController *bender;

    void drawUntilCommand(int stopPos);
    void buildPointVectors(QListWidget* widget);
    void addPointsFromFile(QString fileName);
};

#endif // COMMANDINTERPRETER_H
