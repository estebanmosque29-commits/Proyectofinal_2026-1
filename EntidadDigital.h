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


