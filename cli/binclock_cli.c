#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
// Device node.
int fd;
struct termios oldtio;
int baudrate = B115200;

const int num_alarms = 8;

void printf_information()
{
    uint8_t a; 


    printf("== Alarms ==\n");
    for(a = 0; a < num_alarms; a++ ) {
        char buffer[32] = { 'a', '0'+a, 'r' };
        ssize_t t = 0;
        while(t < 3) {
            t+=write(fd, &buffer[t], 3-t);
        }

        t = 0;
        while(t < 4) {
            t +=read(fd, &buffer[t], 4-t);
        }
        buffer[t] = '\0';
        printf("%d: %02d:%02d ( %7s ) (%3s)\n", a+1, buffer[0]%24, buffer[1],
            (buffer[2])?"Enable":"Disable",
            (buffer[3])?"Ack":"");
        
    }


}

void handle_init ( )
{
    char buffer[9];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    printf("== Setting time: %02d:%02d:%02d ==\n",
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    sprintf(buffer, "sw%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    ssize_t rl = 0;
    while(rl < 8) {
        rl += write(fd, &buffer[rl], 8-rl);
    }

    sprintf(buffer, "sr");
    rl = 0;
    while(rl < 2) {
        rl += write(fd, &buffer[rl], 2-rl);
    }
    rl = 0;
    while(rl < 3) {
        rl += read(fd, &buffer[rl], 3-rl);
    }
    printf("   Time set to: %02d:%02d:%02d\n", buffer[0], buffer[1], buffer[2]);

}

void handle_drift()
{
    char buffer[9];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    sprintf(buffer, "sr");
    ssize_t rl = 0;
    while(rl < 2) {
        rl += write(fd, &buffer[rl], 2-rl);
    }
    rl = 0;
    while(rl < 3) {
        rl += read(fd, &buffer[rl], 3-rl);
    }

    int diff = (buffer[0]- tm->tm_hour)*3600+(buffer[1]-tm->tm_min)*60+(buffer[2]-tm->tm_sec);
    printf("== Drift: %d ==\n", diff);

    printf("   Local set to: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec); 
    printf("   Clock set to: %02d:%02d:%02d\n", buffer[0], buffer[1], buffer[2]);
}

void handle_temperature ( )
{
    char buffer[8] = {'t'};
    write(fd, buffer, 1);
}

void handle_alarm ( int argc, char **argv)
{
    if(argc == 0 ) {

        return;
    }
    long int alarm = strtol(argv[0], NULL, 10)-1;
    if(alarm < 0  || alarm >= num_alarms) {
        fprintf(stderr, "Invalid alarm number.\n");
        return;
    }
    // Skip.
    argv++;
    argc--;

    if(argc == 0 ) {
        // Read time
        char buffer[32] = { 'a', '0'+alarm, 'r' };
        write(fd, buffer, 3);
        ssize_t t = 0;
        while(t < 3) {
            t +=read(fd, &buffer[t], 2);
        }
        buffer[t] = '\0';
        printf("%02d:%02d (%s)\n", buffer[0]%24, buffer[1],
            (buffer[2])?"Enable":"Disable");
    } else {
        char *command = argv[0];
        argv++;
        argc--;
        if(strcasecmp(command,"enable") == 0 ) {
            char buffer[32] = { 'a', '0'+alarm, 'e' };
            write(fd, buffer, 3);
        } else if (strcasecmp(command, "disable") == 0) {
            char buffer[32] = { 'a', '0'+alarm, 'd' };
            write(fd, buffer, 3);
        } else if (strcasecmp(command, "set") == 0 ) {
            if (argc == 2) {
                int hour = strtol(argv[0], NULL, 10);
                int minute = strtol(argv[1], NULL, 10);
                char buffer[16];
                sprintf(buffer,"a%dw%02d%02d", alarm, hour, minute);
                write(fd, buffer, 7);

            }else {
                fprintf(stderr, "Invalid number of arguments\n");
            }
        }

    }

}

int main ( int argc, char **argv )
{
    char *dev_serial = getenv("BC_DEV");
    if(dev_serial == NULL) {
        dev_serial = "/dev/ttyACM0";
    }

    fd = open ( dev_serial, O_RDWR | O_NOCTTY );
    if ( fd < 0 )
    {
        // TODO: THROW error
        fprintf(stderr, "Failed to open device: %s\n", dev_serial);
        return EXIT_FAILURE;
    }
    // save status port settings.
    tcgetattr ( fd, &oldtio );

    // Setup the serial port.
    struct termios newtio = { 0, };
    newtio.c_cflag     = baudrate | CS8 | CREAD | PARODD;
    newtio.c_iflag     = 0;
    newtio.c_oflag     = 0;
    newtio.c_lflag     = 0;       //ICANON;
    newtio.c_cc[VMIN]  = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush ( fd, TCIFLUSH | TCIOFLUSH );
    tcsetattr ( fd, TCSANOW, &newtio );

    if( argc == 1) {
        printf_information();

    } else if (argc > 1 ) {
        const char *command = argv[1]; 
        // Alarm 
        if(strcasecmp(command, "alarm")  == 0 ) {
           handle_alarm( argc-2, &argv[2]); 
        // Initialize
        } else if (strcasecmp(command, "init") == 0) {
            handle_init();
        }
        else if(strcasecmp(command, "temperature")  == 0 ) {
            handle_temperature();
        }
        else if(strcasecmp(command, "drift")  == 0 ) {
            handle_drift();
        }

    }






    sync(fd);
   // tcsetattr ( fd, TCSANOW, &oldtio );
    close(fd);
    return EXIT_SUCCESS;
}
