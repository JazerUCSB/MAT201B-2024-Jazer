#include "al/app/al_DistributedApp.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"
#include "al/app/al_GUIDomain.hpp"

using namespace al;

struct CommonState {
  char data[66000]; // larger than a UDP packet
  int frame;
  float signal;
};

struct MyApp : DistributedAppWithState<CommonState> {
  Parameter signal{"signal", "", 0, 0, 1};

  void onInit() override {
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<CommonState>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }
    if (isPrimary()) {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto &gui = guiDomain->newGUI();
      gui.add(signal);
    }

    if (isPrimary()) {
      state().frame = 0;
      state().signal = signal;
    }
  }

  void onCreate() override {
  }

  void onAnimate(double dt) override {
    if (isPrimary()) {
      state().frame++;
      state().signal = signal;
    } else {
      std::cout << state().frame << " " << state().signal << std::endl;
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(state().signal);
  }

  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = io.out(1) = io.in(0) * signal;
    }
  }
};

int main() {
  MyApp app;
  AudioDevice::printAll();
  app.audioIO().deviceIn(AudioDevice("MacBook Pro Microphone"));
  app.audioIO().deviceOut(AudioDevice("MacBook Pro Speakers"));

  int device_index_out = app.audioIO().channelsOutDevice();
  int device_index_in = app.audioIO().channelsInDevice();
    std::cout << "in:" << device_index_in << " out:" << device_index_out << std::endl;

  app.start();
  return 0;
}
