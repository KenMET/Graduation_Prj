#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


#define PAINTER_WIDTH       (width() - 82)
#define PAINTER_HEIGHT      height()

#define BUFF_WIDTH          1000
#define BUFF_HEIGHT         1000

#define TEST_POINTS         100

#define RESERVE             4

#define MSG_BOX(...)        do{char test[50];sprintf(test, ##__VA_ARGS__);QMessageBox::information(NULL, "Debug", test);}while(0)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;


public:
    bool **canvas;
    struct vector{
        float angle;
        int distance;
    };

    void paintEvent(QPaintEvent *event);


    void User_Init(void);


    void Put_Vector2Canvas(vector *vector, int count);



private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
};

#endif // MAINWINDOW_H
