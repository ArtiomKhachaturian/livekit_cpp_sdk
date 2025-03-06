#include "WebsocketFactory.h"
#include "LiveKitService.h"
#include "LiveKitRoom.h"
#include "Logger.h"
#include "ZaphoydTppFactory.h"
#include <chrono>
#include <thread>
#include <iostream>

using namespace LiveKitCpp;
using namespace std::chrono_literals;

class LoggerIml : public Logger
{
public:
    void log(LoggingSeverity severity, std::string_view message,
             std::string_view category) final {
        switch (severity) {
            case LoggingSeverity::Verbose:
                std::cout << "V (" << category << "): " << message << std::endl;
                break;
            case LoggingSeverity::Info:
                std::cout << "I (" << category << "): " << message << std::endl;
                break;
            case LoggingSeverity::Warning:
                std::cout << "W (" << category << "): " << message << std::endl;
                break;
            case LoggingSeverity::Error:
                std::cerr << "E (" << category << "): " << message << std::endl;
                break;
        }
    }
};

const std::string token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDEzNDc1MDYsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0MTI2MTEwNiwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.19klgtLh8ku2Hn_cKoGD4m6IbKEW6RVdeS3KMVnHSqE";
const std::string host = "ws://localhost:7880";

int main(int argc, char** argv)
{
    const auto logger = std::make_shared<LoggerIml>();
    const auto wsf =  std::make_shared<ZaphoydTppFactory>(logger);
    LiveKitService service(wsf, logger);
    if (auto room = service.makeRoomU()) {
        room->connect(host, token);
        std::this_thread::sleep_for(15s);
        std::cout << "that's the end!" << std::endl;
    }
    return 0;
}
