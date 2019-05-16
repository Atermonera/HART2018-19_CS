#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    int fd, i;
    char * myfifo = "sus.csv";
    char tmp[12]={0x0};
    // len = 281
    char * bst = "104.390000, 687.477976, 13.832415, 0.139173, 150324.684151, -25.921915, -32.172647, -4891.659856, -98.422743, -0.990268, 290.413952, -0.999396, 0.000000, 0.034740, 0.000000, 1.000000, 0.000000, -0.034740, 0.000000, -0.999396, 0.000000, 1525.550246, 0.000000, 32.992224, -106.988837";
    // len = 282
    char * sus = "134.630000, 1169.425196, 18.041010, 0.139173, 135000.727687, -975.959193, -29.905568, -8320.892422, -128.368450, -0.990268, 277.331854, 0.235923, 0.000000, 0.971772, 0.000000, 1.000000, 0.000000, -0.971772, 0.000000, 0.235923, 0.000000, 1514.805315, 0.000000, 32.993587, -106.998530";
    //char * tst = "clock, x_dist, x_vel, x_accel, y_dist, y_vel, y_accel, z_dist, z_vel, z_accel, temperature, gyro_x, gyro_y, gyro_z";
    //char * tst2 = "reallylongteststring\n";
    /* create the FIFO (named pipe) */
    //mkfifo(myfifo, 0666);

    /* write "Hi" to the FIFO */
    fd = open(myfifo, O_WRONLY | O_CREAT, 0644);
    for (i=0;i<483647;i++) {
      //sprintf(tmp,"%11d: ", i);
      //sleep(1);
      //printf("sizeof string: %lu\n", sizeof(tst));
      printf("writing %d: %s\n", i, sus);
      //write(fd, tmp, sizeof(tmp));
      write(fd, sus, 282);
      write(fd, "\n", 1);
    }
    close(fd);

    /* remove the FIFO */
    //unlink(myfifo);

    return 0;
}