
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdio>

#include "motor.hpp"

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include <osc/OscOutboundPacketStream.h>
#include "ip/UdpSocket.h"

#ifndef HOSTNAME
#define HOSTNAME "unknown"
#endif


#define MAX_MOTORS      3
#define DEFAULT_PORT    9292
//#define BROADCAST_ADDR          "255.255.255.255"
#define DEFAULT_RESPONSE_PORT   9393



#define DEFAULT_ADDRESS 1
// 4 = MobSpkr::Motor::MicroStepResolution_16
#define STEPSIZE_RESOLUTION 4
#define INTERPOLATION  1
#define MAX_CURRENT	128
#define POWER_DOWN_DELAY_10MS 20
#define PULSE_DIVISOR 6
#define RAMP_DIVISOR 8
#define MAX_ACCELERATION 200
#define TIMEOUT_MS 1000

// the number of steps required for a complete rotation given the above configuration
#define NSTEPS_ONE_ROTATION 3200



static char * argv0;

static struct {
    struct {
        char * name;
        int address;
        bool direction_right;
    } motors[MAX_MOTORS];
    int port;
    int response_port;
} opts {
    .port = DEFAULT_PORT,
    .response_port = DEFAULT_RESPONSE_PORT
};

static int motor_count = 0;
static MobSpkr::Motor motors[MAX_MOTORS];
static int32_t current_movement[MAX_MOTORS];

static int init_motor(int motor);
static int set_motor_msr(int motor, int msr);

static void print_usage(FILE * f){
    fprintf(f,
            "Usage: %s <motor1-path> <motor2-path> ...\n"
            "Start OSC server to act as proxy for given motors (max %d)\n"
            "Options:\n"
            "\t -p,--port <port>\t OSC server port (default %d)\n"
            "\t -r, --response-port <port>\t OSC response port (default %d)\n"
            "\t -a, --addr <motor-index>:<addr1>\n"
            "\t\t\t Set address of given motor (default %d)\n"
            "\t -d, --dir <motor-index>:[l,r]\n"
            "\t\t\t Set direction of given motor to turn left or right\n"
            "Note:\n"
            "\t Compiled with hostname %s\n"
//            "\t Sending responses to %s\n"
            , argv0, MAX_MOTORS, DEFAULT_PORT, DEFAULT_RESPONSE_PORT, DEFAULT_ADDRESS, HOSTNAME);
}


class packet_listener : public osc::OscPacketListener {
protected:

    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try{

            printf("OSC rx %s\n", m.AddressPattern());

            if (std::strcmp(m.AddressPattern(), "/motor/init") == 0){

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                if( arg != m.ArgumentsEnd() )
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index){
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

		        printf("RE-INIT MOTOR %d\n", motor_index);
                if (init_motor(motor_index))
		            printf("failed\n");
            }

            if (std::strcmp( m.AddressPattern(), "/motor/stop") == 0){

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                if( arg != m.ArgumentsEnd() )
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index){
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

                motors[motor_index].command_stopMotor(TIMEOUT_MS);
                current_movement[motor_index] = 0;
            }


            if (std::strcmp( m.AddressPattern(), "/motor/reset-position") == 0){

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                if( arg != m.ArgumentsEnd() )
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index){
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

                motors[motor_index].command_stopMotor(TIMEOUT_MS);
                current_movement[motor_index] = 0;
                motors[motor_index].command_setAxisParam_ActualPosition(0, TIMEOUT_MS);
            }

            if (std::strcmp( m.AddressPattern(), "/motor/move-by-angle") == 0) {

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                int angle = (arg++)->AsInt32();
                if (arg != m.ArgumentsEnd())
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index) {
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

                if (angle < -360 || 360 < angle) {
                    fprintf(stderr, "Invalid angle: %d [-360, 360]\n", angle);
                    return;
                }

                int32_t pos_target = (angle * NSTEPS_ONE_ROTATION) / 360;

                motors[motor_index].command_moveToPosition(pos_target, MobSpkr::Motor::MovementType_Relative, 0, TIMEOUT_MS);
            }

            if (std::strcmp( m.AddressPattern(), "/motor/move-to-angle") == 0){

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();
                int angle = (arg++)->AsInt32();
                if( arg != m.ArgumentsEnd() )
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index){
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

                if (angle < -360 || 360 < angle){
                    fprintf(stderr, "Invalid angle: %d [-360, 360]\n", angle);
                    return;
                }

                int32_t inverted = 0;
                if (angle < 0){
                    inverted = 1;
                    angle = 360 + angle;
                }

                int32_t desired_angled = (angle * NSTEPS_ONE_ROTATION) / 360;
                fprintf(stderr, "angle %d (%d)\n", angle, desired_angled);


                int32_t pos;
                MobSpkr::Motor::Response::Status status;
                status = motors[motor_index].command_getAxisParam_ActualPosition(pos, TIMEOUT_MS);

                fprintf(stderr, "getting current pos ");
                if (status != MobSpkr::Motor::Response::Status::Success){
                    fprintf(stderr, "failed\n");
                    return;
                }
                fprintf(stderr,"-> %d\n", pos);

                int32_t current_angle = pos % NSTEPS_ONE_ROTATION;
                int32_t pos_base = pos - current_angle;

                int32_t pos_target = 0;

                // if rotating "right" position increments, thus we go for the next bigger possible position, otherwise the next smaller one
                // treat not-rotating as right-rotation
                if (current_movement[motor_index] >= 0){
                    if (current_angle > desired_angled){
                        pos_target = pos_base + NSTEPS_ONE_ROTATION + desired_angled;
                    } else {
                        pos_target = pos_base + desired_angled;
                    }
                    if (inverted){
                        pos_target -= NSTEPS_ONE_ROTATION;
                    }
                } else {
                    if (current_angle < desired_angled){
                        pos_target = pos_base - NSTEPS_ONE_ROTATION + desired_angled;
                    } else {
                        pos_target = pos_base + desired_angled;
                    }
                    if (inverted){
                        pos_target += NSTEPS_ONE_ROTATION;
                    }
                }

                fprintf(stderr, "moving to absolute pos %d\n", pos_target);

                motors[motor_index].command_moveToPosition(pos_target, MobSpkr::Motor::MovementType_Absolute, 0, TIMEOUT_MS);
            }



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
                if (opts.motors[motor_index].direction_right){
                    motors[motor_index].command_rotateRight(velocity, TIMEOUT_MS);
                    current_movement[motor_index] = velocity;
                } else {
                    motors[motor_index].command_rotateLeft(velocity, TIMEOUT_MS);
                    current_movement[motor_index] = -velocity;
                }

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

		        fprintf(stderr, "setting motor %d msr = %d\n", motor_index, msr);
                if (set_motor_msr(motor_index, msr))
                    fprintf(stderr, "failed\n");
            }


            if (std::strcmp(m.AddressPattern(), "/motor/temp") == 0) {

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();

                const char *host = (arg++)->AsString();
                int port = (arg++)->AsInt32();

                if (arg != m.ArgumentsEnd())
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index) {
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

                // try to create transmit socket
                fprintf(stderr, "UDP response addr = %s:%u\n", host, port);
                UdpTransmitSocket transmitSocket( IpEndpointName( host, port ) );

                fprintf(stderr, "Getting motor %d temp ...", motor_index);
                uint32_t temp = 0;
                if (motors[motor_index].command_getGIOTemperature(temp, TIMEOUT_MS) != MobSpkr::Motor::Response::Status::Success)
                    fprintf(stderr, "FAILED\n");
                else
                    fprintf(stderr, "%d deg C\n",temp);

                char buffer[256];
                osc::OutboundPacketStream p( buffer, sizeof(buffer) );

                p << osc::BeginMessage( "/temp" )
                    << HOSTNAME << motor_index << (int)temp
                    << osc::EndMessage;

                transmitSocket.Send( p.Data(), p.Size() );
            }

            if (std::strcmp(m.AddressPattern(), "/motor/volt") == 0) {

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();

                const char *host = (arg++)->AsString();
                int port = (arg++)->AsInt32();

                if (arg != m.ArgumentsEnd())
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || motor_count <= motor_index) {
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, motor_count - 1);
                    return;
                }

                // try to create transmit socket
                fprintf(stderr, "UDP response addr = %s:%u\n", host, port);
                UdpTransmitSocket transmitSocket( IpEndpointName( host, port ) );

                fprintf(stderr, "Getting motor %d volt ...", motor_index);
                uint32_t voltage = 0;
                if (motors[motor_index].command_getGIOVoltage(voltage, TIMEOUT_MS) != MobSpkr::Motor::Response::Status::Success)
                    fprintf(stderr, "FAILED\n");
                else
                    fprintf(stderr, "%d.%d\n",voltage/10, voltage%10);

                char buffer[256];
                osc::OutboundPacketStream p( buffer, sizeof(buffer) );

                p << osc::BeginMessage( "/volt" )
                  << HOSTNAME << motor_index << (int)voltage
                  << osc::EndMessage;

                transmitSocket.Send( p.Data(), p.Size() );
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

int set_motor_msr(int motor, int msr)
{
    printf("microstep resolution MSR = %d\n", msr);
    if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_MicroStepResolution((MobSpkr::Motor::MicroStepResolution)msr, TIMEOUT_MS)){
        fprintf(stderr, "failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int init_motor(int motor)
{
    current_movement[motor] = 0;

#if INTERPOLATION == 1 && STEPSIZE_RESOLUTION == 4
        printf("interpolation = %d\n", INTERPOLATION);
        if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_Interpolation(1, TIMEOUT_MS)){
            fprintf(stderr, "failed\n");
            return EXIT_FAILURE;
        }
#endif
    printf("max current = %d\n", MAX_CURRENT);
    if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_MaxCurrent(MAX_CURRENT, TIMEOUT_MS)){
        fprintf(stderr, "failed\n");
        return EXIT_FAILURE;
    }
    printf("power down delay = %d (10ms = %d)\n", POWER_DOWN_DELAY_10MS, POWER_DOWN_DELAY_10MS);
    if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_PowerDownDelay(POWER_DOWN_DELAY_10MS, TIMEOUT_MS)){
        fprintf(stderr, "failed\n");
        return EXIT_FAILURE;
    }
    printf("Pulse divisor = %d\n", PULSE_DIVISOR);
    if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_PulseDivisor(PULSE_DIVISOR, TIMEOUT_MS)){
        fprintf(stderr, "failed\n");
        return EXIT_FAILURE;
    }
    printf("ramp divisor = %d\n", RAMP_DIVISOR);
    if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_RampDivisor(RAMP_DIVISOR, TIMEOUT_MS)){
        fprintf(stderr, "failed\n");
        return EXIT_FAILURE;
    }
    printf("max acceleration = %d\n", MAX_ACCELERATION);
    if (MobSpkr::Motor::Response::Status::Success != motors[motor].command_setAxisParam_MaxAcceleration(MAX_ACCELERATION, TIMEOUT_MS)){
        fprintf(stderr, "failed\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

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
                {"response-port", required_argument, 0, 'r'},
                {"addr",     required_argument, 0,  'a' },
                {"dir", required_argument, 0, 'd'},
                {0,         0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "h?p:r:a:d:",
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

            case 'r': // --port
                opts.response_port = std::atoi(optarg);
                if (opts.response_port < 1 || 0xffff < opts.response_port) {
                    fprintf(stderr, "invalid response port: %d\n", opts.response_port);
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
	printf("INITIALIZING MOTOR %d: %s\n", i, motors[i].get_portname());
        if (!motors[i].open()){
            fprintf(stderr, "failed\n");
            goto stopping;
        }
        printf("Connected using address %d\n", motors[i].get_address());

        if (init_motor(i))
            return EXIT_FAILURE;

        if (set_motor_msr(i, STEPSIZE_RESOLUTION))
            return EXIT_FAILURE;

    }


    printf("press Ctrl+C (SIGINT) to stop\n");
    osc_rx_socket.RunUntilSigInt();

stopping:

    for(int i = 0; i < motor_count; i++){
        motors[i].close();
    }


    return EXIT_SUCCESS;
}
 
