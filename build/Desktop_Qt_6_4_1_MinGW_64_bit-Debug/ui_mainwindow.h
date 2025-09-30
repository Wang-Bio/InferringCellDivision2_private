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
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
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
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
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

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
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
