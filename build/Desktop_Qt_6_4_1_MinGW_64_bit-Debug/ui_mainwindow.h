/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.4.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionSkeletonization;
    QAction *actionDetect_Vertex;
    QAction *actionMannual_Edit_Mode;
    QAction *actionAdd_Vertex;
    QAction *actionDelete_Vertex;
    QAction *actionDelete_All_Vertices;
    QAction *actionFind_Vertex;
    QAction *actionScreen_Shot_all;
    QAction *actionScreen_Shot_View;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QSplitter *splitter_2;
    QWidget *widget;
    QFormLayout *formLayout;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QSplitter *splitter;
    QGraphicsView *graphicsView;
    QTableWidget *tableWidget;
    QMenuBar *menubar;
    QMenu *menuOpen;
    QMenu *menuProcess;
    QMenu *menuEdit;
    QMenu *menuFind;
    QMenu *menuExport;
    QMenu *menuDisplay;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        actionSkeletonization = new QAction(MainWindow);
        actionSkeletonization->setObjectName("actionSkeletonization");
        actionDetect_Vertex = new QAction(MainWindow);
        actionDetect_Vertex->setObjectName("actionDetect_Vertex");
        actionMannual_Edit_Mode = new QAction(MainWindow);
        actionMannual_Edit_Mode->setObjectName("actionMannual_Edit_Mode");
        actionAdd_Vertex = new QAction(MainWindow);
        actionAdd_Vertex->setObjectName("actionAdd_Vertex");
        actionDelete_Vertex = new QAction(MainWindow);
        actionDelete_Vertex->setObjectName("actionDelete_Vertex");
        actionDelete_All_Vertices = new QAction(MainWindow);
        actionDelete_All_Vertices->setObjectName("actionDelete_All_Vertices");
        actionFind_Vertex = new QAction(MainWindow);
        actionFind_Vertex->setObjectName("actionFind_Vertex");
        actionScreen_Shot_all = new QAction(MainWindow);
        actionScreen_Shot_all->setObjectName("actionScreen_Shot_all");
        actionScreen_Shot_View = new QAction(MainWindow);
        actionScreen_Shot_View->setObjectName("actionScreen_Shot_View");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        splitter_2 = new QSplitter(centralwidget);
        splitter_2->setObjectName("splitter_2");
        splitter_2->setOrientation(Qt::Horizontal);
        widget = new QWidget(splitter_2);
        widget->setObjectName("widget");
        formLayout = new QFormLayout(widget);
        formLayout->setObjectName("formLayout");
        label = new QLabel(widget);
        label->setObjectName("label");

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        label_2 = new QLabel(widget);
        label_2->setObjectName("label_2");

        formLayout->setWidget(0, QFormLayout::FieldRole, label_2);

        label_3 = new QLabel(widget);
        label_3->setObjectName("label_3");

        formLayout->setWidget(1, QFormLayout::LabelRole, label_3);

        label_4 = new QLabel(widget);
        label_4->setObjectName("label_4");

        formLayout->setWidget(1, QFormLayout::FieldRole, label_4);

        label_5 = new QLabel(widget);
        label_5->setObjectName("label_5");

        formLayout->setWidget(2, QFormLayout::LabelRole, label_5);

        label_6 = new QLabel(widget);
        label_6->setObjectName("label_6");

        formLayout->setWidget(2, QFormLayout::FieldRole, label_6);

        splitter_2->addWidget(widget);
        splitter = new QSplitter(splitter_2);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Horizontal);
        graphicsView = new QGraphicsView(splitter);
        graphicsView->setObjectName("graphicsView");
        splitter->addWidget(graphicsView);
        tableWidget = new QTableWidget(splitter);
        tableWidget->setObjectName("tableWidget");
        splitter->addWidget(tableWidget);
        splitter_2->addWidget(splitter);

        horizontalLayout->addWidget(splitter_2);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 20));
        menuOpen = new QMenu(menubar);
        menuOpen->setObjectName("menuOpen");
        menuProcess = new QMenu(menubar);
        menuProcess->setObjectName("menuProcess");
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName("menuEdit");
        menuFind = new QMenu(menubar);
        menuFind->setObjectName("menuFind");
        menuExport = new QMenu(menubar);
        menuExport->setObjectName("menuExport");
        menuDisplay = new QMenu(menubar);
        menuDisplay->setObjectName("menuDisplay");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuOpen->menuAction());
        menubar->addAction(menuProcess->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuFind->menuAction());
        menubar->addAction(menuExport->menuAction());
        menubar->addAction(menuDisplay->menuAction());
        menuProcess->addAction(actionSkeletonization);
        menuProcess->addAction(actionDetect_Vertex);
        menuEdit->addAction(actionMannual_Edit_Mode);
        menuEdit->addAction(actionAdd_Vertex);
        menuEdit->addAction(actionDelete_Vertex);
        menuEdit->addAction(actionDelete_All_Vertices);
        menuFind->addAction(actionFind_Vertex);
        menuExport->addAction(actionScreen_Shot_all);
        menuExport->addAction(actionScreen_Shot_View);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        actionSkeletonization->setText(QCoreApplication::translate("MainWindow", "Skeletonization", nullptr));
        actionDetect_Vertex->setText(QCoreApplication::translate("MainWindow", "Detect Vertex", nullptr));
        actionMannual_Edit_Mode->setText(QCoreApplication::translate("MainWindow", "Mannual Edit Mode", nullptr));
        actionAdd_Vertex->setText(QCoreApplication::translate("MainWindow", "Add Vertex", nullptr));
        actionDelete_Vertex->setText(QCoreApplication::translate("MainWindow", "Delete Vertex", nullptr));
        actionDelete_All_Vertices->setText(QCoreApplication::translate("MainWindow", "Delete All Vertices", nullptr));
        actionFind_Vertex->setText(QCoreApplication::translate("MainWindow", "Find Vertex", nullptr));
        actionScreen_Shot_all->setText(QCoreApplication::translate("MainWindow", "Screen Shot (all)", nullptr));
        actionScreen_Shot_View->setText(QCoreApplication::translate("MainWindow", "Screen Shot (View)", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        menuOpen->setTitle(QCoreApplication::translate("MainWindow", "Open", nullptr));
        menuProcess->setTitle(QCoreApplication::translate("MainWindow", "Process", nullptr));
        menuEdit->setTitle(QCoreApplication::translate("MainWindow", "Edit", nullptr));
        menuFind->setTitle(QCoreApplication::translate("MainWindow", "Find", nullptr));
        menuExport->setTitle(QCoreApplication::translate("MainWindow", "Export", nullptr));
        menuDisplay->setTitle(QCoreApplication::translate("MainWindow", "Display", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
