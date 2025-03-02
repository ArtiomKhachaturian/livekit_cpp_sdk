#include "core/SignalClientWs.h"
#include "websocket/WebsocketFactory.h"
#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
    using namespace LiveKitCpp;
    using namespace std::chrono_literals;
    
    SignalClientWs client(WebsocketFactory::defaultFactory());
    client.setHost("ws://localhost:7880");
    client.setAuthToken("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDA5NzY3NTksImlzcyI6ImRldmtleSIsIm5iZiI6MTc0MDg5MDM1OSwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.GCak9ySzJr93ZbTP5ikFAHwCWffk951XTirVnBW458o");
    client.connect();
    std::this_thread::sleep_for(5000ms);
    const auto st = client.state();
    return 0;
}
