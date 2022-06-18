
#ifndef MOBSPKR_VEHICLE_CTRL_MOTOR_HPP
#define MOBSPKR_VEHICLE_CTRL_MOTOR_HPP

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <libserialport.h>

namespace MobSpkr {

class Motor {

    protected:
        char * m_portname;
        uint8_t m_address;

        struct sp_port *m_port;

    public:
        Motor(){}
        Motor(char portname[], uint8_t address){
            if (portname)
                m_portname = strdup(portname);
            else
                m_portname = NULL;
            m_address = address;
            m_port = NULL;
        }
        ~Motor(){
            if (m_portname)
                std::free(m_portname);
            if (m_port) {
                sp_close(m_port);
                sp_free_port(m_port);
                m_port = NULL;
            }
        }

        const char * get_portname() { return m_portname; }
        void set_portname(char portname[]){
            if (m_portname)
              std::free(m_portname);

            if (portname)
                m_portname = strdup(portname);
        }

        uint8_t get_address() { return m_address; }
        void set_address(uint8_t address){
            m_address = address;
        }

        bool open(int baudrate = 9600, int bits = 8, enum sp_parity parity = SP_PARITY_NONE, int stopbits = 1, enum sp_flowcontrol flowcontrol = SP_FLOWCONTROL_NONE);
        void close();

        bool is_open(){ return m_port != NULL; }


        class Command {

            public:

                const static int SIZE = 9;

                const static int ADDRESS = 0;
                const static int COMMAND_NUMBER = 1;
                const static int TYPE = 2;
                const static int MOTOR = 3;
                const static int VALUE = 4;
                const static int CHECKSUM = 8;

            protected:
                uint8_t m_bytes[SIZE];

            public:
                Command(const uint8_t bytes[9]){
                    std::memcpy(m_bytes, bytes, SIZE);
                }

                Command(const Command &other){
                    std::memcpy(m_bytes, other.m_bytes, SIZE);
                }

                Command(uint8_t address, uint8_t command_number, uint8_t type, uint8_t motor, uint32_t value){
                    m_bytes[0] = address;
                    m_bytes[1] = command_number;
                    m_bytes[2] = type;
                    m_bytes[3] = motor;
                    m_bytes[4] = (value >> 24) & 0xff;
                    m_bytes[5] = (value >> 16) & 0xff;
                    m_bytes[6] = (value >> 8) & 0xff;
                    m_bytes[7] = value & 0xff;
                    m_bytes[8] = 0; // checksum
                }

                Command & with_value(uint32_t value){
                    Command * cmd = new Command(*this);
                    cmd->m_bytes[4] = (value >> 24) & 0xff;
                    cmd->m_bytes[5] = (value >> 16) & 0xff;
                    cmd->m_bytes[6] = (value >> 8) & 0xff;
                    cmd->m_bytes[7] = value & 0xff;
                    return *cmd;
                }

                void set_address(uint8_t address){
                    m_bytes[0] = address;
                }

                void set_command_number(uint8_t command_number){
                    m_bytes[1] = command_number;
                }

                void set_type(uint8_t type){
                    m_bytes[2] = type;
                }

                void set_motor(uint8_t motor){
                    m_bytes[3] = motor;
                }

                void set_value(uint32_t value){
                    m_bytes[4] = (value >> 24) & 0xff;
                    m_bytes[5] = (value >> 16) & 0xff;
                    m_bytes[6] = (value >> 8) & 0xff;
                    m_bytes[7] = value & 0xff;
                }

                void compute_checksum(){
                    m_bytes[8] = 0;
                    for(int i = 0; i < 8; i++){
                       m_bytes[8] += m_bytes[i];
                    }
                }

                uint8_t * bytes(){ return m_bytes; }

//                void get_bytes(uint8_t dst[]){
//                    std::memcpy(dst, m_bytes, SIZE);
//
//                    dst[CHECKSUM] = 0;
//                    for(int i = 0; i < SIZE-1; i++){
//                        dst[CHECKSUM] += dst[i];
//                    }
//                }
//
//                void get_bytes(uint8_t dst[], uint8_t address, uint32_t value){
//                    std::memcpy(dst, m_bytes, SIZE);
//
//                    dst[ADDRESS] = address;
//
//                    m_bytes[4] = (value >> 24) & 0xff;
//                    m_bytes[5] = (value >> 16) & 0xff;
//                    m_bytes[6] = (value >> 8) & 0xff;
//                    m_bytes[7] = value & 0xff;
//
//                    dst[CHECKSUM] = 0;
//                    for(int i = 0; i < SIZE-1; i++){
//                        dst[CHECKSUM] += dst[i];
//                    }
//                }
        };

        class Response {
            public:

                enum Status {
                    WrongChecksum               = 1,
                    InvalidCommand              = 2,
                    WrongType                   = 3,
                    InvalidValue                = 4,
                    ConfigurationEEPROMLocked   = 5,
                    CommandNotAvailable         = 6,
                    Success                     = 100,
                    CommandLoadedIntoEEPROM     = 101,

                    Error                       = 1000,
                };

                const static int SIZE = 9;

                const static int ADDRESS = 0;
                const static int MODULE = 1;
                const static int STATUS = 2;
                const static int COMMAND_NUMBER = 3;
                const static int VALUE = 4;
                const static int CHECKSUM = 8;

            protected:

                uint8_t m_bytes[SIZE];
                uint8_t m_checksum;

            public:

                uint8_t address() const { return m_bytes[0]; }
                uint8_t module() const { return m_bytes[1]; }
                Status status() const { return (Status)m_bytes[2]; }
                uint8_t command_number() const { return m_bytes[3]; }
                uint32_t value() const { return (m_bytes[4] << 24) | (m_bytes[5] << 16) | (m_bytes[6] << 8) | m_bytes[7]; }
                uint8_t checksum() const { return m_bytes[8]; }

                bool valid() const { return m_bytes[8] == m_checksum; }

                void set(uint8_t response[9]){

                    std::memcpy(m_bytes, response, 9);

                    m_checksum = 0;
                    for(int i = 0; i < 8; i++){
                        m_checksum += response[i];
                    }
                }

        };


    Response::Status execute_raw(uint8_t * command, uint8_t * response, unsigned int timeout_ms);

    Response::Status execute(Command command, Response * response, unsigned int timeout_ms) {
        command.set_address(m_address);
        command.compute_checksum();

        uint8_t rx[Response::SIZE];

        Response::Status status = execute_raw(command.bytes(), rx, timeout_ms);

        if (response)
            response->set(rx);

        return status;
    }
    Response::Status execute_with_value(Command command, uint32_t value, Response * response, unsigned int timeout_ms){
        command.set_value(value);
        return execute(command, response, timeout_ms);
    }

    Response::Status command_stopMotor(unsigned int timeout_ms);


    Response::Status command_rotateRight(uint32_t value, unsigned int timeout_ms);
    Response::Status command_rotateLeft(uint32_t value, unsigned int timeout_ms);

    enum MovementType {
        MovementType_Absolute = 0,
        MovementType_Relative = 1,
        MovementType_Coordinate = 2
    };

    Response::Status command_moveToPosition(int32_t pos, enum MovementType type, uint8_t coord, unsigned int timeout_ms);

    Response::Status command_getAxisParam_ActualPosition(int32_t & value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_ActualPosition(int32_t value, unsigned int timeout_ms);

    Response::Status command_setAxisParam_MaxCurrent(uint32_t value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_StandbyCurrent(uint32_t value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_PowerDownDelay(uint32_t value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_Interpolation(uint32_t value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_PulseDivisor(uint32_t value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_RampDivisor(uint32_t value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_MaxAcceleration(uint32_t value, unsigned int timeout_ms);

    enum MicroStepResolution {
        MicroStepResolution_2 = 1,
        MicroStepResolution_4 = 2,
        MicroStepResolution_8 = 3,
        MicroStepResolution_16 = 4,
        MicroStepResolution_32 = 5,
        MicroStepResolution_64 = 6,
        MicroStepResolution_128 = 7,
        MicroStepResolution_256 = 8
    };
    Response::Status command_getAxisParam_MicroStepResolution(enum MicroStepResolution & value, unsigned int timeout_ms);
    Response::Status command_setAxisParam_MicroStepResolution(enum MicroStepResolution value, unsigned int timeout_ms);

    Response::Status command_getGIOVoltage(uint32_t & value, unsigned int timeout_ms);
    Response::Status command_getGIOTemperature(uint32_t & value, unsigned int timeout_ms);

};

    namespace PD_1160 {

        const uint8_t GetVersion0[] = {01, 0x80, 00, 00, 00, 00, 00, 00, 0x89};
        const uint8_t GetVersion1[] = {01, 00, 01, 00, 00, 00, 00, 00, 0x8A};

        const uint8_t MotorStop[] = {01, 03, 00, 00, 00, 00, 00, 00, 04};

        const uint8_t RotateRight[]  = {0, 01, 00, 00, 00, 00, 00, 0, 0};
        const uint8_t RotateLeft[] = {0, 02, 00, 00, 00, 00, 00, 0, 0};
        const uint8_t MoveToPosition[] = {0, 04, 00, 00, 00, 01, 0x5f, 0x90, 0xF5};

        const uint8_t GetAxisParam_ActualPosition[] = {01, 06, 01, 00, 00, 00, 00, 00, 0x0A};
        const uint8_t SetAxisParam_ActualPosition[] = {01, 05, 01, 00, 00, 00, 00, 00, 00};

        const uint8_t GetAxisParam_MicroStepResolution[] = {01, 06, 0x8C, 00, 00, 00, 00, 00, 0x93};
        const uint8_t SetAxisParam_MicroStepResolution[] = {01, 05, 0x8C, 00, 00, 00, 00, 00, 00};

	    const uint8_t SetAxisParam_Interpolation[] = {01, 05, 0xa0, 00, 00, 00, 00, 00, 00};
        const uint8_t SetAxisParam_PulseDivisor[] = {01, 05, 0x9a, 00, 00, 00, 00, 00, 00};
        const uint8_t SetAxisParam_RampDivisor[] = {01, 05, 0x99, 00, 00, 00, 00, 00, 00};
        const uint8_t SetAxisParam_MaxCurrent[] = {01, 05, 06, 00, 00, 00, 00, 00, 00};
        const uint8_t SetAxisParam_StandbyCurrent[] = {01, 05, 07, 00, 00, 00, 00, 00, 00};
        const uint8_t SetAxisParam_PowerDownDelay[] = {01, 05, 0xd6, 00, 00, 00, 00, 00, 00};
        const uint8_t SetAxisParam_MaxAcceleration[] = {01, 05, 05, 00, 00, 00, 00, 00, 00};

	    const uint8_t GetGIOVoltage[] = {01, 0x0f, 8, 1, 00, 00, 00, 00, 0x19};
	    const uint8_t GetGIOTemperature[] = {01, 0xf, 9, 01, 00, 00, 00, 00, 0x1A};
    }
}

#endif //MOBSPKR_VEHICLE_CTRL_MOTOR_HPP
