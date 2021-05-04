#include "motor.hpp"

#include <cstdio>

namespace MobSpkr {

    bool Motor::open(int baudrate, int bits, enum sp_parity parity, int stopbits, enum sp_flowcontrol flowcontrol) {

        close();

        enum sp_return r;

        if ( (r = sp_get_port_by_name(m_portname, &m_port)) != SP_OK){
            std::fprintf(stderr, "sp_get_port_by_name(%s): %d\n", m_portname, r);
            goto open_failed;
        }

        if ( (r = sp_open(m_port, SP_MODE_READ_WRITE)) != SP_OK){
            std::fprintf(stderr, "sp_open(%s): %d\n", m_portname, r);
            goto open_failed;
        }

        if ( (r = sp_set_baudrate(m_port, baudrate)) != SP_OK){
            std::fprintf(stderr, "set_baudrate(%s, %d): %d\n", m_portname, baudrate, r);
            goto open_failed;
        }

        if ( (r = sp_set_bits(m_port, bits)) != SP_OK){
            std::fprintf(stderr, "sp_set_bits(%s, %d): %d\n", m_portname, bits, r);
            goto open_failed;
        }

        if ( (r = sp_set_parity(m_port, parity)) != SP_OK){
            std::fprintf(stderr, "sp_set_parity(%s, %d): %d\n", m_portname, parity, r);
            goto open_failed;
        }

        if ( (r = sp_set_stopbits(m_port, stopbits)) != SP_OK){
            std::fprintf(stderr, "sp_set_stopbits(%s, %d): %d\n", m_portname, stopbits, r);
            goto open_failed;
        }

        if ( (r = sp_set_flowcontrol(m_port, flowcontrol)) != SP_OK){
            std::fprintf(stderr, "sp_set_flowcontrol(%s, %d): %d\n", m_portname, flowcontrol, r);
            goto open_failed;
        }


        return true;

    open_failed:

        sp_free_port(m_port);
        m_port = NULL;

        return false;
    }

    void Motor::close() {
        if (m_port) {
            sp_close(m_port);
            sp_free_port(m_port);
            m_port = NULL;
        }
    }

    Motor::Response::Status Motor::execute_raw(uint8_t command[], uint8_t response[], unsigned int timeout_ms) {
        if (!is_open()){
            fprintf(stderr, "is not open?\n");
            return Response::Status::Error;
        }

        int r;


//        printf("tx (%d) ", Command::SIZE);
//        for(int i = 0; i < Command::SIZE; i++){
//            printf("%02x ", command[i]);
//        }
//        printf("\n");

        if ( (r = sp_blocking_write(m_port, command, Command::SIZE, timeout_ms)) < Command::SIZE ){
            fprintf(stderr, "sp_blocking_write(): %d\n", r);
            return Response::Status::Error;
        }

        if ( (r = sp_blocking_read(m_port, response, Response::SIZE, timeout_ms)) < Response::SIZE ){
            fprintf(stderr, "sp_blocking_read(): %d\n", r);
            return Response::Status::Error;
        }

//        printf("rx (%d) ", r);
//        for(int i = 0; i < r; i++){
//            printf("%02x ", response[i]);
//        }
//        printf("\n");

        return (Response::Status)response[Response::STATUS];
    }

    Motor::Response::Status Motor::command_rotateRight(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::RotateRight, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_rotateLeft(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::RotateLeft, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_getAxisParam_MicroStepResolution(enum MicroStepResolution & value, unsigned int timeout_ms){
        Response response;

        Response::Status status = execute_with_value(MobSpkr::PD_1160::GetAxisParam_MicroStepResolution, value, &response, 1000) ;

        if (status == Response::Status::Success){
            value = (enum MicroStepResolution)response.value();
        }

        return status;
    }

    Motor::Response::Status Motor::command_setAxisParam_MicroStepResolution(enum MicroStepResolution value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_MicroStepResolution, value, NULL, 1000) ;
    }

}