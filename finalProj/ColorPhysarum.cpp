
#include <iostream>
#include <fstream>
#include <vector>
#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace std;
using namespace al;

Vec3f randomVec3f(float scale)
{
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName); // forward declaration

const int numParticles = 1000;

struct CommonState
{
  float pointSize;
  Vec3f positions[numParticles];
  HSV colors[numParticles];
};

struct MyApp : DistributedAppWithState<CommonState>
{

  Parameter pointSize{"pointSize", "", 1, 0.1, 10.0};
  Parameter sight{"sight", "", 0.45, 0.001, 1.0};
  Parameter turnRate{"turnRate", "", 0.35, 0.01, .55};
  Parameter randTurn{"randTurn", "", 0.01, 0.001, .25};
  Parameter sphereK{"sphereK", "", 0.14, 0.01, .55};
  Parameter minDist{"minDist", "", 0.02, 0.0001, .25};
  Parameter moveRate{"moveRate", "", 0.035, 0.01, .25};

  ShaderProgram pointShader;

  Nav particles[numParticles];
  Vec3f oldPos[numParticles];
  Vec3f vel[numParticles];
  Vec3f oldVel[numParticles];
  Vec3f acc[numParticles];
  Vec3f oldAcc[numParticles];

  Mesh mesh;

  void onInit() override
  {
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<CommonState>::enableCuttlebone(this);
    if (!cuttleboneDomain)
    {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }
    if (isPrimary())
    {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto &gui = guiDomain->newGUI();

      gui.add(pointSize);
      gui.add(sight);
      gui.add(turnRate);
      gui.add(randTurn);
      gui.add(sphereK);
      gui.add(minDist);
      gui.add(moveRate);
    }
  }
  void onCreate() override
  {

    // compile shaders
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    mesh.primitive(Mesh::POINTS);

    if (isPrimary())
    {
      for (int i = 0; i < numParticles; i++)
      {


        Nav &p = particles[i];
        Vec3f newPos = randomVec3f(3);
        p.pos(newPos);
        p.faceToward(randomVec3f(1), 1);
        state().positions[i] = p.pos();

        oldPos[i] = newPos;
        vel[i] = 0;
        oldVel[i] = 0;
        acc[i] = 0;
        oldAcc[i] = 0;

        HSV randC = HSV(rnd::uniform(), 1.0f, 1.0f);
        state().colors[i] = randC;
      }

      nav().pos(0, 0, 10);
    }

    for (int i = 0; i < numParticles; i++)
    {
      mesh.vertex(state().positions[i]);
      HSV col = state().colors[i];
      mesh.color(col);
      mesh.texCoord(1.0, 0);
    }
  }

  double phase = 0;

  void onAnimate(double dt) override
  {
    if (isPrimary())
    {
      phase += dt;

      for (int i = 0; i < numParticles; i++)
      {

        Nav &p = particles[i];
        oldPos[i] = p.pos();
        oldVel[i] = vel[i];
        oldAcc[i] = acc[i];

        Vec3f spher = p.pos();
        spher.normalize();
        Vec3f SD = spher * 3 - p.pos();
        float sd = SD.mag();
        Vec3f sphAtt = p.uf() + SD;
        if (sd > .25)
        {
          p.faceToward(p.pos() + p.uf() + sphAtt, sphereK);
        }

        if (p.pos().mag() > 5)
        {
          p.faceToward(Vec3f(0), .15);
        }
        for (int j = 0; j < numParticles; j++)
        {
          Nav &p2 = particles[j];
          HSV h1 = state().colors[i];
          HSV h2 = state().colors[j];

          if (i != j)
          {
            Vec3f D = p2.pos() - p.pos();
            float d = D.mag();
            D.normalize();
            bool viewCheck = (D.x * p.uf().x + D.y * p.uf().y + D.z * p.uf().z) >= 0;
            float hueDif = sin(M_PI_2 * (h2.h - h1.h));
            bool hueCheck = abs(hueDif) > .2;
            Vec3f dirRep = p.uf() - D;
            Vec3f dirAtt = p.uf() + D;

            if (d < sight && viewCheck && hueCheck && d > minDist)
            {
              p.faceToward(p.pos() + dirRep, turnRate);
              state().colors[i].h += hueDif * .002;
            }
            if (d < sight && viewCheck && !hueCheck && d > minDist)
            {
              p.faceToward(p.pos() + dirAtt, turnRate);
              state().colors[i].h += hueDif * .002;
            }
            if (d < minDist)
            {
              p.faceToward(p.pos() + dirRep, 1.);
            }
          }
        }

        vel[i] = p.pos() - oldPos[i];
        acc[i] = vel[i] - oldVel[i];
        Vec3f jerk = acc[i] - oldAcc[i];

        state().positions[i] = p.pos();
        Vec3f rand = randomVec3f(1);
        rand.normalize();
        p.faceToward(p.pos() + p.uf() + rand, randTurn);
        p.moveF(moveRate);
        p.step();

        if (state().colors[i].h > 1.0)
        {
          state().colors[i].h -= 1.0;
        }
      }
      state().pointSize = pointSize;
    }

    for (int i = 0; i < numParticles; i++)
    {
      mesh.vertices()[i] = state().positions[i];
      mesh.colors()[i] = state().colors[i];
    }

  }

  void onSound(AudioIOData &io) override
  {
    // while (io()) {
    //   io.out(0) = io.out(1) = io.in(0) * signal;
    // }
  }

  void onDraw(Graphics &g) override
  {

    g.clear(0, 0, 0, .0);
    g.shader(pointShader);
    float pSize = state().pointSize;
    g.shader().uniform("pointSize", pSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);
  }

  bool onKeyDown(const Keyboard &k) override
  {
  }
};

int main()
{
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

string slurp(string fileName)
{
  fstream file(fileName);
  string returnValue = "";
  while (file.good())
  {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}