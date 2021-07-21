#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>

#include <pigpio.h>

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"

/*
# servo_demo.c
# 2016-10-08
# Public Domain

gcc -Wall -pthread -o servo_demo servo_demo.c -lpigpio

sudo ./servo_demo          # Send servo pulses to GPIO 4.
sudo ./servo_demo 23 24 25 # Send servo pulses to GPIO 23, 24, 25.
*/

#define DEFAULT_PORT    9393

#define NUM_GPIO 32

#define MIN_WIDTH 760
#define MAX_WIDTH 2240

#define WIDTH (MAX_WIDTH - MIN_WIDTH)
#define CENTER_WIDTH (MIN_WIDTH + WIDTH / 2)


static char * argv0;

static struct {
    int port;
} opts {
    .port = DEFAULT_PORT
};

//static int run = 1;

static struct {
    int used;
    int step;
    int width;
} pwms[NUM_GPIO];

//static int randint(int from, int to)
//{
//   return (random() % (to - from + 1)) + from;
//}

//static void stop(int signum)
//{
//   run = 0;
//}

static void print_usage(FILE * f){
    fprintf(f,
            "Usage: %s <gpio1> <gpio2> ...\n"
            "Start OSC server to act as proxy for GPIO pins acting as PWM\n"
            "Options:\n"
            "\t -p,--port <port>\t OSC server port (default %d)\n"
            ,argv0, DEFAULT_PORT);
}

static int position_map(float posf)
{
    if (posf < 0.0){
        posf = 0.0;
    } else if (posf > 1.0){
        posf = 1.0;
    }

    int posi = posf * WIDTH + MIN_WIDTH;

    return posi;
}

class packet_listener : public osc::OscPacketListener {
        protected:

        virtual void ProcessMessage( const osc::ReceivedMessage& m,
        const IpEndpointName& remoteEndpoint )
        {
            (void) remoteEndpoint; // suppress unused parameter warning

            try{

                printf("OSC rx %s\n", m.AddressPattern());

                if (std::strcmp( m.AddressPattern(), "/pwm") == 0){

                    osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                    int pwm_index = (arg++)->AsInt32();
                    float position = (arg++)->AsFloat();
                    if( arg != m.ArgumentsEnd() )
                        throw osc::ExcessArgumentException();

                    if (pwm_index < 0 || NUM_GPIO <= pwm_index){
                        fprintf(stderr, "Invalid pwm index: %d\n", pwm_index);
                        return;
                    }
                    if (pwms[pwm_index].used == 0){
                        fprintf(stderr, "pwm %d NOT used, ignoring\n", pwm_index);
                        return;
                    }

                    int w = position_map(position);

printf("Position %f %d\n", position, w);

                    // don't update if unchannged value
                    if (pwms[pwm_index].width == w){
                        return;
                    }

printf("updating!\n");
                    pwms[pwm_index].width = w;

                    gpioServo(pwm_index, w);

                }


//            if( std::strcmp( m.AddressPattern(), "/test1" ) == 0 ){
//                // example #1 -- argument stream interface
//                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
//                bool a1;
//                osc::int32 a2;
//                float a3;
//                const char *a4;
//                args >> a1 >> a2 >> a3 >> a4 >> osc::EndMessage;
//
//                std::cout << "received '/test1' message with arguments: "
//                          << a1 << " " << a2 << " " << a3 << " " << a4 << "\n";
//
//            }else if( std::strcmp( m.AddressPattern(), "/test2" ) == 0 ){
//                // example #2 -- argument iterator interface, supports
//                // reflection for overloaded messages (eg you can call
//                // (*arg)->IsBool() to check if a bool was passed etc).
//                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
//                bool a1 = (arg++)->AsBool();
//                int a2 = (arg++)->AsInt32();
//                float a3 = (arg++)->AsFloat();
//                const char *a4 = (arg++)->AsString();
//
//                std::cout << "received '/test2' message with arguments: "
//                          << a1 << " " << a2 << " " << a3 << " " << a4 << "\n";
//            }
            }catch( osc::Exception& e ){
                // any parsing errors such as unexpected argument types, or
                // missing arguments get thrown as exceptions.
                fprintf(stderr, "error while parsing message: %s: %s\n", m.AddressPattern(), e.what());
            }
        }
};


int main(int argc, char *argv[])
{
    argv0 = argv[0];

    memset(pwms, 0, sizeof(pwms));

    int c;
    int digit_optind = 0;

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"port",     required_argument, 0,  'p' },
                {"gpio",     required_argument, 0,  'g' },
                {0,         0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "h?p:g:",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {

            case 'p': // --port <port>
                opts.port = std::atoi(optarg);
                if (opts.port < 1 || 0xffff < opts.port) {
                    fprintf(stderr, "invalid port: %d\n", opts.port);
                    return EXIT_FAILURE;
                }
                break;

            case 'h':
            case '?':
                print_usage(stdout);
                return EXIT_SUCCESS;
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    if (optind == argc) {
        fprintf(stderr, "Missing arguments. Try %s -h\n", argv0);
        return EXIT_FAILURE;
    }
    if (argc - optind > NUM_GPIO){
        fprintf(stderr, "Too many gpios (max %d)\n", NUM_GPIO);
        return EXIT_FAILURE;
    }


    while(optind < argc){
        int pin = atoi(argv[optind++]);
        if (pin < 0 || NUM_GPIO <= pin){
            fprintf(stderr, "Invalid pin number! %d\n", pin);
            return EXIT_FAILURE;
        }
        pwms[pin].used = 1;
    }

    int i, g;

   if (gpioInitialise() < 0) return -1;

//   gpioSetSignalFunc(SIGINT, stop);

   printf("Sending servos pulses to GPIO");

   for (g=0; g<NUM_GPIO; g++)
   {
      if (pwms[g].used)
      {
         printf(" %d", g);
         pwms[g].width = CENTER_WIDTH;
         gpioServo(g, CENTER_WIDTH);

      }
   }

    // initialize before motor opening
    packet_listener listener;
    UdpListeningReceiveSocket osc_rx_socket(IpEndpointName( IpEndpointName::ANY_ADDRESS, opts.port ),&listener );

//   printf(", control C to stop.\n");

//   while(run)
//   {
//      for (g=0; g<NUM_GPIO; g++)
//      {
//         if (used[g])
//         {
//            gpioServo(g, width[g]);
//
//            // printf("%d %d\n", g, width[g]);
//
//            width[g] += step[g];
//
//            if ((width[g]<MIN_WIDTH) || (width[g]>MAX_WIDTH))
//            {
//               step[g] = -step[g];
//               width[g] += step[g];
//            }
//         }
//      }
//
//      time_sleep(0.1);
//   }

    printf("press Ctrl+C (SIGINT) to stop\n");
    osc_rx_socket.RunUntilSigInt();


   printf("\ntidying up\n");

   for (g=0; g<NUM_GPIO; g++)
   {
      if (pwms[g].used) gpioServo(g, 0);
   }

   gpioTerminate();

   return 0;
}

