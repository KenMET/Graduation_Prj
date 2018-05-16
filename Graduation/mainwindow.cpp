#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qmessagebox.h>
#include <qpainter.h>
#include <qrect.h>
#include <qbrush.h>
#include <qfont.h>
#include <qfile.h>

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

void MainWindow::Put_Vector2Canvas(bool **canvas, vectors *vector, int count, record_point *record)
{
    int canvas_w, canvas_h;
    int start_w = BUFF_WIDTH / 2, start_h = BUFF_HEIGHT / 2;
    int last_canvas_w = BUFF_WIDTH / 2, last_canvas_h = BUFF_HEIGHT / 2;
    int caiji_count;
    double xielv;
    double chang, kuan;
    bool scan_way = 0;

    record->top = last_canvas_h;
    record->bottom = last_canvas_h;
    record->left = last_canvas_w;
    record->right = last_canvas_w;

    record->pos_x[record->count] = last_canvas_w;
    record->pos_y[record->count] = last_canvas_h;

    record->count++;

    for(int k=1; k<=count; k++)
    {
        chang = cos((M_PI * vector[k-1].angle) / 180) * vector[k-1].distance;
        kuan =  sin((M_PI * vector[k-1].angle) / 180) * vector[k-1].distance;

        if(abs(kuan) > abs(chang))
        {
            if(kuan > 0)
                caiji_count = (int)(kuan + 0.5);
            else
                caiji_count = (int)(kuan - 0.5);
            xielv = chang / kuan;
            scan_way = 1;   //shu zhi sao miao
        }
        else
        {
            if(chang > 0)
                caiji_count = (int)(chang + 0.5);
            else
                caiji_count = (int)(chang - 0.5);
            xielv = kuan / chang;
            scan_way = 0;   //shui ping sao miao
        }


        //MSG_BOX("y= %fx count:[%f or %f->%d] (angle%f distance:%f)", xielv, chang, kuan, caiji_count, vector[k-1].angle, vector[k-1].distance);
        if((vector[k-1].angle >= 0 && vector[k-1].angle <= 90) || (vector[k-1].angle > 270 && vector[k-1].angle <= 360))
        {
            for(int j=0; j<caiji_count; j++)
            {
                if(scan_way == 0)
                {
                    canvas_w = j;
                    canvas_h = j * xielv + 0.5;
                }
                else
                {
                    canvas_h = j;
                    canvas_w = j * xielv + 0.5;
                }
                canvas[canvas_w + last_canvas_w][canvas_h + last_canvas_h] = 1;
                //MSG_BOX("1.scan:%d canvas_w:%d  canvas_h:%d",scan_way, canvas_w + last_canvas_w, canvas_h + last_canvas_h);
            }

        }
        else if(vector[k-1].angle > 90 && vector[k-1].angle <= 270)
        {
            if(caiji_count > 0)
                caiji_count = -caiji_count;
            for(int j=0; j>caiji_count; j--)
            {
                if(scan_way == 0)
                {
                    canvas_w = j;
                    canvas_h = j * xielv - 0.5;
                }
                else
                {
                    canvas_h = j;
                    canvas_w = j * xielv - 0.5;
                }
                canvas[canvas_w + last_canvas_w][canvas_h + last_canvas_h] = 1;
                //MSG_BOX("2.scan:%d canvas_w:%d  canvas_h:%d",scan_way, canvas_w + last_canvas_w, canvas_h + last_canvas_h);
            }
        }
        else
            MSG_BOX("Err Angle");

        last_canvas_w += canvas_w;
        last_canvas_h += canvas_h;
        record->pos_x[record->count] = last_canvas_w;
        record->pos_y[record->count] = last_canvas_h;
        record->count++;

        if(last_canvas_w < record->left)
            record->left = last_canvas_w;
        if(last_canvas_w > record->right)
            record->right = last_canvas_w;
        if(last_canvas_h < record->bottom)
            record->bottom = last_canvas_h;
        if(last_canvas_h > record->top)
            record->top = last_canvas_h;


    }

    if(last_canvas_w != start_w || last_canvas_h != start_h)
    {
        chang = start_w - last_canvas_w;//+
        kuan  = start_h - last_canvas_h;//-
        if(abs(chang) > abs(kuan))
        {
            caiji_count = start_w - last_canvas_w;
            xielv = kuan / chang;
            scan_way = 0;   //shui ping sao miao
        }
        else
        {
            caiji_count = start_h - last_canvas_h;
            xielv = chang / kuan;
            scan_way = 1;   //shu zhi sao miao
        }
        if(caiji_count > 0)
        {
            for(int j=0; j<caiji_count; j++)
            {
                if(scan_way == 0)
                {
                    canvas_w = j;
                    canvas_h = j * xielv - 0.5;
                }
                else
                {
                    canvas_h = j;
                    canvas_w = j * xielv - 0.5;
                }
                canvas[canvas_w + last_canvas_w][canvas_h + last_canvas_h] = 1;
            }
        }
        else
        {
            for(int j=0; j>caiji_count; j--)
            {
                if(scan_way == 0)
                {
                    canvas_w = j;
                    canvas_h = j * xielv - 0.5;
                }
                else
                {
                    canvas_h = j;
                    canvas_w = j * xielv - 0.5;
                }
                canvas[canvas_w + last_canvas_w][canvas_h + last_canvas_h] = 1;
            }
        }
    }
}

void MainWindow::Put_Canvas2File(bool **canvas)
{
    QFile file("Canvas.cav");
    if(!file.open(QIODevice::ReadWrite))
    {
        MSG_BOX("Open Err");
    }

    QString content = "canvas value:\n";
    QString Add;

    for(int i=0; i<BUFF_HEIGHT; i++)
    {
        for(int j=0; j<BUFF_WIDTH; j++)
        {
            Add.sprintf("%d",canvas[i][j]);
            content += Add;
        }
        Add.sprintf("\n");
        content += Add;
    }


    int length = file.write(content.toLatin1(),content.length());
    if(length == -1)
    {
        MSG_BOX("write Err");
    }
    else
    {
        //MSG_BOX("write OK len:%d", length);
    }
}

void MainWindow::Put_Canvas2Screen(record_point *record)
{
    QPainter painter(this);
    QPen pen;

    pen.setColor(Qt::black);
    pen.setWidth(1);
    painter.setPen(pen);

    QPointF *pionts;
    pionts = (QPointF *)malloc(sizeof(QPointF) * PAINTER_WIDTH * PAINTER_HEIGHT);

    double wight_bili, hight_bili, zuijia_bili;
    hight_bili = (double)(record->top - record->bottom + RESERVE) / (double)PAINTER_HEIGHT;
    wight_bili = (double)(record->right - record->left + RESERVE) / (double)PAINTER_WIDTH;
    if(hight_bili > wight_bili)
        zuijia_bili = hight_bili;
    else
        zuijia_bili = wight_bili;

    int test_x, test_y;

    for(int j=0; j<record->count; j++)
    {
        test_x = record->pos_x[j] - record->left + RESERVE / 2;
        test_y = record->pos_y[j] - record->bottom + RESERVE / 2;
        test_x = test_x / zuijia_bili;
        test_y = test_y / zuijia_bili;
        //MSG_BOX("QPointF_x:[%d] QPointF_y:[%d]", test_x, test_y);
        pionts[j] = QPointF(test_x, test_y);
    }

    painter.drawPolygon(pionts, record->count);
    free(pionts);
}

void MainWindow::Caculation_Canvas(bool **canvas, record_point *record)
{
    int rect_w, rect_h;
    int drop_point_x, drop_point_y;
    int in_count = 0;
    bool over_write = 0, meet0 = 0;

    int *pos;
    int duan_count = 0;

    pos = (int*)malloc(sizeof(int*) * 100);

    rect_w = record->right - record->left;
    rect_h = record->top - record->bottom + 1;

    //MSG_BOX("min rect:[%d(%d %d) %d(%d %d)]",
    //    rect_w, record->right, record->left,
    //    rect_h, record->top, record->bottom);

    for(int i = 0; i < rect_w; i++)
    {
        for(int j = 0; j < rect_h; j++)
        {
            //MSG_BOX("now pos:[%d %d]=%d", record->left + i, record->bottom + j,canvas[record->left + i][record->bottom + j]);
            if(canvas[record->left + i][record->bottom + j] == 1 && !over_write && !meet0)
            {\
                over_write = 1;
                pos[duan_count++] = record->bottom + j;
                //MSG_BOX("record start:[%d %d], duan_count=%d", record->left + i, record->bottom + j, duan_count);
            }
            else if(canvas[record->left + i][record->bottom + j] == 0 && over_write)
            {
                meet0 = 1;
            }
            else if(canvas[record->left + i][record->bottom + j] == 1 && over_write && meet0)
            {
                pos[duan_count++] = record->bottom + j;
                //MSG_BOX("record end:[%d %d], duan_count=%d", record->left + i, record->bottom + j, duan_count);
            }
        }
        if(duan_count % 2 != 0 && duan_count != 1)
        {
            MSG_BOX("count Err:%d", duan_count);
        }
        else if(duan_count != 1)
        {
            for(int duan = 0; duan < (duan_count/2); duan ++)
            {
                for(int k = 0; k < (pos[duan*2+1] - pos[duan*2]); k++)
                {
                    canvas[record->left + i][k + pos[duan*2]] = 1;
                }
            }
        }
        duan_count = 0;
        over_write = 0;
        meet0 = 0;
    }

    for(int count=0; count<1000; count++)
    {
        drop_point_x = qrand() % rect_w + record->left;
        drop_point_y = qrand() % rect_h + record->bottom;
        if(canvas[drop_point_x][drop_point_y] == 1)
            in_count ++;
    }


    MSG_BOX("in_count:[%d]", in_count);
    //MSG_BOX("QPointF_x:[%d] QPointF_y:[%d]", rect_w, rect_h);



    Put_Canvas2File(canvas);
    free(pos);
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


    bool **canvas = NULL;

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
    else
        QMessageBox::information(NULL, "Mem Err", "canvas malloc already");


    vectors buff1[BUFF1_SIZE] = {
        {30, 80},
        {150, 80},
        {210, 80},
        //{45, 30},
        //{135, 30},
        //{225, 30},
        //{90, 30},
        //{180, 30},
        //{270, 30},
        //{315, 30},
    };

    record_point *record;
    record = (record_point*)malloc(sizeof(record_point));
    record->pos_x = (int*)malloc(sizeof(int) * BUFF_WIDTH);
    record->pos_y = (int*)malloc(sizeof(int) * BUFF_HEIGHT);
    if(record == NULL || record->pos_x == NULL || record->pos_y == NULL)
    {
        QMessageBox::information(NULL, "Mem Err", "record malloc Err");
        return ;
    }
    record->count = 0;

    Put_Vector2Canvas(canvas, buff1, BUFF1_SIZE, record);

    Put_Canvas2Screen(record);

    Caculation_Canvas(canvas, record);


    free(record);

    for(int k=0; k<BUFF_WIDTH; k++)
        free(canvas[k]);
    free(canvas);
}





