#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


#define PAINTER_WIDTH       (width() - 82)
#define PAINTER_HEIGHT      height()

#define BUFF_WIDTH          500
#define BUFF_HEIGHT         500

#define TEST_POINTS         100

#define RESERVE             2

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
    struct vectors{
        float angle;
        float distance;
    };
    struct record_point{
        int count;
        int left;
        int right;
        int top;
        int bottom;
        int *pos_x;
        int *pos_y;
    };

    //record_point *record;
    //bool **canvas;


    void paintEvent(QPaintEvent *event);


    void User_Init(void);

    void Put_Canvas2File(bool **canvas);
    void Put_Vector2Canvas(bool **canvas, vectors *vector, int count, record_point *record);
    void Put_Canvas2Screen(record_point *record);
    void Caculation_Canvas(bool **canvas, record_point *record);

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
};

#endif // MAINWINDOW_H
