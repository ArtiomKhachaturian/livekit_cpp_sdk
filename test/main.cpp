#include "SignalClientWs.h"
#include "SignalServerListener.h"
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
    client.setAuthToken("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDExOTQ1MjQsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0MTEwODEyNCwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.sgkNRr8Y9-KLbVLlYUUXQB6KCykdiF7-ft6itTw9vG8");
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
