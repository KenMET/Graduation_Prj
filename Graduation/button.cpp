#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qmessagebox.h>
#include <qpainter.h>
#include <qrect.h>
#include <qbrush.h>
#include <qfont.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>





void MainWindow::on_pushButton_clicked()
{
    QMessageBox::information(NULL, "title1", "comment1");
}


void MainWindow::on_pushButton_2_clicked()
{
    //QMessageBox::information(NULL, "title2", "comment2");
    User_Init();
}


