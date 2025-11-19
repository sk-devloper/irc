#include <ehs/io/socket/TCP.h>
#include <ehs/io/socket/DNS.h>
#include <ehs/io/Console.h>

using namespace ehs;

int main() {
    TCP client;
    client.Initialize();
    client.SetBlocking(false);
    client.Connect("127.0.0.1", 1234);

    while (true) {
        Str_8 data(256);

        UInt_64 received = client.Receive((Byte*)&data[0], data.Size());

        if (received)
            Console::Write_8(data);

        Str_8 msg = Console::Read_8();

        client.SendStr(msg);
    }

    return 0;
}
