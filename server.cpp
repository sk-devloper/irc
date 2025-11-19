#include <ehs/io/socket/TCP.h>
#include <ehs/io/Console.h>

using namespace ehs;

Vector<TCP *> clients;

void HandleClients() {
    for (UInt_64 i = 0; i < clients.Size(); ++i) {
        Str_8 data(256);

        UInt_64 received = clients[i]->Receive((Byte *)&data[0], data.Size());
        if (!received)
            continue;

        data.Resize(received);

        Console::Write_8(data);
    }
}

int main()
{
    TCP server;
    server.Initialize();
    server.SetBlocking(false);
    server.Bind("", 1234);
    Console::Write_8("server started on port 1234");
    server.Listen();

    while (true) {
        TCP *client = server.Accept();
        if (client) {
            clients.Push(client);

            client->SendStr("Successfully connected to server.");
        }

        HandleClients();
    }

    return 0;
}
