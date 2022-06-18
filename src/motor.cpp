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

    Motor::Response::Status Motor::command_stopMotor(unsigned int timeout_ms){
        return execute(MobSpkr::PD_1160::MotorStop, NULL, 1000);
    }

    Motor::Response::Status Motor::command_rotateRight(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::RotateRight, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_rotateLeft(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::RotateLeft, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_moveToPosition(int32_t pos, enum MovementType type, uint8_t coord, unsigned int timeout_ms){
        uint8_t cmd[sizeof(MobSpkr::PD_1160::MoveToPosition)];

        std::memcpy(cmd, MobSpkr::PD_1160::MoveToPosition, sizeof(MobSpkr::PD_1160::MoveToPosition));

        switch(type){
            case MovementType_Absolute:
                cmd[2] = MovementType_Absolute;
                cmd[3] = 0;
                break;
            case MovementType_Relative:
                cmd[2] = MovementType_Relative;
                cmd[3] = 0;
                break;
            case MovementType_Coordinate:
                cmd[2] = MovementType_Coordinate;
                cmd[3] = coord;
                break;
            default:
                return Motor::Response::InvalidValue;
        }

        return execute_with_value(cmd, pos, NULL, 1000);
    }


    Motor::Response::Status Motor::command_getAxisParam_ActualPosition(int32_t & value, unsigned int timeout_ms){
        Response response;

        Response::Status status = execute_with_value(MobSpkr::PD_1160::GetAxisParam_ActualPosition, value, &response, 1000) ;

        if (status == Response::Status::Success){
            value = response.value();
        }

        return status;
    }

    Motor::Response::Status Motor::command_setAxisParam_ActualPosition(int32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_ActualPosition, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_setAxisParam_MaxCurrent(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_MaxCurrent, value, NULL, 1000);
    }

    Motor::Response::Status Motor::command_setAxisParam_StandbyCurrent(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_StandbyCurrent, value, NULL, 1000);
    }

    Motor::Response::Status Motor::command_setAxisParam_PowerDownDelay(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_PowerDownDelay, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_setAxisParam_Interpolation(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_Interpolation, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_setAxisParam_PulseDivisor(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_PulseDivisor, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_setAxisParam_RampDivisor(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_RampDivisor, value, NULL, 1000) ;
    }

    Motor::Response::Status Motor::command_setAxisParam_MaxAcceleration(uint32_t value, unsigned int timeout_ms){
        return execute_with_value(MobSpkr::PD_1160::SetAxisParam_MaxAcceleration, value, NULL, 1000);
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


    Motor::Response::Status Motor::command_getGIOVoltage(uint32_t & value, unsigned int timeout_ms){
        Response response;

        Response::Status status = execute_with_value(MobSpkr::PD_1160::GetGIOVoltage, value, &response, 1000) ;

        if (status == Response::Status::Success){
            value = (enum MicroStepResolution)response.value();
        }

        return status;
    }

    Motor::Response::Status Motor::command_getGIOTemperature(uint32_t & value, unsigned int timeout_ms){
        Response response;

        Response::Status status = execute_with_value(MobSpkr::PD_1160::GetGIOTemperature, value, &response, 1000) ;

        if (status == Response::Status::Success){
            value = (enum MicroStepResolution)response.value();
        }

        return status;
    }

}
