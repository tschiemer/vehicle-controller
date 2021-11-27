
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdio>

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include <osc/OscOutboundPacketStream.h>
#include "ip/UdpSocket.h"


#ifndef HOSTNAME
#define HOSTNAME "unknown"
#endif

#define DEFAULT_PORT    9292

#define MOTOR_COUNT 2


static char * argv0;


static void print_usage(FILE * f){
    fprintf(f,
            "Usage: %s\n"
            , argv0);
}


class packet_listener : public osc::OscPacketListener {
protected:

    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try{

            printf("OSC rx %s\n", m.AddressPattern());

            if (std::strcmp(m.AddressPattern(), "/motor/temp") == 0) {

                osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
                int motor_index = (arg++)->AsInt32();

                const char *host = (arg++)->AsString();
                int port = (arg++)->AsInt32();

                if (arg != m.ArgumentsEnd())
                    throw osc::ExcessArgumentException();

                if (motor_index < 0 || MOTOR_COUNT <= motor_index) {
                    fprintf(stderr, "Invalid motor index: %d (0 - %d)\n", motor_index, MOTOR_COUNT - 1);
                    return;
                }

                // try to create transmit socket
                fprintf(stderr, "UDP send to %s:%u\n", host, port);
                UdpTransmitSocket transmitSocket( IpEndpointName( host, port ) );

                fprintf(stderr, "Getting motor %d temp ...", motor_index);
                unsigned int temp = 0;
//                if (motors[motor_index].command_getGIOTemperature(temp, TIMEOUT_MS) != MobSpkr::Motor::Response::Status::Success)
//                    printf("FAILED\n");
//                else
                    printf("%d deg C\n",temp);



                char buffer[256];
                osc::OutboundPacketStream p( buffer, sizeof(buffer) );

                p << osc::BeginMessage( "/temp" )
                  << HOSTNAME << motor_index << (int)temp
                  << osc::EndMessage;

                transmitSocket.Send( p.Data(), p.Size() );


            }

        }catch( osc::Exception& e ){
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            fprintf(stderr, "error while parsing message: %s: %s\n", m.AddressPattern(), e.what());
        }
    }
};

int main(int argc, char * argv[]) {
    argv0 = argv[0];


    // initialize before motor opening
    packet_listener listener;
    UdpListeningReceiveSocket osc_rx_socket(IpEndpointName( IpEndpointName::ANY_ADDRESS, DEFAULT_PORT ),&listener );

    printf("Started OSC receiver at port %d\n", DEFAULT_PORT);


    printf("press Ctrl+C (SIGINT) to stop\n");
    osc_rx_socket.RunUntilSigInt();

    return EXIT_SUCCESS;
}