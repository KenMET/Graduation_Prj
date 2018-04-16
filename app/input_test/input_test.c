#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <sys/ioctl.h>
#include <linux/input.h>
#include <string.h>
#include <math.h>

#define INPUT_DEV_PATH	"/dev/input/event2"
#define MPU_DEV_PATH	"/dev/mpu6050"

#define RAD_TO_DEG 57.295779513082320876798154814105  
#define DEG_TO_RAD 0.01745329251994329576923690768489
#define q30  1073741824.0f


struct mpu6050_event {
	int	q1;
	int	q2;
	int q3;
	int q4;
};


void get_accel(struct mpu6050_event *mpu_ev)
{
	double Pitch,Roll,Yaw;
	double Pitch_Pre = 1.0f, Roll_Pre = 1.0f, Yaw_Pre = 1.0f;
	double q0=1.0f,q1=0.0f,q2=0.0f,q3=0.0f;
	double quat[4] = {mpu_ev->q1, mpu_ev->q2, mpu_ev->q3, mpu_ev->q4};

	static int count = 0;
	
	q0 = (double)quat[0] / q30; 
	q1 = (double)quat[1] / q30;
	q2 = (double)quat[2] / q30;
	q3 = (double)quat[3] / q30;


	
	Pitch = asin(2 * q1 * q3 - 2 * q0* q2)* RAD_TO_DEG;
	
	Roll  = -atan2( -2 * q1 * q1 - 2 * q2* q2 + 1, 2 * q2 * q3 + 2 * q0 * q1)* RAD_TO_DEG;	
	Roll -= 90;
	
	Yaw   = atan2(2*(q1*q2 + q0*q3),q0*q0+q1*q1-q2*q2-q3*q3) * RAD_TO_DEG;	
	if(fabs(Yaw)>180)			
		Yaw = Yaw_Pre;
	else									
		Yaw_Pre = Yaw;

	printf("caculation Pitch:%f, Roll:%f, Yaw:%f\n", Pitch, Roll, Yaw);
}


int main(int argc, const char *argv[])
{
	int fd;
	int ret;
	int version;
	char buf[256] = { 0 };
	struct input_event input_ev;
	struct mpu6050_event mpu_ev;


	fd = open(INPUT_DEV_PATH, O_RDONLY);
	if (fd < 0) {  
		printf("open file failed\n");  
		exit(1);  
	} 

	ioctl(fd, EVIOCGVERSION, &version);
	ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
	printf("	evdev version: %d.%d.%d\n",  
					   version >> 16, (version >> 8) & 0xff, version & 0xff);
	printf("	name: %s\n", buf);

	while(1)
	{
		ret = read(fd, &input_ev, sizeof(struct input_event));
		if (ret < 0) {  
            printf("read event error!\n");  
            exit(1);  
        }
		
		if(input_ev.type == EV_SYN)
		{
			get_accel(&mpu_ev);
			memset(&mpu_ev, 0, sizeof(struct mpu6050_event));
		}
		else if (input_ev.type == EV_ABS) 
		{
			switch(input_ev.code)
			{
				case ABS_X:
					mpu_ev.q1 = input_ev.value;
					break;
				case ABS_Y:
					mpu_ev.q2 = input_ev.value;
					break;
				case ABS_Z:
					mpu_ev.q3 = input_ev.value;
					break;
				case ABS_RX:
					mpu_ev.q4 = input_ev.value;
					break;
				default:
					printf("unknow ABS code\n");
			}
		}
	}

	close(fd);
	return 0;
}
