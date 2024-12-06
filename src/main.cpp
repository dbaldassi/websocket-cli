#include <fmt/core.h>
#include <CLI/CLI.hpp>

#include "websocket_client.h"

template<typename... Args>
void println(fmt::format_string<Args...> s, Args&& ... args)
{
    fmt::print(s, std::forward<Args>(args)...);
    fmt::print("\n");
}

template<typename T>
int run_client(std::string_view host, int port, const std::string& subprotocol, bool interactive)
{
    std::condition_variable cv;
    std::mutex cv_mutex;

    // Create websocket
    T ws{};

    // Init callbacks
    ws.onopen = [&cv](){ fmt::print(stderr, "Websocket opened\n"); cv.notify_all(); };
    ws.onmessage = [](std::string_view msg) { fmt::print("{}\n", msg); fflush(stdout); };
    ws.onclose = [&cv]() { fmt::print(stderr, "Websocket closed\n"); cv.notify_all(); };
    ws.onfail = [&cv]() { fmt::print(stderr, "Failed to connect to host\n"); cv.notify_all(); };

    // Set subprotocol uf any
    if(!subprotocol.empty()) ws.set_subprotocol(subprotocol);

    // Connect to host
    ws.connect(host, port);

    {
    // Wait for websocket to be opened
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock);
    }

    // Could not opened websocket
    if(!ws.is_opened()) return 1; 

    if(interactive) {
        // Get input from standard input to send message to host
        for(std::string line; std::getline(std::cin, line);) {
            // Remove \n
            if(line.back() == '\n') line.pop_back();

            // Send line as message
            ws.send(line);
        }
    }
    else {
        std::cin >> std::ws;
        if(std::cin.peek() != EOF) {
            for(std::string line; std::getline(std::cin, line);) {
                // Remove \n
                if(line.back() == '\n') line.pop_back();

                // Send line as message
                ws.send(line);
            }
        }

        // Wait for websocket to be opened
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv.wait(lock);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    CLI::App app;

    auto client = app.add_subcommand("client", "Run a Websocket client");

    std::string host, subprotocol;
    bool secure = false, interactive = false;
    int ret = 0, port;

    client->add_option("host", host, "Host we want to connect")->required();
    auto port_opt = client->add_option("-p,--port", port, "Set the port to use for the connection");
    client->add_option("-t,--subprotocol", subprotocol, "Use a subprotocol");
    client->add_flag("-s,--secure", secure, "Use secure websocket");
    client->add_flag("-i,--interactive", interactive, "Prompt to send message to host");

    client->callback([&]() { 
        if(secure) ret = run_client<WebSocketSecure>(host, (*port_opt ? port : WebSocketSecure::DEFAULT_PORT), subprotocol, interactive);
        else       ret = run_client<WebSocket>(host, (*port_opt ? port : WebSocket::DEFAULT_PORT), subprotocol, interactive);
    });

    // Add new options/flags here
    CLI11_PARSE(app, argc, argv);

    return ret;
}