#include "radarRenderer.h"
#include <iostream>

int main()
{
    // Define the dimensions of the radar window
    const int radarWidth = 800;
    const int radarHeight = 800;

    try
    {
        RadarRenderer radarRenderer(radarWidth, radarHeight);
        radarRenderer.init();
        while (true)
        {
            radarRenderer.run();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
