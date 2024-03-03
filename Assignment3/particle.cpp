// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale)
{
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}
string slurp(string fileName); // forward declaration

struct AlloApp : App
{
  Parameter pointSize{"/pointSize", "", 1.0, 0.0, 2.0};
  Parameter timeStep{"/timeStep", "", 0.5, 0.01, 0.6};
  Parameter dragFactor{"/dragFactor", "", 0.8, 0.0, 0.9};
  Parameter K{"/K", "", 0.4, 0.0, .99};
  Parameter Ke{"/Ke", "", 0.0, 0.0, .99};
  //

  ShaderProgram pointShader;

  //  simulation state
  Mesh mesh; // position *is inside the mesh* mesh.vertices() are the positions
 // Texture texBlur;
  vector<Vec3f> velocity;
  vector<Vec3f> force;
  vector<float> mass;

  HSV col;

  void onInit() override
  {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize);  // add parameter to GUI
    gui.add(timeStep);   // add parameter to GUI
    gui.add(dragFactor); // add parameter to GUI
    //
    gui.add(K);
    gui.add(Ke);
  }

  void onCreate() override
  {
    // compile shaders
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    // set initial conditions of the simulation
    //

    // c++11 "lambda" function
    auto randomColor = []()
    { return HSV(rnd::uniform(), 1.0f, 1.0f); };
    // HSV col = randomColor();
    mesh.primitive(Mesh::POINTS);
    // does 1000 work on your system? how many can you make before you get a low
    // frame rate? do you need to use <1000?
    for (int _ = 0; _ < 1000; _++)
    {
      mesh.vertex(randomVec3f(5));
      mesh.color(randomColor());

      // float m = rnd::uniform(3.0, 0.5);
      float m = 3 + rnd::normal() / 2;
      if (m < 0.5)
        m = 0.5;
      mass.push_back(m);

      // using a simplified volume/size relationship
      // mesh.texCoord(pow(m, 1.0f / 3), 0); // s, t
      mesh.texCoord(1.0, 0);
      //  separate state arrays
      velocity.push_back(randomVec3f(0.1));
      force.push_back(randomVec3f(1));
    }
    //texBlur.filter(Texture::LINEAR);
    nav().pos(0, 0, 10);
  }

  bool freeze = false;
  void onAnimate(double dt) override
  {
    if (freeze)
      return;

    // Calculate forces

    // XXX you put code here that calculates gravitational forces and sets
    // accelerations These are pair-wise. Each unique pairing of two particles
    // These are equal but opposite: A exerts a force on B while B exerts that
    // same amount of force on A (but in the opposite direction!) Use a nested
    // for loop to visit each pair once The time complexity is O(n*n)
    //
    // Vec3f has lots of operations you might use...
    // • +=
    // • -=
    // • +
    // • -
    // • .normalize() ~ Vec3f points in the direction as it did, but has length 1
    // • .normalize(float scale) ~ same but length `scale`
    // • .mag() ~ length of the Vec3f
    // • .magSqr() ~ squared length of the Vec3f
    // • .dot(Vec3f f)
    // • .cross(Vec3f f)

    // drag
    for (int i = 0; i < velocity.size(); i++)
    {

      Vec3f chgForce;
      Vec3f tempPos = mesh.vertices()[i];
      Vec3f springF = (tempPos.normalize() * 2 - mesh.vertices()[i]) * K;
      for (int j = i + 1; j < velocity.size(); j++)
      {
        HSV q1 = mesh.colors()[i];
        HSV q2 = mesh.colors()[j];
        float charge = 1.0;
        float asymCharge = (abs(sin((q2.h - q1.h) * M_PI))>.08)*2.0 - 1.0;

        Vec3f dir = mesh.vertices()[j] - mesh.vertices()[i];
        Vec3f dist = dir;
        float eps0 = .001;
        if(dist<.07){
          asymCharge *= -1;
        }
        chgForce = (dir.normalize() * asymCharge) / (dist.magSqr() + .001);
        chgForce *= eps0 * Ke;
       

        force[i] -= chgForce;
        force[j] += chgForce;
      }

      force[i] += -velocity[i] * dragFactor + springF;

      force[i] /= mass[i];
    }

    // Integration
    //
    vector<Vec3f> &position(mesh.vertices());
    for (int i = 0; i < velocity.size(); i++)
    {
      // "semi-implicit" Euler integration
      velocity[i] += force[i] / mass[i] * timeStep;
      position[i] += velocity[i] * timeStep;


    }

    // clear all accelerations (IMPORTANT!!)
    for (auto &a : force)
      a.set(0);
  }

  bool onKeyDown(const Keyboard &k) override
  {
    if (k.key() == ' ')
    {
      freeze = !freeze;
    }

    if (k.key() == '1')
    {
      // introduce some "random" forces
      for (int i = 0; i < velocity.size(); i++)
      {
        // F = ma
        force[i] += randomVec3f(2);
      }
    }

    return true;
  }

  void onDraw(Graphics &g) override
  {
    //g.clear(0.0);

    //texBlur.resize(fbWidth(), fbHeight());
    
    
    // g.tint(.98);
    // // g.quadViewport(texBlur, -1.005, -1.005, 2.01, 2.01); // Outward
    // g.quadViewport(texBlur, -0.995, -0.995, 1.99, 1.99); // Inward
    // // g.quadViewport(texBlur, -1.005, -1.00, 2.01, 2.0); // Oblate
    // //  g.quadViewport(texBlur, -1.005, -0.995, 2.01, 1.99); // Squeeze
    // //  g.quadViewport(texBlur, -1, -1, 2, 2); // non-transformed
    // g.tint(1); // set tint back to 1


    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    //g.shader().uniform("tex", texBlur);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);
   // texBlur.copyFrameBuffer();
  }
};

int main()
{
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
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
