// src/EntidadDigital.h
#pragma once
#include <QPointF>
#include <QSizeF>
#include <QPainter>

class EntidadDigital {
public:
    EntidadDigital(QPointF pos, QSizeF size)
        : m_pos(pos), m_size(size) {}
    virtual ~EntidadDigital() = default;

    virtual void update(double dt) = 0;
    virtual void draw(QPainter *painter) const = 0;

    QPointF pos() const { return m_pos; }
    QSizeF  size() const { return m_size; }

    void setPos(QPointF p) { m_pos = p; }

protected:
    QPointF m_pos;
    QSizeF  m_size;
};



#ifndef JUGADOR_H
#define JUGADOR_H

#include "EntidadDigital.h"

class Jugador : public EntidadDigital {

private:
    bool inestable;
    int tiempoManipulacion;

public:
    Jugador();

    void activarHabilidad();
    void manejarInterferencias();
};

#endif

// src/AgenteInteligente.h
#pragma once
#include "EntidadDigital.h"
#include "Pelota.h"
#include <QPixmap>
#include <vector>

class AgenteInteligente : public EntidadDigital {
public:
    static constexpr double W     = 18.0;
    static constexpr double H     = 100.0;
    static constexpr double SPEED = 290.0; // px/s

    // level=1 -> basic AI (track ball Y), level=2 -> advanced (predict + adapt)
    AgenteInteligente(int level, QPointF pos, QSizeF sceneSize);
    ~AgenteInteligente() override = default;

    void update(double dt) override;
    void draw(QPainter *painter) const override;

    void setBall(const Pelota *ball) { m_ball = ball; }
    QRectF rect() const { return {m_pos.x(), m_pos.y(), m_size.width(), m_size.height()}; }

private:
    int            m_level;
    const Pelota  *m_ball    = nullptr;  // non-owning
    QSizeF         m_sceneSize;
    QPixmap        m_pix;
    std::vector<QPointF> m_ballHistory; // Advanced AI: track ball path

    void moveBasic(double dt);
    void moveAdvanced(double dt);
    double predictBallY() const; // simple linear prediction
};



// src/Pelota.h
#pragma once
#include "EntidadDigital.h"
#include <QPixmap>
#include <cmath>

class Pelota : public EntidadDigital {
public:
    // Initial speed (px/s)
    static constexpr double INITIAL_SPEED = 220.0;
    static constexpr double MAX_SPEED     = 600.0;
    static constexpr double ACCEL_FACTOR  = 1.05; // 5% per bounce

    Pelota(QPointF pos, QSizeF sceneSize);
    ~Pelota() override = default;

    void update(double dt) override;
    void draw(QPainter *painter) const override;

    void bounceX();          // invert horizontal component
    void bounceY();          // invert vertical component
    // Paddle hit: offsetFactor in [-1,1] controls rebound angle
    void bounceOffPaddle(double offsetFactor, bool fromLeft);
    void applyGlitch();      // Level 2: random perturbation
    void reset();

    QPointF velocity() const { return m_vel; }
    QRectF  rect()     const { return {m_pos.x(), m_pos.y(), m_size.width(), m_size.height()}; }

private:
    QPointF m_vel;
    QSizeF  m_sceneSize;
    QPixmap m_pix;
};


// src/GestorNivel.h
#pragma once
#include "Jugador.h"
#include "AgenteInteligente.h"
#include "Pelota.h"
#include <memory>
#include <QPainter>
#include <QFont>

class GestorNivel {
public:
    static constexpr double SCENE_W = 900.0;
    static constexpr double SCENE_H = 600.0;

    GestorNivel();
    ~GestorNivel() = default;

    void loadLevel(int level);    // 1 = side-view, 2 = top-down
    void update(double dt);
    void draw(QPainter *painter) const;
    void reset();

    // Input forwarding
    void keyPress(int key);
    void keyRelease(int key);

    int  currentLevel() const { return m_level; }
    int  scorePlayer()  const { return m_scorePlayer; }
    int  scoreAI()      const { return m_scoreAI; }
    bool isRunning()    const { return m_running; }

    // Audio signal helpers (checked & cleared each frame)
    bool popBounceEvent()  { bool v = m_evBounce; m_evBounce = false; return v; }
    bool popGlitchEvent()  { bool v = m_evGlitch; m_evGlitch = false; return v; }
    bool popScoreEvent()   { bool v = m_evScore;  m_evScore  = false; return v; }

private:
    int    m_level        = 1;
    int    m_scorePlayer  = 0;
    int    m_scoreAI      = 0;
    bool   m_running      = true;
    bool   m_evBounce     = false;
    bool   m_evGlitch     = false;
    bool   m_evScore      = false;

    double m_glitchTimer  = 0.0;  // Level 2: countdown to next glitch
    double m_resetTimer   = -1.0; // countdown after scoring

    std::unique_ptr<Jugador>           m_player;
    std::unique_ptr<AgenteInteligente> m_ai;
    std::unique_ptr<Pelota>            m_ball;

    QPixmap m_bgLevel1;
    QPixmap m_bgLevel2;
    QPixmap m_bgLevel3;

    // Level 3 moving center barrier
    double m_barrierY     = 100.0;
    double m_barrierSpeed = 180.0; // px/s
    double m_barrierH     = 160.0;
    double m_barrierW     = 16.0;

    void checkCollisions();
    void drawHUD(QPainter *painter) const;
    void drawBackground(QPainter *painter) const;
    void drawCourt(QPainter *painter) const;
};


// src/AudioEngine.h
#pragma once
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <memory>
#include <stdexcept>

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine() = default;

    void playBounce();
    void playGlitch();
    void playScore();
    void playMusic(int level);
    void stopMusic();

private:
    std::unique_ptr<QSoundEffect>  m_sfxBounce;
    std::unique_ptr<QSoundEffect>  m_sfxGlitch;
    std::unique_ptr<QSoundEffect>  m_sfxScore;
    std::unique_ptr<QMediaPlayer>  m_music;
    std::unique_ptr<QAudioOutput>  m_audioOut;
};
