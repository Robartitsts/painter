#ifndef DRAWONWIDGET_H
#define DRAWONWIDGET_H

#include "Line.h"
#include <QMouseEvent>
#include <QLabel>
#include <QPen>
#include <qlistwidget.h>
#include "commandviewer.h"


class drawOnWidget : public QLabel
{
    Q_OBJECT
public:
    drawOnWidget(QWidget * parent, int num);
    ~drawOnWidget(){}
    void clearAll(int resetBackground);
    Line* currentEditor;
	int currentCommandType;

private:
    int prevX,prevY,pointCount, penWidth, idNumber;
    QString penColor, penStyle;
    QPen pen;
	QLabel frontLabel;

    bool drawPoint(int currentX, int currentY);
	bool drawSquare(int currentX, int currentY);
	bool drawCircle(int currentX, int currentY);

signals:
    void sendPoint(int x, int y, int pointCount);

public slots:
    void updateToEditor(Line* editor);
	void updateToAllEditors(CommandViewer* commandView);

protected:
    void mousePressEvent(QMouseEvent* event);
};

#endif // DRAWONWIDGET_H
