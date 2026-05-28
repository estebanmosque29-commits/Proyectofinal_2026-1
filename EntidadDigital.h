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

#ifndef AGENTEINTELIGENTE_H
#define AGENTEINTELIGENTE_H

#include "EntidadDigital.h"

class Pelota;

class AgenteInteligente : public EntidadDigital {

private:
    float tiempoReaccion;

public:
    AgenteInteligente();

    void predecirTrayectoria(Pelota* p);
    void adaptarDificultad(int nivelJugador);
};

#endif


#ifndef PELOTA_H
#define PELOTA_H

class Pelota {

private:
    float aceleracion;
    bool enJuego;

public:
    Pelota();

    void incrementarVelocidad();
    void resetearPosicion();
};

#endif

#ifndef GESTORNIVEL_H
#define GESTORNIVEL_H

class GestorNivel {

private:
    int nivelActual;
    bool modoBoss;

public:
    GestorNivel();

    void cargarNivel(int id);
    void activarEventoEspecial();
};

#endif
