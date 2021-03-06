#include "../motor.hpp"

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <getopt.h>

static char * argv0;

static void print_usage(FILE * out){
    fprintf(out,
            "Usage: %s [<options> ...]\n"
            "\t -s,--serial=<motor-serial> Open motor through given serial\n"
            "\t -c, --close\t Closes serial port\n"
            "\t -a,--address=<address>\n"
            "\t -w, --wait=[<msec>]\t Wait <msec> until next command if given, or until pressed enter\n"
            "\t -q, --stop\t Stop motor\n"
            "\t --ror=<velocity>\t Rotate right, velocity in [-2049, 2049]\n"
            "\t --rol=<velocity>\t Rotate left, velocity in [-2049, 2049]\n"
            "\t --msr=[<resolution>]\t Get/set axis parameter microstep resolution [8 => 256, 7 => 128, etc]\n"
            "\t --volt\t Get voltage\n"
            "\t --temp\t Get temperature\n"
            "\t --pos [<pos>]\t Get/Set actual position (motor must be stopped)\n"
            "\t --move-to <pos>\t Move to absolute position\n"
            "\t --move-by <pos>\t Move to relative position\n"
            "\t --standby-current <cur>\t Set standby current\n"
            "Examples:\n"
            "%s -s/dev/cu.usbmodemTMCSTEP1 --msr --msr=7 --msr\n",
            argv0, argv0);
}

int main(int argc, char * argv[]){

    argv0 = argv[0];

    if (argc <= 1){
        print_usage(stdout);
        return EXIT_SUCCESS;
    }

    MobSpkr::Motor motor(NULL, 1);

    unsigned int timeout_ms = 1000;

    int c;
    int digit_optind = 0;

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"address",     required_argument, 0,  'a' },
                {"serial", required_argument, 0, 's'},
                {"close", no_argument, 0, 'c'},
                {"wait", optional_argument, 0, 'w'},
                {"ror", required_argument, 0, 1},
                {"rol", required_argument, 0, 2},
                {"msr", optional_argument, 0, 3},
                {"volt", optional_argument, 0, 4},
                {"temp", optional_argument, 0, 5},
                {"pos", optional_argument, 0, 6},
                {"move-to", required_argument, 0, 7},
                {"move-by", required_argument, 0, 8},
                {"stop", no_argument, 0, 'q'},
                {"standby-current", required_argument, 0, 9},
                {0,         0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "h?a:s:cw::q",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {

            case 'h':
            case '?':
                print_usage(stdout);
                return EXIT_SUCCESS;

            case 'a': {// --address
                int address = std::atoi(optarg);
                if (address < 1 || 255 < address){
                    fprintf(stderr, "invalid motor address: %d\n", address);
                }
                motor.set_address(address);
                break;
            }

            case 's': { // --serial
                motor.close();

                motor.set_portname(optarg);

                printf("Connecting to motor %s ", motor.get_portname());
                if (!motor.open()){
                    printf("FAILED\n");
                    return EXIT_FAILURE;
                }
                printf("OK\n");
                break;
            }

            case 'c': // --close
                if (motor.is_open()){
                    printf("Already closed\n");
                } else {
                    printf("Closing\n");
                    motor.close();
                }
                break;

            case 'w': { // --wait [<msec>]
                if (optarg){
                    int msec = std::atoi(optarg);
                    if (msec <= 0){
                        fprintf(stderr, "invalid wait time: %d\n", msec);
                        goto fail;
                    }
                    printf("Waiting %d msec\n", msec);
                    usleep(1000*msec);
                } else {
                    char c;
                    printf(">> Waiting for ENTER <<\n");
                    read(STDIN_FILENO, &c, 1);
                }
                break;
            }

            case 'q': { // -q, --stop
                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }
                printf("Motor STOP ");
                if (motor.command_stopMotor(timeout_ms) != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            case 1: {// --ror <velocity>
                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }
                int v = std::atoi(optarg);
                if (v < -2049 || 2049 < v){
                    fprintf(stderr, "Invalid velocity range (must be in [-2049, 2049]): %d\n", v);
                    break;
                }
                printf("Rotate right %d ", v);
                if (motor.command_rotateRight(v, timeout_ms) != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            case 2: {// --rol <velocity>
                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }
                int v = std::atoi(optarg);
                if (v < -2049 || 2049 < v){
                    fprintf(stderr, "Invalid velocity range (must be in [-2049, 2049]): %d\n", v);
                    goto fail;
                }
                printf("Rotate left %d ", v);
                if (motor.command_rotateLeft(v, timeout_ms) != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            case 3: {// --msr [<resolution>]
                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }

                enum MobSpkr::Motor::MicroStepResolution r;
                MobSpkr::Motor::Response::Status status;
                if (optarg){
                    r = (enum MobSpkr::Motor::MicroStepResolution)std::atoi(optarg);
                    if (r < 1 || 8 < r){
                        fprintf(stderr, "Invalid velocity range (must be in [1, 8], ie 8 -> 256, 7 -> 128, ..): %d\n", r);
                        goto fail;
                    }
                    printf("Set axis param (micro step resolution) %d ", r);
                    status = motor.command_setAxisParam_MicroStepResolution(r, timeout_ms);
                } else {
                    printf("Get axis param (micro step resolution) ");
                    status = motor.command_getAxisParam_MicroStepResolution(r, timeout_ms);
                    if (status == MobSpkr::Motor::Response::Status::Success)
                        printf("-> %d ", r);
                }
                if (status != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            case 4:{ // --volt

                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }

                printf("Get voltage ");
                uint32_t voltage = 0;
                if (motor.command_getGIOVoltage(voltage, timeout_ms) != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else 
                    printf("OK %d.%d\n",voltage/10, voltage%10);
                
		break;

           }

            case 5: {// --temp

                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }

                printf("Get voltage ");
                uint32_t temp = 0;
                if (motor.command_getGIOTemperature(temp, timeout_ms) != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK %d deg C\n",temp);

                break;

           }

           case 6: { // --pos

               if (!motor.is_open()){
                   fprintf(stderr, "Command before was connected to motor!\n");
                   goto fail;
               }

               int32_t pos;
               MobSpkr::Motor::Response::Status status;
               if (optarg){
                   pos = std::atoi(optarg);
                   if (pos < -2147483648 || 2147483647 < pos){
                       fprintf(stderr, "Invalid absolute position (must be in [-2147483648, 2147483647]): %d\n", pos);
                       goto fail;
                   }
                   printf("Set axis param (actual position) %d ", pos);
                   status = motor.command_setAxisParam_ActualPosition(pos, timeout_ms);
               } else {
                   printf("Get axis param (actual position) ");
                   status = motor.command_getAxisParam_ActualPosition(pos, timeout_ms);
                   if (status == MobSpkr::Motor::Response::Status::Success)
                       printf("-> %d ", pos);
               }
               if (status != MobSpkr::Motor::Response::Status::Success)
                   printf("FAILED\n");
               else
                   printf("OK\n");

               break;
           }

            case 7: { // --move-to
                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }

                int32_t pos = std::atoi(optarg);
                if (pos < -2147483648 || 2147483647 < pos){
                    fprintf(stderr, "Invalid absolute position (must be in [-2147483648, 2147483647]): %d\n", pos);
                    goto fail;
                }
                printf("Moving to absolute position %d ", pos);
                MobSpkr::Motor::Response::Status status = motor.command_moveToPosition(pos, MobSpkr::Motor::MovementType_Absolute, 0, timeout_ms);
                if (status != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            case 8: { // --move-by
                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }

                int32_t pos = std::atoi(optarg);
                if (pos < -2147483648 || 2147483647 < pos){
                    fprintf(stderr, "Invalid relative position (must be in [-2147483648, 2147483647]): %d\n", pos);
                    goto fail;
                }
                printf("Moving by relative position %d ", pos);
                MobSpkr::Motor::Response::Status status = motor.command_moveToPosition(pos, MobSpkr::Motor::MovementType_Relative, 0, timeout_ms);
                if (status != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            case 9: { // --standby-current

                if (!motor.is_open()){
                    fprintf(stderr, "Command before was connected to motor!\n");
                    goto fail;
                }

                int32_t value = std::atoi(optarg);
                if (value < 0 || 255 < value){
                    fprintf(stderr, "Invalid standby current (must be in [0, 255]): %d\n", value);
                    goto fail;
                }
                printf("Setting standby current %d ", value);
                MobSpkr::Motor::Response::Status status = motor.command_setAxisParam_StandbyCurrent(value, timeout_ms);
                if (status != MobSpkr::Motor::Response::Status::Success)
                    printf("FAILED\n");
                else
                    printf("OK\n");

                break;
            }

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

fail:

    motor.close();

//    uint8_t rx[9];
//
//    if (motor.execute_raw((uint8_t*)MobSpkr::PD_1160::GetVersion1, rx, 1000) != MobSpkr::Motor::Response::Success){
//        fprintf(stderr, "Failed getVersion1: %d\n", rx[MobSpkr::Motor::Response::STATUS]);
//        goto fail;
//    }

//    printf("GetVersion1: 0x%08x\n", response.value());
//
//    if (motor.execute(MobSpkr::PD_1160::GetVersion0, &response, 1000) != MobSpkr::Motor::Response::Success){
//        fprintf(stderr, "Failed getVersion0: %d\n", response.status());
//        goto fail;
//    }
//
//    printf("GetVersion0: 0x%08x\n", response.value());


    return EXIT_SUCCESS;
}
