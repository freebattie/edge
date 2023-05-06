#include "storage.h"
Preferences _prefs;
profile_t _profileSave;
profile_t _profileLoad;
Storage::Storage()
{
    _prefs = Preferences();
}
Storage::~Storage()
{
    _prefs.end();
}
void Storage::load()
{

    _prefs.getBytes("profile", &_profileLoad, sizeof(_profileSave));
}
void Storage::start()
{
    bool started = _prefs.begin("my-app", false); // use "my-app" namespace
    if (!_prefs.isKey("profile"))
    {
        Serial.println("saving default");

        _profileSave.deviceName = NAME;
        _profileSave.location = "test";
        _profileSave.city = "Stavanger";
        _profileSave.mqtt_pass = MQTT_PASSWORD;
        _profileSave.mqtt_username = NAME;
        _profileSave.fw = 1;
        _profileSave.build = "dev";
        Serial.print("started? ");
        Serial.println(started);
        size_t size = _prefs.putBytes("profile", &_profileSave, sizeof(_profileSave));
        Serial.print("size? ");
        Serial.println(size);
    }
    load();
}
profile_t Storage::getProfile()
{
    load();
    return _profileLoad;
}

void Storage::saveProfile(profile_t profile)
{
    _profileSave = profile;

    save();
}
void Storage::save()
{
    if (isRoom())
    {

        _prefs.putBytes("profile", &_profileSave, sizeof(_profileSave));

        Serial.println("we saved");
    }
    else
        Serial.println("we did not save");
}
bool Storage::isRoom()
{

    size_t schLen = _prefs.getBytesLength("profile"); // get the length
    Serial.print("lengt of this is ");
    Serial.println(schLen);
    // simple check that data fits
    if (0 == schLen || schLen % sizeof(profile_t) || schLen > sizeof(_profileSave))
    {
        Serial.printf("Invalid size of schedule array: %zu\n", schLen);

        return false;
    }
    return true;
}