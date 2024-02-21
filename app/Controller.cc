#include "led-matrix.h"
#include <Magick++.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <signal.h>

using namespace rgb_matrix;

volatile bool interruptReceived = false;
static void InterruptHandler(int signo) {
    interruptReceived = true;
}

bool DisplayImageCenter(const char *filename, RGBMatrix *matrix) {
    FrameCanvas *offscreenCanvas = matrix->CreateFrameCanvas();
    offscreenCanvas->Clear();

    try {
        Magick::Image image;
        image.read(filename);
        image.scale(Magick::Geometry(matrix->width(), matrix->height()));

	image.sharpen(1.0);
	image.contrast(true);
	image.normalize();


        // Reduce the number of colors to create a posterization effect
        image.quantizeColorSpace(Magick::RGBColorspace);
        image.quantizeColors(16); // You can adjust the number of colors as needed
        image.quantize();

        // Calculate the x offset to center the image
        int xOffset = (matrix->width() - image.columns()) / 2;
        // Ensure xOffset is not negative
        xOffset = std::max(0, xOffset);

        for (size_t y = 0; y < image.rows(); ++y) {
            for (size_t x = 0; x < image.columns(); ++x) {
                const Magick::Color &c = image.pixelColor(x, y);

                // Check if the pixel color is white
                if (c.redQuantum() == Magick::Color::scaleDoubleToQuantum(1.0) &&
                    c.greenQuantum() == Magick::Color::scaleDoubleToQuantum(1.0) &&
                    c.blueQuantum() == Magick::Color::scaleDoubleToQuantum(1.0)) {
                    // Set the pixel to black
                    offscreenCanvas->SetPixel(x + xOffset, y, 0, 0, 0);
                } else {
                    // Otherwise, set the pixel color as it is
                    offscreenCanvas->SetPixel(x + xOffset, y,
                                              ScaleQuantumToChar(c.redQuantum()),
                                              ScaleQuantumToChar(c.greenQuantum()),
                                              ScaleQuantumToChar(c.blueQuantum()));
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to load image: " << e.what() << std::endl;
        return false;
    }

    matrix->SwapOnVSync(offscreenCanvas);
    return true;
}

bool DisplayImage(const char *filename, RGBMatrix *matrix) {
    FrameCanvas *offscreenCanvas = matrix->CreateFrameCanvas();
    offscreenCanvas->Clear();

    try {
        Magick::Image image;
        image.read(filename);
        image.scale(Magick::Geometry(matrix->width(), matrix->height()));

        for (size_t y = 0; y < image.rows(); ++y) {
            for (size_t x = 0; x < image.columns(); ++x) {
                const Magick::Color &c = image.pixelColor(x, y);
                
                // Check if the pixel color is white
                if (c.redQuantum() == Magick::Color::scaleDoubleToQuantum(1.0) &&
                    c.greenQuantum() == Magick::Color::scaleDoubleToQuantum(1.0) &&
                    c.blueQuantum() == Magick::Color::scaleDoubleToQuantum(1.0)) {
                    // Set the pixel to black
                    offscreenCanvas->SetPixel(x, y, 0, 0, 0);
                } else {
                    // Otherwise, set the pixel color as it is
                    offscreenCanvas->SetPixel(x, y,
                                              ScaleQuantumToChar(c.redQuantum()),
                                              ScaleQuantumToChar(c.greenQuantum()),
                                              ScaleQuantumToChar(c.blueQuantum()));
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to load image: " << e.what() << std::endl;
        return false;
    }

    matrix->SwapOnVSync(offscreenCanvas);
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image-file>\n";
        return 1;
    }

    Magick::InitializeMagick(*argv);

    RGBMatrix::Options matrixOptions;
    matrixOptions.chain_length = 6;
    matrixOptions.rows = 16; 
    matrixOptions.cols = 32; 
    matrixOptions.multiplexing=4;
    matrixOptions.parallel = 1;
    matrixOptions.show_refresh_rate = true;
    matrixOptions.pwm_bits = 3; 
    matrixOptions.limit_refresh_rate_hz = 100;


    rgb_matrix::RuntimeOptions runtimeOpt;

    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrixOptions, runtimeOpt);
    if (matrix == nullptr) {
        std::cerr << "Could not create matrix object.\n";
        return 1;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    const char *imageFile = argv[1];
    if (!DisplayImageCenter(imageFile, matrix)) {
    //if (!DisplayImage(imageFile, matrix)) {
        std::cerr << "Displaying image failed.\n";
        delete matrix;
        return 1;
    }

    // Keep the image displayed until an interrupt signal is received.
    while (!interruptReceived) {
        sleep(1);
    }

    std::cout << "Exiting program.\n";
    delete matrix;
    return 0;
}
