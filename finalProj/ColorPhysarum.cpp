
#include <iostream>
#include <fstream>
#include <vector>
#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

// #include "al/app/al_OmniRendererDomain.hpp"

using namespace std;
using namespace al;

Vec3f randomVec3f(float scale)
{
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName); // forward declaration

const int numParticles = 1200;

struct CommonState
{
  float pointSize;
  Vec3f positions[numParticles];
  HSV colors[numParticles];
};

struct MyApp : DistributedAppWithState<CommonState>
{

  Parameter pointSize{"pointSize", "", 1, 0.1, 4.0};
  Parameter sight{"sight", "", 0.45, 0.001, .75};
  Parameter turnRate{"turnRate", "", 0.35, 0.01, .55};
  Parameter randTurn{"randTurn", "", 0.01, 0.001, .25};
  Parameter sphereK{"sphereK", "", 0.14, 0.01, .55};
  Parameter minDist{"minDist", "", 0.02, 0.0001, .25};
  Parameter moveRate{"moveRate", "", 0.035, 0.01, .25};
  Parameter steps{"steps", "", 1, 1, 30};
  Parameter dx{"dx", "", 1, 0, 10};
  Parameter dy{"dy", "", 1, 0, 10};

  void updateFBO(int w, int h)
  {
    // Note: all attachments (textures, RBOs, etc.) to the FBO must have the same width and height.

    // Configure texture on GPU, simulation requires using floating point textures
    tex0->create2D(w, h, Texture::RGBA32F, Texture::RGBA, Texture::FLOAT);
    tex1->create2D(w, h, Texture::RGBA32F, Texture::RGBA, Texture::FLOAT);

    // Configure render buffer object on GPU
    rbo0.resize(w, h);

    // Finally, attach color texture and depth RBO to FBO
    fbo0.bind();
    // fbo0.attachTexture2D(tex0);
    fbo0.attachRBO(rbo0);
    fbo0.unbind();
  }

  Texture *tex0, *tex1;
  RBO rbo0;
  FBO fbo0;

  ShaderProgram pointShader;

  Nav particles[numParticles];

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
      gui.add(steps);
      gui.add(dx);
      gui.add(dy);
    }
  }
  void onCreate() override
  {
    tex0 = new Texture();
    tex1 = new Texture();
    updateFBO(fbWidth(), fbHeight());

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
        p.pos(randomVec3f(3));
        p.faceToward(randomVec3f(1), 1);
        state().positions[i] = p.pos();
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
        state().positions[i] = p.pos();
        Vec3f rand = randomVec3f(1);
        rand.normalize();
        p.faceToward(p.pos() + p.uf() + rand, randTurn);
        p.moveF(moveRate);
        p.step();
        if (state().colors[i].h > 1.0)
        {
          state().colors[i].h += 1.0;
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
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);

    g.framebuffer(fbo0);
    
    
    

    for (int i = 0; i < steps * 2; i++)
    {
      fbo0.attachTexture2D(*tex1); // tex1 will act as fbo0 render target
      g.viewport(0, 0, fbWidth(), fbHeight());
      g.camera(Viewpoint::IDENTITY);

     
      pointShader.use();
      pointShader.uniform("pointSize", state().pointSize / 100);
      g.quad(*tex0, -1, -1, 2, 2);

      // swap textures
      Texture *tmp = tex0;
      tex0 = tex1;
      tex1 = tmp;
    }


    

    // draw to screen using colormap shader
    g.framebuffer(FBO::DEFAULT);
    pointShader.use();
    pointShader.uniform("pointSize", state().pointSize / 100);
    g.viewport(0, 0, fbWidth(), fbHeight());
    g.camera(Viewpoint::IDENTITY);
    g.quad(*tex1, -1, -1, 2, 2);
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
