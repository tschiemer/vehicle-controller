#include "../motor.hpp"

#include <cstdlib>
#include <cstdio>

static char * argv0;

static void print_help(FILE * out){
    fprintf(out,
            "Usage: %s <serialname> <address> <cmd>..\n"
            "Where <cmd> can be:\n"
            "\t rotate-right",
            argv0);
}

int main(int argc, char * argv[]){

    argv0 = argv[0];

    if (argc != 3){
        print_help(stdout);
        return EXIT_FAILURE;
    }

    char * serialname = argv[1];
    int address = atoi(argv[2]);

    MobSpkr::Motor motor(serialname, address);

    if (!motor.open()){
        fprintf(stderr, "failed to open motor serial: %s\n", serialname);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Opened motor: %s\n", serialname);

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

    MobSpkr::Motor::Response response;

    if (motor.execute_with_value(MobSpkr::PD_1160::GetAxisParam_MicroStepResolution, 0, &response, 1000) != MobSpkr::Motor::Response::Success){
        fprintf(stderr, "Failed RotateRight: %d\n", response.status());
        goto fail;
    }

    printf("RotateRight: 0x%08x\n", response.value());


fail:

    motor.close();

    return EXIT_SUCCESS;
}