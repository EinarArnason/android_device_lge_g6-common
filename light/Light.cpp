/*
 * Copyright (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LightService"

#include <log/log.h>

#include "Light.h"

#include <cmath>
#include <fstream>

#define LCD_BRIGHTNESS_MIN 1 // Matches config_screenBrightnessSettingMinimum
#define LCD_BRIGHTNESS_MAX 255
#define LCD_BRIGHTNESS_DELTA (LCD_BRIGHTNESS_MAX - LCD_BRIGHTNESS_MIN)

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

/*
 * Write value to path and close file.
 */
static void set(std::string path, std::string value) {
    std::ofstream file(path);
    file << value;
}

static void set(std::string path, int value) {
    set(path, std::to_string(value));
}

static uint32_t applyGamma(const uint32_t brightness){
    if(brightness < LCD_BRIGHTNESS_MIN)
        return 0;
    if(brightness == 1)
        return 1;
    return LCD_BRIGHTNESS_MIN + LCD_BRIGHTNESS_DELTA *
        sqrt(((double)brightness - LCD_BRIGHTNESS_MIN - 0.75)/LCD_BRIGHTNESS_DELTA);
}

static void handleBacklight(const LightState& state) {
    uint32_t brightness = state.color & 0xFF;
    brightness = applyGamma(brightness);
    set("/sys/class/leds/lcd-backlight/brightness", brightness);
}

static std::map<Type, std::function<void(const LightState&)>> lights = {
    {Type::BACKLIGHT, handleBacklight},
};

Light::Light() {}

Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = lights.find(type);

    if (it == lights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    /*
     * Lock global mutex until light state is updated.
     */
    std::lock_guard<std::mutex> lock(globalLock);

    it->second(state);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : lights) types.push_back(light.first);

    _hidl_cb(types);

    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
