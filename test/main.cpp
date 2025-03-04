#include "core/SignalClientWs.h"
#include "core/SignalServerListener.h"
#include "websocket/WebsocketFactory.h"
#include <chrono>
#include <thread>

using namespace LiveKitCpp;
using namespace std::chrono_literals;

class Listener : public SignalServerListener
{
public:
    Listener(SignalClient* client);
    void onOffer(uint64_t, const SessionDescription& sdp) final;
private:
    SignalClient* const _client;
};

int main(int argc, char** argv)
{
    SignalClientWs client(WebsocketFactory::defaultFactory());
    Listener listener(&client);
    client.setHost("ws://localhost:7880");
    client.setAuthToken("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDEwNzk4MTMsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0MDk5MzQxMywic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.jZwSYSU4DCz7vZDtiwFU6RpUfveVQGVcZCp5fmgxRe8");
    client.addServerListener(&listener);
    client.connect();
    std::this_thread::sleep_for(15000s);
    const auto st = client.transportState();
    client.removeServerListener(&listener);
    return 0;
}


Listener::Listener(SignalClient* client)
    : _client(client)
{
}

void Listener::onOffer(uint64_t, const SessionDescription& sdp)
{
    _client->sendAnswer(sdp);
}
