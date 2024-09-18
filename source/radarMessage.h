#pragma once

#include <vector>
#include <random>

struct RadarMessage
{
    float startAzimuth;
    float endAzimuth;
    std::vector<float> intensity; // green channels

    float getAngle() const
    {
        return (startAzimuth + endAzimuth) / 2.0f;
    }

    // Static method to create a RadarMessage with random intensity values
    static RadarMessage createRandom(uint32_t lineIndex, float angleStep, uint32_t nbrCells)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());  // Seeded generator
        static std::uniform_int_distribution<> dist(0, 255);  // Distribution

        RadarMessage radarMessage;
        radarMessage.startAzimuth = lineIndex * angleStep;
        radarMessage.endAzimuth = (lineIndex + 1) * angleStep;

        // Generate random intensity data for the radarMessage
        radarMessage.intensity.resize(nbrCells);
        for (uint32_t j = 0; j < nbrCells; ++j)
        {
            radarMessage.intensity[j] = static_cast<float>(dist(gen)) / 255.0f;
        }

        return radarMessage;
    }
};
