/* 
 * File:   blockContainer.h
 * Author: biyanin
 *
 * Created on April 30, 2015, 3:56 PM
 */

#ifndef BLOCKCONTAINER_H
#define	BLOCKCONTAINER_H

#include <QFrame>
#include <QString>
#include <QPalette>
#include <QColor>
#include <QLinearGradient>
#include <QPoint>
#include <QFont>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpacerItem>

class blockContainer : public QFrame
{
    Q_OBJECT
           
    public:
        blockContainer(const QString& title="", QWidget* parent=NULL);
        
        void setMainWidget(QWidget* widget);
        void setHeaderWidget(QWidget* widget);
        
    public slots:
        void setHeaderTitle(const QString& titleLabel);
        
    signals:
        void doubleClicked();
    
    protected:
        void mouseDoubleClickEvent(QMouseEvent *event);
    
    private:
        QWidget* setupHeader(const QString& title);
        
        QVBoxLayout* mainLayout;
        QWidget* mainWidget;
        
        QGridLayout* headerLayout;
        QLabel* headerTitle;      
        QWidget* headerWidget;
        
};

#endif	/* BLOCKCONTAINER_H */
