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

#include <math.h>

#include "testbuff.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    User_Init();
}

void MainWindow::Put_Vector2Canvas(vector *vector, int count)
{
    //MSG_BOX("vector:%f  %d",vector[1].angle, vector[1].distance);
    QPainter painter(this);
    QPen pen;

    pen.setColor(Qt::black);
    pen.setWidth(1);
    painter.setPen(pen);

    int canvas_w, canvas_h;
    int last_canvas_w = BUFF_WIDTH / 2, last_canvas_h = BUFF_HEIGHT / 2;
    int wight;
    double xielv;
    for(int k=1; k<=count; k++)
    {
        wight = sin(M_PI/180 *vector[k-1].angle)*vector[k-1].distance;
        xielv = (cos(M_PI/180 *vector[k-1].angle)*vector[k-1].distance) / (sin(M_PI/180 *vector[k-1].angle)*vector[k-1].distance);
        if(vector[k-1].angle >= 0 && vector[k-1].angle <= 180)
        {
            for(int j=0; j<=wight; j++)
            {
                canvas_w = j;
                if(xielv >= 0)
                    canvas_h = j * xielv + 0.5;
                else
                    canvas_h = j * xielv - 0.5;
                //MSG_BOX("1.canvas_w:%d  canvas_h:%d",canvas_w + last_canvas_w, canvas_h + last_canvas_h);
                canvas[canvas_w + last_canvas_w][canvas_h + last_canvas_h] = 1;
            }
        }
        else if(vector[k-1].angle > 180 && vector[k-1].angle <= 360)
        {
            for(int j=0; j>=wight; j--)
            {
                canvas_w = j;
                if(xielv >= 0)
                    canvas_h = j * xielv - 0.5;
                else
                    canvas_h = j * xielv + 0.5;
                //MSG_BOX("2.canvas_w:%d  canvas_h:%d",canvas_w + last_canvas_w, canvas_h + last_canvas_h);
                canvas[last_canvas_w + canvas_w][last_canvas_h + canvas_h] = 1;
            }
        }
        else
            MSG_BOX("Err Angle");
        last_canvas_w += canvas_w;
        last_canvas_h += canvas_h;
    }
    bool find = 0;
    int top,bottom,left,right,bansui;
    for(int i=0; i<BUFF_WIDTH; i++)
    {
        for(int j=0; j<BUFF_HEIGHT; j++)
        {
            if(canvas[i][j] == 1)
            {
                left = i;
                bansui = j;
                find = 1;
                break;
            }
        }
        if(find)
            break;
    }
    find = 0;
    for(int i=BUFF_WIDTH-1; i>=0; i--)
    {
        for(int j=0; j<BUFF_HEIGHT; j++)
        {
            if(canvas[i][j] == 1)
            {
                right = i;
                find = 1;
                break;
            }
        }
        if(find)
            break;
    }
    find = 0;
    for(int i=0; i<BUFF_WIDTH; i++)
    {
        for(int j=0; j<BUFF_HEIGHT; j++)
        {
            if(canvas[j][i] == 1)
            {
                top = i;
                find = 1;
                break;
            }
        }
        if(find)
            break;
    }
    find = 0;
    for(int i=BUFF_WIDTH-1; i>=0; i--)
    {
        for(int j=0; j<BUFF_HEIGHT; j++)
        {
            if(canvas[j][i] == 1)
            {
                bottom = i;
                find = 1;
                break;
            }
        }
        if(find)
            break;
    }
    //MSG_BOX("top:%d bottom:%d left:%d right:%d ", top, bottom, left, right);

    QPointF *pionts;
    pionts = (QPointF *)malloc(sizeof(QPointF) * PAINTER_WIDTH * PAINTER_HEIGHT);

    int start_x, start_y, pionts_count = 0;
    int detect_x, detect_y, last_x, last_y, last_record_x=0, last_record_y=0;

    start_x = left;
    start_y = bansui;
    last_x = start_x;
    last_y = start_y;

    int map[8][2]={
        {-1, 0},    //up
        {-1, 1},    //up / right
        {0, 1},    //right
        {1, 1},    //down / right
        {1, 0},    //down
        {1, -1},    //down / left
        {0, -1},    //left
        {-1, -1},    //up / left
    };
    bool paint_OK = 0;
    double wight_bili, hight_bili, bili;
    hight_bili = (double)(bottom - top + RESERVE) / (double)PAINTER_HEIGHT;
    wight_bili = (double)(right - left + RESERVE) / (double)PAINTER_WIDTH;
    if(hight_bili > wight_bili)
        bili = hight_bili;
    else
        bili = wight_bili;

    for(int k=0; k<PAINTER_WIDTH * PAINTER_HEIGHT; k++)
    {
        for(int j=0; j<8; j++)
        {
            detect_x = last_x + map[j][0];
            detect_y = last_y + map[j][1];
            MSG_BOX("detect_x:[%d->%d->%d] detect_y:[%d->%d->%d]\n%d %d %d\n%d %d %d\n%d %d %d"
                                                  , last_record_x, last_x, detect_x, last_record_y, last_y, detect_y
                                                  , canvas[last_x-1][last_y-1], canvas[last_x-1][last_y],   canvas[last_x-1][last_y+1]
                                                  , canvas[last_x][last_y-1],   canvas[last_x][last_y],     canvas[last_x][last_y+1]
                                                  , canvas[last_x+1][last_y-1], canvas[last_x+1][last_y],   canvas[last_x+1][last_y+1]);
            if(detect_x == start_x && detect_y == start_y)
            {
                MSG_BOX("paint_OK");
                paint_OK = 1;
                break;
            }
            if(detect_x == last_record_x && detect_y == last_record_y)
                continue;
            if(canvas[detect_x][detect_y] == 1)
            { 
                last_record_x = last_x;
                last_record_y = last_y;
                last_x = detect_x;
                last_y = detect_y;

                //MSG_BOX("bili:%d %d QPointF_x:[%d] QPointF_y:[%d]", test_x, test_y, (detect_x - (left - 5)), (detect_y - (top - 5)));
                pionts[pionts_count++] = QPointF((double)(detect_x - (left - RESERVE/2)) / bili, (double)(detect_y - (top - RESERVE/2)) / bili);
                break;
            }
        }
        if(paint_OK)
            break;
    }


    painter.drawPolygon(pionts, pionts_count);
    free(pionts);
}

void MainWindow::User_Init(void)
{
    QPainter painter(this);

    QPen pen;

    pen.setColor(Qt::red);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, PAINTER_WIDTH, PAINTER_HEIGHT);

    if(canvas == NULL)
    {
        canvas=(bool**)malloc(sizeof(bool*)*BUFF_WIDTH);
        if(canvas == NULL)
        {
            QMessageBox::information(NULL, "Mem Err", "canvas head malloc Err");
            return ;
        }
        for(int i=0; i<BUFF_WIDTH; i++)
        {
            canvas[i]=(bool*)malloc(sizeof(bool)*BUFF_HEIGHT);
            if(canvas[i] == NULL)
            {
                QMessageBox::information(NULL, "Mem Err", "canvas line malloc Err");
                return ;
            }
        }
        for(int j=0; j<BUFF_WIDTH; j++)
        {
            for(int k=0; k<BUFF_HEIGHT; k++)
                canvas[j][k] = 0;
        }

    }


    vector buff1[BUFF1_SIZE] = {
        {30, 30},
        {150, 30},
        {210, 30},
    };

    Put_Vector2Canvas(buff1, BUFF1_SIZE);
}





