#ifndef COMMANDVIEWER_H
#define COMMANDVIEWER_H

#include "Line.h"
#include "ui_commandviewer.h"
#include "Shape.h"

#include <QWidget>
#include <QListWidget>
#include <QCloseEvent>
#include <iostream>
#include <QXmlStreamReader>
#include <QDebug.h>
#include <QMainWindow>
#include "commandinterpreter.h"

namespace Ui {
class CommandViewer;
}

class CommandViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit CommandViewer(QWidget *parent = 0);
    ~CommandViewer();
    QListWidget *list;
    void clear();
	void MakeEditor();
	void setProjectName(QString name);
	void setProjectLocation(QString loc);

    void setMainClosed(bool closed);
    CommandInterpreter *interpreter;
	bool *saved;
	bool fileChanged;
	Line *currentEditor;
	QString projectName, projectLocation;

private:
    void ConnectToolBar();
	int FillEditor(QString editorName);
	void runFrom();
	void runOnly();
	void setBreakpoint();

    Ui::CommandViewer *ui;
	bool mainClosed, freshlyMade;

private slots:
    void MoveUp_clicked();
    void MoveDown_clicked();
    void DeleteCommand_clicked();
    void on_EditCommand_clicked();
    void Add_Command_Clicked();
    void Stop_triggered();
    void Pause_triggered();
    void StepForward_triggered();
    void StepBackwards_triggered();
	void launchRightClick(QPoint pos);
	void menuSort(QAction *a);

public slots:
    void fileSaved(bool saved);
	void RunFromStart_triggered();

signals:
    void fileStatusChanged();
    void Add_External_Command();
	void MustSave();
	void EmitConnectEditor(Line* currentEditor);

protected:
    void closeEvent(QCloseEvent *);
};

#endif // COMMANDVIEWER_H
