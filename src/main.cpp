
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdio>

#include "motor.hpp"

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"

#define MAX_MOTORS      2
#define DEFAULT_PORT    9292
#define DEFAULT_ADDRESS 1
#define STEPSIZE_RESOLUTION 6

static char * argv0;

static struct {
    struct {
        char * name;
        int address;
        bool direction_right;
    } motors[MAX_MOTORS];
    int port;
} opts {
    .port = DEFAULT_PORT
};

static int motor_count = 0;
static MobSpkr::Motor motors[MAX_MOTORS];

static void print_usage(FILE * f){
    fprintf(f,
            "Usage: %s <motor1-path> <motor2-path> ...\n"
            "Start OSC server to act as proxy for given motors (max %d)\n"
            "Options:\n"
            "\t -p,--port <port>\t OSC server port (default %d)\n"
            "\t -a, --addr <motor-index>:<addr1>\n"
            "\t\t\t Set address of given motor (default %d)\n"
            "\t -d, --dir <motor-index>:[l,r]\n"
            "\t\t\t Set direction of given motor to turn left or right\n",
            argv0, MAX_MOTORS, DEFAULT_PORT, DEFAULT_ADDRESS);
}


class packet_listener : public osc::OscPacketListener {
protected:

    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try{

            printf("OSC rx %s\n", m.AddressPattern());

            if (std::strcmp( m.AddressPattern(), "/motor/rotate") == 0){

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                int velocity = (arg++)->AsInt32();
                if( arg != m.ArgumentsEnd() )
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index){
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }
                if (velocity < -2049 || 2049 < velocity){
                    fprintf(stderr, "Invalid velocity range: %d [-2049, 2049]\n", velocity);
                    return;
                }
                if (opts.motors[motor_index].direction_right)
                    motors[motor_index].command_rotateRight(velocity, 1000);
                else
                    motors[motor_index].command_rotateLeft(velocity, 1000);

            }

            if (std::strcmp(m.AddressPattern(), "/motor/msr") == 0){

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                enum MobSpkr::Motor::MicroStepResolution msr = (enum MobSpkr::Motor::MicroStepResolution)(arg++)->AsInt32();
                if( arg != m.ArgumentsEnd() )
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index){
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }
                if (msr < 1 || 8 < msr){
                    fprintf(stderr, "Invalid microstrep resolution range: %d [1, 8]\n", msr);
                    return;
                }

                motors[motor_index].command_setAxisParam_MicroStepResolution(msr, 1000);
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

int main(int argc, char * argv[])
{
    argv0 = argv[0];

    for(int i = 0; i < MAX_MOTORS; i++){
        opts.motors[i].address = DEFAULT_ADDRESS;
        opts.motors[i].direction_right = true;
    }

    int c;
    int digit_optind = 0;

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"port",     required_argument, 0,  'p' },
                {"addr",     required_argument, 0,  'a' },
                {"dir", required_argument, 0, 'd'},
                {0,         0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "h?p:a:d:",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {

            case 'p': // --port
                opts.port = std::atoi(optarg);
                if (opts.port < 1 || 0xffff < opts.port) {
                    fprintf(stderr, "invalid port: %d\n", opts.port);
                    return EXIT_FAILURE;
                }
                break;

            case 'a': {// --addr
                if (std::strlen(optarg) < 3 || std::strchr(optarg, ':') == NULL) {
                    fprintf(stderr, "invalid addr option\n");
                    return EXIT_FAILURE;
                }
                int motor_index = std::atoi(optarg);

                if (motor_index >= MAX_MOTORS) {
                    fprintf(stderr, "motor index too high (max %d)\n", MAX_MOTORS);
                    return EXIT_FAILURE;
                }

                int address = std::atoi(std::strchr(optarg, ':') + 1);

                if (address < 1 || 255 < address) {
                    fprintf(stderr, "invalid motor address, must be 1-255\n");
                    return EXIT_FAILURE;
                }
                opts.motors[motor_index].address = address;
                break;
            }

            case 'd': {// --dir
                if (std::strlen(optarg) < 3 || std::strchr(optarg, ':') == NULL) {
                    fprintf(stderr, "invalid addr option\n");
                    return EXIT_FAILURE;
                }
                int motor_index = std::atoi(optarg);

                if (motor_index >= MAX_MOTORS) {
                    fprintf(stderr, "motor index too high (max %d)\n", MAX_MOTORS);
                    return EXIT_FAILURE;
                }

                bool direction_right;
                if ( *(std::strchr(optarg, ':') + 1) == 'r')
                    direction_right = true;
                else if ( *(std::strchr(optarg, ':') + 1) == 'l')
                    direction_right = false;
                else {
                    fprintf(stderr, "invalid direction (must be r or l): %s\n", std::strchr(optarg, ':') + 1);
                    return EXIT_FAILURE;
                }

                opts.motors[motor_index].direction_right = direction_right;
                break;
            }

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
    if (argc - optind > MAX_MOTORS){
        fprintf(stderr, "Too many motors (max %d)\n", MAX_MOTORS);
        return EXIT_FAILURE;
    }

    while(optind < argc){
        motors[motor_count].set_portname(argv[optind++]);
        motors[motor_count].set_address(opts.motors[motor_count].address);
        motor_count++;
    }

    // initialize before motor opening
    packet_listener listener;
    UdpListeningReceiveSocket osc_rx_socket(IpEndpointName( IpEndpointName::ANY_ADDRESS, opts.port ),&listener );

    printf("Started OSC receiver at port %d\n", opts.port);

    for(int i = 0; i < motor_count; i++){
        if (!motors[i].open()){
            fprintf(stderr, "failed to start motor %d: %s\n", i, motors[i].get_portname());
            goto stopping;
        }
        printf("Connected to motor %s using address %d\n", motors[i].get_portname(), motors[i].get_address());

        if (MobSpkr::Motor::Response::Status::Success != motors[i].command_setAxisParam_MicroStepResolution(MobSpkr::Motor::MicroStepResolution_64, 1000)){
            fprintf(stderr, "failed to set microstepresolution to %d: %s\n", 1, motors[i].get_portname());
            goto stopping;
        }
    }


    printf("press Ctrl+C (SIGINT) to stop\n");
    osc_rx_socket.RunUntilSigInt();

stopping:

    for(int i = 0; i < motor_count; i++){
        motors[i].close();
    }


    return EXIT_SUCCESS;
}
 