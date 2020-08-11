#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>
#include <vector>

#include <fstream>

#include "raspicam/raspicam.h"
#include "ws2811.h"

struct Color {
    float m_Red;
    float m_Green;
    float m_Blue;

    ws2811_led_t toLED() const {
        uint32_t r = static_cast<uint32_t>(m_Red * 255.f) & 0xFF;
        uint32_t g = static_cast<uint32_t>(m_Green * 255.f) & 0xFF;
        uint32_t b = static_cast<uint32_t>(m_Blue * 255.f) & 0xFF;
        return (r << 16) | (g << 8) | (b);
    }
};

struct Point {
    float m_X;
    float m_Y;

    std::tuple<size_t, size_t> toDC(unsigned int w, unsigned int h) const {
        float scale = 0.5f * static_cast<float>(h);
        float x_offset = static_cast<float>(w / 2);
        float y_offset = static_cast<float>(h / 2);
        float x_dc = m_X * scale + x_offset;
        float y_dc = m_Y * scale + y_offset;
        size_t x_i = static_cast<size_t>(std::max(0.f, std::min(static_cast<float>(w-1), x_dc)));
        size_t y_i = static_cast<size_t>(std::max(0.f, std::min(static_cast<float>(h-1), y_dc)));
        return std::make_tuple(x_i, y_i);
    }
};

class Image {
public:
    Image(int w, int h, raspicam::RASPICAM_FORMAT fmt) :
        m_Width(w),
        m_Height(h),
        m_Format(fmt) {
        if (m_Width > 0 && m_Height > 0 && m_Format == raspicam::RASPICAM_FORMAT_RGB) {
            m_Data = std::vector<unsigned char>(m_Width * m_Height * 3, 0);
        }
    }

    unsigned char* data() {
        if (m_Data.empty()) {
            return nullptr;
        }
        return &m_Data[0];
    }

    Color getPixel(size_t c, size_t r) {
        size_t x = m_Width - c;
        size_t y = m_Height - r;
        Color result = { 0.f, 0.f, 0.f };
        if (y < m_Height && x < m_Width) {
            unsigned char red = m_Data[3*(y*m_Width + x)    ];
            unsigned char green = m_Data[3*(y*m_Width + x) + 1];
            unsigned char blue = m_Data[3*(y*m_Width + x) + 2];
            result.m_Red = static_cast<float>(red) / 255.f;
            result.m_Green = static_cast<float>(green) / 255.f;
            result.m_Blue = static_cast<float>(blue) / 255.f;
        }
        return result;
    }

private:
    size_t m_Width;
    size_t m_Height;
    raspicam::RASPICAM_FORMAT m_Format;
    std::vector<unsigned char> m_Data;
};

ws2811_t getLEDParams() {
    ws2811_t result;

    result.render_wait_time = 0;
    result.device = nullptr;
    result.rpi_hw = nullptr;
    result.freq = WS2811_TARGET_FREQ;
    result.dmanum = 10;

    result.channel[0].gpionum = 18;
    result.channel[0].invert = 0;
    result.channel[0].count = 36;
    result.channel[0].strip_type = WS2811_STRIP_RGB;
    result.channel[0].leds = nullptr;
    result.channel[0].brightness = 255;
    result.channel[0].wshift = 0;
    result.channel[0].rshift = 0;
    result.channel[0].gshift = 0;
    result.channel[0].bshift = 0;
    result.channel[0].gamma = nullptr;

    for (int i=1; i<RPI_PWM_CHANNELS; ++i) {
        result.channel[i] = { 0 };
    }

    return result;
}

void boot_animation(ws2811_t &ledparams) {
    if (ledparams.channel[0].count != 36 || !ledparams.channel[0].leds) {
        return;
    }

    for (int i=0; i<36; ++i) {
        ledparams.channel[0].leds[i] = 0;
    }

    ledparams.channel[0].leds[0] = 0x00FF0000;
    ledparams.channel[0].leds[1] = 0x003F0000;
    ledparams.channel[0].leds[2] = 0x000F0000;
    ledparams.channel[0].leds[3] = 0x00070000;

    ledparams.channel[0].leds[6] = 0x0000FF00;
    ledparams.channel[0].leds[7] = 0x00003F00;
    ledparams.channel[0].leds[8] = 0x00000F00;
    ledparams.channel[0].leds[9] = 0x00000700;

    ledparams.channel[0].leds[18] = 0x000000FF;
    ledparams.channel[0].leds[19] = 0x0000007F;
    ledparams.channel[0].leds[20] = 0x0000000F;
    ledparams.channel[0].leds[21] = 0x00000007;

    int ret = 0;
    int framerate = 1000 / 24;
    for (int i=0; i<18; ++i) {
        ret = ws2811_render(&ledparams);
        if (ret != WS2811_SUCCESS) {
        std::cerr << "ws2811_render returned " << ret << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(framerate));

        int t = ledparams.channel[0].leds[0];
        for (int j=0; j<5; ++j) {
            ledparams.channel[0].leds[j] = ledparams.channel[0].leds[j+1];
        }
        ledparams.channel[0].leds[5] = t;

        t = ledparams.channel[0].leds[6];
        for (int j=6; j<17; ++j) {
            ledparams.channel[0].leds[j] = ledparams.channel[0].leds[j+1];
        }
        ledparams.channel[0].leds[17] = t;

        t = ledparams.channel[0].leds[18];
        for (int j=18; j<35; ++j) {
            ledparams.channel[0].leds[j] = ledparams.channel[0].leds[j+1];
        }
        ledparams.channel[0].leds[35] = t;
    }

    for (int i=0; i<8; ++i) {
        for (int j=0; j<36; ++j) {
            int r = ledparams.channel[0].leds[j] & 0x00FF0000;
            int g = ledparams.channel[0].leds[j] & 0x0000FF00;
            int b = ledparams.channel[0].leds[j] & 0x000000FF;
            r = (r >> 1) & 0x00FF0000;
            g = (g >> 1) & 0x0000FF00;
            b = (b >> 1) & 0x000000FF;
            ledparams.channel[0].leds[j] = r | g | b;
        }

        ret = ws2811_render(&ledparams);
        if (ret != WS2811_SUCCESS) {
        std::cerr << "ws2811_render returned " << ret << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
    }
}

void main_loop(raspicam::RaspiCam &cam, ws2811_t &ledparams, std::vector<Point> &points) {
    int ret = 0;
    int framerate = 1000 / 24;
    while (true) {
        cam.grab();

        Image frame(cam.getWidth(), cam.getHeight(), cam.getFormat());

        cam.retrieve(frame.data());

        for (int i=0; i<ledparams.channel[0].count; ++i) {
        const bool log = false;
        if (log) {
                std::cout << "led[" << (i+1) << "] NDC = [" << points[i].m_X << ", " << points[i].m_Y << "]";
            }
            size_t x = 0;
            size_t y = 0;
            std::tie (x, y) = points[i].toDC(cam.getWidth(), cam.getHeight());
            if (log) {
                std::cout << ", DC = [" << x << ", " << y << "]";
        }
            Color p = frame.getPixel(x, y);
            if (log) {
                std::cout << ", RGB = ("
                          << p.m_Red << ", " << p.m_Green << ", " << p.m_Blue
                  << ")";
            }
            ws2811_led_t v = p.toLED();
            if (log) {
                std::cout << ", p = 0x"
                          << std::hex << v << std::dec << std::endl;
            }
            ledparams.channel[0].leds[i] = v;
        }

        ret = ws2811_render(&ledparams);
        if (ret != WS2811_SUCCESS) {
        std::cerr << "ws2811_render returned " << ret << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
    }
}

int main(int argc, char **argv) {
    raspicam::RaspiCam cam;
    std::cout << "opening camera" << std::endl;
    if (!cam.open()) {
        std::cerr << "Could not open camera" << std::endl;
        return -1;
    }

    std::cout << "Connect to camera " << cam.getId() << std::endl;
    std::cout << "Size = " << cam.getWidth() << " X " << cam.getHeight() << std::endl;
    std::cout << "Format = ";
    switch (cam.getFormat()) {
    case raspicam::RASPICAM_FORMAT_YUV420:
        std::cout << "YUV420";
        break;
    case raspicam::RASPICAM_FORMAT_GRAY:
        std::cout << "GRAY";
        break;
    case raspicam::RASPICAM_FORMAT_BGR:
        std::cout << "BGR";
        break;
    case raspicam::RASPICAM_FORMAT_RGB:
        std::cout << "RGB";
        break;
    case raspicam::RASPICAM_FORMAT_IGNORE:
    default:
        std::cout << "<UNKNOWN>";
        break;
    }
    std::cout << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    ws2811_t ledparams = getLEDParams();
    int ret = ws2811_init(&ledparams);
    if (ret != WS2811_SUCCESS) {
        std::cerr << "ws2811_init returned " << ret << std::endl;
        exit(-1);
    }

    boot_animation(ledparams);

    float den = 1.f / 5.f;
    float c = 0.866f * den;
    float s = 1.5f * den;

    Point c0 = { 0.f, 0.f };
    std::vector<Point> points;

    Point c1 = { c0.m_X + c, c0.m_Y + s };
    points.push_back(c1);
    points.push_back({c1.m_X +       c, c1.m_Y -       s});
    points.push_back({c1.m_X          , c1.m_Y - 2.f * s});
    points.push_back({c1.m_X - 2.f * c, c1.m_Y - 2.f * s});
    points.push_back({c1.m_X - 3.f * c, c1.m_Y -       s});
    points.push_back({c1.m_X - 2.f * c, c1.m_Y          });

    Point c7 = { c1.m_X - c, c1.m_Y + s };
    points.push_back(c7);
    points.push_back({c7.m_X + 2.f * c, c7.m_Y});
    points.push_back({c7.m_X + 3.f * c, c7.m_Y -       s});
    points.push_back({c7.m_X + 4.f * c, c7.m_Y - 2.f * s});
    points.push_back({c7.m_X + 3.f * c, c7.m_Y - 3.f * s});
    points.push_back({c7.m_X + 2.f * c, c7.m_Y - 4.f * s});
    points.push_back({c7.m_X          , c7.m_Y - 4.f * s});
    points.push_back({c7.m_X - 2.f * c, c7.m_Y - 4.f * s});
    points.push_back({c7.m_X - 3.f * c, c7.m_Y - 3.f * s});
    points.push_back({c7.m_X - 4.f * c, c7.m_Y - 2.f * s});
    points.push_back({c7.m_X - 3.f * c, c7.m_Y -       s});
    points.push_back({c7.m_X - 2.f * c, c7.m_Y          });

    Point c19 = { c7.m_X - c, c7.m_Y + s };
    points.push_back(c19);
    points.push_back({c19.m_X + 2.f * c, c19.m_Y});
    points.push_back({c19.m_X + 4.f * c, c19.m_Y});
    points.push_back({c19.m_X + 5.f * c, c19.m_Y - s});
    points.push_back({c19.m_X + 6.f * c, c19.m_Y - 2.f * s});
    points.push_back({c19.m_X + 7.f * c, c19.m_Y - 3.f * s});
    points.push_back({c19.m_X + 6.f * c, c19.m_Y - 4.f * s});
    points.push_back({c19.m_X + 5.f * c, c19.m_Y - 5.f * s});
    points.push_back({c19.m_X + 4.f * c, c19.m_Y - 6.f * s});
    points.push_back({c19.m_X + 2.f * c, c19.m_Y - 6.f * s});
    points.push_back({c19.m_X          , c19.m_Y - 6.f * s});
    points.push_back({c19.m_X - 2.f * c, c19.m_Y - 6.f * s});
    points.push_back({c19.m_X - 3.f * c, c19.m_Y - 5.f * s});
    points.push_back({c19.m_X - 4.f * c, c19.m_Y - 4.f * s});
    points.push_back({c19.m_X - 5.f * c, c19.m_Y - 3.f * s});
    points.push_back({c19.m_X - 4.f * c, c19.m_Y - 2.f * s});
    points.push_back({c19.m_X - 3.f * c, c19.m_Y -       s});
    points.push_back({c19.m_X - 2.f * c, c19.m_Y});

    main_loop(cam, ledparams, points);

    cam.release();
}
