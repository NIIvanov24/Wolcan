#include "MenuLayout.h"

#include <algorithm>

MenuLayout createMenuLayout(int screenWidth, int screenHeight)
{
    float width = static_cast<float>(std::max(screenWidth, 1));
    float height = static_cast<float>(std::max(screenHeight, 1));
    float scale = std::min(width / 1000.0f, height / 700.0f);
    float top = (height - 700.0f * scale) * 0.5f;
    float centerX = width * 0.5f;
    return {scale, top, centerX,
        {centerX - 228.0f * scale, top + 288.0f * scale, 456.0f * scale, 320.0f * scale},
        {
            {centerX - 194.0f * scale, top + 344.0f * scale, 44.0f * scale, 44.0f * scale},
            {centerX + 150.0f * scale, top + 344.0f * scale, 44.0f * scale, 44.0f * scale},
            {centerX - 190.0f * scale, top + 452.0f * scale, 380.0f * scale, 56.0f * scale},
            {centerX - 190.0f * scale, top + 522.0f * scale, 380.0f * scale, 46.0f * scale},
            {centerX - 190.0f * scale, top + 394.0f * scale, 116.0f * scale, 32.0f * scale},
            {centerX - 58.0f * scale, top + 394.0f * scale, 116.0f * scale, 32.0f * scale},
            {centerX + 74.0f * scale, top + 394.0f * scale, 116.0f * scale, 32.0f * scale}
        }};
}

Rectangle calculateCoverSource(int imageWidth, int imageHeight, int screenWidth, int screenHeight)
{
    if (imageWidth <= 0 || imageHeight <= 0 || screenWidth <= 0 || screenHeight <= 0)
    {
        return {};
    }
    float width = static_cast<float>(imageWidth);
    float height = static_cast<float>(imageHeight);
    float screenRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    float cropWidth = std::min(width, height * screenRatio);
    float cropHeight = std::min(height, width / screenRatio);
    return {(width - cropWidth) * 0.5f, (height - cropHeight) * 0.5f, cropWidth, cropHeight};
}
