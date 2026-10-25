/*
    loadimage.h - load images to be used for sprites and textures
*/

#include "commons.h"

//TODO: replace tileset and .bmp images with proprietary file system to load images similar to DOOM wads
image loadTile(image &src, int width, int height, int index)
{
    //gets a tile from tileset or a smaller image from source image
    image tile;
    tile.width = width;
    tile.height = height;
    tile.pixelData = new unsigned char[width*height*3];
    
    int widthOffset = width*index*3;
    int heightOffset = 0;
    while (src.width*3 <= widthOffset)
    {
        heightOffset += height;
        widthOffset -= src.width*3;
    }
    
    int maxWidth = width * 3;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < maxWidth; x += 3) {
            tile.pixelData[y*maxWidth + x + 0] = src.pixelData[(y+heightOffset)*(src.width*3) + x+widthOffset + 0]; //red
            tile.pixelData[y*maxWidth + x + 1] = src.pixelData[(y+heightOffset)*(src.width*3) + x+widthOffset + 1]; //green
            tile.pixelData[y*maxWidth + x + 2] = src.pixelData[(y+heightOffset)*(src.width*3) + x+widthOffset + 2]; //blue
        }
    }

    return tile;
}

bool loadBMP(std::string filepath, image &img) {
    std::fstream bmpFile(filepath, std::ios::in | std::ios::binary);
    if (!bmpFile) {
        std::cerr << "Image Load Error: Failed to open BMP file at \"" << filepath << "\"." << std::endl;
        return false;
    }

    // Read BMP header
    char header[54];
    bmpFile.read(header, 54);

    // Check the BMP signature
    if (header[0] != 'B' || header[1] != 'M') {
        std::cerr << "Image Load Error: Not a valid BMP file: BM signature missing" << std::endl;
        bmpFile.close();
        return false;
    }

    // Extract image width and height from the header
    int width = *reinterpret_cast<int*>(&header[18]);
    int height = *reinterpret_cast<int*>(&header[22]);
    img.width = width;
    img.height = height;

    short bitsPerPixel = *reinterpret_cast<short*>(&header[28]);
    int compression = *reinterpret_cast<int*>(&header[30]);
    int pixelDataSize = *reinterpret_cast<int*>(&header[34]);

    //read pixel data
    if (bitsPerPixel != 24 || compression != 0) {
        std::cerr << "Load Image Error: Unable to read pixel data. Only uncompressed 24-bit BMP files are supported." << std::endl;
        bmpFile.close();
        return false;
    }

    char* pixelData = new char[pixelDataSize];
    bmpFile.read(pixelData, pixelDataSize);

    bmpFile.close();
    
    img.pixelData = new unsigned char[width*height*3];

    int padding = (width * 3) % 4;
    
    int maxWidth = width * 3;
    for (int heightAmount = 0; heightAmount < height; heightAmount++) {
        for (int i = 0; i < maxWidth; i += 3) {
            
            unsigned char blue = static_cast<unsigned char>(pixelData[heightAmount * maxWidth + heightAmount*padding + i]);
            unsigned char green = static_cast<unsigned char>(pixelData[heightAmount * maxWidth + heightAmount*padding + i + 1]);
            unsigned char red = static_cast<unsigned char>(pixelData[heightAmount * maxWidth + heightAmount*padding + i + 2]);

            img.pixelData[heightAmount*maxWidth + i + 0] = red;
            img.pixelData[heightAmount*maxWidth + i + 1] = green;
            img.pixelData[heightAmount*maxWidth + i + 2] = blue;
        }
    }

    // Remember to free the allocated memory after use
    delete[] pixelData;

    return true;
}