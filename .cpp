// src/EntidadDigital.cpp
#include "EntidadDigital.h"
// Abstract base – no extra implementation needed.


// src/Jugador.cpp
#include "Jugador.h"
#include <stdexcept>

Jugador::Jugador(QPointF pos, QSizeF sceneSize)
    : EntidadDigital(pos, QSizeF(W, H))
    , m_sceneSize(sceneSize)
{
    try {
        m_history.reserve(200);
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error(std::string("Jugador: failed to reserve history – ") + e.what());
    }
    m_pix = QPixmap(":/sprites/paddle.png");
}

void Jugador::update(double dt)
{
    QPointF delta(0, 0);
    if (m_up)    delta.setY(delta.y() - SPEED * dt);
    if (m_down)  delta.setY(delta.y() + SPEED * dt);
    if (m_left)  delta.setX(delta.x() - SPEED * dt);
    if (m_right) delta.setX(delta.x() + SPEED * dt);

    m_pos += delta;

    // Clamp within scene
    if (m_pos.y() < 0)                               m_pos.setY(0);
    if (m_pos.y() + m_size.height() > m_sceneSize.height())
        m_pos.setY(m_sceneSize.height() - m_size.height());
    if (m_pos.x() < 0)                               m_pos.setX(0);
    if (m_pos.x() + m_size.width()  > m_sceneSize.width() / 2)
        m_pos.setX(m_sceneSize.width() / 2 - m_size.width());

    // Record history
    m_history.push_back(m_pos);
    if (m_history.size() > 200)
        m_history.erase(m_history.begin());
}

void Jugador::draw(QPainter *painter) const
{
    QRectF r(m_pos, m_size);
    if (!m_pix.isNull()) {
        painter->drawPixmap(r, m_pix, m_pix.rect());
    } else {
        painter->save();
        painter->setBrush(QColor(0, 200, 255));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(r, 5, 5);
        painter->restore();
    }
}


// src/AgenteInteligente.cpp
#include "AgenteInteligente.h"
#include <cmath>
#include <stdexcept>

AgenteInteligente::AgenteInteligente(int level, QPointF pos, QSizeF sceneSize)
    : EntidadDigital(pos, QSizeF(W, H))
    , m_level(level)
    , m_sceneSize(sceneSize)
{
    try {
        m_ballHistory.reserve(100);
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error(std::string("AgenteInteligente: ") + e.what());
    }
    m_pix = QPixmap(":/sprites/paddle_ai.png");
}

void AgenteInteligente::update(double dt)
{
    if (!m_ball) return;

    // Record ball position for pattern analysis (Advanced AI)
    if (m_level == 2) {
        m_ballHistory.push_back(m_ball->pos());
        if (m_ballHistory.size() > 100)
            m_ballHistory.erase(m_ballHistory.begin());
    }

    if (m_level == 1)
        moveBasic(dt);
    else
        moveAdvanced(dt);

    // Clamp
    if (m_pos.y() < 0)                               m_pos.setY(0);
    if (m_pos.y() + m_size.height() > m_sceneSize.height())
        m_pos.setY(m_sceneSize.height() - m_size.height());
    if (m_level == 2) {
        if (m_pos.x() + m_size.width() > m_sceneSize.width())
            m_pos.setX(m_sceneSize.width() - m_size.width());
        if (m_pos.x() < m_sceneSize.width() / 2)
            m_pos.setX(m_sceneSize.width() / 2);
    }
}

void AgenteInteligente::moveBasic(double dt)
{
    // Track the ball's Y center
    double ballCenterY = m_ball->pos().y() + m_ball->size().height() / 2;
    double myCenter    = m_pos.y() + m_size.height() / 2;
    double diff        = ballCenterY - myCenter;

    if (std::abs(diff) > 4.0) {
        double move = std::min(SPEED * dt, std::abs(diff));
        m_pos.setY(m_pos.y() + (diff > 0 ? move : -move));
    }
}

double AgenteInteligente::predictBallY() const
{
    if (!m_ball || m_ballHistory.size() < 2) return m_ball ? m_ball->pos().y() : m_pos.y();

    // Linear extrapolation from last two recorded ball positions
    const QPointF &prev = m_ballHistory[m_ballHistory.size() - 2];
    const QPointF &cur  = m_ballHistory.back();

    double vx = cur.x() - prev.x();
    double vy = cur.y() - prev.y();

    if (std::abs(vx) < 0.001) return cur.y();

    // Time for ball to reach AI's x
    double timeToReach = (m_pos.x() - cur.x()) / vx;
    if (timeToReach < 0) return cur.y();

    double predictedY = cur.y() + vy * timeToReach;

    // Simulate bounces within scene boundaries
    double h = m_sceneSize.height();
    predictedY = std::fmod(std::abs(predictedY), 2.0 * h);
    if (predictedY > h) predictedY = 2.0 * h - predictedY;

    return predictedY;
}

void AgenteInteligente::moveAdvanced(double dt)
{
    double targetY = predictBallY();
    double myCenter = m_pos.y() + m_size.height() / 2;
    double diff = (targetY + m_ball->size().height() / 2) - myCenter;

    // Move Y
    if (std::abs(diff) > 4.0) {
        double move = std::min(SPEED * dt, std::abs(diff));
        m_pos.setY(m_pos.y() + (diff > 0 ? move : -move));
    }

    // Move X (level 2: AI can move horizontally in its half)
    double ballCenterX = m_ball->pos().x() + m_ball->size().width() / 2;
    double myX         = m_pos.x() + m_size.width() / 2;
    // Stay within right half; lean toward expected intercept
    double targetX = std::max(m_sceneSize.width() / 2,
                              std::min(m_sceneSize.width() - m_size.width(),
                                       ballCenterX - m_size.width() / 2));
    double diffX = targetX - m_pos.x();
    if (std::abs(diffX) > 4.0) {
        double move = std::min(SPEED * dt, std::abs(diffX));
        m_pos.setX(m_pos.x() + (diffX > 0 ? move : -move));
    }
}

void AgenteInteligente::draw(QPainter *painter) const
{
    QRectF r(m_pos, m_size);
    if (!m_pix.isNull()) {
        painter->drawPixmap(r, m_pix, m_pix.rect());
    } else {
        painter->save();
        painter->setBrush(QColor(255, 80, 80));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(r, 5, 5);
        painter->restore();
    }
}
