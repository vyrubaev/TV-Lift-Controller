#pragma once

class Core
{

public:

    static bool init();

    static void loop();

    static void reboot();

private:
    static void printSystemInfo();        

};