// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"

using namespace al;
using namespace std;

#include <fstream>
#include <vector>
using namespace std;

string slurp(string fileName); // forward declaration

struct AlloApp : App
{
  Parameter pointSize{"/pointSize", "", 2.0, 0.1, 3.0};
  Parameter timeStep{"/timeStep", "", 0.25, 0.01, 1.6};
  //

  ShaderProgram pointShader;

  void onInit() override
  {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize); // add parameter to GUI
    gui.add(timeStep);  // add parameter to GUI
    //
  }

  // a mesh for every style
  Mesh current;
  Mesh original;
  Mesh rgb;
  Mesh hsv;
  Mesh noice;
  Mesh next;
  Mesh previous;
  double t = 0;

  void onCreate() override
  {

    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    current.primitive(Mesh::POINTS);

    auto file = File::currentPath() + "../colorful.png";
    auto image = Image(file);
    if (image.width() == 0)
    {
      cout << "did not load image" << endl;
      exit(1);
    }
    auto aspect_ratio = 1.0f * image.width() / image.height();

    // can you believe this works with only one set of curly brackets?
    for (int j = 0; j < image.height(); j++)
      for (int i = 0; i < image.width(); i++)
      {
        auto pixel = image.at(i, j); // 0-255 (unsigned char / uint8)
        current.vertex(2.0 * (1.0 * i / image.width() - 0.5) * aspect_ratio, 2.0 * (1.0 * j / image.height() - 0.5), 0);
        current.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        current.texCoord(0.05, 0); // s, t

        original.vertex(2.0 * (1.0 * i / image.width() - 0.5) * aspect_ratio, 2.0 * (1.0 * j / image.height() - 0.5), 0);
        original.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        original.texCoord(0.05, 0); // s, t

        // XXX add your RGB and HSV mesh construction here.
        rgb.vertex(2.0 * pixel.r / 255.0 - 1.0, 2.0 * pixel.g / 255.0 - 1.0, 2.0 * pixel.b / 255.0 - 1.0);
        rgb.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        rgb.texCoord(0.05, 0);

        HSV hsvCol(RGB(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0));

        hsv.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        hsv.vertex(sin(M_PI * 2.0 * hsvCol.h) * hsvCol.s, hsvCol.v - 0.5, cos(M_PI * 2.0 * hsvCol.h) * hsvCol.s);
        hsv.texCoord(0.05, 0);

        noice.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        noice.vertex(rnd::normal() * hsvCol.h, rnd::normal() * hsvCol.h, rnd::normal() * hsvCol.h);
        noice.texCoord(0.05, 0);
      }

    previous.copy(original);
    next.copy(original);
    nav().pos(0, 0, 5);
  }

  void onAnimate(double dt) override
  {
    //
    // XXX accumulate dt to animate transitions between meshes
    t += dt * timeStep;
    t = min(t, 1.0);

    // A * (1 - t) + B * t;

    const int siz = current.vertices().size();

    for (int i = 0; i < siz; i++)
    {
      current.vertices()[i] = previous.vertices()[i] * (1.0 - t) + next.vertices()[i] * t;
      // current.vertices()[i] = previous.vertices()[i].lerp(next.vertices()[i], t);
    }
  }

  bool onKeyDown(const Keyboard &k) override
  {
    if (k.key() == '1')
    {
      // XXX trigger a transition from the current state to the RGB state
      previous.copy(current);
      next.copy(rgb);

      t = 0;
    }
    if (k.key() == '2')
    {
      // XXX trigger a transition from the current state to the RGB state
      previous.copy(current);
      next.copy(hsv);
      t = 0;
    }
    if (k.key() == '3')
    {
      // XXX trigger a transition from the current state to the RGB state
      previous.copy(current);
      next.copy(noice);
      t = 0;
    }
    if (k.key() == '0' || k.key() == '4')
    {
      // XXX trigger a transition from the current state to the RGB state
      previous.copy(current);
      next.copy(original);
      t = 0;
    }
    // XXX add more key-based triggers here
    return true;
  }

  void onDraw(Graphics &g) override
  {
    // XXX no need to change anything in this function
    g.clear(0.2);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(current);
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
