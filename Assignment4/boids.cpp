// how to include things ~ how do we know what to include?
// fewer includes == faster compile == only include what you need
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp" // addCone
#include "al/app/al_GUIDomain.hpp"

#include <fstream>
#include <vector>

using namespace al;
using namespace std;

Vec3f randomVec3f(float scale)
{
    return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

const int numPred = 4;
const int numPrey = 100;
const int numFood = 5;

bool follow = false;
bool followPred = false;

struct MyApp : public App
{

    Parameter cohesion{"/cohesion", "", 0.02, 0.0, 0.4};
    Parameter appetite{"/appetite", "", 0.02, 0.002, 0.3};
    Parameter minDist{"/minDist", "", 0.2, 0.002, 0.5};
    Parameter aversion{"/aversion", "", 0.12, 0.002, .3};
    Parameter preyAwareness{"/preyAwareness", "", 0.12, 0.002, .5};
    Parameter predAversion{"/predAversion", "", 0.22, 0.002, .3};
    Parameter predAppetite{"/predAppetite", "", 0.22, 0.002, .5};
    Parameter predAwareness{"/predAwareness", "", 0.22, 0.02, 1.0};

    Mesh predMesh;
    Mesh preyMesh;
    Mesh foodMesh;

    Nav preds[numPred], preys[numPrey];
    Vec3f foods[numFood];

    void onInit() override
    {
        // set up GUI
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto &gui = GUIdomain->newGUI();
        gui.add(cohesion);
        gui.add(appetite);
        gui.add(minDist);
        gui.add(aversion);
        gui.add(preyAwareness);
        gui.add(predAversion);
        gui.add(predAppetite);
        gui.add(predAwareness);
    }

    void onCreate()
    {
        addCone(predMesh);
        predMesh.generateNormals();
        addCone(preyMesh);
        preyMesh.generateNormals();
        addSphere(foodMesh);
        foodMesh.generateNormals();

        for (int i = 0; i < numPred; i++)
        {
            Vec3f pos = randomVec3f(1);
            preds[i].pos() = pos;
            preds[i].faceToward(randomVec3f(10));
        }
        for (int i = 0; i < numPrey; i++)
        {
            Vec3f pos = randomVec3f(1);
            preys[i].pos() = pos;
            preys[i].faceToward(randomVec3f(10));
        }

        nav().pos(0, 0, 7);
        nav().faceToward(Vec3f(0, 0, 0), 1.0);
    }

    double phase = 0;
    void onAnimate(double dt)
    {

        phase += dt;
        if (phase >= 3)
        {
            phase -= 3;
            for (int i = 0; i < numFood; i++)
            {
                foods[i] = rnd::ball<Vec3f>() * 2;
            }
        }

        double turn_rate = 0.05;
        double move_rate = 0.02;

        for (int i = 0; i < numPred; i++)
        {
            Nav &pr = preds[i];
            if (pr.pos().mag() > 2)
            {
                pr.faceToward(Vec3f(0, 0, 0), .015);
            }
            pr.moveF(move_rate / 2);
            pr.step();
        }

        Vec3f preyDir;
        for (int i = 0; i < numPrey; i++)
        {
            Nav &p = preys[i];
            preyDir += p.uf();
            if (p.pos().mag() > 2)
            {
                p.faceToward(Vec3f(0, 0, 0), .015);
            }
        }

        for (int i = 0; i < numPrey; i++)
        {
            Nav &p = preys[i];
            p.faceToward(preyDir, cohesion);
            float dist = 100000;
            int dex = 0;
            for (int j = 0; j < numFood; j++)
            {
                float tDist = (foods[j] - p.pos()).mag();
                dist = tDist < dist ? tDist : dist;
                dex = tDist < dist ? i : dex;
            }
            p.faceToward(foods[dex], appetite);

            if (dist < .02)
            {
                foods[dex] = rnd::ball<Vec3f>() * 2;
            }

            for (int k = i + 1; k < numPrey; k++)
            {
                Nav &q = preys[k];
                Vec3f dif = (q.pos() - p.pos());
                float d = dif.mag();
                if (d < minDist)
                {

                    p.faceToward(p.pos() + p.uf() - dif, aversion);
                    q.faceToward(q.pos() + q.uf() + dif, aversion);
                }
            }
            for (int l = i + 1; l < numPred; l++)
            {
                Nav &s = preds[l];
                Vec3f dif = (s.pos() - p.pos());
                float d = dif.mag();
                if (d < preyAwareness)
                {

                    p.faceToward(p.pos() + p.uf() - dif, predAversion);
                    s.faceToward(s.pos() + s.uf() - dif, predAppetite);
                }
                if (d < predAwareness)
                {

                    s.faceToward(s.pos() + s.uf() - dif, predAppetite * .5);
                }
            }

            p.moveF(move_rate);
            p.step();
        }
        if (follow == true)
        {
            nav().pos(preys[0].pos() - preys[0].uf() * 2);
            nav().faceToward(preys[0].pos(), 1.0);
        }
        if (followPred == true)
        {
            nav().pos(preds[0].pos() - preds[0].uf() * 3);
            nav().faceToward(preds[0].pos(), 1.0);
        }
    }

    bool onKeyDown(const Keyboard &k) override
    {

        if (k.key() == '1')
        {
            // introduce some "random" forces

            nav().pos(0, 0, 7);
            nav().faceToward(Vec3f(0, 0, 0), 1.0);
            follow = false;
            followPred = false;
        }
        if (k.key() == '2')
        {

            follow = true;
            followPred = false;
        }
        if (k.key() == '3')
        {

            followPred = true;
            follow = false;
        }

        return true;
    }

    void onDraw(Graphics &g)
    {
        g.depthTesting(true);
        g.lighting(true);
        g.clear(0);

        g.color(1, 0, 0);
        for (int i = 0; i < numPred; i++)
        {
            g.pushMatrix();
            g.translate(preds[i].pos());
            g.rotate(preds[i].quat());
            g.scale(Vec3f(0.05, .05, .15));
            g.draw(predMesh);
            g.popMatrix();
        }
        g.color(1, 0, 1);
        for (int i = 0; i < numPrey; i++)
        {
            g.pushMatrix();
            g.translate(preys[i].pos());
            g.rotate(preys[i].quat());
            g.scale(Vec3f(0.03, .03, .07));
            g.draw(preyMesh);
            g.popMatrix();
        }

        g.color(0, 0, 1);
        for (int i = 0; i < numPrey; i++)
        {
            g.pushMatrix();
            g.translate(foods[i]);
            // g.rotate(foods[i].quat());
            g.scale(0.03);
            g.draw(foodMesh);
            g.popMatrix();
        }
    }
};

int main()
{
    MyApp app;
    app.configureAudio(48000, 512, 2, 0);
    app.start();
}

// int main() {  MyApp().start(); }
