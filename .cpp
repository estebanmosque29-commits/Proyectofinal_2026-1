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


// src/GestorNivel.cpp
#include "GestorNivel.h"
#include <QPainterPath>
#include <QLinearGradient>
#include <cmath>
#include <stdexcept>

GestorNivel::GestorNivel()
{
    m_bgLevel1 = QPixmap(":/sprites/background_level1.png");
    m_bgLevel2 = QPixmap(":/sprites/background_level2.png");
    m_bgLevel3 = QPixmap(":/sprites/background_level3.png");
    loadLevel(1);
}

void GestorNivel::loadLevel(int level)
{
    m_level       = level;
    m_scorePlayer = 0;
    m_scoreAI     = 0;
    m_running     = true;
    m_resetTimer  = -1.0;
    m_glitchTimer = 3.0;

    m_barrierY     = SCENE_H / 2 - m_barrierH / 2;
    m_barrierSpeed = 180.0;

    const QSizeF scene(SCENE_W, SCENE_H);

    try {
        if (level == 1) {
            // Side-view: player on left, AI on right
            m_player = std::make_unique<Jugador>(
                QPointF(30, SCENE_H / 2 - Jugador::H / 2), scene);
            m_ai = std::make_unique<AgenteInteligente>(
                1,
                QPointF(SCENE_W - 30 - AgenteInteligente::W, SCENE_H / 2 - AgenteInteligente::H / 2),
                scene);
        } else {
            // Top-down (Nivel 2 and Nivel 3): player on left half, AI on right half — both move freely
            m_player = std::make_unique<Jugador>(
                QPointF(30, SCENE_H / 2 - Jugador::H / 2), scene);
            m_ai = std::make_unique<AgenteInteligente>(
                2,
                QPointF(SCENE_W - 30 - AgenteInteligente::W, SCENE_H / 2 - AgenteInteligente::H / 2),
                scene);
        }
        m_ball = std::make_unique<Pelota>(
            QPointF(SCENE_W / 2 - 9, SCENE_H / 2 - 9), scene);
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error(std::string("GestorNivel::loadLevel: alloc failed – ") + e.what());
    }

    m_ai->setBall(m_ball.get());
}

void GestorNivel::reset()
{
    loadLevel(m_level);
}

// ─── Input ───────────────────────────────────────────────────────────────────

void GestorNivel::keyPress(int key)
{
    if (!m_player) return;
    if (m_level == 1) {
        if (key == Qt::Key_W) m_player->setMoveUp(true);
        if (key == Qt::Key_S) m_player->setMoveDown(true);
    } else {
        if (key == Qt::Key_W) m_player->setMoveUp(true);
        if (key == Qt::Key_S) m_player->setMoveDown(true);
        if (key == Qt::Key_A) m_player->setMoveLeft(true);
        if (key == Qt::Key_D) m_player->setMoveRight(true);
    }
}

void GestorNivel::keyRelease(int key)
{
    if (!m_player) return;
    if (key == Qt::Key_W) m_player->setMoveUp(false);
    if (key == Qt::Key_S) m_player->setMoveDown(false);
    if (key == Qt::Key_A) m_player->setMoveLeft(false);
    if (key == Qt::Key_D) m_player->setMoveRight(false);
}

// ─── Update ──────────────────────────────────────────────────────────────────

void GestorNivel::update(double dt)
{
    if (!m_running) return;

    // Post-score delay
    if (m_resetTimer > 0) {
        m_resetTimer -= dt;
        if (m_resetTimer <= 0) {
            m_ball->reset();
            m_resetTimer = -1.0;
            m_running = true;
        }
        return;
    }

    m_player->update(dt);
    m_ball->update(dt);

    // Level 2 glitch
    if (m_level == 2) {
        m_glitchTimer -= dt;
        if (m_glitchTimer <= 0) {
            m_ball->applyGlitch();
            m_evGlitch    = true;
            m_glitchTimer = 2.5 + static_cast<double>(std::rand() % 300) / 100.0;
        }
    }

    // Level 3 barrier movement
    if (m_level == 3) {
        m_barrierY += m_barrierSpeed * dt;
        if (m_barrierY <= 8.0) {
            m_barrierY = 8.0;
            m_barrierSpeed = std::abs(m_barrierSpeed);
        }
        if (m_barrierY + m_barrierH >= SCENE_H - 8.0) {
            m_barrierY = SCENE_H - 8.0 - m_barrierH;
            m_barrierSpeed = -std::abs(m_barrierSpeed);
        }
    }

    m_ai->update(dt);
    checkCollisions();
}

// ─── Collision ────────────────────────────────────────────────────────────────

void GestorNivel::checkCollisions()
{
    QRectF ball  = m_ball->rect();
    QRectF plr   = m_player->rect();
    QRectF ai    = m_ai->rect();

    // Ball vs player paddle
    if (ball.intersects(plr)) {
        double paddleCenterY = plr.y() + plr.height() / 2;
        double ballCenterY   = ball.y() + ball.height() / 2;
        double offset        = (ballCenterY - paddleCenterY) / (plr.height() / 2);
        m_ball->setPos(QPointF(plr.right() + 1, m_ball->pos().y()));
        m_ball->bounceOffPaddle(offset, true);
        m_evBounce = true;
    }

    // Ball vs AI paddle
    if (ball.intersects(ai)) {
        double paddleCenterY = ai.y() + ai.height() / 2;
        double ballCenterY   = ball.y() + ball.height() / 2;
        double offset        = (ballCenterY - paddleCenterY) / (ai.height() / 2);
        m_ball->setPos(QPointF(ai.left() - m_ball->size().width() - 1, m_ball->pos().y()));
        m_ball->bounceOffPaddle(offset, false);
        m_evBounce = true;
    }

    // Ball exits left → AI scores
    if (ball.right() < 0) {
        m_scoreAI++;
        m_evScore   = true;
        m_running   = false;
        m_resetTimer = 1.5;
    }

    // Ball exits right → Player scores
    if (ball.left() > SCENE_W) {
        m_scorePlayer++;
        m_evScore   = true;
        m_running   = false;
        m_resetTimer = 1.5;
    }

    // Ball vs Center Barrier (Level 3)
    if (m_level == 3) {
        QRectF barrier(SCENE_W / 2 - m_barrierW / 2, m_barrierY, m_barrierW, m_barrierH);
        if (ball.intersects(barrier)) {
            if (m_ball->velocity().x() > 0) {
                m_ball->setPos(QPointF(barrier.left() - m_ball->size().width() - 1, m_ball->pos().y()));
                m_ball->bounceX();
            } else {
                m_ball->setPos(QPointF(barrier.right() + 1, m_ball->pos().y()));
                m_ball->bounceX();
            }
            m_evBounce = true;
        }
    }
}

// ─── Draw ────────────────────────────────────────────────────────────────────

void GestorNivel::drawBackground(QPainter *painter) const
{
    const QPixmap &bg = (m_level == 1) ? m_bgLevel1 : ((m_level == 2) ? m_bgLevel2 : m_bgLevel3);
    if (!bg.isNull()) {
        painter->drawPixmap(QRectF(0, 0, SCENE_W, SCENE_H), bg, bg.rect());
        return;
    }

    // Procedural fallback background
    if (m_level == 1) {
        QLinearGradient grad(0, 0, 0, SCENE_H);
        grad.setColorAt(0, QColor(5, 10, 30));
        grad.setColorAt(1, QColor(10, 25, 60));
        painter->fillRect(QRectF(0, 0, SCENE_W, SCENE_H), grad);
    } else if (m_level == 2) {
        QLinearGradient grad(0, 0, SCENE_W, SCENE_H);
        grad.setColorAt(0, QColor(20, 5, 40));
        grad.setColorAt(1, QColor(5, 20, 30));
        painter->fillRect(QRectF(0, 0, SCENE_W, SCENE_H), grad);
    } else {
        QLinearGradient grad(0, 0, SCENE_W, SCENE_H);
        grad.setColorAt(0, QColor(30, 0, 20));
        grad.setColorAt(1, QColor(10, 0, 30));
        painter->fillRect(QRectF(0, 0, SCENE_W, SCENE_H), grad);
    }
}

void GestorNivel::drawCourt(QPainter *painter) const
{
    painter->save();
    QPen pen(QColor(255, 255, 255, 60));
    pen.setStyle(Qt::DashLine);
    pen.setWidth(2);
    painter->setPen(pen);
    // Center divider
    painter->drawLine(QPointF(SCENE_W / 2, 0), QPointF(SCENE_W / 2, SCENE_H));

    // Center circle
    painter->drawEllipse(QPointF(SCENE_W / 2, SCENE_H / 2), 60, 60);

    // Border
    QPen border(QColor(255, 255, 255, 40));
    border.setWidth(3);
    painter->setPen(border);
    painter->drawRect(QRectF(3, 3, SCENE_W - 6, SCENE_H - 6));
    painter->restore();
}

void GestorNivel::drawHUD(QPainter *painter) const
{
    painter->save();

    // Score
    QFont font("Consolas", 28, QFont::Bold);
    painter->setFont(font);
    painter->setPen(QColor(255, 255, 255, 200));
    painter->drawText(QRectF(SCENE_W / 2 - 120, 12, 100, 50), Qt::AlignRight,
                      QString::number(m_scorePlayer));
    painter->drawText(QRectF(SCENE_W / 2 + 20, 12, 100, 50), Qt::AlignLeft,
                      QString::number(m_scoreAI));

    // Level badge
    QFont small("Consolas", 10);
    painter->setFont(small);
    painter->setPen(QColor(200, 200, 200, 140));
    QString lvlStr;
    if (m_level == 1) lvlStr = "NIVEL 1 – FASE ESTABLE";
    else if (m_level == 2) lvlStr = "NIVEL 2 – ENTORNO INESTABLE";
    else lvlStr = "NIVEL 3 – SINGULARIDAD DIGITAL";
    painter->drawText(QRectF(0, SCENE_H - 22, SCENE_W, 20), Qt::AlignCenter, lvlStr);

    // Glitch flash overlay (Level 2)
    if (m_level == 2 && m_evGlitch) {
        painter->fillRect(QRectF(0, 0, SCENE_W, SCENE_H), QColor(255, 0, 255, 18));
    }

    painter->restore();
}

void GestorNivel::draw(QPainter *painter) const
{
    drawBackground(painter);
    drawCourt(painter);

    // Draw center barrier if Level 3
    if (m_level == 3) {
        painter->save();
        QRectF barrier(SCENE_W / 2 - m_barrierW / 2, m_barrierY, m_barrierW, m_barrierH);
        QLinearGradient grad(barrier.left(), 0, barrier.right(), 0);
        grad.setColorAt(0, QColor(255, 120, 0, 220));
        grad.setColorAt(0.5, QColor(255, 255, 255, 255));
        grad.setColorAt(1, QColor(255, 50, 0, 220));
        painter->setBrush(grad);
        painter->setPen(QPen(QColor(255, 180, 0), 1.5));
        painter->drawRoundedRect(barrier, 3.5, 3.5);
        painter->restore();
    }

    if (m_player) m_player->draw(painter);
    if (m_ai)     m_ai->draw(painter);
    if (m_ball)   m_ball->draw(painter);

    drawHUD(painter);
}
