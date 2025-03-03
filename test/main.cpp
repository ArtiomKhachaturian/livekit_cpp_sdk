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
    client.setAuthToken("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDEwNzk4MTMsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0MDk5MzQxMywic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.jZwSYSU4DCz7vZDtiwFU6RpUfveVQGVcZCp5fmgxRe8");
    client.connect();
    std::this_thread::sleep_for(15000s);
    const auto st = client.transportState();
    return 0;
}
