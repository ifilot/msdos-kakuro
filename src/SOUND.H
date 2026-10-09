#ifndef KAKURO_SOUND_H
#define KAKURO_SOUND_H

// Resident menu/puzzle music and effects for the selected format. No disk access
// during playback; hardware synthesis for all three supported interfaces.
class Sound {
public:
    enum Device { Automatic = -1, Off = 0, AdLib = 1, SoundBlaster = 2, MPU401 = 3 };
    enum Track { MenuMusic, GameMusic, GameMusic2, GameMusic3, GameMusic4, TrackCount };
    enum Effect { Click, Select, Back, Page, Place1, Place2, Place3, Place4,
                  Place5, Place6, Place7, Place8, Place9, VerifyOn, VerifyOff,
                  Wrong, Dialog, Solved, EffectCount };
    Sound();
    ~Sound();
    void initialize();
    int configure(Device choice);
    void shutdown();
    void playMusic(Track track);
    static Track puzzleTrack(unsigned puzzleId);
    void playPuzzleMusic(unsigned puzzleId) { playMusic(puzzleTrack(puzzleId)); }
    void playEffect(Effect effect);
    void service();
    void toggleMusic();
    void toggleEffects();
    int saveSettings() const;
    Device activeDevice() const { return active; }
    Device selectedDevice() const { return selected; }
    unsigned blasterPort() const { return sbPort; }
    unsigned midiPort() const { return mpuPort; }
    void setPorts(unsigned sb, unsigned mpu);
    int musicEnabled() const { return musicOn; }
    int effectsEnabled() const { return effectsOn; }
    const char *status() const { return message; }
    static const char *deviceName(Device device);
private:
    Device active, selected;
    unsigned sbPort, mpuPort;
    enum { AssetCount = TrackCount + 1 };
    unsigned char *data[AssetCount];
    unsigned sizes[AssetCount];
    int musicOn, effectsOn, currentTrack;
    const char *message;
    int loadAssets(Device device);
    int startDevice(Device device);
    Sound(const Sound &);
    Sound &operator=(const Sound &);
};
#endif
